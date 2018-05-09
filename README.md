# SteinerTreeHeuristics

This repository contains a program that computes a heuristic approximation for the Weighted Steiner Tree problem.
It was created for the PACE challenge 2018 Track C (heuristic track) <https://pacechallenge.wordpress.com/pace-2018/>.

## Compile instructions
The root folder contains a Makefile with all the neccessary targets.
Our program uses the [*boost library*](https://www.boost.org/) (it was tested with version 1.66.0).
We recommend to dowload its sources (`make boost` does the job for you).
Then, you can compile the sources by running `make` which uses g++ compiler.


## Input/Output description
To download public test data use `make data` which downloads test public data from PACE challenge into directory *data/*.

The input&output format is described on the [PACE Challenge website](https://pacechallenge.wordpress.com/pace-2018/).


## How to run the program
The compiled binary (*bin/star_contractions_test*) expects input data on the standard input.
The program run until it recieves the SIGTERM signal. 
After that it outputs a solution to the standard output within 30 seconds.


## Concise description of our algorithm

Our algorithm proceeds with the following steps:

1. Run simple 1-safe heuristics.
   * Handle small degree vertices.
        - Suppress Steiner vertices of degree two.
        - Remove Steiner verticies of degree one.
        - Buy edges incident to terminal of degree one.
   * Buy zero cost edges.
   * Run *shortest path test*:
     Remove edges longer than the length of the shortest path between the endpoints.
   * Run [Terminal distance test](https://onlinelibrary.wiley.com/doi/pdf/10.1002/net.10035):
     Buy the lightest edge on a cut if the second lightest edge in the cut is heavier than shortest path between
     any two terminals that are on different sides of the cut.
2. Perform star contractions based on paper [Parameterized Approximation Schemes for Steiner Trees with Small Number of Steiner Vertices](https://arxiv.org/abs/1710.00668) for at most 10 minutes.
     * Buy the star (center is any vertex and lists are Terminals) in the metric closure that has the best ratio
       (sum of the weights over number of terminals minus one).
     * After each step run following heuristics: handle small degree Steiner vertices and fast special case of shortest path test.
     * If time runs out, finish the partial solution using MST-approximation.
3. Improve the solution by following methods: 
     * *Local search using MST-approximation.*
       From the solution compute the MST-approximation on all terminals and all branching Steiner verticies
       plus a few randomly chosen promising Steiner vertices.
       We clean up the solution and repeat without additional random vertices.
    * *Local search using Dreyfus-Wagner partition.*
      Inspired by [Dreyfus-Wagner FPT algorithm](https://doi.org/10.1002/net.3230010302) we derive a heuristic that 
      obtains the partition of the vertices given one of the promising solutions and then compute an optimal Steiner tree with this structure.
      This step is much more time consuming so we run it only sparsely.

## Resources

## Acknowledgments
#### Authors
+ **Radek Hušek** Computer Science Institute of Charles University, Faculty of Mathematics and Physics, Charles University, Prague, Czech Republic
+ **Tomáš Toufar** Computer Science Institute of Charles University, Faculty of Mathematics and Physics, Charles University, Prague, Czech Republic
+ **Dušan Knop** Department of Informatics, University of Bergen, Norway
+ **Tomáš Masařík** Department of Applied Mathematics, Faculty of Mathematics and Physics, Charles University, Prague, Czech Republic
+ **Eduard Eiben** Department of Informatics, University of Bergen, Norway

We thank to **Torstein Strømme** for helpful discussions.

#### Support
The work was supported by the project GAUK 1514217 and the project SVV–2017–260452.
R. Hušek, D. Knop, T. Masařík we also supported by the Center of Excellence – ITI, project P202/12/G061 of GA ČR.
D. Knop was supported by the project NFR MULTIVAL of the Research Council of Norway.
E. Eiben was supported by Pareto-Optimal Parameterized Algorithms (ERC Starting Grant 715744).




