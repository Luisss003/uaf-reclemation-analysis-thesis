# SoK: Eternal War in Memory

**Year:** 2025  
**Relevance to Thesis:** This paper helps to categorize all the different types of dynamic memory exploits, and how to think of them.

---

## Modeling a Memory Corruption Attack

### 1 Initial Memory Corruption
All exploits require an initial memory error. There are two types:
1. Temporal Errors: dereferencing deallocated memory
     - Exploitation using this type require:
         - An object to be freed, creating a dangling pointer
         - Dangling pointer to be used afterwards
         - mismatch between old/new object layouts, and often some control over the replacement allocation
2. Spatial Error: dereferencing out of bounds memory
     - Exploitation using this type require:
         - Control over pointer arithmetic/index
         - ability to reach arbitrary index
         - useful read or write behavior
  
### 2 Leveraging the Corrupted Pointer
Irregardless of the pointer's purpose (read/write) or whether it is a spatial/temporal error, we can achieve any corruption/leakage in step 3. The four possible combinations:

1) Spatial Read: Attacker provides an index that leads to an unitended value being read.
```C
//No corruption of values occurs
//we simply trick the program into reading something unsafe.
func_addrs = {0x012, 0x23, 0x54} //Array of safe function addresses
p = user_input();
execute(func_addr[p]) //Attacker might send index to shellcode

//We can also leak data with a spatial read; consider string format
//exploits, which are simply us having the function printf read a corrupted
//format string
printf(user_input) //input = %3$x
```

2) Spatial Write: Causing the wrong value to be written somewhere via providing a out of bounds index
```C
//In a spatial error write, the allocation is alive, 
//but we go out of its bounds
p = malloc(2);
p[0] = 1;
p[1] = 2;
p[2] = 3; //Spatial error write.

//We can also use spatial error writes to leak info. 
//If we assume that we somehow corrupted err_msg
printf("%s\n, err_msg");
```

3) Temporal Read: Caused by dangling pointer, and a heap chunk being reused by another heap object
```C++
// If we assume that the replacement heap object is controlled by the attacker
// then if the program uses that old dangling ptr it will treat attacker derived
// data according to the objects old type

//Consider our initial heap obj's type to have a function ptr
class Victim { public: virtual void run();}

Victim *p = new Victim;
delete p; //p is now dangling

//We can now assume that we create a new allocation the same size
//as the old heap obj, and assume that the allocator reuses the chunk
char *replacement = new char[sizeof(Victim)];
replacement.run = 0xdeadbeef; //shellcode addr

//Then when the program reuses the dangling pointer, it reads an
//attacker derived value.
p->run();

//Another Example: a reuse can cause a info leak if
//we have something like
printf(p->sens_info);
```

5) Temporal Write: Same dangling situation as temp read, except here we explicitly overwrite a value via that dangling pointer.
```C
struct A { int x; };

struct B { int important;};

struct A *p = malloc(sizeof(struct A));
free(p);

struct B *b = malloc(sizeof(struct B));   // assume same address reused
p->x = 123;   // stale/temporal write
```

6) Double Free: This is a unique case, where a ptr isn't used for a load/store, but rather for the purpose of corrupting allocator metadata.


At this point, if a program is memory safe (bounds checks/malloc checks), any exec of these type of footholds should cause program termination. Because of this, a lot of proposals for mem safety aim to specificallty stop execution from going beyond 

### Step 3-6 Different Exploit Paths  
Once the user is able to perform some arbitrary spatial/temporal read/write, theye can take many paths to finally "pwn".
#### Path 1: Code Corruption
This is one possible path the attacker can take. Simply write code where it shouldnt.
1) The common mitigation is code integirty policy (not allowed to write code at this memory page).

This is bypassed due to the usage of JiT compilation because CIP doesn't support polymorphic code. (recall JiT makes a page writable, updates code, and makes it read only again leaving a window for injection).

##### Exploit Path: Control Flow Hijack
Another possible path, where objective is to control program execution i.e. where the RIP register is. This is the workaround to simple code integrity policies, since okay, if we cant overwrite anything, lets just control execution with existing code (think ROP). The policies that are against this generally are known as "control flow integrity policies"

These types of attacks require knowledge of where the payload/shellcode is, what addr we intend to overwrite to cause a control flow hijack, etc. 

