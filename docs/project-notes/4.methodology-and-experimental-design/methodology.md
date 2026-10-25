# Methodology

## 1. Research Design
The research here is primarily an experimental systems security study using dynamic program analysis. We are observing concrete executions rather than
statically considering every possible program behavior.

We can define our unit to be one execution of a C program containing a known heap UAF that is used either naturally or via user input that exploits that vulnerability.

The research itself can be broken into the following steps:
1) Given a program + a known UAF triggering input
2) We control the environment which the program runs in
    1) There should be 2 controlled variables for each experiment. The version of Glibc used, and the level of observation used.
3) Execute and analyze the program via our prototype
4) Make runtime observations
5) Classify those observations to make our final claims

## 2. Experimental Environment
The experimenetal environment will be ran on QEMU, `TODO: insert how I plan on using static QEMU files to have a consistent environemnt`.  A virtual machine is being used primarily for the benefits of rollbacking after conducting experiments, as well as keeping a consistent starting point. As our environment will not require internet connectivity, nor rely on internal Unix time, there should be no issue beginning experiments at a common starting point.

The architecture will be limited to x86-64. The purpose of this thesis is to explore glibc heap behavior, and supporting other architectures would introduce additional variables, pointer widths, and so on. Additionally, x86-64 is a standard, and actively supported architecture for our OS of choice.

The operating system will be Debian 13 `stable` with the v6.12 LTS kernel release. Debian 13 provides a slow changing and reproducible Linux environment, which will work well for our use. The v6.12 LTS release also provides us with little change to our environment, and is currently the release Debian 13 uses

## 3. Test Corpus

### 3.1 Controlled Synthetic Cases
Small, manually created C programs that either naturally contain a UAF or enable user input to cause a UAF bug.

There are two reasons behind synthetic cases:
1. We control the ground truth. Glibc's allocator has deterministic behavior if allocations/frees are controlled
    1. Ex. Say we use modern glibc, with default configurations, and perform  `p=malloc(32) -> free(p) -> q=malloc(32)`
        1. We can say deterministically that the `p` original allocation space will be reclaimed by `q`  via tcache  because there would be no other allocations to take up space in tcache except that most recently free'd one, and the size of the allocation fits within tcache's range.
2. General test suites do not take into account the allocator or specific cases
    1. Given this is a glibc-aware thesis, we can write test cases that explicitly take into account ptmalloc's behavior.
    2. Ex. If we want a testcase that utilizes fastbin, we can allocate and then free an allocation larger than tcache's supported size or overflows of an uncertain size.
        1. We can see this in HeapHopper's use of variable sized allocations + only accounting for 1 or 8 byte overwrites.

Our test cases will vary in:
1. Reclaimed vs Unreclaimed allocations
2. Partial, Full, or No overwrite on stale write
3. Partial, Full, or no stale read
4. Pointer arithmetic and provenance, or just straight pointer usage

### 3.2 Standardized Test Cases
Cases that were not designed with my tool in mind. The main source here will be the CWE-416 test cases from the Juliet test suite.

The CWE-416 C test cases all follow the general pattern of:
1. Create a pointer to some data type or struct
2. May or may not have a conditional data flow in between
3. Allocate an array of the type/struct, initialize with data, and then free it (if `bad` is set)
4. Pass the free'd pointer to a custom print function

Worth noting is that there aren't events specific to glibc that cause allocation reuse, or usage of any of the glibc allocator bins. However, there is pointer usage across files, and across functions, which do represent meta-level difference in UAF executions rather than differences in the code itself.

Therefore, at the most basic level, we can say that these test cases are to evaluate the the tool in regard to multi-file executions, pointer provenance, struct handling, and handling conditional code

We will use the following test cases:
1. `CWE416_Use_After_Free_malloc_free_char_01-18`
    1. These are the basic versions that contain simple conditional statements before the execution of buggy code
    2. I feel there is no need to also account for the `int`, `int64`, etc variants as they fundementally do the same thing, just with different types.
