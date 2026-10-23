# SoK: Sanitizing for Security
This paper goes over the need for dynamic bug findgers aka sanitizers, and the different techniques used, and what impact they have on native code execution. This will be useful for implementing a prototype of my own tool.

## Techniques for Detection
There are two types of tools for detecting spatial + temporal bugs:
1. Location Based Access Checker
    1. Detect accesses to invalid mem regions via metadata store that keeps state for each byte of the addresssable addr space. On each mem access, consult the metadata store
    2. This concept is implemented via red zones or guard pages for spatial detection. Either technique is combined with reuse delay for temporal detection.
    3. Low runtime ovh, highly compatible with uninstrumented code, but high mem ovh  
2. Identity Based Access Checker
    1. Detect accesses to mem that do not go to their intended target via metadata (bounds or alloc status) for each heap obj, and have a mechanism to determine intended target for each pointer in the program
        1. Ex. `q = p* + 10` would trigger a metadata lookup even if this technically doesn't execute any control flow issues
    2. Metadata can be implemented a few ways
        1. per-heap object bounds or per-pointer bounds for spatial
        2. pick one technique above, and add reuse delay, dangling pointer tagging, or lock-and-key for temporal.
    3. Must use instrumentation to function
    4. My thesis prototype will essentially act as an identity based checker, using bounds + allocation generations

More on techniques for location based:
1. Red Zones
    1. Used for spatial checking in location based sanitizers
    2. Based on injected memory that is marked as invalid in the metadata store. The program should have no reason to ever access these red zone cells
    3. Low runtime ovh, increases memory ovh, and limited precision (recall lancet ASan misses an exploit)
2. Guard Pages
    1. Used for spatial checking in location based sanitizers
    2. Inserts memory pages before and/or after every allocated mem obj. Basically RZs but at hardware level
    3. High mem ovh, so alternative is to use guard blocks, fill the blks with a pattern, and then when the obj is freed, check if pattern is unchanged. Delays detection though

More on techniques for identity based:
1. Per-pointer Bounds
    1. For each pointer, store the base addr + size of the referent (what the pointer points to) as metadata in the ptr.
    2. Now for pointer calculations or assignments, propogate the metadata.
    3. For spatial detection, simply check if the calculated addr via the pointer is within the base + size of referent space via its derived metadata. (includes intra-object checking)
    4. Big con is low compatability since requires custom pointers to store MD
2. Per-object Bounds
    1. Store metadata for each heap obj, rather than each pointer (more mem efficient since multiple pointers can refer the same object, so no dupe metadata)
    2. This does make it harder to associate pointers to their intended referent.
        1. If pointer points to in-bounds addr, its easy to associate it with an obj. Simply do range-based lookup in metadata store.
        2. However, doing a lookup with an OOB pointer wont return any metadata since the pointer doesnt point anywhere legal.
            1. One solution is to consider OOB pointers in the model, and keep metadata for `OOB objects` which are nonexisting objects that these pointers refer to
            2. A better solution is to not track OOB objects, and to store the distance between the OOB pointer and its referent in the MSB of the pointer. This will make the pointer clearly invalid, indicating that it is OOB.
            3. There are further improvements...
    3. Support near complete spatial memory detection but cons:
        1. Don't detect intra object ovf
        2. Handling of OOB pointers causes compatability issues
    4. More compatible, since dont need to modify pointers; only need to intercept calls to malloc/free.

Recall for temporal safety, we need to enhance whatever of the 4 techniques we use above with one of the following:
1. Reuse Delay
    1. Location Based: Mark recently dealloc objs as invalid in metadata store by replacing them with red zones
    2. Identity Based: Simply invalidate the identity of the object or pointer in the metadata, but this assumes that reuse doesnt occur
        1. Some implementations delay reuse in order to "age" the allocations. ASan does this via quarantine. Another idea is to create 1 page per object, and then when we free an obj, make that page into a guard page
        2. We of course will use the concept of generations
2. Lock and Key
    1. Identity Based: assign unique allocation IDs (keys) to each alloc mem obj and by storing this key in a "lock" location.
    2. The expected key and lock loc is stored as ptr metadata
    3. When free() is called, the key is revoked from the lock loc, and if that pointer attempts to access the original stale address, it sees that the lock location and stored key do not match.
    4. Since this is based on lock locations, reuse would simply generate a new key, meaning stale pointers with old keys cant access the same space
    5. Only drawback is uncompatability
