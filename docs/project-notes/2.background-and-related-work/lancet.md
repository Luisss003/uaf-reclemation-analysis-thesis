# Lancet: A Formalization Framework for Crash and Exploit Pathology

**Venue / Year:** USENIX 2025  
**Relevance to Thesis:** High

---

## 1. Problem

**What problem is the paper addressing?**

- Lack of formalization in cause, development, and impact of common software errors.

**Why does this problem matter?**

- In order to secure programs, we need to understand what happens to a program state BEFORE, DURING, and AFTER exploitation
- Also, on the red team side, helps understand how to create better exploits

---

## 2. Gap in Existing Work

**What is missing or inadequate in previous approaches?**

- Existing formalizations:
    - Stop working in prescence of mem corruption (i.e. weird machine states)
    - And/or stop once a bug is found (modern exploits chain together vulnerabilities)
- They are not formal i.e. CWE.

**Why can't existing work solve the problem?**

- Sanitizers: Due to memory modifications such as redzones/quarantine, reports are often incorrect/misleading.
- Root Cause Diagnostics: these methods rely on reverse execution/tracing back programs steps, so they don't work for sitations lacking direct data flow between corruption site and root cause.
- Manual Explot Analysis: each exploit is unique, so it isn't scalable.
- AEG: Along with generating the exploit, some works used fingerprinting to "characterize" parts of the exploit workflow, but not in depth.
- Natural Language Formalization: these only describe an exploit/vuln in general descriptions. Not deep enough for analysis
- Real Formalization: existing ones focus on program correctness/abscence of vulns, so fail (in terms of analyzing the state of a program AFTER corruption) when a weird machine state is introduced

**Closest prior work:**

- There is no closest prior work. Lancel explicitly states this.

---

## 3. Research Goal / Questions

**What is the paper trying to determine, demonstrate, or achieve?**

- Develop a framework to formally define common mem corrupt vulns for any general allocator.

**Explicit research questions or hypotheses, if any:**

-  Tracking 3 pieces of info about a heap ownership is enough to capture spatial + temporal safety violations, even in weird machine states.

---

## 4. Proposed Solution

**Core idea**

- Use the concept of ownership to reason about vulnerabilities and exploit primitives.

**Key techniques / components:**

- Three types of ownership are tracked:
    - Cell owner: the subject that owns a memory cell
        - when a heap obj is allocated, it becomes the cell owner of all the mem cells that host the object
    - Value Owner: the subject that owns the values in a memory cell
        - By default, newly used cells have a value owner of allocator (since no other subject has written a new value there)
        - Once a cell is used and something is stored there, the writer becomes the value owner
    - Pointee Owner: this tracks the subject a pointer points to 
        - The cells that store the pointer itself, commonly stack cells, are given a pointee owner value IF they contain a pointer.
    - A cell can only have one owner of each kind, but the owners can vary for each field