2. `CWE416_Use_After_Free_malloc_free_struct01-18`
    1. Cover the same thing as the `free_char` cases, but this explicitly goes over struct usage rather than simple data types
3. `CWE416_Use_After_Free_return_freed_ptr_01-18`
    1. These cases involve returning a free'd char array pointer
    2. This is useful for representing inter-function UAF executions
4. `CWE416_Use_After_Free_malloc_char_63a-b`
    1. This case involves passing a data pointer from one func to another func in a diff src file
5. `CWE416_Use_After_Free_malloc_char_64a-b`
    1. This case involves passing a void pointer from one func to another func in a diff src file

### 3.3 Direlct Exploitation Test Cases
So far, the previous test cases may or may not involve an input that directly transforms a memory corruption primitive into a larger primitive. Therefore, we will also employ test cases from CTF sources like how2heap and pwn.college. 

The test cases will cover the following techniques:
1. Tcache Poisoning
2. House of Lore
3. House of Storm
4. House of Water

These techniques use some form of UAF, allowing our model to characterize the execution above just a simple bug.

### 3.4 Production Code Benchmark Case
This will be a single test case from the NIST Software Analysis Tool Exposition VI. Basically, this test case will take real world code from some FOSS code base, and then manually inject bugs for the purpose of testing a static analysis tool. We'll be using testcase 501114. TODO: expand on this.

## 4. Formalization
As stated, we will account for single threaded, single process userspace C programs under a Linux glibc environment, with selected glibc versions. We will assume there is a known UAF triggering input, or the program naturally performs a UAF on its own via a programming error. We will cover stale reads and stale writes, and these would be conducted via direct pointers, copied aliases, or simply pointer arithmetic. Also, relcaimed, unreclaimed, partially reclaimed, and unknown states. Also, we account that one or more replacement allocations occupied the same range.

### Allocation Generation
An allocation generation is one observed lifetime of a heap allocation. It begins when an allocation function succesfully returns an address, and ends when a memory free'ing function, given the allocation address, returns. 
      
Given all of this, we need to track baseline the following information:
1. `generation_id`
    1. Global identifer for a successful allocation i.e. `malloc(20) at line 10 creates G15`
    2. generation ID's MUST NOT be tied to addresses due to allocation reuse
2. `user_base`
    1. Starting addr of usable data AKA the address returned to the application
3. `requested_size`
    1. Size passed to allocation function NOT true glibc chunk size or usable data
    2. In order to do byte level observation i.e. stale allocation requested 20 bytes, new one can only requested 18, we need the size of the data the user only requested. If we had something like `malloc(8), free, malloc(9)`, using true chunk size would return `0x20` for both
4. `allocation_event` and `free_event`
    1. Simply track that a malloc or free occured, with a global counter to indicate passage of time.
    2. This technique works because again, we are only using a single threaded single process program, so only one thread is creating allocations at a time.
    3. Practically, we'll use one event namespace, and then simply tie it to a specific generation. For example, `G15.alloc_event = E42`, `G15.free_event = E43`. This would make it simple to compare the events between generations.
5. `state`
    1. Simply track if allocation generation is live or expired

A new allocation gen is created only when a supported allocation function returns successfully. Deallocation ends a generation, but doesn't remove it's historical record. Internal movements like consolidation, chunk splitting, and bin movement don't affect generations, but do affect chunk history aka how glibc actually interacts with memory.

This isn't all though. Our model needs to be glibc aware, so we need to account for the following characteristics of glibc in how real memory chunks are handled, and how allocations are handled:
1. Chunk is reused via tcache/bins
    1. We cannot assume that the new allocation has access to the previous generation's usable space
    2. Ex. `malloc(10), free, then malloc(6)`: if tcache, we'd reuse the same 0x20 sized chunk, but clearly not have access to all of the stale chunk bytes