A specific implementation of CFH is function pointer corruption, which has its own defense called "code pointer integrity policies":
1) An example would be overwriting the return addr pointer and then when ret is called, that addr is loaded into the instruct pointer.

Now okay, we can overwrite a pointer, but we need ot know what we want to write. ASLR is the main defense against this.

If we now say the hacker knows what they want to write, we don't really any defense against this. Gadgets/instructs will blindly execute what they have to work with (we can't stop ret from loading the stored ret addr to RIP)

Now once they have done all these steps, execution of "shellcode/hacker injected code" is executed. This exploitation is mitigated via NOEXEC policy, which just states bytes in certain pages cannot be executed, since we'd never want to execute stuff on the stack or heap, which is where most shellcode ends up.

Combining noexec + code integrity gives us the strong policy $W\oplus E$ meaning a page can either be executable or writable:
1) Again, this is great for normal languages, but for those that use JiT, it doesnt really help.
	1) An approach is that of ISR (instruct randomization) which acts like ASLR, but randomizes the positions of instructions
	2) This is slow, so isnt really used anymore.

$W\oplus E$ works well, but again it is vulnerable to code reuse attacks i.e ret2 or ROP. The paper states there aren't mitigations against code reuse, but recently there are features like shadow stacks, which store hidden copies of the current stack frame, and if they find that ret is using an addr it shouldnt, it calls an error.

A CFH is considered successful the moment attacker controlled code is executed.

##### Exploit Path: Data Only Attacks
This is a path where the attcker changes program values to modify the logic itself in terms of permissions, control flow, etc. Any non-const variable is open to an attack like this. 

An attack is only considered data-only as long as it doesn't corrupt code or code pointers:
1) The steps are the same as CFH, except the target is data, not code
2) We can't define "security critical data", so we need to blindly assume all data has to be protected; this policy is known as data integrity, which naturally includes code integrity, and code ptr integrity.

Like CFH, what we overwrite must be known beforehand; the data-only equivalent of ASLR would be DSR (data space randomization). DSR is basically a catch all, in the fact that it randomizes the layout of all memory; not just the location of code like ASLR.

A DOA is considered successful as soon as the corrupted variable is used; not when the corruption occurs.

#### Path 2: Info Leak
These types of attacks usually arent' used to leak like SSN numbers or anything specific. Rather, the objective is to leak an address, location of a funciton, etc, which is then later used for other exploits. This relates to stuff like DSR, ASLR, canaries, etc.

Any memory corruption error (temporal/spatial/read/write) can be used for info leaking, and it especially useful for remote targets like finger daemon services. This is mostly done via exploiting an arbitrary mem write to corrupt another pointer, which is then read later in a func like printf.
### Current Protection Techniques
The three main protections used (but there are alot more since this paper is from 2018):
1) DEP/$W\oplus E$: stop code injection
	1) Protects against injection, but not code reuse since those rely on code that already exists
2) ASLR [[0 ASLR]]: randomize addresses in memory
	1) Main purpose is to prevent data leaks (where code gadgets are)
3) Canaries [[1 Stack Canaries]]

Because 2/3 of these preventative measures rely on secret data (aslr and canaries), alot of emphasis is put into bypassing these for the purpose of info leakage. Once an addr or smth else is leaked, then ROP, and other attacks can commence.

There are 2 types of protection techniques
#### Probabilistic
These  rely on randomized/secret information (ISR, ASLR, DSR). ISR is consdered outdated, but ASLR and DSR are still widely used. Read notes on [[0 ASLR]] to get an idea of how it woprks and bypasses.

The biggest threat to probabilistic techniques are info leaks. Since injection is basically obselete (exceptfor JIT), ASLR, and randomization as a whole is focused around code position randomization, since again, ROP is the answer to write xor exec protections.

A new technique proposed is pointer encryption, which mixes ASLR and DSR; simply encrypt pointers in memory, and only decrypt upon use. Not widely used though, since binary and source code incompatable.

