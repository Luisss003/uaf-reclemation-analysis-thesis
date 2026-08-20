# AddressSanitizer
### Existing Techniques and their Limitations
#### Shadow Memory
 - SM is a method to store metadata about real program data i.e. stack, heap, etc.
 - There are two methods for mapping to shadow memory
	 - Map x real memory bytes to y shadow memory bytes at some offset address. This can either be one to one, or some scale i.e. 64 bytes of real mem map to a single byte of shadow. 
	 - Multilevel translation: split the shadow mem into pieces, and use a lookup table to get a shadow addr; essentially we are doing the page table translation twice.
#### Instrumentation
 - Idea is to insert instructions into a binary, which introduce checks on memory instructs
 - 2 types of instrumentation techniques:
	 - Compile instrumentation: insert instructs at compile time, which reduces overhead on execution, and it has a deepeer understanding of global, stack. and heap objects.
	 - Runtime Instrumentation: insert instructs during runtime, so we don't have to modify the existing binary, but it increases overhead dramatically.
 - They have no false positives, but increase overhead dramatically.

#### Debug Allocators
- Idea is to use a custom memory allocator other than malloc()
- There are two main types of these custom allocators:

Page Memory Protection: upon allocation, the allocator takes a chunk out of a protected virtual page. If an access is attempted to an adjacent, guard page, then the page faults.

Magic Value Allocators:  the idea is to fill memory up with recognizable patterns/values. When an allocation occurs, these magic values become adjacent to the allocation. If a corruption then occurs, those magic values will be overwritten illegally.

Note, if an illegal write occurs which modifies a redzone, the allocator may not detect it immediately, but the proof that it happened is there. If say our alloaction looks like:
```
allocation                 redzone
[........] [BB BB BB BB BB BB BB BB]
```

And we overwrite:
```
allocation                 redzone
[........] [58 BB BB BB BB BB BB BB]
            ^
            'X'
```

the CPU wont do anything immediately, but once we call `free()` on that allocation, the allocator will check and note that a magic value was changed, and then create an error. 

This technique is effective against arbitrary writes, but doesn't really work against arbitrary reads. If we select certain magic values such that, if the user read them maliciously into a value, it alters the way the program behaves, and then makes the memory corruption detectable aka the magic value poisined the program, but we cant guarantee that, so its known as probablistic detection.

This general idea is commonly applied to stack canaries.

### AddressSanitizer Algorithm
The general idea is to use shadow memory to record whether each byte of application mem is safe to access; this is aided with instrumentation, which checks shadow mem on each real program load/store; this is how valgrind works, but ASan does it with optimization tricks.

#### Shadow Memory
 ASan optimizes how it represents shadow memory. Malloc works in alignments of 8 bytes. This tells us each memory chunk of 8 bytes has 9 possible valid bytes. This just means each one of these aligned 8 byte blocks can be fully non addressable, partially addressable, or not addressable at all. To visualize this:
 ```
k = 0: [X X X X X X X X]
k = 1: [A X X X X X X X]
k = 2: [A A X X X X X X]
k = 3: [A A A X X X X X]
...
k = 7: [A A A A A A A X]
k = 8: [A A A A A A A A]

where X = unaddressable, and A = addressable
 ```

Because there are a limited number of states, we can simply represent the state an aligned 8 byte block is in with a single hex byte of shadow mem.

To map a real mem addr to a shadow mem addr, it uses `(real_addr>>scale)+offset`. Pick an offset such that the region `offset to (offset+max_addr/8)` is not occupied on startup; this is because ASan uses a shadow mem that takes up 1/8 of virtual memory for the proc.
1) As an example, if we were using linux 32-bit which has an addressable space of `0x00000000-0xffffffff`, use offset `0x2000000`.
2) The scale indicates how much space the shadow memory will take via `1/(2^N)`; anything greater than 3 is not necessary.

Now, since we are mapping real memory to shadow memory, which is jsut meant to be a "mini reprsentation" it can cause recursion where shadow mem represents shadow mem, and so on. So, we mark the real shadow memory range as "bad" and inaccessible memory in the shadow representaion.
![[Pasted image 20260724184752.png|405]]

Now, to actually encode 8 byte blocks to shadow mem:
1) `0x00` = all bytes are addressable
2) `0xk for 1 <= k <= 7` = the first k bytes are addressable
3) Any negative value = the entire 8 byte word is unaddresssable
	1) It can be diff neg valeus since they can refer to heap, stack, or global redzones.

#### Instrumentation
 If performing an 8-byte store/load instruction, simply compute the corresponding shadow mem addr, and check if its addressable; if not, crash.

For 1,2, or 4 byte aligned store/loads, we compute the corresponding shadow addr:
1) If the shadow value is positive, compare the last 3 bits of the addr with 'k', where k means the first k bytes are addressable in this 8 byte block. This check is to ensure that the byte being accessed is within that k addressable range within the block.

Instrumentation is only performed after LLVM optimization, to reduce overhead (why would we instrument instructs that are just optimized away).

#### Runtime Library
These are custom functions used in ASan:
1) This lib is used to initially create shadow mem at process startup preventing the proc from using it.
2) malloc/free are replaced with custom implementations:
	1) malloc allocates extra mem for the redzone, which are unaccessible.
	2) After an allocation is used, free() will poison the entire memory region, then puts it into quarantine preventing chunk reuse.
	3) The allocator works similarly to bins, in the sense that it keeps a list of free chunks of different sizes, and if a bin ever becomes empty, it fills it with mmap.
	4) Since overflows typically occur left to right, the left redzone stores allocator metadata i.e. allocation size, thread ID, meaning that the minimum size of a heap redzone must be 32 bytes.

#### Handling Stack and Global Memory
Everything before has mostly been in regard to heap memory, but we also need to protect OoB accesses to global + stack objs:
1) For globals, just create redzones at compile time and then the runtime library can handle them
2) For stacks, we need to create it at runtime.
	1) These use the same 32 byte redzones are used here
	2) Basically, we're doing the equivalent of adding new variables in a function, so that they appear on the stack, and then posiining the redzones around real func variables.

### Handling False Negatives
The current layout works well, but it misses a few rare bug types. 

The most common is unaligned acesses that result in partial out of bounds accesses. Consider, which is caused by starting the the write at a valid location, but writing too many bytes.:
```C
//vanilla ASan can't deal wit hthis kind of bug.
int *a = new int[2]; // 8-aligned
int *u = (int*)((char*)a + 6);
*u = 1; // Access to range [6-9]
```

Te reason this is missed is because recall, ASan uses 8 bytes of application mem for 1 shadow byte. When we access `*u=1`, it calculates the corresponding shadow byte of to see if we are in a valid memory location. Since the entire 8 bytes are valid ,caused by `*a`, `*u`, which is at the 6th byte of the valid 8 bytes, is considered valid. Since we pass the initial check, the 4 byte write of `u* = 1` goes without error, since recall, an int write is 4 bytes and we are only limited to 8 bytes. This causes an unchecked overwrite of bytes 6 - 9.

Some other types of uncatched errors:
1) A far away out of bound access is missed since it isnt within the red zone. Ex `char *a = char[100]; a[500] = 0;` is unchecked.
	1) this can be deterred if we use a large redzone space, but that wastes alot of memory.
2) UAF is undetected if a large amount of mem is allocated, then deallcoated between the "free" and following use. This is because this huge allocation then deallocation drains the quarantine queue.

#### Handling False Positives
ASan doesn't have any false positives. its always right about detecting a corruption, but again, it may miss some.
