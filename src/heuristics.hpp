#ifndef HEURISTICS_HPP
#define HEURISTICS_HPP

#include <cassert>
#include <vector>
#include <unordered_set>
#include "boost/graph/dijkstra_shortest_paths_no_color_map.hpp"
#include "boost/functional/hash.hpp"
#include "tdist.hpp"

#include "graph.hpp"
#include "debug.hpp"
#include <signal.h>
extern volatile sig_atomic_t g_stop_signal;

using namespace boost;

void handle_small_Steiner_degrees(Graph& g) {
	int deg1_steiner = 0, suppress = 0;

	TIMER_BEGIN {

	for(Vertex v = 0; v < g.vertex_count; v++) {
		if (!g.is_terminal(v)) {
			if( g.degrees[v] == 1) {
				g.remove_edge(g.inc_edges[v].front());
				deg1_steiner++;
			} else if(g.degrees[v] == 2) {
				g.suppress_vertex(v);
				suppress++;
			}
		}
	}

	} TIMER_END("  %s: steiner %d, suppress %d in %lg s\n",
		__func__, deg1_steiner, suppress, timer);
}

void handle_small_terminal_degrees(Graph& g, std::vector<bool>& invalid_map) {
	int deg1_terms = 0;

	TIMER_BEGIN {

	for(Vertex v = 0; v < g.vertex_count; v++) {
		if(( g.degrees[v] == 1) && (g.is_terminal(v))) {
				g.buy_edge(g.inc_edges[v].front());
				invalid_map[v] = true;
				deg1_terms++;
		}
	}

	} TIMER_END("  %s: terms %d in %lg s\n",
		__func__, deg1_terms, timer);
}

Edge cheapest_edge_from(Graph& g, Vertex v) {
	Edge min_edge = null_edge;
	for(auto e : g.inc_edges[v]) {
		if( e.weight() < min_edge.weight() ) {
			min_edge = e;
		}
	}
	return min_edge;
}

void shortest_edge_between_terminals(Graph& g) {
	int count = 0;
	TIMER_BEGIN {
	std::vector<Edge> possible_contractions;

	if (g.terminal_count == 1) return;

	for (auto t : g.terminals) {
		Edge e = cheapest_edge_from(g,t);
		assert(e != null_edge);

		if (g.is_terminal(e.target()))
			possible_contractions.push_back(e);
	}

	for (auto e : possible_contractions) count += (g.buy_edge(e) != -1);

	} TIMER_END("  %s: %d edges in %lg s\n", __func__, count, timer);
}


void buy_zero(Graph& g) {
	int count = 0;

	TIMER_BEGIN {
		std::vector<Edge> to_buy;

		for (auto e : g.edge_list) {
			if (e.weight() == 0) to_buy.push_back(e);
		}

		for (auto e : to_buy) count += (g.buy_edge(e) != -1);

	} TIMER_END("  %s: %d zero edges in %lg s\n", __func__, count, timer);
}

//Easy deletions without dijkstra, only cheries.
void delete_edges(Graph& g) {
	int count = 0;
	int rounds = 0;
	bool rerun=false;

	TIMER_BEGIN { do {
	rerun = false;
	rounds++;
	std::vector<Edge> to_del;

	std::vector<bool> marked(g.vertex_count,false);

	for(auto e : g.edge_list){
//    debug_printf("%d source %d target", e.source(),e.target());
		auto i = &g.inc_edges[e.source()];
		auto j = &g.inc_edges[e.target()];
		auto it = i->begin();
		auto jt = j->begin();

		while (it != i->end() && jt != j->end()) {
			if (it->target() == jt->target()) {
				if (jt->weight() + it->weight() <= e.weight()) {
					to_del.push_back(e);
					break;
				}
				++it; ++jt;
			} else if (it->target() < jt->target()) {
				++it;
			} else {
				++jt;
			}
		}
	}
	//Rewrite to separate function perhaps.
	for(auto e : to_del){
		if(marked[e.source()]!=true&&marked[e.target()]!=true){
			marked[e.source()]=true;
			marked[e.target()]=true;
			//debug_printf("deleted edge %d source %d target  %d e.weight \n", e.source(),e.target(), e.weight() );
			g.remove_edge(e);
			count++;
		}else{
			//debug_printf("NOT deleted edge %d source %d target  %d e.weight \n", e.source(),e.target(), e.weight() );
			rerun=true;
		}
	}

	} while (rerun);
	} TIMER_END("  %s: deleted %d edges in %lg s\n", __func__, count, timer);
}


