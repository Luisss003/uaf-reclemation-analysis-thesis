# Methodology

## 1. Research Design
The research here is primarily an experimental systems security study using
dynamic program analysis. We are observing concrete executions rather than
statically considering every possible program behavior.

We can define our unit to be one execution of a C/C++ program containing a
known heap UAF, using an input that exploits that vulnerability.

The reserach itself can be broken into the following steps:
1) Given a program + a known UAF triggering input
2) We control the environment which the program runs in
    1) There should be 2 controlled variables for each experiment. The version of Glibc used, and the level of observation used.
3) Execute and analyze the program via our prototype
4) Make runtime observations
5) Classify those observations to make our final claims

## 2. Experimental Environment
The experimenetal environment will be ran on QEMU, `TODO: insert how I plan on using static QEMU files to have a consistent environemnt`.  A virtual machine is being used primarily for the benefits of rollbacking after conducting experiments, as well as keeping a consistent starting point. As our environment will not require internet connectivity, nor rely on internal Unix time, there should be no issue beginning experiments at a common starting point.

The architecture will be limited to x86-64. The purpose of this thesis is to explore glibc heap behavior, and supporting other architectures would introduce additional variables, pointer widths, and so on. Additionally, x86-64 is a standard, and actively supported architecture for our OS of choice.

The operating system will be Debian 13 `stable` with the v6.12 LTS kernel release. Debian 13 provides a slow changing and reproducible Linux environment, which will work well for our use. The v6.12 LTS release also provides us with little change to our environment, and is currently the release Debian 13 uses.


## 3. Test Corpus
### Controlled Synthetic Cases
These are small, manually created programs + UAF inputs where we know exactly what
will happen, and the relationships between generations/objects.

The reason for including this type of test case is because it will allow us to establish certain ground truths about the type of UAF executed, the consequences of it, and the relationships between objects/generations. These tests won't really be for proving our main hypothesis or research question, as they won't accurately simulate a real world C program.

They will be all be compiled with the same Makefile.

### Existing Security Examples
These are selected UAF/heap examples from CTF sources like how2heap/pwn.college and academic sources like the Juliet dataset. These tests are meant to determine
whether the approach survives cases that we did not manually design for the
analysis. 

### Real World Cases
These are real world vulnerabilities found in FOSS code bases. This will likely
be the hardest challenge for our framework, and will consist at most 1 or 2
examples. The results of these tests will give us the most accurate answer to our hypothesis, and research questions.

## 4. Instrumented Observation
This describes what info we plan on collecting during each execution. 

Without
going into the technical details, we should be collecting at least:
1) Allocation events
2) Deallocation events
3) Allcoation address ranges 
4) allocation generations
5) pointer provenance
6) memory accesses, including the address, size, and operation
7) attacker derived data

The intention is to reconstruct a timeline and state. The key here is that we
will not be simply looking at numerical addresses to identify allocations, but
the collected data above. 

This decision is supported by CETS discussion on temporal checking, where they 
conclude that location based checking is insuffucient due to the possibility 
of address/chunk reuse.

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

## 8. Validation and Baseline Comparison

We will need a ground truth to determine how accurate our findings are. This can easily be done with our synthetic test cases, where we take advantage of the deterministic components of the heap to use as a baseline. For example, a program like:

```c
a = malloc(x)
free(a)
b = malloc(x)
read(input, b, 16)
stale_read(a + 4, 8)
```

We know for a fact that A is expired, and B is the replacement. By performing a stale read with A, we can record that 8 stale bytes reach B, and are attacker derived.



Additionally, we can use existing tools as baselines, such as ASan, as while this tool does not give as much information about UAFs as our plans to, we can use it to determine if our findings line up with what is given by ASan. These observations will be taken with a grain of salt, as ASan themselves implement a quarantine zone for chunks which modifies the way default allocator runs.
