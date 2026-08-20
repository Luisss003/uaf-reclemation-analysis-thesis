# Terminology

 - Heap Object: A single unique live allocation created by something like malloc. It contains a lifetime and range.
 
 - Object Lifetime: This refers to the period from allocation until deallocation. This means the object logically does not exist anymore, but any pointers to it, and the data originally there still exists in some form.

 - Generation: This is a "label" for distinguishing different allocation lifetimes that reuse the same address.

 - Pointer Provenance: Defines which allocation/generation did a pointer come from. So, if we have a pointer, and make copies to it, understanding PP will tell us the original source, and what it points to.

 - Dangling/Stale Pointer: A pointer whose target object's lifetime has ended, but who still carries the provenance from an expired generation.

 - Stale read: A read through a stale pointer.
     - Ex. If pointer `q` points to object `A:g1`, but A is freed, and then that address is replaced with `B:g2`, if the program then performs `x = q->field`, we performed a stale read on `B` via the stale pointer `q`.

 - Stale Write: A write through a stale pointer
     - Ex. If pointer `q` points to `A:g1`, but `A` is freed, and that addr is replaced with `B:g2`, if the program performs `q->field = input()`, we did a stale write to `B` via `q`.
  
 - Reclaim/Reuse: the allocator gives some, or all of the old objects address range to a later allocation. This means that the new allocation doesnt have to be exactly the same range as the old allocation; a partial reuse is also considered a reclaim.

 - Replacement Object: the new object that occupies memory previously belonging to a previous generation