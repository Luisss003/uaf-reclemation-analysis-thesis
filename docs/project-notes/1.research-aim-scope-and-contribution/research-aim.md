# What is the research aim?
The aim of this thesis is to develop and evaluate a dynamic analysis technique
for characterizing the heap state associated with known UAF executions. There is
an emphasis on same address reclemation, replacement object identity, attacker
derived replacement data, and the concrete memory operation performed through
the stale pointer.

There is no intention to discover UAFs, explore arbitrary program paths, heap
grooming, prove exploitability, or any kind of AEG. This objective is to strictly 
make observations given allocator state, and make claims about that state.