2. Free chunk can contain different types of metadata depending on which bin it ends up in
    1. We need to consider these special kinds of allocator derived writes into expired allocations
3. Consolidation can cause a reused chunk to be in the middle of a new allocation
    1. Ex. Consider we use 0x100, but the allocator decides to consolidate from 0x90 and onwards to return a large chunk allocation starting at 0x90. 0x100 was technically reused, because it comes ahead of 0x90 therefore reuse technically occured
4. The Wilderness
    1. When a program is fresh, it may directly carve out heap space from the wilderness, and then return that to the program. This doesn't involve any reuse
    2. However, if the program has gone on for some time, consolidation may have occured on previously used chunks, an allocation from the wilderness causes reuse.
5. Chunk Splitting
    1. If the allocator finds a chunk larger than the request, it will split this chunk, return the correctly sized one, and leave the other in a bin or the wilderness.
    2. This implies that a single generation may eventually overlap many later generations
6. Multiple expired allocations/generations can be replaced by one allocation; typically caused by consolidation.
7. mmap
    1. If allocated through mmap, a very large allocation may simply be `unmmaped`. NOTE: This is based on configuration, but we assume defaults
    2. It doesn't pass through the bins. 
8. Trimming
    1. This is where part of the wilderness is returned to the OS, if a process doesn't require it.
9. Overlapping Allocations
    1. Glibc by default cannot have overlapping allocations, but certain exploits do corrupt metadata such that this kind of state can be achieved
    2. Therefore, we can't assume every range of bytes can only have 1 owner at a given time.

Given this, we do need to track glibc chunk history along with the allocation history above. The allocator 

We can do this via the following records:
1. `chunk_state`
    1. This essentially tells us whether a chunk is inuse, or not. If not, what bin it's currently in, or if an mmap chunk, if its mapped/unmapped. Or if its part of the wilderness.
    2. This can be done by simply storing one of following:
        1. `allocated`: tells us if chunk is actively being used
        2. If free, then we would store one of the following: `tcache`, `fastbin`, `unsorted_bin`, `smallbin`, `largebin`, or `top`
            1. If we simply dont know, which is possible for certain exploits where we free/malloc a non heap chunk, `unknown` or `forged_corrupt`
2. `chunk_base`
3. `chunk_range`
4. `alloc_source`
    1. Each chunk could have been originally allocated via 3 sources: arena, mmap, or unknown.
5. `chunk_id`.
    1. An identifier for what glibc considered a single chunk.
    2. We can create incremental IDs since single threading ensures chunk events have a global execution order.
    3. `chunk_state` changing i.e. tcache to fastbin would not technically create a new chunk, so we would not create a new ID
    4. To handle splitting, we simply need to create 2 new IDs for both splits of the original chunk. For consolidation, simply create a single new ID for the new consolidated chunk
    5. For historical reasons, we dont want to simply discard the old IDs; we can use a directional graph, to indicate the flow of transformations of chunks.
    6. `created_event` and `ended_event`
        1. Like the events in the allocation history, we will store a continous history of chunks of when chunks are first created, and when they are ended.
        2. Creation includes events like wilderness carving, consolidation creating a new single chunk, and chunk splitting creating 2 new chunks
        3. Ending is the inverse, where the 2 consolidated chunks are now considered "ended", and the single split chunk being ended to create 2 new ones.
    7. `end_reason`
        1. This is just the reason a chunk ended. This inclides split, conslidated, absorbed into wilderness/top, or unmapped.

Lastly, to tie the glibc chunk tracking to the allocation history tracking, we will have field `chunk_generation_binding`, which will contain:
1. `chunk_id`
2. `generation_id`
3. `bind_event` and `unbind_event`
    1. A bind event occurs when an allcoation returns the thuink fo the applicaiton, and ends when the generation is freed.

