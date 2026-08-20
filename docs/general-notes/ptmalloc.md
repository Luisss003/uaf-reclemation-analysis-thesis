# Ptmalloc Notes

## Version Notes

- 2.31 didn't introduce any major changes to ptmalloc  

## Vulnerability Notes

- [1] found that versions 2.3 and 2.6 (introduced tcache) had security and consistency checks, but HeapHopper was able to bypass them

- [1] Tcache Case Study
  
  - 2.6 introduced more vulns because of the tcache structures. 
  
  - Constraints that limit `overlapping allocation` and `non heap allocation` attacks are lost when tcache enabled
  
  - On this version, when ptmalloc is used without tcache, the only way to get an arbitrary write required UAF via unsafe unlink attack. 
    
    - However, when tcache enbaled, AW was achievable via fake-free operation.
  
  - This was found to be because tcache didnt replicate many of the security checks on the existing free chunks list i.e. fastbin/largebin/etc.
    
    - And because tcache came first before those old bins, chunks often were used without the proper security checking.

# Sources

1) HeapHopper, 2018
