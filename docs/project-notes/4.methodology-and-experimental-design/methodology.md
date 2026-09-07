# Methodology

## 1. Research Design
The research here is primarily an experimental systems security study using
dynamic program analysis. We are observing concrete executions rather than
statically considering every possible program behavior.

We can define our unit to be one execution of a C/C++ program containing a
known heap UAF, using an input that exploits that vulnerability.

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
### Controlled Synthetic Cases
These are small, manually created programs + UAF inputs where we know exactly what
will happen, and the relationships between generations/objects.

The reason for including this type of test case is because it will allow us to establish certain ground truths about the type of UAF executed, the consequences of it, and the relationships between objects/generations. These tests won't really be for proving our main hypothesis or research question, as they won't accurately simulate a real world C program.

They will be all be compiled with the same Makefile.

### Existing Security Examples
These are selected UAF/heap examples from CTF sources like how2heap/pwn.college and academic sources like the Juliet dataset. These tests are meant to determine whether the approach survives cases that we did not manually design for the analysis. 

### Real World Cases
These are real world vulnerabilities found in FOSS code bases. This will likely
be the hardest challenge for our framework, and will consist at most 1 or 2
examples. The results of these tests will give us the most accurate answer to our hypothesis, and research questions.

## 4. Instrumented Observation
What we collect during runtime is based on our reserach questions, and the pieces of evidence they require to be solved. 

If we consider RQ1, we would collect the information in the following methods:
1. For the operation and range of the stale access, we can get this information straight from the call to malloc/free/etc.
    1. We need to consider 

## 5. Runtime Classification

After the execution, we need to then take our observations, and classify what
occured as a result of the UAF input. This allows us to make claims about the
state of the program. For example:

1) Was the original allocation alive?
   1) If so, not a stale generation. 
   2) Otherwise, continue
2. Is the addr curr unallocated?
   1. If so, a stale access to unreclaimed memory
   2. If not, then a stale access to reclaimed memory 
3. What bytes of the stale access overlap the replacement obj?
   1. This could be computed as $A\cap P$ where A = stale access range, and P = replacement allocation range

## 6. Experiment Variables

### Controled Variables

1) Architecture: x86-64

2) OS: Debian 13

3) Compiler: glibc 2.31 and 2.41

4) Program Input (we assume an existing UAF input)

5) Instrumentation configuration

6) glibc allocator configuration (tuning)



### Varied Variables

1) UAF test case

2) Replacement allocation behavior

3) Attacker data placement

4) glibc configuration (while also a controlled variable, we will vary between both versions)

5) instrumented vs native execution (to determine purturbation)

## 7. Repetition of Experiments

Because certain heap behavior isn't deterministic, we will rely on more than one run for experiements. Rather than recording absolute values, we will store statistical data. For example, rather than storing the absolute address used for some allocation, we will record same address reuse frequency.  

## 8. Evaluation

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