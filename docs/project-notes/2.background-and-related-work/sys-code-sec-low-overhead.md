# High System Code Sec

## Key Takeaways + Relevance to Thesis (TLDR)
 - Depending on goal (theirs is overhead, ours is allocator modification), we can achieve the same performance with different levels of instrumentation. The way we instrument matters more.
 - They turn abstract ideas, such as instrumentation level, and make them into concrete metrics i.e. sanity level.
 - See measurements for how they use their metrics, how they set up their environment, to make diff readings. Clearly for our case, we need to set a ground truth, since for them, the ground truth is detection rate.                                                                                                                                                                       
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

### Quantifiying Security and Performance
ASAP doesn't modify the used sanity checks; it prunes most expensive ones if the algorithm used deems it unnecessary/too expesive.  

There are three implications of this:
1. If done correctly, a post-prune program should maintain the same level of security as a fully instrumented one WITH the performance benefit.
    1. Pruning removes individual "sanity checks"; it does not alter the fundemental protection mechanism
2. Formal proof of protection cannot be guaranteed anymore, since these security mechanisms rely on all of the "sensitive operations" to be protected.
    1. Despite this, ASAP still proves security via empirical data (detection rate)

So, we need 2 quantifiable metrics to determine how much:
1. a mechanism contributes to the protection of a prog
2. a mechanism increases overherad

Those 2 are:
1. Sanity Level: $\frac{\text{guarded critical instructs}}{\text{\# of total critical instructs}}$
    1. ASAP lacks info about which critical operations are more likely to be vulnerable, it assigns equal value to every sanity check guarding a critical operation.
    2. For mechanism X that init guards every critical op, sanity lvl is the % of static checks remaining after pruning
    3. This metric only indicates our remaining % of static checks after pruning; it doesn't comment on how good security is.
    4. We use this mechanism to measure something like `if we remove X,Y,Z checks, we are left with a 90% sanity level, but only a 5% overhead cost, meaning we get almost 100% of the checks of a fully instrumented program without the giant overhead`. 
2. Performance Impact: `# of CPU cylces for each check`
    1. Diff mechanisms have unique steps (some may require accessing shadow mem, some may not) on each sanity check, but ASAP doesn't take this into account. It only cares about a rough `CPU cycles per execution * execution count` for a specific check.
    2. This is because as stated, ASAP mainly removes checks via most expensive runtime for X unique checks.
    3. In other words, they aren't claiming something like `we optimized the way ASan does shadowmem accesses decreasing overall runtime by X%`.
    4. Rather, they are just removing the checks that take the longest, giving us an absolute runtime decrease (measured in CPU cycles)
    5. This is basically fine grained optimization.


### Evaluation
`This will be useful for evaluating our different levels of observation in the thesis. We don't really care about overhead, but, we need an idea on how to measure purturbance on native allocator behavior.`
Three evaluation questions are asked:
1. Effectiveness: How secure is a program given a low overhead budget after ASAP processing.
2. Performance: How much can ASAP reduce overhead?
3. Security: Does ASAP preserve the protection gained by instrumenting software?

To measure this, we need to operationalize:
1. Performance
    1. Measure runtime of instrumented prog + uninstrumented baseline
    2. Overhead is determined as $\frac{\text{instrumented runtime - baseline runtime}}{\text{baseline runtime}}$
    3. Cost Level: min, max, and target overhreads of a prog
    4. Sanity Level: already discu_ssed._
2. Secutity
    1. measure detection rate (bugs detected/total bugs)
    2. The measured detection rate it compared to the measurements of when the program is fully instrumented, and not instrumented at all.

Now to into how we evaluate given our metrics:
1. Performance
    1. Given 19 progs from SPEC, they first compiled a baseline + fully instrumented vers, and calculated the overhread.
    2. There are 2 ways to configure ASAP:
        1. Cost level:
            1. This basically means that for c = x, retain only x%  of check related cost between min and max instrumentation overhead
                1. min = unavoidalble overhead
                2. max = overhead with all checks
            2. In other words, if max = 54%, and min = 4%, and c = 0.1, with the equation they give us, we would get 4.5%, which means it discards almost all the expensive checks, with the residual 4% remains.
        2. Sanity Level
            1. Another way to configure is simply to put a target sanity level; simply remove checks until the desired security level is achieved.
            2. This allows a more fine grained measurement since you can remove individual checks.
    3. Given this, they simply compared the trade off between a lower santiy level and runtime overhread.
    4. Now, they did a training and reference workload for the test suites
        1. Training: ASAP uses it to profile the program, and learn what checks are hot/expensive
        2. Reference: actually used to measure performance
    5. In addition to this, real life tests with OpenSSL and Python interpreter were done
        1. This is because SPEC simply tests performance; not security.
        2. The same performance measurements were made, ensuring both software were configured to mimick production environments.