#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <stdlib.h>
#include <list>
#include <limits>
#include <vector>
#include <deque>
#include <algorithm>
#include <cassert>
#include <memory>

#include "boost/iterator/counting_iterator.hpp"
#include "boost/graph/graph_traits.hpp"

#include "debug.hpp"

//forward declarations
struct Edge;
struct EdgeData;
struct WeightMap;
struct Graph;


typedef int Vertex;
typedef int Weight;

typedef std::tuple<Vertex, Vertex, Weight> OriginalEdge;
typedef std::vector< OriginalEdge > edge_history_t;
typedef std::vector<Edge> incidence_list_t;
typedef std::vector<Edge>::const_iterator incidence_iterator;

enum {
	EDGE_REF_OFFSET = 10,
	EDGE_EXT_REF = -5,
	EDGE_NO_EDGE = -3
};

struct EdgeData {
	bool removed;

	Vertex s;
	Vertex t;

	int weight;
	unsigned edge_list_pos;
	int orig_edge_1, orig_edge_2;
	int edge_index;

	int successor_index;
};

struct Edge {
	Vertex source() const;
	Vertex target() const;
	Vertex orig_source() const {
		assert(edge_data()->orig_edge_1 >= 0);
		return edge_data()->orig_edge_1;
	}
	Vertex orig_target() const {
		EdgeData *d = edge_data();
		Assert(d->orig_edge_2 >= 0 && d->orig_edge_1 >= 0, "%d %d (%d, %d, %d)",
			d->s, d->t, d->weight, d->orig_edge_1, d->orig_edge_2);
		return d->orig_edge_2;
	}

	int weight() const;
	int orig_edge() const {
		EdgeData *d = edge_data();
		Assert(d->orig_edge_1 == EDGE_EXT_REF, "%d %d\n", d->orig_edge_1, d->orig_edge_2);
		return d->orig_edge_2;
	}

	bool is_removed() const { return edge_data()->removed; }

	bool operator==(const Edge& rhs) const;
	bool operator!=(const Edge& rhs) const;

	// use for sorting before calling std::unique on Edge container, *not* for weight comparison
	bool operator<(const Edge& rhs) const;
	bool operator<=(const Edge& rhs) const;
	bool operator>(const Edge& rhs) const;
	bool operator>=(const Edge& rhs) const;

	Edge opposite_dir() const;

	Edge() : Edge(0) {}
	Edge(EdgeData *d, bool rev = false) {
		ptr = reinterpret_cast<decltype(ptr)>(d);
		assert((ptr & 1) == 0);
		ptr += rev;
	}

	friend struct Graph;
	friend void print_graph(FILE* out, const Graph& g);
	friend Edge get_null_edge();
	friend incidence_list_t _merge_inc_list(incidence_list_t* a, incidence_list_t* b, std::vector<Edge>& to_remove);
	friend bool test_edge(Graph& g, Edge e, int threshold);
private:
	uintptr_t ptr;

	EdgeData* edge_data() const {
		return reinterpret_cast<EdgeData*>(ptr & (~(uintptr_t)1));
	}
	bool reverse() const { return ptr & 1; }
};

class CompressedEdge {
	unsigned x;
	friend class Graph;
	public:
	CompressedEdge(unsigned x = std::numeric_limits<unsigned>::max()) : x(x) {}
	bool is_null() const {
		return x == std::numeric_limits<unsigned>::max();
	}
};

struct Graph {
	int vertex_count;
	int edge_count;
	int terminal_count;

	std::shared_ptr<const Graph> orig_graph;

	std::vector<int> degrees;
	std::vector< incidence_list_t > inc_edges;

	std::vector<Edge> edge_list;

	std::vector<Vertex> terminals;
	std::vector<char> terminal_mask;

	std::vector<Edge> partial_solution;

	std::deque<EdgeData> all_edge_data;

	Weight partial_solution_weight() const {
		Weight weight = 0;
		for (auto e : partial_solution) weight += e.weight();
		return weight;
	}
	Graph get_solution() const;

	bool is_terminal(Vertex v) const;

	void mark_terminal(Vertex v);
	void unmark_terminal(Vertex v);

	Edge add_edge(Vertex s, Vertex t, int weight, int orig_s, int orig_t);

	bool remove_edge(Edge e);
	Vertex contract_edge(Edge e);

