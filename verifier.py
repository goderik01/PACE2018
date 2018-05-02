#!/usr/bin/env python3
import argparse

parser = argparse.ArgumentParser(description='Validity checker of SteinerTreeHeuristic (CUiB).')
parser.add_argument('graphfile', metavar='INPUTFILE', type=str, nargs=1,
                     help='file containing input graph')
parser.add_argument('ansfile', metavar='ANSWERFILE', type=str, nargs=1,
                     help='file containing answer')
                     
args = parser.parse_args()

## Part 1: Check that the input file containing graph follows the specified format.
graph = []
terminals = []
alledges = {}
with open(args.graphfile[0], 'r') as f:
    assert(f.readline().strip() == "SECTION Graph")
    
    s, n = f.readline().split()
    assert(s == "Nodes")
    n = int(n)
    s, m = f.readline().split()
    assert(s == "Edges")
    m = int(m)
    
    graph = [[] for _ in range(n)]
    for _ in range(m):
        s, u, v, w = f.readline().split()
        assert(s == "E")
        u, v, w = int(u)-1, int(v)-1, int(w) # File is 1-indexed, we are 0
        alledges[(min(u,v), max(u,v))] =  w
        graph[u].append((v, w))
        graph[v].append((u, w))
        
    assert(f.readline().strip() == "END")
    assert(f.readline().strip() == "")
    assert(f.readline().strip() == "SECTION Terminals")
    s, t = f.readline().split()
    assert(s == "Terminals")
    t = int(t)
    for _ in range(t):
        s, i = f.readline().split()
        assert(s == "T")
        i = int(i)-1
        terminals.append(i)
        
    assert(f.readline().strip() == "END")
    assert(f.readline().strip() == "")
    assert(f.readline().strip() == "EOF")
    
    
## Part 2: Read the suggested solution, and build the graph induced on these edges. Then check that all terminals are reachable, and that there are no cycles. Also check that the provided weigth is correct, that all our edges in fact exists, and that we don't give the same edge more than once.

sedges = [] # Solution edges
seenedges = set() # Set of already processed solution edges
with open(args.ansfile[0], 'r') as f:
    s, value = f.readline().split()
    assert(s == "VALUE")
    v = int(value)
    sedges = [list(map(lambda x: int(x)-1, line.split())) for line in f.readlines() if len(line.strip()) > 0]
    for e in sedges:
        assert((min(e), max(e)) in alledges), "nonexistent edge ({0},{1})\n".format(min(e)+1,max(e)+1) # Check that solution edge exists
        assert((min(e), max(e)) not in seenedges), "duplicate edge ({0},{1})\n".format(min(e)+1,max(e)+1) # Check that solution edge is provided only once
        seenedges.add((min(e), max(e)))
        e.append(alledges[(min(e), max(e))])
    

assert(sum(x[2] for x in sedges) == v)

sgraph = [[] for _ in range(n)] # Solution graph
for u, v, w in sedges:
    sgraph[u].append((v, w))
    sgraph[v].append((u, w))
    
    
# Check reachability. At the same time, double check that we don't output a cycle. We do this with something in-between a BFS and a DFS (it is a BFS with a stack rather than a queue. Works fine for cycle detection and reachability)
parent = [-2]*n
parent[terminals[0]] = -1
q = [terminals[0]]
while len(q) > 0:
    node = q.pop()
    for nb, w in sgraph[node]:
        if parent[node] == nb:
            continue
        elif parent[nb] <= -2:
            parent[nb] = node
            q.append(nb)
        else:
            print("Cycle: ", end="")
            while node != parent[nb]:
                print("{} ".format(node+1), end="")
                node = parent[node]
            print("{} {}".format(node+1,nb+1))
            assert False, "Detected cycle"
            
for term in terminals:
    assert (parent[term] >= -1), "Terminal {} not reachable from terminal {}".format(term+1, terminals[0]+1)

print("Seems fine!")