// WARNING: requires no 0-edges in graph
void delete_edges_shortest_path(Graph &g) {
	unsigned count = 0;

	TIMER_BEGIN {
		std::vector<Weight> dist(num_vertices(g));
		const auto cmp = [&](Vertex a, Vertex b) { return dist[a] < dist[b]; };
		Heap<Vertex, decltype(cmp), std::vector<unsigned>, 4> heap{cmp};
		heap.map.assign(g.vertex_count, heap.not_in_heap);

		std::vector<int> pred_count(num_vertices(g), 0);
		std::vector<char> neighbor_mask(num_vertices(g), 0);

		for (Vertex v = 0; v < num_vertices(g); ++v) {
			dist.assign(g.vertex_count, std::numeric_limits<Weight>::max());
			int neighbors_to_go = g.inc_edges[v].size();
			for (auto e : g.inc_edges[v]) neighbor_mask[e.target()] = true;
			pred_count[v] = 1;

			dist[v] = 0;
			heap.push(v);
			try {
				Dummy pred_map;
				Dijkstra(g, dist, pred_map, heap,
					[&](Vertex v) {
						if (neighbor_mask[v]) {
							neighbors_to_go--;
							neighbor_mask[v] = false;
						}
						if (neighbors_to_go <= 0) throw EarlyTerminate();
					},
					[&](Edge e) { pred_count[e.target()] = 1; },
					[&](Edge e) {
						if (dist[e.source()] + e.weight() == dist[e.target()])
							pred_count[e.target()]++;
					}
				);
			} catch (EarlyTerminate e) {}

			while (!heap.empty()) heap.pop();

			// edges vs. shortest path & possibly delete
			std::vector<Edge> to_remove;
			for (auto e : g.inc_edges[v]) {
				assert(e.source() == v);
				Vertex u = e.target();
				if (e.weight() > dist[u] || (e.weight() == dist[u] && pred_count[u] > 1))
					to_remove.push_back(e);
			}

			count += to_remove.size();
			for (auto e : to_remove) g.remove_edge(e);
		}

	} TIMER_END("  %s: deleted %u edges in %lg s\n", __func__, count, timer);
}


std::vector<Vertex> find_branching_vertices(const Graph& g) {
	std::vector<int> degrees(g.vertex_count, 0);

	for (auto e : g.partial_solution) {
		degrees[e.source()]++;
		degrees[e.target()]++;
	};

	std::vector<Vertex> ret;
	for (Vertex v = 0; v < g.vertex_count; v++)
		if (degrees[v] >= 3) ret.push_back(v);

	return ret;
}

#include "paal_glue.hpp"

template < typename Out >
Weight clean_up_solution(const Graph &g, const std::vector<Edge>& sol, Out out) {
	Graph tmp{g.vertex_count};
	for (int i = 0; i < (int)sol.size(); i++) {
		tmp.add_edge(sol[i].source(), sol[i].target(), 0, EDGE_EXT_REF, i);
	}

	Weight w = 0;
	int terminals_seen = 1;
	int non_tree_edges_found = 0;
	std::vector<bool> marked(g.vertex_count, false);
	DFS(tmp, g.terminals[0],
		[&](Vertex v, Vertex p, Edge e) {
			if (g.is_terminal(v)) {
				marked[v] = true;
				terminals_seen++;
			}
			if (marked[v]) {
				marked[p] = true;
				Edge g_edge = sol[e.orig_edge()];
				w += g_edge.weight();
				*out++ = g_edge;
			}
		},
		[&](Edge) { non_tree_edges_found++; }
	);

	if (non_tree_edges_found > 0)
		debug_printf("%d non-tree edges found!\n", non_tree_edges_found);
	Assert(terminals_seen == (int)g.terminals.size(),
		"G has %d terminals but we found %d\n", (int)g.terminals.size(), terminals_seen);

	return w;
}

