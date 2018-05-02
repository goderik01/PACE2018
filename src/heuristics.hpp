#ifndef HEURISTICS_HPP
#define HEURISTICS_HPP

#include <cassert>
#include <vector>

#include "graph.hpp"
#include "debug.hpp"

void handle_small_degrees(Graph& g) {
	for(Vertex v = 0; v < g.vertex_count; v++) {
		if( g.degrees[v] == 1) {
			if( g.is_terminal(v) ) {
				g.buy_edge( g.inc_edges[v]->front() );
			}
			else {
				g.remove_edge( g.inc_edges[v]->front() );
			}
		}
		else if(g.degrees[v] == 2 && !g.is_terminal(v)) {
			g.suppress_vertex(v);
		}
	}
}

Edge cheapest_edge_from(Graph& g, Vertex v) {
	Edge min_edge = null_edge;
	for(auto e : *g.inc_edges[v]) {
		if( e.weight() < min_edge.weight() ) {
			min_edge = e;
		}
	}
	return min_edge;
}

void shortest_edge_between_terminals(Graph& g) {
	std::vector<Edge> possible_contractions;
	std::vector<bool> affected_vertices(g.vertex_count, false);

	if(g.terminal_count == 1) { //we are already done
		return;
	}
	for(auto t : g.terminals) {
		Edge e = cheapest_edge_from(g,t);
		assert(e != null_edge);

		if( g.is_terminal(e.target()) ) {
			possible_contractions.push_back(e);
		}
	}

	for(auto e : possible_contractions ) {
		if( !affected_vertices[e.source()] && !affected_vertices[e.target()] ) {
			affected_vertices[e.source()] = true;
			affected_vertices[e.target()] = true;
			g.buy_edge(e);
		}
	}
}


void run_all_heuristics(Graph& g) {
	handle_small_degrees(g);
	shortest_edge_between_terminals(g);
}


#endif // HEURISTICS_HPP
