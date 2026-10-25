# 1.2 Research Aim
The aim of this thesis is to develop and evaluate a dynamic analysis technique for characterizing the heap state associated with known UAF executions. This is achieved by extracting, combining, and analyzing both logical allocation events and underlying real memory chunk events/properties of the glibc allocator. The characterization includes observations such as allocation generations involved, stale pointers involved, the reach and immediate impact of attacker derived bytes, whether allocation reuse was involved, whether the UAF happened naturally or due to user input, and the underlying glibc mechanisms state transitions which led to and after the post UAF heap state.

There is no intention to discover UAFs, explore arbitrary program paths, heap grooming, prove exploitability, or any kind of AEG. The objective is to strictly 
make observations about allocation and chunk events, draw conclusions from those observations, and generate a final report which can later be used for program or exploitability analysis. 

