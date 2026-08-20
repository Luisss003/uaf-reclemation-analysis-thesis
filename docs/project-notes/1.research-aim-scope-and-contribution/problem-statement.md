# Problem Statement
Existing memory error detection mechanisms can identify that a use-after-free
occured, but that on its own doesn't characterize the post-UAF heap state
relevant to actual exploitation. A UAF execution might access memory that
remains allocated, or the same address could've been reclaimed by a new
allocation, whose contents may or may not have came from the attacker.
Address based validity isn't enough to distinguish allocation generations 
after reuse. Therefore, there is a need for a dynamic analysis technique 
that relates a stale pointer to its original allocation, determines what
allocation currently owns the accessed memory, and characterizes whether
attacker derived replacement data overlaps the stale access.
