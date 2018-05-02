#!/usr/bin/env python3

import utils.gr2nx as gr2nx
import argparse
import networkx as nx

parser = argparse.ArgumentParser(description='Connection checker of SteinerTreeHeuristic (CUiB).')
parser.add_argument('graphfile', metavar='INPUTFILE', type=str, nargs=1,
                     help='file containing input graph')
parser.add_argument('ansfile', metavar='ANSWERFILE', type=str, nargs=1,
                     help='file containing answer')

args = parser.parse_args()

inG = gr2nx.input_to_graph(args.graphfile[0])
solG = gr2nx.solution_to_graph(args.ansfile[0])

if not nx.is_connected(solG):
    for c in nx.connected_components(solG):
        print(c)
        print([ inG[u][v]['weight'] for u,v in solG.subgraph(c).edges ])
        print("--------------------")
    assert False, "The solution is not connected"

for t in gr2nx.graph_to_terminals(inG):
    assert solG.has_node(t), "Terminal {} not in solution".format(t)

print("The solution is connected")
