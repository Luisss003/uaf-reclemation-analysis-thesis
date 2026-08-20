# Research Questions

## Primary Research Question

To what extent can a glibc aware runtime analysis characterize observed heap
reclamation and its consequences following use-after-free?

## Secondary Research Questions

### RQ1: What runtime evidence is required at a minimum to classify an observed UAF execution?

#### General Idea
Because we are able to measure/observe at different levels, what are the minimum components we should be observing/tracking while keeping our classification of the type of UAF performed accurate? We want minimum because the more we track, the heavier the framework becomes. 

#### Required Observations
All heap/allocator events, generations, reuses, etc that can be measured during
a programs execution.

#### Measurement
For each level, which is TBD, of required observations, we will compute a
classification of a UAF execution. For each level, the number of components
measured increases.

In order to classify a UAF execution, we will use the following dimensions:
1. Reclaimed/Unreclaimed
2. Operation (write or read)
3. TBD

#### Ground Truth
For a ground truth, we will have to manually construct a test program where we
can manually classify the UAF executed, and then test the result of the
algorithm.

#### Reported Result
We should recieve a tuple of dimensions which we deem at a minimum are required
to classify a UAF with certainty 100% of the time.

### RQ2: Can allocation generation tracking and pointer provenance tracking correctly associate a stale access with the the participating objects and/or generations?

#### General Idea
With all the measurements we plan to keep, which in a raw state, seem to be unrelated, can we say for certain that some pointer is associated with this generation of objects, what objects it can reach, etc. Generally, with our measurements, can we really create a "story".

#### Required Observations
Pointer origin, allcoation generation IDs, free events, reuse events, access address/range.

#### Measurement
Compare the generations identified by the analysis with the
known participating generations and objects.

#### Ground truth
manually constructed or otherwise known allocation/free/reuse
relationships for the test case

#### Reported result
Proportion of stale accesses assocaited with correct
generations

### RQ3: What conditions determine whether attacker derived data in a replacement allocation influences a subsequent stale access?

#### General Idea
When does attacker supplied data placed into the replacement object actually become relevant to the later UAF? Like for example, say we have the sequence: 
```
malloc(A)
free(A)
malloc(B) reuses A's memory

attacker data enters B

stale pointer to A is used by the program for whatever reason
```
For attacker supplied data to actually influence the stale access, or "be useful" there have to be some assumed conditions. Thats the objective of this RQ, but some general ones would be:
1. The memory must be reclaimed (the replacement alloc B needs to reuse at least part of A's old address range)
2. Attacker derived bytes actually have to align with B's structure in a useful manner; otherwise it could work with corrupted data, which just leads to a non exploitable state (recall weird machines)
3. The stale access actually has to overlap with where we put the attacker bytes. If the attacker bytes are at ranges [23-30], but the read only goes to [20-22], then the attacker doesnt gain anything.
 
#### Required Observations
access address/range, original bytes at reuse location, attacker derived bytes at reuse location, object generations, structures (if applicable) to old and new objects at range

#### Measurements
Because there are different classifications of UAF, for each one, we will need to observe the state the heap is in + its relationships just before a successful, or failing UAF exploit occurs. Then, we can just note down what states/conditions were required for this specific test run.

#### Ground truth
The GT would simply be the answer to our original question: did the attacker derived bytes in the replacement allocation actually even influence the stale access, and if so, which bytes?

#### Reported Result
List of conditions/measurements that were most commonly required for a successful influential attack.

### RQ4: What concrete runtime consequence follows from stale accesses to reclaimed memory?

#### General Idea
So we've measured different components at runtime, such as the type of UAF performed, reuse of chunks, etc, but now what actually happened because of that use? This is where we would classify that the outcome of of a sequence of events and measurements is X.

#### Required Observations
access address/rage, overall program state after
the access i.e. condition flags in the program, addresses/values stored.

#### Measurements
We need to classify potential outcomes (since there can be more than one). We can support ones, but not limited to:
1. Incorrect value propogation
2. Pointer corruption -> Use
5. Information disclosure

#### Ground truth
Our GT would simply be test cases where we know the outcome of a UAF triggering input. Like, the objective of the input was to overwrite a pointer in memory, therefore for this test case in particular, any classification other than pointer corruption -> use would be wrong.

#### Reported Result
The RR would be a number of cases classified correctly.

### RQ5: How repeatable are observed results and classifications?

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
```
After having ran program X under configuration Z 30 times, the classification was "PWN" 25 times, meaning its repeatable ~83% of the time, we cannot confidently say that the results of this environment is reproducible because it is under our threshhold of ≤ 98%
```

#### Reported Result
True or False for whether a specific observed
result/classification can be reliably recreated.

### RQ6: How do selected glibc versions affect results and classifications?

#### General Idea
Our two selected versions (2.31 and 2.41) both contain mature versions of the glibc allocator + the important tcache mechanism. However, 2.31 contains the version before the implementation of safe linking, which is an important safety mechanism for preventing simple overwriting of the vulnerable `next` pointer for each chunk. 2.41 acts as a modern baseline which is much stronger than 2.31, so the idea is to check how both versions, with their differing levels of security checking, modify the results and classifications our algorithm makes.

#### Required Observations
Classification results + measurements with the same test cases and algorithm across both glibc versions used. 

Addiitonally, we need to observe changes in the allocator behavior itself, which I go into more detail in RQ7 (i wrote RQ7 before this one, which is why this RQ refers a future one)

#### Measurements
This will be a table containing each result + classification between both versions of glibc on the same test program, highlighting whereever differences occur.

Additionally, we will highlight the unique behaviors of both versions, and mark whether that key behavior did or did not effect the results.

#### Ground Truth
There must be a simple test program that has known results  + classification to determine in what ways each version causes a differing result/classification. There really isn't a "bad" result in this case, since we cannot expect two different versions of an allocator to behave the same way.

#### Expected Result
Based on what is differing for both tests, the expected result will be an analysis as to why a feature of version X caused measurement Y or classification Z to be incorrect.

### RQ7: How much does each analysis component alter the heap behavior being measured

#### General Idea
We are intererested in determining in what ways the heap itself is changed through our observation mechanisms. This isn't related to the algorithm/classifications/relationships itself, but rather things like what bins become used (tcache,fastbin,etc), how arena behavior is changed, and possible benchmarks, since a key measurement is the effect frameworks like ours have on runtime/memory consumption.

#### Required Observations
alloc/free/reuse events and layout of the heap itself (could be obtained via GDB)

#### Measurements
For a program X, we can declare "key points" such as calls to `malloc` or `free` and then measure the allocator behavior. This behavior includes, but is not limited to:
1. What is the true size of the allocation?
2. Upon being free'd what bin did the chunk end up in?
3. Upon a next allocation, will we get a reused or fresh chunk? if reused, from what bin.
4. Are additional arenas created?

#### Ground Truth
The program's natural, repeatable heap behavior upon executing the UAF input without any analysis components. This would have to be gathered manually via GDB.

#### Expected Result
This should be an explanation/analysis as to how each different level of observation modifies the way the heap operates.

For example, if for each store operation, we instrument with a call to a heap modifying function like `printf`, doing this causes tcache to be prefilled with unrelated chunks, that wouldn't have been there in a clean run.