- LancetISA is a way to formalize this, with its own rules:
    - `P[p] != C[addr]` is the state defines UAF. The object the ptr was meant to refer to is not longer that currently owns that memory.
    - They track the following:
        - `R`: register contents
        - `M`: memory contents
        - `H`: map tracking the size of allocated heap objects.
        - `S`: map tracking the subject a stack obj was last allocated for
        - `C`: ownership maps to track cell owners
        - `V`: value owners
        - `P`: pointee owners
    - An individual cell can have many owners to account for overlaps in allocations
    - Cell ownership is updated when a new atomic piece of data is created/destroyed either via malloc/ free.
        - Upon Creation: 
            - Default ownership of cell is set from default `allocator` to new subject
            - The size of the allocation is stored in the `H` map via the starting addr of the allocation
        - Upon Desctruction, the free'd obj's size is read via `H`, and the stored addr is set back to null. The cell owner is set back to `allocator. To ensure other pointers that point to the same subject are not effected, we only cancel the specified pointer in free(), pointee ownership. the rest are left alone
    - Value Ownership is updated whenever data is written to mem.
        - Upon a store, the value owner of the target mem cells is updated to the pointee owner of the register referring to the written memory.
        - This is how we know where a value came from.
    - Pointee Ownership
        - Ptrs can be copied to other mem or regs, pointee ownership needs to be propogated so that even copies know where the pointer should point to
        - Pointer arithmetic is handled differently. Given a pointer p, if we do `q = p + 10`, and the resulting address is within the allocation, then the new `q` gets the same pointee owner
            - However, if its outside the allcoation, and crosses into another allocation, lancet accounts for this by simply storing that this pointer has 2 pointee Owners, at which is later sued to detect a spatial error.
            - The reason it doesnt just give ownership of the overlapped allocation is because we would lose pointer provenance; we wouldnt know where this new pointer was originally derived from.
            - This can cause overestimation for things like `p=(p+q)/2`. Lets say that the result address ends up in p, therefore it should keep the same single pointee owner. But, we did use q, which has another pointee owner, so Lancet keeps both
                - This is because `p=(p+q)/2` entirely relies on the heap layout. This computed address could land in p again, or end up completely somewhere else, so better to track all pointee ownerships involved.
    - In general, Lancet over-approximates ownership for cell, value and pointee, but when it comes to concrete state/values i.e. register/bytes/heap objects, lancet is precise.
        - i.e., RAX must have a single value, it cant be overapproximated how a cell can have multiple owners, indicating a violation.
- This is how Lancet defines vulnerabilities with those 3 pieces of tracked data:
    - To check for an expired write, it checks if the pointee owner of the source register is the same as the cell owner of the refrenced cell.
    - For UAF writes, it simply takes the rule above, and restricts it to heap. And for stack write after scope,restrict it to the scope.
    - Note, these detections rely on overapproximation as stated before, meaning false positives can occur.
---

## 5. Use Cases

**Exploit Primitive Fingerprinting**
They developed a tool called EPF, which excetends Lancet to define common exploit primtives and techniques. The idea is that an exploit is escalation from an initial corruption into stronger primitives, so EPF fingerprints those transitions. 


---

## 6. Method / System Design

**What information does the system observe or collect?**

- 

**Analysis / instrumentation techniques:**

- 

**Important algorithms or data structures:**

- 

**Important assumptions:**

- 

**Scope:**

- 

---

## 7. Experimental Design

**What claims are the experiments intended to test?**

- How Efficiently can Lancet and its two tools dissect crashes and exploits?
- Can FCS effectively annotate the cause + progression of errors, and can EPF illustrate primitive transitions in explopits?
- Did FCS contribute to patch synthesis in the AIxCC semi finals?

**Corpus / benchmarks / programs:**

- Internal dataset consisting of pull issues of popular FOSS
- Vulns in NGinx CP
- Programs meant to show off techniques from sources like how2heap
- CTF challenge programs
- Real world exploits
- Reproducible mem corrupt cases from REPT
- Representative cases from JULIET
- NOTE: for vulns from 1 and 2, they manually debugged them to create a ground truth to then evaluate their tools on.

**Independent variables / configurations varied:**

- 

**What is held constant?**

- The hardware used to run the tools

**Baselines / comparison systems:**

- 

**Metrics:**

- 

**Number of runs / repetitions:**

- 

**Environment:**

- Architecture:
- OS:
- Compiler:
- Relevant libraries / versions:
- Hardware:
- Other:


---
## 7.5 House of Einherjar Fingerprinting Case Study

This is the exploit where we overwrite the PREV_INUSE bit of the next chunk such that the allocator believes the chunk before it is free. Also, the attacker sets a fake prev_size field so thwen the corrupted chunk is later freed, glibc performs backwards consolidation to a fake chunk chosen by the attacker.

This is how EPF handles it:
1. First, EPF locates the initial off-by-one and notes that the cell owner of the overwritten mem is  `allocator`. So, EPF correctly reports a corruption of heap metadata.
2. Next, EPF accurately reports a mem overlap when the malicious malloc() (after the initial corruption) is executed.
 
---
## 8. Results

### Main Findings

1. 
2. 
3. 

### Important Numbers

- 
- 

### Claim → Evidence

| Claim | Experiment / Evidence | Result |
|---|---|---|
| | | |
| | | |
| | | |

---

## 9. Limitations / Threats to Validity

**Limitations acknowledged by the authors:**

- 

**Things the approach cannot handle:**

- 

**Assumptions that may limit generalization:**

- 

**Potential sources of measurement error / false positives / false negatives:**

- 

---
