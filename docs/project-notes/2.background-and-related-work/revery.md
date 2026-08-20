## Revery

### Terminology

1) Crashing Path: path taken by PoC input to crash
2) Crashing Point: instruct whrer program crashes/security violation caught by sanitizers
3) Vulnerability Point: instruct wehre vuln happens
4) Exploitable point: instruct which could lead to successful exploit
5) diverging path: path were explotable states could be found
6) hijacking point: instruct where the control flow could be hijacked
7) exploitable path: path taklen by successful exploit
8) stitching points: special instructs in the diverging path and crashing path, which could be stitched together to generate exploitable path.
9) ### Section 1: Introduction
   
   A PoC input provers a vuln can be triggered but the particular execution path and corruption state produced may not be exploitable. The PoC can:
10) take a branch that only causes a crash
11) Corrupt data into unusable state
12) Trigger allocator sanity check, and more

So, crash point analysis alone isn't enough for exploitability asessmenet of a PoC. 

Existing AEG solutions struggle with:

1) Exploit deliverability
2) Symbolic execution path finding isn't scalable
3) Heap implementation is too complex for typical AEG

Given a binary and PoC input that causes a crash, Revery:

1) Analyzes the crashing execution and extracts the relevant corrupted object layout
2) uses layout-oriented fuzzing to search for diverging paths that construct a similar layout and contain exploitable operations;
3) stitches the vulnerability-triggering path with the useful diverging path;
4) uses lightweight symbolic execution to synthesize a new EXP input;
5) sometimes extends that input into a working control-flow-hijacking exploit.

Core Insight: We don't have to rely on the original control flow path used in a PoC input. If we can follow the same milestone beats of a PoC input's CFG, but diverge in different manners, we can exploit the binary with this input more effectively. Memory layout is more important that the individual instructions the exploit takes.

Thesis Relevance: My thesis does not solve exploit derivability and does not search diverging paths. Revery is relevant because it establishes that:

1) a PoC represents only one observed vulnerability state
2) heap-object identity must be tracked separately from raw addresses;
3) freed storage may later belong to a replacement object;
4) corrupted or replacement derived data becomes meaningful only when connected to a sensitive later use.

My thesis should therefore report observed reclamation evidence, not claim complete exploitability or non-exploitability.

### How Revery Works

1) Analyze What created the vulnerable heap state
   1) Run the binary with the supplied PoC input, and perform dynamic analysis.
   2) Each heap alloc gets a unique tag, and pointers derived of that alloc get the same tag. Heap objs also get a lifetime status i.e. uninit, busy, free. With this, we can determine if a mem access is suspicious i.e. pointer with tag A is accessing obj with tag B.
   3) There is also the concept of exceptional objects, which are objects whose corruption matters aka leads to an exploit. Ex. the exception obj in a UaF exploit would be the obj that is stale, but is still referd to by a stale ptr.
   4) We also store indexing objs, which are objs/ptrs that rrefrence the exceptional objects.
   5) Given the indexing and exceptional objs, we can then create a layout contributor digraph where nodes are exceptional and indexing objs, and edges are the pointer relationships for accessing the exceptional obj.
   6) This graph is then used to extract important instructions that cause the conenctions to exist.
2) Search for Another Path with Similar Layout
   1) Now, we have our contributor digraph that lists out ways we can reach an exceptional objects, and the instructions used to get there (layout contrib instructs)
   2) Traditional AEG uses symbolic execution, but here we use layout oriented fuzzing.
   3) This type of fuzzing aims to hit/reach the layout contrib instructs and ignores most other instructs. The goal isnt to reach the final crash state, but rather just find a path that gives us the exact flow of contrib instructions we need.
   4) This essentially allows the fuzzer to branch and explore unseen paths, but still follow the same overall milestones. 
   5) Now, simply hitting the right milestone contrib instructions doesn't guarantee that an exploit will work because memory could start off at different layouts. Therefore, for each diverging path we take, we reconstruct the layout contributor graph, and update our contrib instruct list, compare it with the graph obtained from the PoC input, and if we find that this trial divergent path doesnt match up with the PoC input, discard this attempt entirely.
