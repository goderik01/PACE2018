#ifndef PAAL_GLUE_HPP
#define PAAL_GLUE_HPP

#include "graph.hpp"
#include "boost/graph/kruskal_min_spanning_tree.hpp"
#include "boost/range/algorithm/sort.hpp"
#include "boost/range/algorithm/unique.hpp"
#include "boost/range/algorithm/copy.hpp"

// Heavily modified paal::steiner_tree_greedy
template < typename OutputIterator >
void greedy_2approx(const Graph& g, OutputIterator out) {
	std::vector<Weight> distance(g.vertex_count, std::numeric_limits<Weight>::max());

	const auto cmp = [&](Vertex a, Vertex b) { return distance[a] < distance[b]; };
	Heap<Vertex, decltype(cmp), std::vector<unsigned>, 4> heap{cmp};
	heap.map.assign(g.vertex_count, heap.not_in_heap);

	std::vector<Vertex> nearest_terminal(g.vertex_count);
	for (int i = 0; i < (int)g.terminals.size(); i++) {
		Vertex t = g.terminals[i];
		nearest_terminal[t] = i;
		distance[t] = 0;
		heap.push(t);
	}

	// compute voronoi diagram each vertex get nearest terminal and last edge on
	// path to nearest terminal
	std::vector<Edge> vpred(g.vertex_count);

	Dummy dummy;
	Dijkstra(g, distance, dummy, heap, dummy,
		[&](Edge e) {
			nearest_terminal[e.target()] = nearest_terminal[e.source()];
			vpred[e.target()] = e;
		}, dummy
	);

	// computing distances between terminals
	// creating terminal_graph
	Graph tmp(g.terminals.size());
	for (int i = 0; i < (int)g.edge_list.size(); i++) {
		auto e = g.edge_list[i];
		Vertex st = nearest_terminal[e.source()];
		Vertex tt = nearest_terminal[e.target()];
		if (st != tt) {
			Weight d = distance[e.source()] + distance[e.target()] + e.weight();
			tmp.add_edge(st, tt, d, EDGE_EXT_REF, i);
		}
	}

	// computing spanning tree on terminal_graph
	std::vector<Edge> terminal_edges;
	boost::kruskal_minimum_spanning_tree(tmp, std::back_inserter(terminal_edges));

	// computing result
	std::vector<Edge> tree_edges;
	for (auto t_edge : terminal_edges) {
		Edge e = g.edge_list[t_edge.orig_edge()];
		tree_edges.push_back(e);
		for (auto v : { e.source(), e.target() }) {
			while (g.terminals[nearest_terminal[v]] != v) {
				tree_edges.push_back(vpred[v]);
				v = vpred[v].source();
			}
		}
	}

	// because in each voronoi region we have unique patch to all vertex from
	// terminal, result graph contain no cycle
	// and all leaf are terminal
	boost::sort(tree_edges);
	boost::copy(boost::unique(tree_edges), out);
}


#endif