With the purely runtime analysis, which relies mostly on allocator events and tracking any information gathered from that, we can establish allocation generations, ranges, state, and a timeline of those allocation's being created/killed. However, at this level we aren't able to note anything about how the allocator is treating/using the underlying chunks that make up these logical allocations. We only want to track and record these glibc specific notes only if there is sufficient evidence; otherwise, we just report unknown.

### Allocation Generation Provenance (pointer provenance kind of)
AGP is essentially asking what allocaiton generation did a pointer come from? In our case, each alloc gen gets it's own unique ID, so to simply handle AGP, we pass the AG ID to it.

We need AGP/PP because this allows us to observe fairly easily what pointer was used in the UAF, what generation is was meant to refer to, and what it ended up referring to.

What we store per-pointer via metadata is relatively small:
1. `allocation_gen`
    1. A relationship that states a pointer originates from some generation.
    2. This can be either `gen` for direct generation i.e. G1, `none` for NULL pointers, and `unknown` if we dont know where the ptr came from.
2. `addr`
    1. Since pointers can refer to the same allocation generation, but on different offsets, we can use the straight address to differentiate
  
Given our pointer metadata, the rules at which an allocation generation propogates across pointers is where we make our model glibc aware. Below, we consider the behavior of pointers, and then the rule that should be applied
1. Pointer creation through allocator function `a = malloc(20);`
    1. The pointer should recieve the allocation generation ID that is generated.
2. Pointer copy `b = a;`
    1. The copy should recieve the same AG ID of the original pointer
3. Pointer arithmetic/aliasing `c = a + 0x100;`
    1. The new pointer should recieve the same AG ID as the original pointer
4. Saving pointer in-memory `obj->ptr = a;`
    1. The field/destination pointer slot recieves the provenance of the source pointer.
5. Loading pointer from in-memory `b = obj->ptr;`
    1. The destination pointer recieves the provenance of the source memory slot  containing the pointer.
6. Integer Conversion `ptrtoint` or `inttoptr`
    1. On a pointer to integer conversion, which has existing metadta, we could just store shadow memory metadata about the resulting integer. Any transformation of the integer would then result in the integer to pointer conversion to recieve an `unknown` alloc generation.
    2. However, if the integer is the exact same value, and no changes/arithmetic were done on it, then we can preseve the allocation generation on the transformation back to pointer.
7. Memory copy `memcpy` or `memmove`
    1. Unsure on how to handle
8. Pointer movement: this can happen in many ways so below are different examples with their rules
    1. Pointer passed as func arg: maintain provenance across the passing
    2. Return pointer as func ret: maintain provenance
9.  Pointer casting:
    1.  This should preserve the provenance of the original type.
10.  Failed allocation `a = malloc(2); //fails`
    11.  We would give it the `none` value for generation since it recieved none.
11. Pointer free `free(a);`
    1. In this case, we would actually keep the generation of the pointer intact, and leave the address alone as well.
  
Given this, we can now define a few terms:
1. Stale Pointer
    1. Given a pointer X, which stores the allocation generation ID of allocation A, X is considered stale if and only if A has been free'd/is a dead allocation, regardless of whether the address X points to is currently in-use or not.
2. Stale Access
    1. Given a stale pointer X, which stores the AG ID of allocation A, a stale access occurs when X is dereferenced either for the purpose of a write or read, regardless of whether the memory which once stored allocation A is in-use or not.

### Range Definitions
Now we need to formalize what range is in this context. Due to the fact that allocations are ephemeral, we can think of range as having three properties:
1. Boundaries: the actual space the object takes up with a start and end address.
2. Meaning: what do these bytes actually belong to? are they allocated bytes, top chunk bytes that belong to the allocator? are they accessed via a read or write?
3. Time: Are these bytes dynamic in their ownership, or are they static during execution?