	Vertex buy_edge(Edge e);

	Edge find_edge(Vertex s, Vertex t) const;

	void suppress_vertex(Vertex v);

	void compress_graph();

	CompressedEdge compress_edge(Edge e) const {
		return { (unsigned)e.edge_data()->edge_index * 2 + e.reverse() };
	}
	Edge decompress_edge(CompressedEdge ce) {
		assert(!ce.is_null());
		return Edge(&all_edge_data[ce.x / 2], ce.x & 1);
	}

	template < typename Vector, typename Lambda >
	Lambda for_each_orig_edge(const Vector& edges, Lambda f) const {
		std::vector<int> stack;

		for (auto e : edges) {
			EdgeData *d = e.edge_data();
			if (d->orig_edge_1 <= -EDGE_REF_OFFSET) {
				assert(d->orig_edge_2 <= -EDGE_REF_OFFSET);
				stack.push_back(-(d->orig_edge_1 + EDGE_REF_OFFSET));
				stack.push_back(-(d->orig_edge_2 + EDGE_REF_OFFSET));
			} else {
				assert((d->orig_edge_1 >= 0 && d->orig_edge_2 >= 0) ||
					d->orig_edge_1 == EDGE_EXT_REF);
				f(d->orig_edge_1, d->orig_edge_2);
			}
		}

		while (!stack.empty()) {
			const EdgeData *ed = &all_edge_data[stack.back()];
			stack.pop_back();

			if (ed->orig_edge_1 <= -EDGE_REF_OFFSET) {
				assert(ed->orig_edge_2 <= -EDGE_REF_OFFSET);
				stack.push_back(-(ed->orig_edge_1 + EDGE_REF_OFFSET));
				stack.push_back(-(ed->orig_edge_2 + EDGE_REF_OFFSET));
			} else {
				assert((ed->orig_edge_1 >= 0 && ed->orig_edge_2 >= 0) ||
					ed->orig_edge_1 == EDGE_EXT_REF);
				f(ed->orig_edge_1, ed->orig_edge_2);
			}
		}
		return f;
	}

	Graph(int vertex_count);
	~Graph();
	Graph(const Graph&) { assert("Copy was not elided!" && false); }
	Graph& operator =(const Graph&) { assert("Copy was not elided!" && false); }

	struct copy_tag {};
	Graph(const Graph& g, copy_tag);
	void save_orig_graph();
private:
	void _change_edge_target(Edge e, Vertex new_target);
};

// Edge {{{


Edge get_null_edge() {
	Edge e(new EdgeData());
	EdgeData* ed = e.edge_data();
	ed->s = -1;
	ed->t = -1;
	ed->weight = std::numeric_limits< decltype(ed->weight) >::max();
	return e;
}

const Edge null_edge = get_null_edge();

Vertex Edge::source() const { return reverse() ? edge_data()->t : edge_data()->s; }
Vertex Edge::target() const { return reverse() ? edge_data()->s : edge_data()->t; }
int Edge::weight() const { return edge_data()->weight; }

bool Edge::operator==(const Edge& rhs) const {
	return this->edge_data() == rhs.edge_data();
}
bool Edge::operator<(const Edge& rhs) const {
	return this->edge_data()->edge_index < rhs.edge_data()->edge_index;
}
bool Edge::operator!=(const Edge& rhs) const { return !(*this == rhs); }
bool Edge::operator<=(const Edge& rhs) const { return !(rhs < *this); }
bool Edge::operator>(const Edge& rhs) const { return rhs < *this; }
bool Edge::operator>=(const Edge& rhs) const { return !(*this < rhs); }

Edge Edge::opposite_dir() const { return Edge(edge_data(), !reverse()); }

// }}}



Graph::Graph(int vertex_count) {
	this->vertex_count = vertex_count;
	edge_count = 0;
	terminal_count = 0;

	degrees.resize(vertex_count);
	std::fill(degrees.begin(), degrees.end(), 0);

	terminal_mask.resize(vertex_count);
	std::fill(terminal_mask.begin(), terminal_mask.end(), false);

	inc_edges.resize(vertex_count);
}

void _ins_sorted(Edge e, incidence_list_t* l);

