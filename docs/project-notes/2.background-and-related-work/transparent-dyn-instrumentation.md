# Transparent Dynamic Instrumentation

# Existing Problem and Gap
Instrumentation techniques need to be transparent, as in the existing program shouldn't be able to tell that any code insertion is even occuring.

At the time of this paper, there were no formalizations for what transparency should be, and what criteria it should meet to be considered transparent, so this paper aimed to define those gaps.

Therefore, they give us 3 guidelines to designing transparent instrumentation:
1. Leave the application unchanged whenever possible, even if changing would give a performance boost
2. When changing the application is unavoidable, ensure change is imperceivable to the application
3. Avoid resource usage conflicts

## Maintaining Heap Data Transparency

 - Memory allocated by the instrumentation must be seperate by applicaiton used memory
 - Allocators are typically thread safe, but not re-entrant (consider the impact on bins/tcache)
     - If our instrumentation allocates chunks, then we will completely mess up the UAF behavior in the program