We'll represent ranges half-open ranges i.e. `[)` since generally the first address aka the base is the first usable address in a pointer. And since systems are zero indexed, we do not include the address as usable or "in range" at `base + range`. For example `a = malloc(10);` returns address 0x100, so the range is `(0x100 

Given these rules, we can define different types of ranges:
1. Generation range
    1. Boundary: We can formally define this as `[base, base + requested_bytes)`
        1. Ex. `malloc(10)` returns addr `0x100`, we consider that generations range to be `0x100 - 0x109` since recall, the returned address is the first byte in the application requested bytes.
    2. Meaning:  This range indicates what bytes belong to the assigned allocation generation.
    3. Time: A generation's range remains static, as allocations cannot be resized in glibc (realloc technically creates a new allocation). Once stale, that range simply becomes historical, but doesn't change.
2. Chunk range
    1. Boundary: Formally define as `[chunk_base, chunk_base + chunk_size)`
    2. Menaing: This range indicates the underlying bytes that make up a chunk. 
    3. Time: A chunk's range remains static; if that chunk is split or consolidated, we store the curr chunk's range as historical, and then start maintaining the range of the new chunk(s).
3. Access range
    1. Boundary: Formally define as `[ptr_base, base + access_width)`
        1. The `access_width` is based on the actual width of the x86_64 operation, or if a glibc function, the parameters passed to it.
    2. Meaning: This range defines what bytes were accessed in a stale access, whether it be a read or write. Knowing this range allows us to determine what information may have been inadvertently read or leaked via a read, or what may have been arbitrarily written somewhere via a write
    3. Time: This range doesn't really change overtime. Once an access occured, the range of that acces cannot be changed. Further acceses may have different ranges, but that i'th one stays static.
4. Allocator metadata write ranges
    1. Boundary: We can define as `[access_base, access_base + metadata_range)`
        2. Note, this range may overlap two disjoint ranges; this is because glibc may move a chunk to another bin, meaning the associated chunk gets new metadata fields, therefore changing the range. we still want to keep the original historical one, but also account for the new one.
    2. Meaning: These ranges record where glibc wrote metadata at a particular event.
    3. Time: This range of the metadata can remain unchanged depending on how the state of the chunk changes. For example, moving from tcache to fastbin doesn't actually introduce any new metadata writes, tehrefore the range stays the same; the same doesnt apply to tcache to largebin for example.
5. Input derived ranges
    1. Boundary: We can define as `[base, base+input_derived_bytes_count)`
    2. Meaning: This range tracks what range of bytes contain data that came from the attacker. We assume that the UAF triggering input comes through stdin, so we can directly map where those bytes are in memory
    3. Time: Copies create new ranges, while overwrites remove them.

Given the defines ranges, we now need to consider how two ranges can relate at any given time:
1. No overlap
    1. This implies that the boundaries of both ranges do not intersect, or share any common bytes.
2. Partial Overlap
    1. Given range A and B, A's end boundary ends beyond B's start boundary. We assume this because overlaps generally occur with adjacent bytes.
3. Exact Range
    1. Given range A and B, their start and end boundary are exactly the same, and go over the same bytes.
4. One contains another
    1. Given range A and B, if we assume range A is the larger one, all bytes in range B are also in the range of A.
    2. More formally, A.start <= B.start AND B.end <= A.end

### Immediate Effect
Given we track attacker derived data and/or program data reused by the attacker, we need to essentially perform taint analysis. This means, if the attacker used a stale pointer, we need to track what memory addresses that data.We can define effect as:
1. `origin`
    1. Where did the data come from?
    2. On a stale read, this data may have came from a replacement allocation that reused the same address.
    3. On a stale write, this data may have come from existing memory or passed through a STDIN-read function.
2. `destination`
    1. Now, what location was directly affected by the access?
    2. On a read, this means an inintended value was accessed, and then stored somewhere where it does not belong
    3. On a write, memory was overwritten with an unintended value