Graph::Graph(const Graph &g, Graph::copy_tag) : vertex_count(g.vertex_count),
	edge_count(g.edge_count), terminal_count(g.terminal_count),
	orig_graph(g.orig_graph), degrees(g.degrees),
	terminals(g.terminals), terminal_mask(g.terminal_mask),
	all_edge_data(g.all_edge_data) {

	// fix inc_edges, edge_list, and partial_solution
	inc_edges.resize(vertex_count);

	for (auto e : g.partial_solution)
		partial_solution.push_back(Edge(&all_edge_data[e.edge_data()->edge_index]));

	for (auto& ed : all_edge_data) if (!ed.removed) {
		edge_list.push_back(Edge(&ed));
		_ins_sorted(Edge(&ed), &inc_edges[ed.s]);
		_ins_sorted(Edge(&ed, true), &inc_edges[ed.t]);
	}
}

Graph::~Graph() {}

void Graph::save_orig_graph() {
	debug_printf(">>> %s\n", __func__);

	orig_graph = std::shared_ptr<const Graph>(new Graph(*this, copy_tag()));

	for (auto& ed : all_edge_data) {
		ed.orig_edge_1 = EDGE_EXT_REF;
		ed.orig_edge_2 = ed.edge_index;
	}
}

Graph Graph::get_solution() const {
	debug_printf(">>> %s\n", __func__);
	assert(orig_graph);

	Graph sol{*orig_graph, copy_tag()};
	sol.partial_solution.clear();

	for_each_orig_edge(partial_solution, [&](Vertex u, Vertex v) {
		assert(u == EDGE_EXT_REF);
		sol.partial_solution.push_back(Edge{&sol.all_edge_data[v]});
	});

	return sol;
}

bool Graph::is_terminal(Vertex v) const {
	return terminal_mask[v];
}

void Graph::mark_terminal(Vertex v) {
	if(is_terminal(v)) {
		return;
	}
	terminals.push_back(v);
	terminal_mask[v] = true;
	terminal_count++;
}

void Graph::unmark_terminal(Vertex v) {
	if(!is_terminal(v)) {
		return;
	}

	if (terminals.back() == v) terminals.pop_back();
	else terminals.erase(std::find(terminals.begin(), terminals.end(), v));

	terminal_mask[v] = false;
	terminal_count--;
}


Edge Graph::find_edge(Vertex s, Vertex t) const {
	for (auto e : inc_edges[s]) {
		if (e.target() == t) {
			return e;
		}
	}
	return null_edge;
}

bool _is_inc_list_sorted(incidence_list_t* l) {
	Vertex prev_t = -1;

	for(auto e : *l) {
		if( e.target() <= prev_t ) {
			return false;
		}
		prev_t = e.target();
	}
	return true;
}

void _ins_sorted(Edge e, incidence_list_t* l) {
	auto it = l->begin();

	Vertex t = e.target();

	for( ; it != l->end(); ++it ) {
		if( it->target() > t) {
			break;
		}
	}
	l->insert(it, e);

	assert(_is_inc_list_sorted);
}



Edge Graph::add_edge(Vertex s, Vertex t, int weight, Vertex orig_s, Vertex orig_t) {
	assert(s != t);

	// if edge (s,t) already is in graph
	Edge old_e = find_edge(s,t);
	if( old_e != null_edge ) {
		// if it has lower weight, return original edge
		if( old_e.weight() < weight ) {
			return old_e;
		}
		// otherwise delete it and insert new one
		else {
			remove_edge(old_e);
		}
	}

	// insert new edge
	all_edge_data.push_back(EdgeData());
	EdgeData* d = &all_edge_data.back();
	d->edge_index = all_edge_data.size() - 1;
	Edge e_fw(d), e_rw(d, true);

	d->s = s;
	d->t = t;
	d->weight = weight;

	d->edge_list_pos = edge_list.size();
	d->successor_index = -1;

	edge_list.push_back(e_fw);
	edge_count++;

	_ins_sorted(e_fw, &inc_edges[s]);
	_ins_sorted(e_rw, &inc_edges[t]);

	degrees[s]++;
	degrees[t]++;

	d->orig_edge_1 = orig_s;
	d->orig_edge_2 = orig_t;

	return e_fw;
}

