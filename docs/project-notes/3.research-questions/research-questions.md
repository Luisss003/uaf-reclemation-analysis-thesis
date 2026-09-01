# Research Questions

## Primary Research Question

To what extent can a glibc aware runtime analysis characterize observed heap
reclamation and its consequences following use-after-free?

## Secondary Research Questions
 
### RQ1: How does increasing runtime observation detail affect the correctness and completeness of stale access classification in known UAF execution?

#### General Idea
We understand that the more data we observe, the more correct and complete the classification will be of a UAF execution. So, we want to know the "why" behind that. What detriment does the lack of some info have that can cause the classification to be unsure, or even incorrect? And by adding that info, why does the classifier now have the ability to answer the question more correctly?

#### Required Observations
We will require the observations for each specified level we define in `Measurement`. This is because each level observes more than the last one.

#### Measurement
First, we must define our output fields for the classifier. This is simply one specific fact our tool claims about one observed UAF execution:
1. `stale_pointer_generation`: the generation the dangling pointer was derived from
2. `stale_access_op`: was the dangling ptr used for a read or write?
3. `stale_access_range`: what bytes did the stale access touch?
4. `expired_object`: the object that no longer is alive, but the dangling pointer still points to
5. `expired_object_range`: the address space on heap that original object used
6. `owner_at_access_map`: at time of stale access, a set list for who owned the byte range accessed (since recall acceses can leak into unallocated/unowned bytes).
7. `reclamation_state`: we can derive this from `owner_at_access`; either `unreclaimed`, `reclaimed`, or `partially_reclaimed`.
8. `replacement_obj`: if reclamation of an addr occured, what object replaced it?

For these 8 output fields, we need evidence to support the claim made by the classifier. These are the types of evidence:
1. (AE) Allocator Events: whatever we can gague for calls to `malloc`, `free`, `calloc`, etc.
2. (MAE) Memory Access Events: These are the actual isntructions that work with memory i.e. `x = *p`, `p->field = v`, `memcpy(dst, src, n)`
3. (PP) Pointer Provenance: where a pointer a came from and how it moved
4. We can generate two pieces of additional evidence from the main 3:
    1. (TO) Temporal Ordering: this is simply the order at which "events" occured
    2. (RO) Range/Overlaps: the potential overlaps in allocations, and whether the stale access range overlap an expired objs old range.

For our different types of evidence, here are the output fields that rely on them:
1. `stale_pointer_generation`: AE, PP, MAE
2. `stale_access_op`: MAE
3. `stale_access_range`: MAE
4. `expired_object`: AE, PP, TO
5. `expired_object_range`: AE, PP
6. `owner_at_access_map`: AE, TO, MAE, RO
7. `reclamation_state`: AE, TO, PP, MAE, RO
8. `replacement_obj`: AE, TO, PP, MAE, RO

Now, we need to define the levels of observation we want to work with:
1. Level A: Allocator Events Only
    1. We begin with AE's first because fundementally, we need to know at a minimum about allocation generations. That is the purpose of this paper.
2. Level B: Allocator Events + Memory Access Events, which has byproduct of Temporal Ordering and Range/Overlaps
    1. We add MAE next because allocator events are the best natural progression. They can tell us of a UAF's existence, but not much else. This gap is filled at the next level
3. Level C: AE + MAE + PP + Byproducts
    1. Finally, we can understand how each generation and address range changed, and interacted with each other 

#### Ground Truth
We will have X manually constructed synthetic cases where we know the true allocation, free, reuse, stale pointer, access range, and operation. From there, we can simply compare the output of each level of observation with the ground truth.

#### Reported Result
For our three levels, we must mark `known`, `unknown`, `wrong` for the 8 fields they output. From here, we need to analyze and write down what each previous level was missing, and why it led to a unknown or wrong output.

### RQ2: When a stale read targets reclaimed heap memory, how much of the stale read range overlaps the STDIN derived data in the replacement object?

#### General Idea
First, let's define attacker derived data as coming strictly through STDIN. So, lets say a stale read occured. Now how much of the attacker derived bytes were actually touched through that read? And for partial and full touches of those bytes, what are some possible reasons why some failed/partially failed/succeded? We don't care about how those read bytes later influence exploitation, just the access itself.
 
#### Required Observations
Stale read range and replacement object range, and STDIN derived byte ranges inside the replacement object.