template < typename Out >
Weight refine_solution(Graph &g, const std::vector<Vertex>& fake_terminals, Out out) {
	Weight w = 0;
	TIMER_BEGIN {

	size_t real_terminal_count = g.terminals.size();

	for (auto v : fake_terminals) g.mark_terminal(v);
	for (auto v : find_branching_vertices(g)) g.mark_terminal(v);

	std::vector<Edge> new_sol;
	greedy_2approx(g, std::back_inserter(new_sol));

	while (g.terminals.size() > real_terminal_count)
		g.unmark_terminal(g.terminals[g.terminals.size() - 1]);

	w = clean_up_solution(g, new_sol, out);

	} TIMER_END("  %s: new weight %u (was %u) in %lg s\n", __func__,
		w, g.partial_solution_weight(), timer);
	return w;
}

bool structure_rotate(std::vector<std::pair<int, int>>& S, int node, bool right) {
#define Node(x, right) (right ? S[x].second : S[x].first)
	const auto is_term_node = [&](int n) {
		return S[n].first == -1;
	};

	int son = Node(node, right);
	if (is_term_node(node) || is_term_node(son)) return false;

	Node(node, right) = Node(son, right);
	Node(son, right) = Node(son, !right);
	Node(son, !right) = Node(node, !right);
	Node(node, !right) = son;

#define A(x, y) Assert(Node(x, y) < x, \
	"node(%d): %d %d  son(%d): %d %d\n", node, Node(node, 0), Node(node, 1),\
	son, Node(son, 0), Node(son, 1))
	A(node, 0);
	A(node, 1);
	A(son, 0);
	A(son, 0);
#undef A
#undef Node

	return true;
}

auto get_solution_structure(const Graph& g) {
	std::vector<std::pair<int, int>> ret;
	std::vector<int> index(g.vertex_count, -1);
	std::vector<int> degrees(g.vertex_count, 0);

	for (int i = 1; i < (int)g.terminals.size(); i++) {
		index[g.terminals[i]] = ret.size();
		ret.push_back({-1, g.terminals[i]});
	}

	Graph tmp{g.vertex_count};
	for (auto e : g.partial_solution)
		tmp.add_edge(e.source(), e.target(), 0, -1, -1);

	auto finish_vertex = [&](Vertex v, Vertex, Edge tree_edge) {
		std::vector<int> children;
		for (auto e : tmp.inc_edges[v]) if (e != tree_edge) {
			assert(index[e.target()] != -1);
			children.push_back(index[e.target()]);
		}

		// v is also a terminal
		if (index[v] != -1) children.push_back(index[v]);

		assert(children.size() > 0);
		while (children.size() > 1) {
			int i = rand() % children.size();
			int j = rand() % (children.size() - 1);
			if (i <= j) j++;
			else std::swap(i, j);

			ret.push_back({children[i], children[j]});
			children[i] = ret.size() - 1;
			if (j != (int)children.size() - 1) std::swap(children[j], children.back());
			children.pop_back();
		}
		index[v] = children[0];
	};

	DFS(tmp, g.terminals[0], finish_vertex, dummy);
	finish_vertex(g.terminals[0], -1, null_edge);

	return ret;
}