void _find_and_remove(Graph& g, Vertex s, Vertex t) {
	incidence_list_t* l = &g.inc_edges[s];
	for(auto it = l->begin(); it != l->end(); ++it) {
		if( it->target() == t ) {
			l->erase(it);
			break;
		}
	}
}


bool Graph::remove_edge(Edge e) {
	EdgeData *ed = e.edge_data();
	if (ed->removed) {
		return false;
	}

	Vertex s = e.source();
	Vertex t = e.target();

	_find_and_remove(*this,s,t);
	_find_and_remove(*this,t,s);

	if (ed->edge_list_pos + 1 != edge_list.size()) {
		std::swap(edge_list[ed->edge_list_pos], edge_list[edge_list.size() - 1]);
		edge_list[ed->edge_list_pos].edge_data()->edge_list_pos = ed->edge_list_pos;
	}
	edge_list.pop_back();

	ed->removed = true;

	degrees[s]--;
	degrees[t]--;
	edge_count--;

	return true;
}



incidence_list_t _merge_inc_list(incidence_list_t* a, incidence_list_t* b, std::vector<Edge>& to_remove) {
	auto it_a = a->begin();
	auto it_b = b->begin();

	assert(_is_inc_list_sorted(a));
	assert(_is_inc_list_sorted(b));

	incidence_list_t result;

	while(it_a != a->end() && it_b != b->end()) {
		Edge e = *it_a;
		Edge f = *it_b;

		if(e.target() == f.target()) {
			if(e.weight() < f.weight()) {
				result.push_back(e);
				to_remove.push_back(f);
				f.edge_data()->successor_index = e.edge_data()->edge_index;
			}
			else {
				result.push_back(f);
				to_remove.push_back(e);
				e.edge_data()->successor_index = f.edge_data()->edge_index;
			}
			++it_a;
			++it_b;
		}
		else if(e.target() < f.target()) {
			result.push_back(e);
			++it_a;
		}
		else {
			result.push_back(f);
			++it_b;
		}
	}

	while(it_a != a->end()) {
		result.push_back(*it_a);
		++it_a;
	}
	while(it_b != b->end()) {
		result.push_back(*it_b);
		++it_b;
	}

	assert(_is_inc_list_sorted(&result));
	return result;
}

void Graph::_change_edge_target(Edge e, Vertex new_target) {
	Vertex s = e.source();
	Vertex t = e.target();

	assert( _is_inc_list_sorted(&inc_edges[s]) );

	incidence_list_t* l = &inc_edges[s];

	// debug_printf("Edge (%d,%d), new target = %d\n", s,t,new_target);
	// print_targets(l);

	if( t < new_target ) { // rotate left
		auto rot_start = l->begin();
		auto rot_end = l->end();

		for(auto it = l->begin(); it != l->end(); ++it) {
			if(it->target() == t) { // found e's position
				rot_start = it;
			}
			if(it->target() > new_target) {
				rot_end = it;
				break;
			}
		}
		if(rot_start + 1 < rot_end) { // rotation is non-trivial
			std::rotate(rot_start, rot_start+1, rot_end);
		}

	} else { //rotate right
		auto rot_start = l->rbegin();
		auto rot_end = l->rend();
		for(auto it = l->rbegin(); it != l->rend(); ++it) {
			if(it->target() == t) { // found e's position
				rot_start = it;
			}
			if(it->target() < new_target) {
				rot_end = it;
				break;
			}
		}
		if(rot_start + 1 < rot_end) { // rotation is non-trivial
			std::rotate(rot_start, rot_start+1, rot_end);
		}
	}


	// print_targets(l);
	if (e.reverse()) {
		e.edge_data()->s = new_target;
	}
	else {
		e.edge_data()->t = new_target;
	}
	// print_targets(l);
	assert( _is_inc_list_sorted(&inc_edges[s]) );
}


Vertex Graph::contract_edge(Edge e) {
	EdgeData *ed = e.edge_data();
	if (ed->removed) {
		return -1;
	}

	Vertex s = ed->s;
	Vertex t = ed->t;


	if(is_terminal(t)) {
		std::swap(s,t);
	}

	remove_edge(e);

	// merge s and t
	std::vector<Edge> to_delete;
	incidence_list_t merged_st = _merge_inc_list(&inc_edges[s], &inc_edges[t], to_delete);

	// remove arising parallel edges
	for(auto f : to_delete) {
		remove_edge(f);
	}

	// renumber edges from t
	for(auto f : inc_edges[t]) {
		_change_edge_target(f.opposite_dir(),s);
	}

	std::swap(inc_edges[s], merged_st);
	inc_edges[t].clear();

	degrees[t] = 0;
	degrees[s] = inc_edges[s].size();

	assert( _is_inc_list_sorted(&inc_edges[s]) );
	assert( _is_inc_list_sorted(&inc_edges[t]) );

	if(is_terminal(s) && is_terminal(t)) {
		unmark_terminal(t);
	}

	return s;
}