#### Measurements
For one stale read to a replacement object, we need 3 ranges:
1. `stale_access_range`: the range at which the stale access touched
2. `replacement_object_range`: the range the replacement heap object takes up
3. `stdin_derived_ranges_in_replacement`: the range within the object of the attacker's STDIN bytes

 Given this, we can simply compute X as $(\text{stale\_access\_range} \cap \text{stdin\_derived\_ranges\_in\_replacement})$ and reports bytes/ratio.

 We except there to be 3 possible outcomes:
 1. `full_read`: all STDIN derived bytes were accessed
 2. `partial`: only some of the STDIN derived bytes were accessed
 3. `miss`: none of the STDIN derived bytes were accessed

Each of these can be easily proven via a simple intervals algorithm.

Now when it comes to conditions that lean torwards why a UAF execution ended up in a certain case, 

#### Ground truth
The GT would be synthetic test cases where we know:
1. Stale read range
2. Replacement Object range
3. STDIN bytes range
4. Which gives us whether the stale read was full, partial, or missed the STDIN bytes completely.

Along with this, reasons why the stale read did, didn't, or only partially read those bytes:
TBD

#### Reported Result
For each test case, we report whether the stale read accessed attacker derived data. Then we aggregate these results, and measure how often a stale read accessed STDIN data as well as the simple conditions that seemingly led to a successful read of attacker derived data.

### RQ3: What observable runtime effects follow from stale reads or writes to reclaimed memory?

#### General Idea
So we've measured different components at runtime, such as the type of UAF performed, reuse of chunks, etc, but now, we want to quantify the effects of the stale access.:
1. For reads, the analysis records whether bytes from the replacement object were loaded and whether the loaded value is later observed being used.
2. For writes, the analysis records whether bytes belonging to the replacement object were modified and whether those modified bytes are later observed being used. The goals arent proving exploitability or program semantics, but rather just making concrete program observations after the fact.

#### Required Observations
We will require the results of RQ1 + need to observe some new behavior here including bytes loaded in by a stale read, track what loaded bytes came from the replacement object, whether the loaded value is later used, bytes wirtten through a stale pointer, what replacement bytes were overwritten, the ranges of the ovewrite, and what modified bytes were later read/or used.

#### Measurements
For each stale access, we want to measure the following:
1. `stale_op`
2. `access_range`
3. `curr_owner`
4. `immediate_effect`
5. `later_use`

The key things are `immediate_effect` and `later_use`. These two will vary between a stale read and write.

First, we need to define what an `immediate_effect` is:
1. Stale Reads
    1. We can define a `stale read effect` as simply reading bytes from a replacement object. The effect itself is those bytes being stored somewhere possibly important
    2. We keep track of the replacement object, and for any reads to it, we track what range of the object was read
    3. We can measure this as $\text{stale\_read\_range} \cap \text{replacement\_object\_range}$
2. Stale Writes
    1. keep track of the range within the replacement object that is overwritten
    2. We can define a stale write effect as simply writing bytes to a replacement object via the stale pointer, so we should measure:
    3. We can measure this as $\text{stale\_write\_range} \cap \text{replacement\_object\_range}$

So, we can track the following metrics:
1. read_replacement_overlap_range
2. read_replacement_overlap_bytes
3. read_replacement_overlap_ratio
OR for writes:
4. write_replacement_overlap_range
5. write_replacement_overlap_bytes
6. write_replacement_overlap_ratio

And that's really it for `immediate_effect`; simply some attacker derived bytes were read from a replacement allocation, or were written to it. The more complex part is tracking `later_use`.

Now we need to define what `later_use` is. For both read/write,  we can define it as simply the first observed instruction that consumed the affected value or byte range:
1. Reads: did the value loaded by the stale read get later used?
2. Writes: did the replacement bytes overwritten by the stale write later get read or used?

The operations we can keep track of are:
1. `unused`
2. `used_in_branch`
3. `used_as_mem_addr`
4. `used_as_write_val`
5. `used_as_size_or_index`
6. `used_as_output_data`
7. `used_as_free_arg`
8. `used_as_indirect_call_trgt`
9. `crash_after_tracked_use`: possibly UAF led to unusable weird machine state
10. `unknown`: if there is not enough data (given the level of observabiltiy) to track the usage

We follow these events until an eventual crash, or program end.

#### Ground truth
This would be a synthetic case where we know:
1. Stale Op
2. Stale access range
3. Replacement obj range
4. expected replacement overlap range
5. expected first later use categy, if one exists.

#### Reported Result
For each test case, we report the immediate effect of a stale access, as well as the first observable usge of this effect. We then compare the results to our ground truth to determine the accuracy.

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
```
After having ran program X under configuration Z 30 times, the classification was "PWN" 25 times, meaning its repeatable ~83% of the time, we cannot confidently say that the results of this environment is reproducible because it is under our threshhold of ≤ 98%
```

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
