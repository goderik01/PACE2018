#ifndef PAAL_GLUE_HPP
#define PAAL_GLUE_HPP

#include "graph.hpp"
#include "paal/greedy/steiner_tree_greedy.hpp"
#include "paal/steiner_tree/dreyfus_wagner.hpp"
//#include "paal/steiner_tree/zelikovsky_11_per_6.hpp"

template < typename OutputIterator >
void greedy_2approx(const Graph& g, OutputIterator out) {
	paal::steiner_tree_greedy(g, out, WeightMap(), &g.terminal_mask[0]);
}

// paal::data_structures::graph_metric assumes num_vertices is globally visible
using boost::num_vertices;

template < typename OutputIterator >
void dreyfus_wagner(const Graph& g, OutputIterator out) {
	paal::data_structures::graph_metric<Graph, int> metric{g};

	std::vector<Vertex> nonterminals;
	for (Vertex v = 0; v < g.vertex_count; v++)
		if (!g.is_terminal(v)) nonterminals.push_back(v);

	auto dw = paal::make_dreyfus_wagner<16>(metric, g.terminals, nonterminals);
	dw.solve();

  for (const auto& e : dw.get_edges()) {
    *out++ = g.find_edge(e.first, e.second);
  }
}

#endif
