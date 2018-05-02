#ifndef HEURISTICS_HPP
#define HEURISTICS_HPP

#include <cassert>
#include <vector>
#include "boost/graph/dijkstra_shortest_paths_no_color_map.hpp"

#include "graph.hpp"
#include "debug.hpp"

using namespace boost;

struct EarlyTerminate : public std::exception {};

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


//This keeps connectivnes but it might produce unnecerary edges and may be cycles as well.
//However, It shouldnt be a problem in the competition.
void buy_zero(Graph& g){
	int count = 0;
	bool rerun=false;
	std::vector<Edge> to_buy;
	std::vector<bool> marked(g.vertex_count,false);

	debug_printf("START to buy zero EDGES! \n------\n");

	for(auto e : g.edge_list){
//		debug_printf("%d source %d target", e.source(),e.target());
		if(e.weight()==0 ){
			to_buy.push_back(e);
			count++;
		}
	}

	for(auto e : to_buy){
		if ((marked[e.source()]!=true) && (marked[e.target()]!=true)) {
			marked[e.source()]=true;
			marked[e.target()]=true;
			debug_printf("bought edge %d source %d target  %d e.weight \n", e.source(),e.target(), e.weight() );
			g.zero_edges[e.source()]->push_back(e);
			g.contract_edge(e);
		}else{
			debug_printf("NOT bought edge %d source %d target  %d e.weight \n", e.source(),e.target(), e.weight() );
			rerun=true;
		}	
	}
	if(rerun == true){
		debug_printf("Additional round");
		buy_zero(g);
	}
		//	assert(false);
	debug_printf("BOUGHT ZERO EDGES %d! \n------\n, count", count);
}


struct neighbor_counting_visitor : public dijkstra_visitor<> {
	Vertex source;
	int visited_neighbors;

	neighbor_counting_visitor(Vertex source) : source(source), visited_neighbors(0)
	{}

	inline void examine_vertex(int v, const Graph& g) {
		if ( g.find_edge(source, v) != null_edge ) {	// is there an edge between v and the source?
			//debug_printf("Another neighbor %d of %d visited and closed", v, source);
			visited_neighbors++;
		}

		if( visited_neighbors == g.degrees[source] ) {
			//debug_printf("Visited the last neighbor of %d\n", source);
			throw EarlyTerminate();
		}
	}
};

void delete_edges_shortest_path(Graph &g)
{
	std::vector<Edge> to_del;
	unsigned count = 0;

	for(Vertex v = 0; v < num_vertices(g) ; ++v) {	// for every vertex
		// run dijkstra with neighbor_counting_visitor
		std::vector<int> dist(num_vertices(g));	// maybe just zero it?
		iterator_property_map< std::vector<int>::iterator,
				property_map<Graph, vertex_index_t>::const_type >
				dist_pm(dist.begin(), get(vertex_index, g));
		neighbor_counting_visitor nc(v);
		try {
			dijkstra_shortest_paths_no_color_map(g, v,
				distance_map(dist_pm).
				visitor(nc) );
		}
		catch (EarlyTerminate e) {
		}
		// edges vs. shortest path & possibly delete
		incidence_list_t* l = g.inc_edges[v];
		for ( auto it = l->begin(); it != l->end(); ++it ) {
			Vertex u = (v == it->source()) ? it->target() : it->source();	// the other end
			if ( it->weight() > dist[u] ) {	// edge is longer than the shortest path
				debug_printf("[delete_edges_SP]\tdeleted edge %d source %d target and weight %d > %d the length of the shortest path.\n", v, u, it->weight(), dist[u]);
				to_del.push_back(*it);
				count++;
			}
		}
	}
	for(auto e : to_del){
		g.remove_edge(e);
	}
	debug_printf("[delete_edges_SP]\tdeleted %u edges!\n", count);
}

void run_all_heuristics(Graph& g) {
	handle_small_degrees(g);
	shortest_edge_between_terminals(g);
	delete_edges_shortest_path(g);
	handle_small_degrees(g);
	shortest_edge_between_terminals(g);
}


#endif // HEURISTICS_HPP
