# High System Code Sec

## Key Takeaways + Relevance to Thesis (TLDR)
 - Depending on goal (theirs is overhead, ours is allocator modification), we can achieve the same performance with different levels of instrumentation. The way we instrument matters more.
 - 
 - Consider a metric like "sanity level" but in case of purturbation.                                                                                                                                                                                                                                                                                                                                                                                                                                                                    

## Full Notes

### Problem Statement + Gap in Existing Work
 - Existing mem safety mechanisms have one of the following issues:
     - High overhead
         - Blindly instrument, meaning 100% safe code snippets are unnecessarily checked
         - Require large amount of metadata/shadow storage
     - Low overhead, but very narrow defense scope
         - I.e Code Ptr Integrity only defend against addr overwrites; other vals can be corrupted

What is missing is a framework to apply the the benefits of existing tools dynamically/only where its necessary to reduce overhead.

### Research Question/Goal
Prove that program instrumentation can be made elastic/dynamic to prioritize cold/obscure code such that we get the same security for a fraction of the overhead. 

### Quantifiying Performance
ASAP doesn't modify the used sanity checks; it just reduces unecessary ones. 

So, a metric is required to determine how safe a program is AFTER ASAP. 

these metrics quantify the contributions a mechanism has to sec and overhead:
1. Sanity Level: $\frac{\text{guarded critical instructs}}{\text{\# of critical instructs}}$
    1. Because we don't know what is truly a sensitive instruct beforehand, we assume all possibly sensitive instructs, like mem stores, are equally as important.
    2. So, a mechanism is only as "safe" as the percentage of critical instructs it protects.
    3. This metric only indicates how much protection is remaining after pruning; not about how good the protection is.
2. Performance Impact: `# of CPU cylces for each check`
    1. This is mostly because ASAP is incomplete in this metric; there are other factors to consider when measuring runtime of mechanisms.


### Evaluation
`This will be useful for evaluating our different levels of observation in the thesis.`

 - Performance Metric:
     - Measure runtime of instrumented and uninstrumented programs and computin the overhead
     - the overhead is the additional runtime added by instrumentation, which is computed as a percentage of the uninstrumented runtime
- Cost Level: the min, max, and target overheads for a program
- Sanity Level: fraction of static checks remaning in the program.
- Security: To quantify security of an instrumented program, they use detection rate $\frac{\text{\# of detected vulns}}{\text{\# of total vulns}}$
    - Note: this will be compared to a detection rate of a program that is fully instrumented with the same features. i.e 50% coverage with ASan vs 100% coverage with ASan