3. `usage`
    1. Refers to the immediate use of the `dest`
    2. This could be through calling a function, using the value in a conditional, or propogating it via memory copies or movement.

## 5. Instrumented Observation
Instrumentation will be conducted at the intermediate representation level using LLVM instrumentation and passes. Because one of our core questions is in regard to different levels of observation, we will simply do a single pass of the target program, and then remove information from there. This info will then be passed to our classification algorithm.

The overall implementation has three aspects to it:
1. LLVM instrumentation level
    1. Here, we write our passes, and then insert them into the target binary. This simply dictates what to observe
    2. I.e. LLVM looks for calls to `malloc`, and inserts calls to functions we define, called passes.
    3. Note: LLVM only does work once. that work is inserting calls to our runtime functions.
2. Runtime Level
    1. This is a seperate program compiled with the LLVM instrumented binary, but not written with LLVM.
    2. As the instrumented program executes, it will execute those LLVM instrumented calls to the runtime level functions.
    3. It actually maintains shadow memory, keeps track of information, etc.

## 6. Runtime Analysis and Classification

After we've executed the program, we need to take the measurements we've made, and run them through two algorithms in real time:
1. Analysis Algorithm
2. Classification Algorithm

I go over these in the next section.

## 7. Experiment Variables

### Controled Variables

1. Architecture: x86-6
2. OS: Debian 13
3. Glibc Version: 2.42
4. Program Input (we assume an existing UAF input)
5. Levels of Observation
6. glibc allocator configuration (tuning)

### Varied Variables
1. UAF test case
2. Replacement allocation behavior
3. Attacker data placement
4. glibc configuration (while also a controlled variable, we will vary between both versions)
5. instrumented vs native execution (to determine purturbation)

## 8. Repetitexperiementsion of Experiments

Because certain heap bbehaviorehavior isn't deterministic, we will rely on more than one run for eexperiementsxperiements. Rather than recording absolute values, we will store statistical data. For example, rather than storing the absolute address used for some allocation, we will record same address reuse frequency.  

## 9. Evaluation

We will need a ground truth to determine how accurate our findings are. This can easily be done with our synthetic test cases, where we take advantage of the deterministic components of the heap to use as a baseline. For example, a program like:
```c
a = malloc(x)
free(a)
b = malloc(x)
read(input, b, 16)
stale_read(a + 4, 8)
```

We know for a fact that A is expired, and B is the replacement. By performing a stale read with A, we can record that 8 stale bytes reach B, and are attacker derived.

Given our test cases, we can fill a table indicating how accurate our different configurations of the tool was. Recall that the independent variables for each run are (NOTE: WE CHANGE ONE ONE AT A TIME FOR EACH RUN):
1. glibc version
2. Observation level

This will give us a very objective look at the accuracy of our tool, since we will be using raw values.

In addition to simply using empirical data to determine accuracy, we will also consider possible threats to validity, as well as reliability:
1. Version Sensitivity: 
    1. Our algorithm and implementation will primarly be designed around the known behavior of the glibc allocator; particularly the newest version 2.41.
    2. Despite this, the general algorithm should give accurate results, regardless of the version we are using for glibc, as while the answers may change, they change because of something directly related to the differing version of allocator.
    3. So, we are using 2.32 as a mature, but pre-safe linking (one of the most important safety features in glibc alloc) as this version is likely to result in different results, but we need to ensure they are still correct, given how that version handles reuse, bin selection, etc.
2. Purturbation:
    1. Research question 1 goes a bit into this, but clearly, each level will require a different technology to capture the required data i.e. LLVM, LD_PRELOAD, observers,etc. As seen with ASan, these observation techniques can directly modify true, clean program execution, which is what we use in production.
    2. Therefore, to enforce the validity of our results, we need to verify that for each level, the components used are not directly responsible for a mis-classification, incorrect operation, etc.
    3. For example, using a non-signal safe function in an LD_PRELOAD wrapper can cause the heap to be prematurely warmed up as the program first calls malloc().
