#include <stdlib.h>
#include <iostream>

#include "graph.hpp"
#include "read.hpp"
#include "boost/graph/dijkstra_shortest_paths_no_color_map.hpp"
#include "heuristics.hpp"
#include "paal_glue.hpp"

#include <signal.h>
extern volatile sig_atomic_t g_stop_signal;

using namespace boost;

//struct EarlyTerminate : public std::exception {};

struct Star {
	int center;
	std::vector<int> terminals;
};

struct Ratio {
	long long weight;
	int terminal_count;

	int inline work() const {
		if (terminal_count == 0) {
			return 0;
		}
		else {
			return terminal_count - 1;
		}
	}

	bool operator<(const Ratio& r) const {
		return weight * r.work() < r.weight * work();
	}
	bool operator<(long long x) const {
		return weight < x*work();
	}

	bool operator>(const Ratio& r) {
		return r < *this;
	}
	bool operator>(long long x) {
		return weight > x*work();
	}

	bool operator<=(const Ratio& r) {
		return !(*this > r);
	}
	bool operator<=(long long x) {
		return !(*this > x);
	}

	bool operator>=(const Ratio& r) {
		return !(*this < r);
	}
	bool operator>=(long long x) {
		return !(*this < x);
	}

	Ratio operator+(const Ratio&r) {
		Ratio result;
		result.weight = weight*r.work() + r.weight*work();
		result.terminal_count = r.work() * work() + 1;

		return result;
	}

	bool operator==(const Ratio& r) {
		return weight*r.work() == r.weight*work();
	}
};


struct ratio_counting_visitor : public dijkstra_visitor<> {
	std::vector<int>& dist;
	Ratio& ratio;

	ratio_counting_visitor(std::vector<int>& dist, Ratio& ratio) : dist(dist), ratio(ratio) { }

	inline void examine_vertex(int v, const Graph& g) {
		if( ratio.work() >= 1 && ratio <= dist[v] ) {
			//fprintf(stderr, "    Current ratio %lld/%d, vertex %d at dist %d, terminating\n", ratio.weight, ratio.work(), v, dist[v]);
			throw EarlyTerminate();
		}

		if(g.terminal_mask[v]) {
			//fprintf(stderr, "    Adding terminal %d at dist %d to star; ", v, dist[v]);
			ratio.weight += dist[v];
			ratio.terminal_count++;
			//fprintf(stderr, "Current ratio %lld/%d\n", ratio.weight, ratio.work());
		}
	}
};


struct best_star_visitor : public dijkstra_visitor<> {

	std::vector<int>& dist;
	std::vector<Edge>& pred_edge;

	Ratio& best_ratio;
	std::vector<Ratio>& best_ratio_at;
	std::vector<bool>& ratio_invalid;
	Star& result;

	bool star_completed;
	Ratio current_ratio;

	best_star_visitor(
			std::vector<int>& dist,
			std::vector<Edge>& pred_edge,
			Ratio& best_ratio,
			std::vector<Ratio>& best_ratio_at,
			std::vector<bool>& ratio_invalid,
			Star& result
			) :
		dist(dist),
		pred_edge(pred_edge),
		best_ratio(best_ratio),
		best_ratio_at(best_ratio_at),
		ratio_invalid(ratio_invalid),
		result(result)
	{
		star_completed = false;
		current_ratio.weight = 0;
		current_ratio.terminal_count = 0;
	}

	inline void examine_vertex(int v, const Graph& g) {

		//Ratio r = best_ratio_at[v] + best_ratio;
		//fprintf(stderr, "  At vertex %d: ratio[v] = %lld/%d, center_ratio = %lld/%d, sum_ratio = %lld/%d dist = %d\n", v, best_ratio_at[v].weight, best_ratio_at[v].work(), best_ratio.weight, best_ratio.work(), r.weight, r.work(), dist[v]);
		if (best_ratio_at[v] + best_ratio >= dist[v]) {
			//fprintf(stderr, "  Invalidating %d\n",  v);
			ratio_invalid[v] = true;
		}


		if (! star_completed ) {
			if ( g.terminal_mask[v] ) {
				//fprintf(stderr, "Adding terminal %d at disance %d to star\n", v, dist[v]);
				result.terminals.push_back(v);

				current_ratio.weight += dist[v];
				current_ratio.terminal_count++;

				//fprintf(stderr, "at %d; current_ratio = %lld/%d, best_ratio = %lld/%d\n", v, current_ratio.weight, current_ratio.work(), best_ratio.weight, best_ratio.work());

				if( current_ratio.work() > 0 && current_ratio <= best_ratio) {
					star_completed = true;
				}
			}
		}
	}

	inline void edge_relaxed(Edge e, const Graph& g) {
		pred_edge[target(e,g)] = e;
	}
};





void contract_star(Graph& g, Star& s, std::vector<Edge>& pred_edge) {
	pred_edge[s.center] = null_edge;

	std::vector<Edge> edges_to_contract;

	for(auto v : s.terminals) {
		// we first gather all edges as contracting will change some vertex indices
		// (and thus invalidate predecessor map)
		while( pred_edge[v] != null_edge ) {
			Edge e = pred_edge[v];

			// some edges might be present on more than one path from center to terminal;
			// we first gather them and later push them to contracted_edges only once
			edges_to_contract.push_back(e);
			v = source(e, g);
		}
	}

	for(auto e : edges_to_contract) {
		g.buy_edge(e);
	}
}