template < typename Out >
Weight dreyfus_zid(Graph& g, const std::vector<std::pair<int, int>>& structure, Out out) {
	debug_printf("\nCalling %s\n", __func__);
	Weight weight = 0;

	if (structure.size() * g.vertex_count > 400*1000*1000) {
		debug_printf("Graph is too large (%d vertices, %d terminals), givin up!\n",
			g.vertex_count, (int)g.terminals.size());
		return -1;
	}

	TIMER_BEGIN {

	struct S {
		std::vector<Weight> dist;
		std::vector<CompressedEdge> pred_e;
	};
	std::vector<S> stack;

	Weight* dist = nullptr;
	const auto cmp = [&](Vertex a, Vertex b) { return dist[a] < dist[b]; };
	Heap<Vertex, decltype(cmp), std::vector<unsigned>, 4> heap{cmp};
	heap.map.assign(g.vertex_count, heap.not_in_heap);

	for (int i = 0; i < (int)structure.size(); i++) {
		CHECK_SIGNALS(return -1);

		stack.push_back({});
		auto& cur = stack.back();

		if (structure[i].first == -1) {
			Vertex t = structure[i].second;
			cur.pred_e.assign(g.vertex_count, CompressedEdge());
			cur.dist.assign(g.vertex_count, std::numeric_limits<Weight>::max());
			cur.dist[t] = 0;

			heap.push(t);
			dist = &cur.dist[0];
			Dijkstra(g, cur.dist, dummy, heap, dummy, [&](Edge e){
				cur.pred_e[e.target()] = g.compress_edge(e);
			});
		} else {
			cur.pred_e.assign(g.vertex_count, CompressedEdge());
			std::swap(cur.dist, stack[structure[i].first].dist);
			auto& s_dist = stack[structure[i].second].dist;
			for (int i = 0; i < g.vertex_count; i++)
				cur.dist[i] += s_dist[i];
			s_dist.clear();

			for (Vertex v = 0; v < g.vertex_count; v++) heap.push(v);
			dist = &cur.dist[0];
			Dijkstra(g, cur.dist, dummy, heap, dummy, [&](Edge e){
				cur.pred_e[e.target()] = g.compress_edge(e);
			});
		}
	}

	std::vector<std::pair<int, Vertex>> ret_stack;
	ret_stack.push_back({stack.size() - 1, g.terminals[0]});

	while (!ret_stack.empty()) {
		const int i = ret_stack.back().first;
		const auto& cur = stack[i];
		Vertex v = ret_stack.back().second;
		ret_stack.pop_back();

		while (!cur.pred_e[v].is_null()) {
			Edge e = g.decompress_edge(cur.pred_e[v]);
			*out++ = e;
			weight += e.weight();
			v = e.source();
		}

		if (structure[i].first != -1) {
			ret_stack.push_back({structure[i].first, v});
			ret_stack.push_back({structure[i].second, v});
		}
	}

	} TIMER_END("%s: %lg s\n", __func__, timer);

	return weight;
}

void run_cheap_heuristics(Graph& g) {
	std::vector<bool> invalid_map(g.vertex_count, true);
	int edge_count = num_edges(g);
	int prev_count = -1;
	while(prev_count != edge_count) {
		handle_small_terminal_degrees(g, invalid_map);
		handle_small_Steiner_degrees(g);
		shortest_edge_between_terminals(g);
		prev_count = edge_count;
		edge_count = num_edges(g);
	}
}

void run_all_heuristics(Graph& g) {
	run_cheap_heuristics(g);
	g.compress_graph();

	delete_edges(g);
	run_cheap_heuristics(g);
	g.compress_graph();

	delete_edges_shortest_path(g);
	run_cheap_heuristics(g);
	g.compress_graph();

	terminal_distance_test(g);
	run_cheap_heuristics(g);
	g.compress_graph();

	delete_edges_shortest_path(g);
	run_cheap_heuristics(g);
	g.compress_graph();
}

void run_noninvalidating_heuristics(Graph& g) {
	static short run = 0;

	if (run==0&& g.terminal_count>20){
		delete_edges(g);
		run=1;
	}
	handle_small_Steiner_degrees(g);
	run = run<<1;
}