More on DSR: 
1) This technique randomizes how memory itself is represented, which is just a fancy way of saying it encrypts everything in memory, not the location like ASLR.
2) This is achieved via instrumentation where we encrypt/decrypt variables as they are store/used/etc.
#### Deterministic:
These rely on a known policy/condition via a monitor (canaries,  DEP)
	1) These monitors simply check if a policy like code corrupt integirty or control flow integrity are being violated.
	2) Monitors can be software (code instrumentation in ASan) or hardware (DEP) although hardware is less common
		1) Software monitors can be very lightweight, such as shadow stacks, but also be very heavy, so less likely to be used, like ROP checkers, code instrumentation, and taint checking.

### Evaluating A Technique
A protection technique must satisfy certain requirements, while still being practical (we can't use 100% overhead increase techniques).

Protection:
1) For a technique to be considered strong, you must determine what behavior the technique permits, and what does it forbid? 
	1) Given these answers, does it enforce our policy (i.e. no writes in certain pages)? Does it have many trade offs (what exploits does it allow as a result)?
	2) If it enforces it well and has few tradeoffs, then its a strong technique; if not, its weak.
2) Based on the policy, our technique (probablistic/deterministic) sohuld either have a false negative rate >0 and >= 0 respectively.
3) A technique should never have false positives, since that will cause numerous faults in normal operation.

Cost:
1) CPU and IO bound benchmarks determine overhead/performance cost
	1) CPU bound benchmarks are generally prefered, since IO benchmarks spend alot of time in the kernel space, not reflecting the CPU overhead generated by techniques
2) Memory overhead should also be taken into account
	1) Techniques like instrumentation and shadow stacks can increase the amount of RAM required by a program, since we are needing to load additional instructions/duplicate the stack.
	2) This isn't really an issue nowadays, except in the world of embedded.

Compatability:
1) A technique is compatible if no modifications are needed to the application source code.
2) It must also be such that, if a binary uses some protection technique, it should have no issue linking/working with other object/binary files that were not programmed with it in mind.
	1) if main.exe uses canaries, that shouldnt interfere with main.exe's usage/linking of libc.so
3) A technique should also support modularity
	1) Modern software is modular, so the technique must be able to process each module/library/file individually.

### Enforcing Memory Safety
As stated, if we enforce memory safety, we stop the exploit from going past steps 1 and 2. Modern languages enforce this via:
1) Checking obj bounds at access time of arrays
2) Automated garbage collector

The same effect can be achieved for languages like C via the low level reference monitors talked about before.
#### Enforcing Spatial Safety
1) The only way  to enforce memory safety with these type of errors is to keep track of pointer bounds i.e. lowest and highest valid addr a ptr can use
	1) The existing implementations for this technique work, but introduce too much overhead (SoftBound)
	2) Also, not very binary compatible, since if an outside library updates the pointer, it does not update the metadata for that poitner.
2) Another proposed technique is object bounds
	1) Rather than associating pointer bounds with the pointer itself, associate it with the object it points to. So, track where allocated objects exist.
	2) If an outside lib updates the pointer, since the metadata is tied to the obj, we aren't lost (therefore very binary compatible)
	3) These have 2 main issues:
		1) Knowing object bounds isnt sufficient on its own.
			1) Say we have 2 objects of two different types A and B, and a pointer 'p' which refrences obj A. If we then accidently += 40 such that now 'p' points to a location inside B's memory space, if we derefrence p, it will now give us B's data, which shouldnt be possible since p was ONLY Meant for A
			2) To solve this, certain implementations check pointer arithmetic. simply, if pointer addr + offset > outside of bounds of the pointer assigned obj bounds, then call an error.
		2) Pointers may have legit reasons to go out of bounds, as long as they are derefrenced
			1) Consider a loop with a pointer as a base address. At the last iteration, it increments the pointer value to be out of bounds, but we check the condition and therefore the loop ends normally
			2) The pointer is still technically out of bounds even though the code is safe, so we end up with a false alarm.
			3) A workaround was to just add padding bytes to the real objects bound, so that this behavior wouldnt happen, but it isnt very reliable.
		3) Also, its a bit weak. It enforces "do not leave this allcoated object" but not "do not leave the curr field of the obj"
			1) Assume we have an struct type of A, which has 2 fields [junk,important]. Object checking ensures our accesses/writes stay within the object, but not within the specific field. Therefore, we can do something like `strcpy(obj_A, attck_input)` where the attacker input overwrites the junk field, into the important field.
			2) We can't just solve this by adding struct bound checks, since C operations treat a whole struct object as a contigous sequence of bytes rather than individual fields. `memset` doesnt care about fields for example.