3) Stitch vuln path and useful path together
   1) Assuming a diverging path found reaches a useful operation + follows the milestone contrib instructions we need.
   2) This essentially gives us evidence that under thisnobject layout, there exist a control flow suffix that performs an exploitable oepration. 
   3) Now we just switch together the inputs that do both trigguring the vulnerability, as well as following the divergent path we discovered via fuzzing. 
      
      ### Section 2: Motivating Example
      
      #### Purpose
      
      Show a simple heap overflow example to establish:
4) Why a PoCs crashing path may not contain useful exploitation primitive
5) What revery means by exploit deliverability
6) How its 3 stage pipeline addresses the problem
   
   #### Mechanism
   
   Consider:
   ![[Pasted image 20260731151651.png]]
7) Obj1 and obj2 are allocated consecutively, so likely to be adjacent in heap.
8) If we can cause condition on line 9 = True, we can overwrite from obj1 into obj2's heap data.
9) Line 12 can cause a crashing point, or simply normal execution with an incorrect value. It isn't really exploitable UNLESS `res` is used effects control flow in the caller of `foo`.
10) Line 14 is more valuable, since it overwrites an address with an attcker controlled input, and is later used in the global handler at line 15.

Now on Exploit Deliverability:

1) A blind fuzzer can find bugs/crashes in this code easily, but will lead us to a weird machine state that isn't exploitable. i.e. using line 11 to overwrite with junk, causes a crash, but this produced state does nothing for exploitability.
2) Therefore, we need to consider different paths, and bring together stitching points for a succesful exploit.
3) We need to search for exploitable states in diverging paths; otherwise, we can meet crashing points, but they wil never have the deliverability required for an exploit, as stated in the line 11 example.

The key insight here is that we need to handle both vulnerability AND exploitability as seperate issues which we then stitch together to create a final exploit input.

#### Important Distinction/Limitation

#### Relevance to Thesis

### Section 3: Vulnerability Analysis (important)

#### Purpose

Revery needs to identify vulnerability points and track program state since it's method of determining exploitability is via analysis of the processes layout.

#### Mechanism

First need a method to detect vulnerabilities in code:

1) ASan/Valgrind work but are intrusive (modify true mem layout)
2) Solution: special form of memory tagging, which means to mark/tag a memory pointer and region
   1) To make it non-perturbable, use shadow mem to track trags of ptrs and heap objs.
   2) Also track status of heap objs to support temporal and spatial vuln detection.
3) Now, just use these tags to check each memory usage:
   1) Each time malloc or store/loads are performed, pass the request to shadow mem, which if correct, passes the refrence to real memory
   2) i.e. if ptr accesses obj of different tags or invalid status, throw a security violation

Now to actually implement memory tags non-invasively:

1) Each heap obj + ptr is associated with a memory tag, indicating lineage.
2) Uniquely created per obj, and is propogated to related ptrs of this obj; acts as taint label.
3) Each obj is also associated with status i.e. uninitialized, busy, free
   1) Status accounts for chunk reuse i.e. free -> busy, as well as unused to busy 
4) Certain special heap functions use pointer arithmetic to check other heap objets i.e. allocator uses ptr - offset to check the `prev_used` bit in another chunk )
   1) Therefore, we don't propogate flags in this case, since it would cause red flags.
5) We also track all indexing objects

Okay now we have the pointers and objects tagged and status; how to validate?:

1) For each heap mem access, get ptrs tag and target memory region tag + its status. The access must not violate:
   1) Instruct should only access intended obj i.e. `tag_obj` == `tag_ptr`
   2) load instructs shouldn't acess freed or uninit mem i.e. `status_obj == busy`
   3) store instructs shouldn't access free mem i.e. `status_obj == busy || unint`
2) With these 3 rules, we can handle all memory vulnerabilities.

