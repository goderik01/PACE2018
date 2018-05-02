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
	Star& result;

	bool &star_completed;
	Ratio &current_ratio;

	best_star_visitor(
			std::vector<int>& dist,
			std::vector<Edge>& pred_edge,
			Ratio& best_ratio,
			std::vector<Ratio>& best_ratio_at,
			Star& result,
			bool& sc,
			Ratio& real_ratio
			) :
		dist(dist),
		pred_edge(pred_edge),
		best_ratio(best_ratio),
		best_ratio_at(best_ratio_at),
		result(result),
		star_completed(sc),
		current_ratio(real_ratio)
	{
		star_completed = false;
		current_ratio.weight = 0;
		current_ratio.terminal_count = 0;
	}

	inline void examine_vertex(int v, const Graph& g) {
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





Vertex contract_star(Graph& g, Star& s) {
	std::vector<Edge> edges_to_contract;

	s.terminals.push_back(s.center);
	// UGLY DIRTY HACK !!!
	std::swap(s.terminals, g.terminals);
	greedy_2approx(g, std::back_inserter(edges_to_contract));
	std::swap(s.terminals, g.terminals);

	Vertex ret = -1;
	for(auto e : edges_to_contract) {
		ret = g.buy_edge(e);
	}
	return ret;
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
		Star& result) {

	bool completed;
	Ratio real_ratio;
	std::vector<int> dist(g.vertex_count);

	best_star_visitor bv(dist,
			pred_edge,
			best_ratio,
			best_ratio_at,
			result,
			completed,
			real_ratio
			);
	result.center = center;

	dijkstra_shortest_paths_no_color_map(g, center,
		distance_map(&dist[0]).
		visitor(bv) );

#ifdef NDEBUG
	if (false) debug_printf(
#else
	Assert(real_ratio == best_ratio,
#endif
		"Ratio expected %lld/%d but got only %lld/%d\n"
		"Got star centered at %i with %i terminals\n",
		best_ratio.weight, best_ratio.work(), real_ratio.weight, real_ratio.work(),
		result.center, (int)result.terminals.size());
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

	std::vector<Weight> dist(g.vertex_count);
	const auto cmp = [&](Vertex a, Vertex b) { return dist[a] < dist[b]; };
	Heap<Vertex, decltype(cmp), std::vector<unsigned>, 4> heap{cmp};
	heap.map.assign(g.vertex_count, heap.not_in_heap);

	int round = 1;

	debug_printf("|V| = %d, |E| = %d, |R| = %d\n", g.vertex_count, g.edge_count, g.terminal_count);
	while (g.terminal_count > CONST_STOP_CONTRACTIONS_AT_NUMBER_TERMINALS) { TIMER_BEGIN {
		int invalid_ratio_count = 0;
		debug_printf("Starting round %d\n",round);

		best_ratio = inf_ratio;
		best_ratio_center = n;

		int isolated_counter = 0;
		for (int i = 0; i < n; i++) {
			CHECK_SIGNALS(goto interrupted);

			if( g.degrees[i] == 0) {
				isolated_counter++;
				//fprintf(stderr, "  Vertex %d skipped (contraction orphan)\n", i);
				continue;
			}
			if( ratio_invalid[i]) {
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
		debug_printf("Found %d isolated vertices\n", isolated_counter);

		CHECK_SIGNALS(goto interrupted);

		debug_printf("Invalid ratios recomputed: %d\n", invalid_ratio_count);
		debug_printf("Best ratio is %lld/%d, centered at %d\n", best_ratio.weight, best_ratio.work(), best_ratio_center);

		Star s;
		std::vector<Edge> pred_edge(n,null_edge);
		// find star; also invalidates ratios at centers too close to this star
		debug_printf("Finding the best star... ");
		find_star(g, pred_edge, best_ratio, best_ratio_center, best_ratio_at, s);
		// contract star
		debug_printf("Done\n");
		debug_printf("Star with %zu terminals\n", s.terminals.size());
		debug_printf("Contracting... ");
		Vertex c = contract_star(g, s);
		assert(c != -1);
		assert(c != -2);

		dist.assign(g.vertex_count, std::numeric_limits<Weight>::max());
		dist[c] = 0;
		heap.push(c);
		Dijkstra(g, dist, dummy, heap, [&](Vertex v){
			if (best_ratio_at[v] >= dist[v])
				ratio_invalid[v] = true;
		});

		debug_printf("Done\n");
		round++;
		debug_printf("|V| = %d, |E| = %d, |R| = %d\n", g.vertex_count, g.edge_count, g.terminal_count);
		/*
		debug_printf("Terminals: ");
		for(Vertex v : g.terminals) {
			debug_printf("%d ", v);
		}
		*/

		CHECK_SIGNALS(goto interrupted);

		// perfom heuristics to clean it before next run
		run_noninvalidating_heuristics(g);
	} TIMER_END("round took: %lg s\n\n", timer); }
	
	if(CONST_STOP_CONTRACTIONS_AT_NUMBER_TERMINALS > 1)
		greedy_2approx(g, std::back_inserter(g.partial_solution));
	
	return;

	interrupted:
	debug_printf("%s interrupted\n", __func__);
	greedy_2approx(g, std::back_inserter(g.partial_solution));
}
