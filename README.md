# Observed Attacker-Controlled Heap Reclamation in Use-After-Free Executions

This repository contains the prototype and reproducibility artifact for a
master's thesis investigating runtime heap reclamation following use-after-free
in Linux x86-64 programs using glibc malloc.

Given a known UAF-triggering input, the system reconstructs allocation
generations, associates stale pointer accesses with expired and replacement
objects, tracks supported attacker derived byte ranges, measures byte level
overlap, and reports the observed runtime consequence.

The system does not discover vulnerabilities, synthesize heap layouts, or
generate complete exploits.



For more on methodology, research questions, analysis techniques, and more, please check the documents under `/docs/project-description`. 