Ratio find_best_ratio_at(Graph& g, int center) {
	std::vector<int> dist(num_vertices(g));

	iterator_property_map< std::vector<int>::iterator,
			property_map<Graph, vertex_index_t>::const_type >
		dist_pm(dist.begin(), get(vertex_index, g));


	Ratio init_ratio;
	init_ratio.terminal_count = 0;
	init_ratio.weight = 0;

	ratio_counting_visitor rv(dist, init_ratio);

	try {
		dijkstra_shortest_paths_no_color_map(g, center,
			distance_map(dist_pm).
			visitor(rv) );
	}
	catch (EarlyTerminate e) {
	}

	return rv.ratio;
}


void find_star(
		Graph& g,
		std::vector<Edge>& pred_edge,
		Ratio& best_ratio,
		int center,
		std::vector<Ratio>& best_ratio_at,
		std::vector<bool>& ratio_invalid,
		Star& result) {

	int n = g.vertex_count;
	std::vector<int> dist(n);
	iterator_property_map< std::vector<int>::iterator,
			property_map<Graph, vertex_index_t>::const_type >
		dist_pm(dist.begin(), get(vertex_index, g));

	best_star_visitor bv(dist,
			pred_edge,
			best_ratio,
			best_ratio_at,
			ratio_invalid,
			result
			);
	result.center = center;

	dijkstra_shortest_paths_no_color_map(g, center,
		distance_map(dist_pm).
		visitor(bv) );
}



void print_emergency_result(FILE* out, Graph& g) {
	std::vector<Edge> mst_solution;
	greedy_2approx(g, std::back_inserter(mst_solution));

	for(auto e : mst_solution) {
		g.partial_solution_weight += e.weight();
		g.partial_solution.push_back(e);
	}

	print_solution(out,g);
}


void contract_till_the_bitter_end(Graph& g) {

	int n = g.vertex_count;

	Ratio inf_ratio;
	inf_ratio.weight = 1;
	inf_ratio.terminal_count = 0;

	std::vector<Ratio> best_ratio_at(n, inf_ratio);
	std::vector<bool> ratio_invalid(n, true);

	Ratio best_ratio;
	int best_ratio_center;

	// perfom heuristics
	buy_zero(g);
	run_all_heuristics(g);

	int round = 1;

	debug_printf("|V| = %d, |E| = %d, |R| = %d\n", g.vertex_count, g.edge_count, g.terminal_count);
	while (g.terminal_count > 1) {
		int invalid_ratio_count = 0;
		debug_printf("Starting round %d\n",round);

		best_ratio = inf_ratio;
		best_ratio_center = n;

		for(int i = 0; (i < n) && (g_stop_signal == 0); i++) {
			if( g.degrees[i] == 0) {
				//fprintf(stderr, "  Vertex %d skipped (contraction orphan)\n", i);
				continue;
			}
			if( ratio_invalid[i] ) {
				invalid_ratio_count++;
				//fprintf(stderr, "  Recomputing ratio at %d\n", i);
				best_ratio_at[i] = find_best_ratio_at(g, i);
				ratio_invalid[i] = false;

			}

			if(best_ratio_at[i] < best_ratio) {
				best_ratio_center = i;
				best_ratio = best_ratio_at[i];
			}
		}

		if (g_stop_signal == 0) {
			debug_printf("Invalid ratios recomputed: %d\n", invalid_ratio_count);
			debug_printf("Best ratio is %lld/%d, centered at %d\n", best_ratio.weight, best_ratio.work(), best_ratio_center);

			Star s;
			std::vector<Edge> pred_edge(n,null_edge);
			// find star; also invalidates ratios at centers too close to this star
			debug_printf("Finding the best star... ");
			find_star(g, pred_edge, best_ratio, best_ratio_center, best_ratio_at, ratio_invalid, s);
			// contract star
			debug_printf("Done\n");
			debug_printf("Star with %zu terminals\n", s.terminals.size());
			debug_printf("Contracting... ");
			contract_star(g, s, pred_edge);
			debug_printf("Done\n");
			round++;
			debug_printf("|V| = %d, |E| = %d, |R| = %d\n", g.vertex_count, g.edge_count, g.terminal_count);
			/*
			debug_printf("Terminals: ");
			for(Vertex v : g.terminals) {
				debug_printf("%d ", v);
			}
			*/
			debug_printf("\n");
		} else { // compute the rest using MST approximation
			print_emergency_result(stdout, g);
			exit(EXIT_SUCCESS);
		}
	
		// perfom heuristics to clean it before next run
		if(g_stop_signal != 0) {
			run_all_heuristics(g);
		}
	}
}



void print_emergency_solution(FILE* out, Graph &g) {
	// MST approximation
	greedy_2approx(g, std::back_inserter(g.partial_solution) );
	g.partial_solution_weight = 0;
	for(auto e : g.partial_solution) {
		g.partial_solution_weight += e.weight();
	}
	print_solution(out, g);
}


