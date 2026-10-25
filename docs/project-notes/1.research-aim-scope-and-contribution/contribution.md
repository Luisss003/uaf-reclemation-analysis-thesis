# 1.3 Contributions

1. A runtime characterization model for post UAF executions

2. An evidence based classification method
    1. Rules for classifying reclamation and input derived overlap from recorded events, including allocation, accesses, and pointer provenance, and input provenance evidence, including explicit unknown results when the evidence is insufficient.
    
3. A prototype implementation of analysis
    1. A program that, given a test case with a known UAF input, should record and classify the UAF execution as expected based on our characterization model and method. The scope is limited to a documented subset aka our test cases
   
4. Empirical Evaluation
    1. An oracle based eval showing the methods accuracy and completeness, which evidence is necessary for each conclusion, how repeatable the results are, and how instrumentation affects observed execution, and how the method behvaves on real vs synthetic UAF cases.