Now, we need to consider heap objects as entities rather than just addresses:

1) All heap based vulns are realted to one exceptional object. There are two main cases:
   1) Write Vulnerability: 
      1) If a bad ptr writes to another heap obj, the target obj `tag_obj` is the exceptional obj
      2) This is because the write directly corrupts the target obj
      3) Ex. ptr with tag A writes into obj with Tag B, so B is exceptional obj
   2) UAF Read Vulnerability
      1) The old freed obj assocaited with `tag_ptr` is the exceptional obj
      2) NOTE: the dangling ptr itself isn't the exceptional obj. its tag just identifies the old obj with originall pointed to
      3) Ex:
         1) Obj A allocated -> ptr `p` gets tag A
         2) A is freed, but `p` still has tag A
         3) ObjB is allocated over the same mem, so that memory now has tag B
         4) `p` reads this mem, so we have a `tag_ptr=A, tag_obj=B`
         5) So, A is the exceptional obj, since A is the old freed obj whose mem has now been replaced by B
      4) So for UAF reads, new allocs can be thought of as corrupting/replacing the exceptional objs old memory
   3) Revery can't handle other types of invalid read accesses; this exceptional obj model is for UAF reads.

Now what counts as vulnerable code?:

1) As stated, we care most about the layout in memory. we need our weird machine to enter a state that has a matching memory layout to the memory layout of the vulnerability state via those contributing instructions
2) There are two important instruct types:
   1) Memory allocator instructions
   2) Store operations that assign objects field with a pointer to another obj
3) The pointers and objects created are how we create our digraph
   1) Each exceptional obj and indexing obj are nodes
   2) the edges are points-to relationships between 2 objects
4) We use backwards tracing to recreate the isntructions required to get the desired memory layout via the digraph representation:

#### Section 4.4 Exploitable State Searching

The key takeaway here is that a corrupted/reclaimed obj isn't automatically useful for exploits. We need to ask whether that obj influences some sensitive operation.

We could have diverging paths with the same memory layout as a vulnerability, but if the exceptional object of this path doesn't interact with something a sensitive operation, its useless, so this is another way to filter out paths to consider.

The paper gives two examples:

1) Memory write i.e. a path and its expcetional obj are only worth considering if that obj is used in a memory write which can cause a AAW vulnerability i.e. `*p = value` where p is attacker controlled
2) Indirect call i.e. does the exceptional obj influence where control flow goes? this would be the case if the exceptional object influences the call target such as containing the function pointer.

Therefore, a key step is to identify sensitive instructs whose operands are affected by exceptional objects; Revery just uses taint analysis:

1) Mark each obj creation operation as a taint source, then attach a unique taint label to it
2) Each operation propogates all source operands taint labels to the destination.
   

For example:

```c
x = exceptional_obj->field
p = x + offset
*p = 123
```



Ends up as:

```C
x = exceptional_obj->field
p = x + offset
*p = 123

Ends up as:
exceptional_obj->field   [taint E]
          ↓
x                        [taint E]
          ↓
p                        [taint E]
          ↓
*p = 123
 ^
 destination address has taint E
```

3) Now at each sensitive instruct call, the target addr taint labels will be checked if they contain the excpetional objects taint labels
       1) If so, we consider this instruction to be exploitable.

### Section 6.2.2-3 Exploitable State, but No Exploit

Revery sometimes found exploitable states + a stitched exploitation path, but could not exploit the program.

This is because the programs contained no critical data fields like Vtable pointers/ function pointers in the exceptional object. Instead, it found that corrupted heap allocator metadata could lead to an unlink attack, but this required extra manual setup to achieve.

This is important since this shows that in my thesis, a UAF giving us a stale read/write over a straight up attacker byte replacement is still meaningful evidence. It jsut requires the attacker to do a little bit more work.

Sometimes Revery also fails with detection of certain types of vulnerabilities:

1) It can detect buffer overflows that overlap into other memory objects, but wont detect struct/object field overflow.