void run_possibly_invalidating_heuristics(Graph& g, std::vector<bool>& invalid_map) {
	TIMER_BEGIN {
		handle_small_Steiner_degrees(g);
		handle_small_terminal_degrees(g, invalid_map);
	} TIMER_END("%s: %lg s\n", __func__, timer);
}


size_t hash_sol(const std::vector<Edge>& sol) {
	size_t ret = 0;
	for (auto e : sol) {
		boost::hash_combine(ret, e.source());
		boost::hash_combine(ret, e.target());
	}
	return ret;
}

Graph end_heu(const Graph& g, const std::vector<Vertex>& possible_vertices) {
	debug_printf("\nCalling %s\n", __func__);
	Graph tmp = g.get_solution();
	int loops = 0;
	std::unordered_set<size_t> known_solutions;

	TIMER_BEGIN {
	typedef std::vector<Edge> Solution;
	std::deque< Solution > cur_queue, old_queue;

	std::vector<Edge> sure_edges;
	Solution best_sol;
	for (auto e : tmp.partial_solution)
		(e.is_removed() ? sure_edges : best_sol).push_back(e);
	tmp.partial_solution = best_sol;
	Solution orig_sol = best_sol;

	const Weight orig_weight = tmp.partial_solution_weight();
	Weight best_weight = orig_weight;
	size_t best_hash = 0;

	Weight cur_weight, weight;
	int vert_size;
	std::vector<Edge> sol;

	auto check_best = [&]() -> bool {
		boost::sort(sol);
		size_t hash = hash_sol(sol);
		if (known_solutions.count(hash) > 0 && weight >= best_weight) {
			// debug_printf("Found already known solution %zu\n", hash);
			return false;
		}

		if (best_weight > weight) {
			debug_printf("  BEST solution improved %d -> %d (vert_size %d, round %d)%s\n",
				cur_weight, weight, vert_size, loops,
				(hash_sol(tmp.partial_solution) != best_hash ? " *" : ""));
			best_sol = sol;
			best_weight = weight;
			best_hash = hash;
		}

		known_solutions.insert(hash);
		return true;
	};

	auto step = [&](int relax) {
		std::vector<Vertex> vert;
		for (int i = 0; i < vert_size; i++) {
			vert.push_back(possible_vertices[rand() % possible_vertices.size()]);
		}

		Weight w_old = -2;
		sol.clear();
		PAUSE_DEBUG weight = refine_solution(tmp, vert, std::back_inserter(sol));
		Solution old;
		std::swap(tmp.partial_solution, old);

		for (int i = 0; i < relax && weight != w_old; i++) {
			w_old = weight;
			std::swap(tmp.partial_solution, sol);
			sol.clear();
			PAUSE_DEBUG weight = refine_solution(tmp, {}, std::back_inserter(sol));
		}
		std::swap(tmp.partial_solution, old);
	};

	{
		Weight w = 0;
		for (auto e : sure_edges) w += e.weight();
		debug_printf("sure_edges weight %d\n", w);
	}

	static int vert_sizes[] = CONST_VERT_SIZES;

	Weight dz_last = -1;
	int tries = 0;
	while (true) {
		reset:
		Solution solut;
		while (cur_queue.size() + old_queue.size() > 150) {

			if (cur_queue.size() > 0){
				solut = cur_queue.back();
				cur_queue.pop_back();
			}else{
				 solut= old_queue.back();
				 old_queue.pop_back();
			}
				size_t hash_remove = hash_sol(solut);
				known_solutions.erase(hash_remove);
		}

		cur_weight = tmp.partial_solution_weight();
		tries += 30 + (orig_weight - best_weight > 0 ?
			((100 * (orig_weight - cur_weight)) / (orig_weight - best_weight)) : 0) +
			(cur_weight == orig_weight ? 30 : 0) +
			(cur_weight == best_weight ? 50 : 0);
		tries = std::max(tries, 30);

		while (tries-- > 0) {
			CHECK_SIGNALS(goto end);

			loops++;

			if (loops % 1000 == 1) {
				vert_size = 0;
				tries = 0;
				if (dz_last != best_weight || rand() % 100 < 30) {
					tmp.partial_solution = best_sol;
					cur_weight = best_weight;
					dz_last = best_weight;
				} else {
					vert_size = 7;
					unsigned i = rand() % (1 + cur_queue.size() + old_queue.size());
					if (i < cur_queue.size()) tmp.partial_solution = cur_queue[i];
					else if (i < cur_queue.size() + old_queue.size())
						tmp.partial_solution = old_queue[i - cur_queue.size()];

					cur_weight = tmp.partial_solution_weight();
				}

				step(3);
				known_solutions.erase(hash_sol(tmp.partial_solution));
				std::swap(tmp.partial_solution, sol);
				auto S = get_solution_structure(tmp);
				sol.clear();
				PAUSE_DEBUG weight = dreyfus_zid(tmp, S, std::back_inserter(sol));
				if (weight != -1) {
					std::swap(tmp.partial_solution, sol);
					vert_size = 0;
					step(3);
					debug_printf("DŽ: %d -> %d\n", cur_weight, weight);
					check_best();
					std::swap(tmp.partial_solution, sol);
					goto reset;
				}
			}

			vert_size = vert_sizes[rand() % (sizeof(vert_sizes)/sizeof(vert_sizes[0]))];

			step(1);

			if (cur_weight > weight || (cur_weight == weight && rand() % 100 < 20)) {
				if (!check_best()) continue;

				std::swap(tmp.partial_solution, sol);
				cur_queue.push_front(std::move(sol));
				sol = {};
				tries = CONST_TRIES_AFTER_RESET;
				goto reset;
			}
		}

		if (cur_queue.empty()) {
			debug_printf("\n start over %d loops %zu unique solutions \n",loops, known_solutions.size() );
			std::swap(cur_queue, old_queue);
			cur_queue.push_front({});
			std::swap(cur_queue.front(), tmp.partial_solution);
			if (rand() % 100 < 40) {
				debug_printf("Approx + random \n" );
				greedy_2approx(tmp, std::back_inserter(tmp.partial_solution));
				vert_size = 13;
			} else {
				tmp.partial_solution = orig_sol;
				vert_size = (rand() % 100 < 60 ? 0 : 13);
				debug_printf("Original + vert_size %d \n", vert_size );
			}
			cur_weight = tmp.partial_solution_weight();
			step(5);
			std::swap(tmp.partial_solution, sol);
			auto S = get_solution_structure(tmp);

			sol.clear();
			PAUSE_DEBUG weight = dreyfus_zid(tmp, S, std::back_inserter(sol));
			if (weight != -1) {
				std::swap(tmp.partial_solution, sol);
				vert_size = 0;
				step(5);
				debug_printf("DŽ: %d -> %d\n", cur_weight, weight);
				check_best();
				std::swap(tmp.partial_solution, sol);
			}
			std::swap(cur_queue.front(), tmp.partial_solution);
		}

		if (!cur_queue.empty()) {
			Solution t;
			std::swap(cur_queue.front(), t);
			cur_queue.pop_front();
			std::swap(t, tmp.partial_solution);
			old_queue.push_back(std::move(t));
		}
	}

	end:
	cur_queue.clear();
	old_queue.clear();

	tmp.partial_solution.clear();
	clean_up_solution(tmp, best_sol, std::back_inserter(tmp.partial_solution));
	for (auto e : sure_edges) tmp.partial_solution.push_back(e);

	} TIMER_END("%s: %d loops with %zu unique solutions in %lg s\n",
		__func__, loops, known_solutions.size(), timer);

	return tmp;
}

#endif // HEURISTICS_HPP