3) In order to make obj bounded work, "automatid pool allocation" was developed named CRED.
	1) The idea is to divide the object metadata into "pools". Each pool gets a smaller metadata structure meaning metadata access is faster
	2) Also, it fixes the loop issue
	3) the struct field issue is unfixed.
4) The overall motivation for using object based is because it allows you to remain compatible with unprotected libraries.

#### Enforcing Temporal Safety
1) Special Custom Allocators
	1) A naive approach is to simply never reuse a free'd chunk so clearly no
	2) Cling is a custom allocator: basically a version of malloc that only lets the reuse of chunks be by the same type. So a chunk used to store struct type B can only then be used again for objects of that type.
		1) It doesnt remove dangling ptr exploits, but prevents use after free specifically.
2) Obj Based Approaches
	1) Obj based temporal protection is very limited against temporal attacks. Recall we store the address range of specific objects.
	2) In the scenario where we free() an obj, then use the pointer again directly after, it will catch that, since the objects address range metadata was invalidated after free().
	3) However, in the scenario where we free() an object, then malloc() to reuse that same address range but with a different object, that original pointer to the first obj will be able to reuse that space (UAF). 
	4) It can detect stale pointer reuse before address reuse, but after address reuse, its blind to it.
3) Pointer Based Approach
	1) Again, we associate each ptr with the identify of the allocation it came from; not just its address.
	2) Each allocation gets a unique identifier, meaning each pointer carry both the address, + the allocation ID.
	3) With the unique ID, we can query a global table which stores whether the ID itself is valid.
		1) If not, then whatever assocaited memory its with must not be valid/allocated.
	4) This means calling free() on a pointer makes the associated allcaotion ID false/invalid, so any attempt to later use this pointer again causes a query to the table, it returns false, so cant use after free.
		1) If we later allocate again and the same chunk is reused, it gets a completely different allocator ID, therefore UaF doesnt work here, or double free.
	5) It makes sense to just maintain a validity bit with each pointer, but this doesnt work since pointers can be copied, making it such that if we free the master pointer, all other copies will become invalid, even when they shouldnt have.
	6) This is a good solution, but again, metadata must propogate whenever poitners are used/created/etc which makes them binary incompatable.

### Generic Attack Defense
Memory safety is generally considered the more important aspect of preventing exploitation, but there are more data oriented defenses for preventing data corrption and control data hijacking.
#### Enforcing Data Integirty
Consider the question "Can an unsafe pointer overwrite important memory?". That is what we prevent in this protection techniques. Data integirty simply means that we prevent modification of important data on the program stack, heap, etc.

We mainly care about spacial integrity here. Note:
1) Spatial Safety, which is in relation to memory safety, is about protecting each objects exact boundaries.
2) In our case, spacial integirty, we try to stop an overflow from corrupting improtant protected objects.
	1) This means with these techniques, we might tolerate some out of bounds writes, but ONLY if they dont effect protected program data/objects.

The early techniques rely on pointer safety static analysis:
1) Before a prog runs, analyze for "unsafe pointers"
	1) unsafe pointers = a ptr that COULD go out of bounds for a myriad of reasons like it relying on a computed index , or if what it points to relies on a condition.
2) Given the possible places a ptr can point, we associate an unsafe pointer with a "points-to-set" which lists all the possible places a pointer could point to in the program.
	1) Note the addr of any objects in this set are considered "unsafe objects".

Given this idea of safe/unsafe, Yong et al gives us the technique:
1) Divide system memory into safe and unsafe objects, with the IDs of 0 and 1 respectively.
2) Shadow memory is used to record this information. 
	1) Shadow memory is seperate metadata corresponding to the applications memory. Think of it as a mapping of For the addr byte 0x123151, it maps to the value 0, because we consider it safe.
	2) If an unsafe obj is created, then each byte of memory that obj takes up recieves a 1. When that obj is dealloc, those 1's are cleared.