Vertex Graph::buy_edge(Edge e) {
	EdgeData* ed = e.edge_data();
	while( ed->successor_index != -1 ) {
		ed = &all_edge_data[ed->successor_index];
	}

	if(ed->removed) {
		return -2;
	}
	else {
		partial_solution.push_back(Edge(ed));
		return contract_edge(Edge(ed));
	}
}

void Graph::suppress_vertex(Vertex v) {
	assert( !is_terminal(v) );
	assert( degrees[v] == 2 );


	Edge e = inc_edges[v][0];
	Edge f = inc_edges[v][1];

	Vertex s = e.target();
	Vertex t = f.target();

	add_edge(s,t,e.weight() + f.weight(), -(e.edge_data()->edge_index + EDGE_REF_OFFSET),
		-(f.edge_data()->edge_index + EDGE_REF_OFFSET));

	remove_edge(e);
	remove_edge(f);
}

void Graph::compress_graph() {
	std::vector<Vertex> forward_map(vertex_count, -1);
	int j = 0;
	for(int i = 0; i < vertex_count; i++) {
		if(degrees[i] > 0) {
			forward_map[i] = j;

			degrees[j] = degrees[i];
			std::swap(inc_edges[j], inc_edges[i]);
			terminal_mask[j] = terminal_mask[i];
			j++;
		}
	}

	int compressed_size = j;

	degrees.resize(compressed_size);
	inc_edges.resize(compressed_size);
	terminal_mask.resize(compressed_size);

	for(auto& t : terminals) {
		t = forward_map[t];
	}

	for(auto e : edge_list) {
		e.edge_data()->s = forward_map[e.edge_data()->s];
		e.edge_data()->t = forward_map[e.edge_data()->t];
	}

	debug_printf("Compressed graph %d -> %d\n", vertex_count, compressed_size);
	vertex_count = compressed_size;
}



namespace boost {
	template < typename WeightMap >
	Weight get(WeightMap, Edge e)  {
		return e.weight();
	}
};

#include "boost/graph/properties.hpp"
#include "boost/property_map/property_map.hpp"

struct WeightMap {
	typedef int value_type;
	typedef value_type reference;
	typedef Edge key_type;
	typedef boost::readable_property_map_tag category;

	reference operator[](key_type e) const {
		return e.weight();
	}
};

struct Graph_traversal_category:
	virtual public boost::incidence_graph_tag,
	virtual public boost::vertex_list_graph_tag,
	virtual public boost::edge_list_graph_tag
{};

namespace boost {
	template <>
	struct graph_traits< Graph > {
		typedef Vertex vertex_descriptor;
		typedef Edge edge_descriptor;
		typedef undirected_tag directed_category;
		typedef disallow_parallel_edge_tag edge_parallel_category;
		typedef Graph_traversal_category traversal_category;

		typedef int degree_size_type;
		typedef incidence_iterator out_edge_iterator;

		typedef int edges_size_type;
		typedef decltype(Graph::edge_list)::const_iterator edge_iterator;

		typedef boost::counting_iterator<int> vertex_iterator;
		typedef int vertices_size_type;

		static vertex_descriptor null_vertex() { return -1; }
	};

	template <> struct graph_traits< const Graph > : graph_traits< Graph > {};

	template <>
	struct property_map< Graph, edge_weight_t > {
		typedef WeightMap type;
		typedef const WeightMap const_type;

	};

	template <>
	struct property_map< Graph, vertex_index_t > {
		typedef identity_property_map type;
		typedef const identity_property_map const_type;
	};


	template <>
	struct property_traits< WeightMap > {
		typedef int value_type;
		typedef int reference;
		typedef Edge key_type;
		typedef readable_property_map_tag category;
	};



