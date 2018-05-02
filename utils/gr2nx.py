import networkx as nx

# nx.Graph
def solution_to_graph(path):
    with open(path, "r") as f:
        value = int(f.readline().split()[1])
        G = nx.Graph()
        for l in f.readlines():
            u, v = l.split()
            G.add_edge(int(u), int(v))

    return (G, value)

# nx.Graph
def input_to_graph(path):
    with open(path, "r") as f:
        G = nx.Graph()
        f.readline()
        f.readline()
        nedges = int(f.readline().split()[1])

        for _ in range(nedges):
            u,v,w = f.readline().split()[1::]
            G.add_edge(int(u),int(v), weight=int(w))

        f.readline()
        f.readline()
        f.readline()
        nterminals = int(f.readline().split()[1])
        for _ in range(nterminals):
            G.node[int(f.readline().split()[1])]['terminal'] = True

    return G

# nx.Graph
def solution_to_graph(path):
    with open(path, "r") as f:
        G = nx.Graph()
        f.readline()

        for line in f:
            u,v = line.split()
            G.add_edge(int(u), int(v))

        return G


# [int]
def graph_to_terminals(G):
    return [v for v in G.nodes() if len(G.node[v]) == 1]

def solution_contains_all_terminals(G, terminals):
    t_sol = {v for v in G.nodes() if v in terminals}

    return len(t_sol.difference(set(terminals))) == 0
