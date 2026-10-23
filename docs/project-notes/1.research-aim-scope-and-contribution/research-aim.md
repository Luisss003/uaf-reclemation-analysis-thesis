# 1.2 Research Aim
The aim of this thesis is to develop and evaluate a dynamic analysis technique for characterizing the heap state associated with known UAF executions that extracts and combines both logical allocation events, as well as the underlying real memory chunk events unique to the glibc allocator. The aim is to analyze, and then characterize aspects about the heap post-UAF execution; this includes allocation generations involved, stale pointers involved, the reach and immediate impact of attacker derived bytes, as well as the finite properties of the UAF such as whether allocation reuse was involved, whether the UAF happened naturally or due to user input, and the underlying glibc mechanisms state transitions which led to the post UAF state.

There is no intention to discover UAFs, explore arbitrary program paths, heap grooming, prove exploitability, or any kind of AEG. This objective is to strictly 
make observations about allocation and chunk events, draw conclusions from those observations, and generate a final report which can later be used for program or exploitability analysis. 

