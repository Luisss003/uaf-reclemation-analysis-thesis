# Undangle

## Key Takeaways + Relevance to Thesis (TLDR)

## Full Notes

### Problem Statement + Gap in Existing Work
UAF and Double Free are big threats to security. They are hard to detect because involve two seperate events that can happen far apart in time:
1. Creation of dangling ptr
2. Refrencing that dangling ptr

For browsers specifically:
1. parent objects may share a child obj among each other, not knowing that the child is deallocated
2. Browsers use a mix of technologies i.e. JS, XPCOM, etc, which all use different memory management methods/garbage collection

Existing work focus primarly on detection via the dangling pointer AFTER it's been used:
1. This means fuzz testing may require a large input size that causes both events
 challenge/seeking-spanless-secrets-easy

### Contributions
1. Rather than using a giant input size for testing, keep track of dangling pointers created, but not used, and use the intution that long lived DPs are inherently dangerous. This is done via time window detection
2. Tool provides info about program state when DP is used and created