3. Dangling Pointer Tagging
    1. The idea is to simply mark a dangling pointer, but have the spatial detection mechanism deal with it.
    2. A simple technique is just to NULL the value or bounds associated with pointers that are passed to `free()`. Then, the existing spatial detection mechanism would flag use of these pointers, since they point to NULL.
    3. We also need to tag copies of the dangling ptr, and one way is to maintain auxilluary data structs that link all mem objs to any poitners that refer to them

## Instrumentation
Sanitizers focus on instrumenting:
1. Mem loads/stores
2. Stack frame (de)allocations
3. Calls to mem funcs i.e malloc/free
4. System calls

Instrumentation can be done at four levels:
1. Language Level
    1. Work at source code/AST level
    2. Best used for santiizers that detect pointer type errors via pointer cast monitoring
2. IR Level
    1. Work at stage where compiler goes from AST/source -> intermediate representation
    2. This is where LLVM instrumentation operates at
    3. Best general instrumentation, but lacks compatability if code uses closed source libraries
3. Binary Level
    1. Work at the execution level, meaning it takes the source code, instruments it, and translates it into machine code
    2. Clearly, this works best for any general execution, no matter if src code is available, but it introduces massive runtime ovh
4. Library Interposition
    1. Works at library call level. This is what LD_PRELOAD is
    2. Sanitizers use it for intercepting calls to malloc/free
    3. It has the same benefits of binary level, but only works for inter-library calls. Calls of two funcs within the same library dont work.

For our thesis, IR level works best since we are the assumption that source code is available, so no need for binary level. Additionally, library interposition will also be used as one of the listed use cases applies to us closely (monitoring malloc/free).

## Metadata Management
1. Object Metadata
    1. The sanitizer stores metadata for each allocated memory object.
    2. This includes obj size, type, alloc state, and alloc identifier.
    3. There are four ways to actually store this obj metadata:
        1. Embedded
            1. When an obj is created, increase its size enough to fit the metadata as part of the obj.
            2. Good for compatability, since the metadata is stored on either end of the allocation, a third party library wouldn't know about it.
        2. Direct Mapped Shadow
            1. Map every n-sized block of real proc mem to an m-sized block of metadata via `metadata_addr = shadow_base + m * (block_addr / n)`.
            2. For example, ASan maps 8 real bytes to 1 byte of metadata.
            3. Easy to implement, but memory inefficient as it has to account for each possible virtual address.
            4. Technically, this is storing metadata per-byte
        3. Multi Level Shadow
            1. Essentially the idea of paging but for metadata stores. Rather than direct mapped, where its one contigous block, we maintain fragmented directory tables.
            2. DTs can either point to metadata tables, or other DTs.
            3. Each DT is correlated with only a specific chunk of virtual memory.
            4. Most tools that do per-object metadata actually use this since objects aren't a static size. They vary
                1. So, a variable compression ratio multi level shadow mapping scheme is used, where directory table maps variable sized objs to constant sized metadata.
            5. The obvious con is that accessing metadata once can require multiple memory accesses through DTs
        4. Custom Datastructure
            1. The idea here is to use a data structure that has support for concurrent accesses.
            2. If it does not, you need to explicitly use mutexes for multithreaded programs
2. Pointer Metadata:
    1. Here, we store the metatadata about the pointers themselves
    2. There are three main methods for implementation
        1. Fat Pointers
            1. Entirely replace the pointer structure for a fat pointer struct, which stores the original pointer value, base address for intended referent, and size of the referent.
            2. Fat Ptrs are actually alot more efficient than normal pointers, since all the metadata is already at the pointer; no need to refer to other metadata.
            3. The big con is that they aren't very compatible, and requires heavy instrumentation
        2. Tagged Pointers
            1. Replace the default ptr with tagged pointers, which embed metadata into the pointer itself,without changing its size.
            2. They require alot less instrumentation, but need to be translated to be used as normal pointers.
        3. Disjoint Metadata
            1. Simply store ptr metadata elsewhere via multi level lookups.
    3. Static Metadata
        1. This is metadata that is inserted beforehand and used as refrence during execution.
        2. For example, if a sanitizer solely checks for correct type usage, it can import a type table to refer to during runtime checks.