3. Repeatability:
    1. Clearly, we want our results to be consistent across executions.
    2. We can run each test case N times, under the same program, input, glibc vers, and observation level
    3. Then, we can report whether classification fields remain stable across runs, i.e. reclemation state, replacement obj identifty, etc.

Given that these used to be research questions, but I decided that they arent really directly related to my primary research question, I will add the notes downhere as an "archive" for refrence of what I deemed necessary to answer these questions:
```
### RQ4: How repeatable are observed results and classifications?

#### General Idea
Simply, for whatever our framework outputs for program A, input B, glibc version C, and level of observation D, do we get the same results if we run the test N times? 

#### Required Observations
Results of classification and analysis algorithm after N runs of the program. Additionally, the variables we control such as program, input, glibc version, and the level of observation used.

#### Measurements
Compare the results of the N runs to determine how
repeatable the same result is under the same conditions.

Defined as $\frac{\text{\# of same results}}{N}$ for each result/classification

#### Ground Truth
We define a threshold for whether certain results can be
repeated with certainty for the result of the definition above. 

While not defined now, an example could be: 

After having ran program X under configuration Z 30 times, the classification was "PWN" 25 times, meaning its repeatable ~83% of the time, we cannot confidently say that the results of this environment is reproducible because it is under our threshhold of ≤ 98%


#### Reported Result
True or False for whether a specific observed
result/classification can be reliably recreated.

### RQ5: How do selected glibc versions affect results and classifications?

#### General Idea
Given the same program, input, analysis, but with two versions of glibc, what changed in the observed UAF execution? We'll use the fields from RQ1-3 to compare. If there are differences, we can't consider them incorrect because simply, the heap layout will differ between versions.

#### Required Observations
The output results from RQ1-3, but ran on different glibc versions.

Addiitonally, we need to observe changes in the allocator behavior itself.

#### Measurements
This will be a table containing each result + classification between both versions of glibc on the same test program, highlighting whereever differences occur.

Additionally, we will highlight the unique behaviors of both versions, and mark whether that key behavior did or did not effect the results.

#### Ground Truth
A ground truth can't really exists until we compare the results. For each field that was different, we will have to examine the most likely reason why certain fields were different between versions. We can then simply look at the unique allocator behavior in the glibc versions that caused the differences, and write those as our ground truth.

#### Expected Result
Based on what is differing for both tests, the expected result will be an analysis as to why a feature of version X caused measurement Y or classification Z to be incorrect.

### RQ6: How much does each analysis component alter the heap behavior being measured

#### General Idea
We are intererested in determining in what ways the heap itself is changed through our observation mechanisms. Because we have 3 levels of observation, each one will require a different technology i.e. LD_PRELOAD, LLVM instrumentation, etc. So, for each of those levels, using no instrumentation as a baseline, what concrete observations can we make that end up altering natural heap behavior?

#### Required Observations
alloc/free/reuse events and layout of the heap itself (could be obtained via GDB)

#### Measurements
We only want to measure relevant behaviors to our analysis. So this includes things like:
1. Allocation count before stale access (perhaps reuse will happen more aggressively)
2. Free order
3. whether the freed object is reused
4. which allocation replaces it (generation)
5. Replacement range/size
6. owner_at_access_map
7. tcache/bin destination, if observable
8. program abort/crash before stale access

We will measure these for each level of observation, using the modern baseline glibc version, since this question is more about a general purturbance.

#### Ground Truth
The program's natural, repeatable heap behavior upon executing the UAF input without any analysis components. This would have to be gathered manually via GDB.

#### Expected Result
This should be an explanation/analysis as to how each different level of observation modifies the way the heap operates.

For example, if for each store operation, we instrument with a call to a heap modifying function like `printf`, doing this causes tcache to be prefilled with unrelated chunks, that wouldn't have been there in a clean run.
```