3) Whenever we attempt to write to an unsafe object, a check is done such that an unsafe pointer may write only to memory belongong to an unsafe object. This means any variable that isn't marked as "unsafe" can't be written to with some corrupted pointer.
	1) iT doesnt mean that a certain pointer owns this unsafe object. It just means that an unsafe pointer somewhere may write to this unsafe object legitimately.

This technique does two things:
1) It ensures writes to "unsafe objects" can only be done via an unsafe pointer
2) It completely protects safe objects i.e. return addresses, stack variables never accessed via pointer, etc. any attempt to write to these with an unsafe pointer is rejected.

There are two major flaws in this technique:
3) All unsafe objs are treated alike
	1) This means if we have two unsafe pointers, one points to sensitive memory, and the other to junk, since this technique ONLY checks to see if an unsafe pointer is writing to unsafe memory, we can use the junnk pointer to write to the sensitive pointers unsafe object.
	2) A stronger policy would be "does this particular unsafe object belong to this unsafe pointer?".
4) Reads are not checked when considering unsafe poitners. This means something like `value = p[i]` where p is a corrupted ptr stil lworks.

Now, Yong's system has an improved version called Write Integrity Testing:
1) Now, unsafe ptrs can only write to unsafe objs within its points-to set.
	1) This introduces an issue, since we have to now protect these points-to sets.
	2) In shadow mem metadata, we now also have to create a unique ID for each points-to set created, not just 0 or 1 like Yongs.
2) It also protects indirect calls to stop hijacking attacks
	1) It does this by calculating the points-to set of ptrs used by indirect call instructs and associate then with IDs
	2) note `'ret` is not checked since, the only reason ret would be used in an attack is if the stck already contained an illegal address, which we dont let happen with WIP ,so there isnt any need to protect it.
3) This technique is stil vulenrable to data read corruption
4) Note like the first technique, this is completely vulnerable to temporal techniques since again, object integrity is meant for spatial integirty.

#### Enforcing Data Flow Integrity
DFI checks whether a value being read came from an allowed write instruction:
1) DFI uses each read instruction’s reaching-definition set.
	1) A definition is an instruction that writes a value.
	2) The reaching-definition set of a read contains every write instruction whose value could reach the target read instruct along a valid control-flow path without another write replacing it first.
	3) Each set is assigned a unique ID.
2) When an instruction writes to memory, DFI records the writing instruction’s ID in shadow memory.
3) Before another instruction reads that memory location, DFI checks whether the recorded writer ID belongs to the read’s statically calculated reaching-definition set.
4) If the last writer is not allowed, DFI concludes that the value may have been corrupted.

The big issue with this technique is that since it only checks for reads of valid data, it doesnt by default check for valid writes (which means its metadata is vulnerable to being overwritten). This forces DFI to check for valid writes, which just increases the overhead where this method isnt even viable.

#### Enforcing Code Pointer Integrity
Straight away, we can't protect all code pointers, but can protect immutable ones. This includes ones in the GOT table, or vtable, since those have no reason to change. Function pointers (i.e. user functions or return addresse) can have valid reasons to change, therefore we can't just blindly protect them.

Therefore, we shouldnt really bother with code pointer integrity, and should focus more on code pointer corruption prevention in the first place.

#### Enforcing Control Flow Integrity
This is where alot of the known protections are:
1) Dynamic Return Integrity:
	1) Main objective is to have some integirty check that detects when a control flow hijack attack, like a buf ovf, is attempted.
	2) The most common technique is that of stack canaries, but there are other ones like shadow stacks. 
		1) Shadow stacks are efficient, since even if we don't hide it, the attacker has to go out of their way to now corrupt the ret addr in the real stack + the shadow stack.
2) Static Control Flow Integirty
	1) Here, we rely on static analysis + use of control flow graphs. 
		1) Essentially determine beforehand what a program should ever be allowed to do. If it goes into a path not seen in the computed CFG, abort.
	2) This is done similarly to how WIT works, in the sense that we compute the valid call locations that a program/or indirect call would make. If its not in the set, its probably malicious.
	3) We would embed these points-to sets within the code itself, which the program would then check before each ret or call instruction.
	4) This is hard to implement because again, we need to know beforehand where instructions are expected to ret/call, and if we have modular code, we can't really know that information, especially if we use dynamic libraries.
		1) Therefore, this technique is actually used alot in monolithic kernels and hypervisors where everything is statically linked.
