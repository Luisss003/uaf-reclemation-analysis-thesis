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

## Research question
Can a low-perturbation, glibc aware runtime analysis accurately classify
observed heap reclamation after use-after-free by associating an expired pointer
generation with the current replacement object, attacker derived byte ranges,
and the resulting stale access consequence?

## Current status
Repository reboot and allocation-generation prototype development.

## Supported scope
- Linux x86-64
- Source available C and C++
- glibc malloc
- Known UAF triggering inputs
- Single process programs
- Primarily single-threaded evaluation