	typename graph_traits< Graph >::vertex_descriptor
	source(
		typename graph_traits< Graph >::edge_descriptor e,
		const Graph&) {
		return e.source();
	}

	typename graph_traits< Graph >::vertex_descriptor
	target(
		typename graph_traits< Graph >::edge_descriptor e,
		const Graph&) {
		return e.target();
	}

	typename graph_traits< Graph >::degree_size_type
	out_degree(
		typename graph_traits< Graph >::vertex_descriptor u,
		const Graph& g) {
		return g.degrees[u];
	}

	typename
	std::pair<
		graph_traits< Graph >::out_edge_iterator,
		graph_traits< Graph >::out_edge_iterator >
	out_edges(
		typename graph_traits< Graph >::vertex_descriptor u,
		const Graph& g) {
		return std::pair<
			graph_traits< Graph >::out_edge_iterator,
			graph_traits< Graph >::out_edge_iterator >
			( g.inc_edges[u].begin(), g.inc_edges[u].end() );
	}


	typename
	std::pair<
		graph_traits< Graph >::vertex_iterator,
		graph_traits< Graph >::vertex_iterator >
	vertices(const Graph& g)
	{
		return std::pair<
			graph_traits< Graph >::vertex_iterator,
			graph_traits< Graph >::vertex_iterator
			> ( boost::counting_iterator<int>(0), boost::counting_iterator<int>(g.vertex_count));

	}


	typename graph_traits< Graph >::vertices_size_type
	num_vertices(const Graph& g) {
		return g.vertex_count;
	}

	typename graph_traits< Graph >::edges_size_type
	num_edges(const Graph& g) {
		return g.edge_count;
	}

	typename
	std::pair<
		graph_traits< Graph >::edge_iterator,
		graph_traits< Graph >::edge_iterator >
	edges(const Graph& g) {
		return std::pair<
			graph_traits< Graph >::edge_iterator,
			graph_traits< Graph >::edge_iterator
			> ( g.edge_list.begin(), g.edge_list.end() );
	}

	// properties


	typename property_map< Graph, edge_weight_t >::type
	get(
		edge_weight_t,
		const Graph&
		)
	{
		return WeightMap();
	}

	typename property_traits< WeightMap >::value_type
	get(
		edge_weight_t,
		const Graph&,
		Edge e
		)
	{
		return e.weight();
	}


	identity_property_map get(vertex_index_t, const Graph&) {
		return identity_property_map();
	}

} // namespace boost

#include "dfs.hpp"
#include "heuristics.hpp"

void print_solution(FILE* out, Graph &g) {
	debug_printf("\nCalling %s\n", __func__);
	TIMER_BEGIN {
		const Weight orig_weight = g.partial_solution_weight();
		Graph tmp = g.get_solution();
		assert(!tmp.orig_graph);

		std::vector<Edge> sol;
		clean_up_solution(tmp, tmp.partial_solution, std::back_inserter(sol));
		std::swap(sol, tmp.partial_solution);
		sol.clear();

		Weight weight = refine_solution(tmp, std::vector<Vertex>(), std::back_inserter(sol));
		if (orig_weight != weight)
			debug_printf("Oops orig weight was %d but we got %d\n", orig_weight, weight);

		fprintf(out, "VALUE %d\n", weight);
		for (auto e : sol) {
			fprintf(out, "%d %d\n", e.orig_source(), e.orig_target());
		}
	} TIMER_END("%s: %lg s\n", __func__, timer);
}

/*
void print_graph(FILE* out, const Graph& g) {
	fprintf(out, "|V| = %d, |E| = %d, |R| = %d\n", g.vertex_count, g.edge_count, g.terminal_count);

	for(int i = 0; i < g.vertex_count; i++) {
		fprintf(out, g.terminal_mask[i] ? "* " : "  ");
		fprintf(out, "Inc(%d) = ", i);
		for(auto e : g.inc_edges[i]) {
			fprintf(out, "(%d,%d) : %d", boost::source(e,g), boost::target(e,g), e.weight());
			fprintf(out, "<");
			for(auto p : e.d->orig_edges) {
				fprintf(out, "[%d,%d] ", get<0>(p), get<1>(p));
			}
			fprintf(out, ">    ");
		}
		fprintf(out, "\n");
	}

}
*/


#endif // GRAPH_HPP
