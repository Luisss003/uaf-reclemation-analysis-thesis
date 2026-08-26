# How to Write a Thesis
## 1. Prep Work
### 1. Finding a Reserach Topic

 - This is just a general direction that the research will be about; not the specific questions to ask
 - Determine if existing research already exists about this. If question has been answered, consider another angle to tackling the question
 - If not, then comile your own reserach to see what already exists on this topic as to not reinvent the wheel.
     - DONTS:
         - Ask a vague, unclear question
         - Ignore existing reserach
         - Collect data that only supports your view
         - Draw conclusions based on weak analysis
  
Using this data, now begin the actual reserach topic
### 2. Deine a Research Question
Identify a research question at this stage. A good RQ has:
1. Clear: must be clear on what we want to figure out and in what context.
2. Uniqueness: RQ shouldnt ask what has already been answered.
3. Value: it should be of some important. it shouldnt be something useless i.e. how does the time module affect glibc behavior for allcoator.

## 2. Writing the Thesis

### 0. Title
Use a title that catches the readers eye. The most common is something like `COOLNAME: 10 word summary`.

### 0.5. Abstract
100-500 words. It should contain:
1. Reserach Focus/Problem to tackle
2. Main conclusion: endproduct and why it works
3. Methodology ( high level, just tease it)
4. Major results

### 1. Introduction
Use alot of what was on the proposal for the first 3 chapters.
 - Avoid using same sentences as in abstract.
Set with around 4 paragraphs:
1. Introduce the problem statement + background of the area. Why is the problem important?
2. Next, the gap in existing reserach. 
3. What you propose, and its main features. Use `This work presents` start to the paragraph.
4. Finally, what contributions of your work. A bullet point is common.
5. Paper organizaiton is simply for 2 secntences saying what each section contains. Just "Setion X contains, Y, Section Z contains ..."

### 2. Literature Review/ Related Work
The literature review should come AFTER digesting existing reserach; otherwise a bunch of rewrites will occur. Read. Take a deep dive into existing literature. ensure you catagorize:
1. Point is to show that you arent doing outdated reserach
2. indicate your advancements for what currently exists. `The work of X and Y[3] is restricted to ... however with our system...`

Only after then, you can start writing the literature review chapter. This chapter should:
1. Synthesize the existing reserach. Show how different pieces of theory fit togeher
2. Highlight what gap your research is filling
3. Show how existing reserach informs your methodology i.e. why dont just raw addresses work for marking allocations

### 3. Methodology
Design your reserach specifically to find answer to reserach questions. There are 2 steps:
1. Design reserach strategy
    1. Explain the HOW of how you plant to answer your research questions
    2. Make desicions about reserach design i.e. philosphy, overal methodology (quantative, qualitive, etc), sampling strategy, data collection methods,
2. Execute the reserach strategy
    1. Data Collection
        1. Its normal if data creates new questions during collection/analysis.
      
General Tips:
1. DONT focus on explaining how known libraries/tools work. I.e. dont explain instrumentation, but rather explain how the tool works.
2. Always ensure you justify choices in methodology
3. Add illustrations, pseudocode (with explanations).

### 4. Present Findings
This is usually done over the span of two chapters:
1. Results chapter
    1. Presents the data without opinion
2. Discussion chapter
    1. Explain what the data means.
    2. Benchmarks, why you used certain ones, why you didnt use others.
    3. COmparison tables if applicable

Sometimes, they are combined.

### 5. Conclusion
Conclusion chapter is where you highlight the key findings of the study, and what the implications are.

The key findings are the findings that directly relate to the original reserach questions. Follow a form that reminds the reader of the research question, and the results relation to that question. For example `the results suggest that the following factors have a large impact on consumer  trust: factor X, Y, Z`.

Implications are related to how our findings relate to real industry/reserach. Like how does this help the greater good of our field?
