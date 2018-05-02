#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <stdlib.h>
#include <list>
#include <limits>
#include <vector>
#include <algorithm>
#include <cassert>

#include "boost/iterator/counting_iterator.hpp"
#include "boost/graph/graph_traits.hpp"
#include "boost/graph/properties.hpp"
#include "boost/property_map/property_map.hpp"

#include "debug.hpp"

//forward declarations
struct Edge;
struct EdgeData;
struct WeightMap;
struct Graph;


typedef int Vertex;

typedef std::vector< std::pair<int, int> > edge_history_t;
typedef std::vector<Edge> incidence_list_t;
typedef std::vector<Edge>::const_iterator incidence_iterator;
typedef std::list<Edge> edge_list_t;

struct Edge {
	bool reverse;

	Vertex source() const;
	Vertex target() const;
	int weight() const;

	bool operator==(const Edge& rhs) const;
	bool operator!=(const Edge& rhs) const;

	// use for sorting before calling std::unique on Edge container, *not* for weight comparison
	bool operator<(const Edge& rhs) const;
	bool operator<=(const Edge& rhs) const;
	bool operator>(const Edge& rhs) const;
	bool operator>=(const Edge& rhs) const;

	Edge opposite_dir() const;

	void print(FILE* out) const;

	Edge();

	friend struct Graph;
	friend void print_graph(FILE* out, const Graph& g);
	friend Edge get_null_edge();
private:
	EdgeData* d;
};

struct EdgeData {
	bool removed;

	Vertex s;
	Vertex t;

	int weight;

	edge_list_t::iterator edge_list_pos;

	edge_history_t orig_edges;
};


struct Graph {
	int vertex_count;
	int edge_count;
	int terminal_count;

	std::vector<int> degrees;
	std::vector< incidence_list_t* > inc_edges;

	edge_list_t edge_list;

	std::vector<Vertex> terminals;
	std::vector<char> terminal_mask;

	std::vector<Edge> partial_solution;
	int partial_solution_weight;

	bool is_terminal(Vertex v) const;

	void mark_terminal(Vertex v);
	void unmark_terminal(Vertex v);

	Edge add_edge(Vertex s, Vertex t, int weight);
	Edge add_edge(Vertex s, Vertex t, int weight, int orig_s, int orig_t);
	Edge add_edge(Vertex s, Vertex t, int weight, edge_history_t hist);

	void remove_edge(Edge e);
	void contract_edge(Edge e);

	bool buy_edge(Edge e);

	Edge find_edge(Vertex s, Vertex t) const;

	void suppress_vertex(Vertex v);

	Graph(int vertex_count);
	Graph(const Graph& g);
private:
	void _change_edge_target(Edge e, Vertex new_target);
};

// Edge {{{


Edge get_null_edge() {
	Edge e;
	EdgeData* ed = new EdgeData();
	ed->s = -1;
	ed->t = -1;
	ed->weight = std::numeric_limits< decltype(ed->weight) >::max();
	e.d = ed;
	return e;
}

const Edge null_edge = get_null_edge();

Edge::Edge() {
	reverse = false;
}

Vertex Edge::source() const {
	return reverse ? d->t : d->s;
}

Vertex Edge::target() const {
	return reverse ? d->s : d->t;
}

int Edge::weight() const {
	return d->weight;
}

bool Edge::operator==(const Edge& rhs) const {
	return this->d == rhs.d;
}

bool Edge::operator!=(const Edge& rhs) const {
	return !(*this == rhs);
}

bool Edge::operator<(const Edge& rhs) const {
	return this->d < rhs.d; // yes, i meant pointer comparison
}

bool Edge::operator<=(const Edge& rhs) const {
	return this->d <= rhs.d; // yes, i meant pointer comparison
}

bool Edge::operator>(const Edge& rhs) const {
	return this->d > rhs.d; // yes, i meant pointer comparison
}

bool Edge::operator>=(const Edge& rhs) const {
	return this->d >= rhs.d; // yes, i meant pointer comparison
}


Edge Edge::opposite_dir() const {
	Edge e;
	e.d = this->d;
	e.reverse = !this->reverse;
	return e;
}

void Edge::print(FILE* out) const {
	for(auto p : d->orig_edges) {
		fprintf(out, "%d %d\n", p.first, p.second);
	}
}

// }}}



Graph::Graph(int vertex_count) {
	this->vertex_count = vertex_count;
	edge_count = 0;
	terminal_count = 0;

	partial_solution_weight = 0;

	degrees.resize(vertex_count);
	std::fill(degrees.begin(), degrees.end(), 0);

	terminal_mask.resize(vertex_count);
	std::fill(terminal_mask.begin(), terminal_mask.end(), false);

	inc_edges.resize(vertex_count);
	for(int i = 0; i < vertex_count; i++) {
		inc_edges[i] = new incidence_list_t();
	}
}

Graph::Graph(const Graph& g) : Graph(g.vertex_count) {
	this->terminal_count = g.terminal_count;
	this->terminals = g.terminals;
	this->terminal_mask = g.terminal_mask;

	this->partial_solution_weight = g.partial_solution_weight;
	this->partial_solution = g.partial_solution;

	this->edge_count = 0;

	for(auto e : g.edge_list) {
		this->add_edge(e.source(), e.target(), e.weight(), e.d->orig_edges);
	}
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
	terminals.erase(std::find(terminals.begin(), terminals.end(), v));
	terminal_mask[v] = false;
	terminal_count--;
}


Edge Graph::find_edge(Vertex s, Vertex t) const {
	incidence_list_t* l = inc_edges[s];
	for(auto it = l->begin(); it != l->end(); ++it) {
		if( it->target() == t) {
			return *it;
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



Edge Graph::add_edge(Vertex s, Vertex t, int weight) {
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
	Edge e_fw, e_rw;
	EdgeData* d = new EdgeData();
	e_fw.d = e_rw.d = d;

	e_fw.reverse = false;
	e_rw.reverse = true;

	d->s = s;
	d->t = t;
	d->weight = weight;

	d->edge_list_pos = edge_list.insert(edge_list.begin(), e_fw);
	edge_count++;

	_ins_sorted(e_fw, inc_edges[s]);
	_ins_sorted(e_rw, inc_edges[t]);

	degrees[s]++;
	degrees[t]++;

	return e_fw;
}

Edge Graph::add_edge(Vertex s, Vertex t, int weight, edge_history_t hist) {
	Edge e = add_edge(s,t,weight);
	if( e.d->orig_edges.empty() ) { // edge is new
		e.d->orig_edges = hist;
	}
	return e;
}

Edge Graph::add_edge(Vertex s, Vertex t, int weight, Vertex orig_s, Vertex orig_t) {
	Edge e = add_edge(s,t,weight);
	if( e.d->orig_edges.empty() ) { // edge is new
		e.d->orig_edges.push_back(std::make_pair(orig_s, orig_t));
	}
	return e;
}

void _find_and_remove(Graph& g, Vertex s, Vertex t) {
	incidence_list_t* l = g.inc_edges[s];
	for(auto it = l->begin(); it != l->end(); ++it) {
		if( it->target() == t ) {
			l->erase(it);
			break;
		}
	}
}


void Graph::remove_edge(Edge e) {
	if( e.d->removed ) {
		return;
	}

	Vertex s = e.source();
	Vertex t = e.target();

	_find_and_remove(*this,s,t);
	_find_and_remove(*this,t,s);

	edge_list.erase( e.d->edge_list_pos );

	e.d->removed = true;

	// e.d->orig_edges.clear();

	degrees[s]--;
	degrees[t]--;
	edge_count--;
}



incidence_list_t* _merge_inc_list(incidence_list_t* a, incidence_list_t* b, std::vector<Edge>& to_remove) {
	auto it_a = a->begin();
	auto it_b = b->begin();

	assert(_is_inc_list_sorted(a));
	assert(_is_inc_list_sorted(b));

	incidence_list_t* result = new incidence_list_t();

	while(it_a != a->end() && it_b != b->end()) {
		Edge e = *it_a;
		Edge f = *it_b;

		if(e.target() == f.target()) {
			if(e.weight() < f.weight()) {
				result->push_back(e);
				to_remove.push_back(f);
			}
			else {
				result->push_back(f);
				to_remove.push_back(e);
			}
			++it_a;
			++it_b;
		}
		else if(e.target() < f.target()) {
			result->push_back(e);
			++it_a;
		}
		else {
			result->push_back(f);
			++it_b;
		}
	}

	while(it_a != a->end()) {
		result->push_back(*it_a);
		++it_a;
	}
	while(it_b != b->end()) {
		result->push_back(*it_b);
		++it_b;
	}

	assert(_is_inc_list_sorted(result));
	return result;
}

void print_targets(incidence_list_t* l) {
	for(auto t : *l) {
		debug_printf("%d ", t.target());
	}
	debug_printf("\n");
}

void Graph::_change_edge_target(Edge e, Vertex new_target) {
	Vertex s = e.source();
	Vertex t = e.target();

	assert( _is_inc_list_sorted(inc_edges[s]) );

	incidence_list_t* l = inc_edges[s];

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
	if(e.reverse) {
		e.d->s = new_target;
	}
	else {
		e.d->t = new_target;
	}
	// print_targets(l);
	assert( _is_inc_list_sorted(inc_edges[s]) );
}


void Graph::contract_edge(Edge e) {
	if( e.d->removed ) {
		return;
	}

	Vertex s = e.source();
	Vertex t = e.target();


	if(is_terminal(t)) {
		std::swap(s,t);
	}

	remove_edge(e);

	// merge s and t
	std::vector<Edge> to_delete;
	incidence_list_t* merged_st;
	merged_st = _merge_inc_list(inc_edges[s], inc_edges[t], to_delete);

	// remove arising parallel edges
	for(auto f : to_delete) {
		remove_edge(f);
	}

	// renumber edges from t
	for(auto f : *inc_edges[t]) {
		_change_edge_target(f.opposite_dir(),s);
	}

	delete inc_edges[s];
	delete inc_edges[t];

	inc_edges[s] = merged_st;
	inc_edges[t] = new incidence_list_t();

	degrees[t] = 0;
	degrees[s] = inc_edges[s]->size();

	assert( _is_inc_list_sorted(inc_edges[s]) );
	assert( _is_inc_list_sorted(inc_edges[t]) );

	if(is_terminal(s) && is_terminal(t)) {
		unmark_terminal(t);
	}
}

bool Graph::buy_edge(Edge e) {
	if(e.d->removed) {
		return false;
	}
	else {
		partial_solution.push_back(e);
		partial_solution_weight += e.weight();
		contract_edge(e);
		return true;
	}
}

void Graph::suppress_vertex(Vertex v) {
	assert( !is_terminal(v) );
	assert( degrees[v] == 2 );


	Edge e = (*inc_edges[v])[0];
	Edge f = (*inc_edges[v])[1];

	Vertex s = e.target();
	Vertex t = f.target();

	Edge new_e = add_edge(s,t,e.weight() + f.weight());

	// we actually added new edge
	if( new_e.d->orig_edges.empty() ) {
		EdgeData* d = new_e.d;

		d->orig_edges.insert(d->orig_edges.end(), e.d->orig_edges.begin(), e.d->orig_edges.end());
		d->orig_edges.insert(d->orig_edges.end(), f.d->orig_edges.begin(), f.d->orig_edges.end());

		e.d->orig_edges.clear();
		f.d->orig_edges.clear();
	}

	remove_edge(e);
	remove_edge(f);
}


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
		typedef std::list<Edge>::const_iterator edge_iterator;

		typedef boost::counting_iterator<int> vertex_iterator;
		typedef unsigned int vertices_size_type;

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



	typename property_traits< WeightMap >::value_type
	get(
		WeightMap ,
		typename property_traits< WeightMap>::key_type e
	)
	{
		return e.weight();
	}


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
			( g.inc_edges[u]->begin(), g.inc_edges[u]->end() );
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


void print_solution(FILE* out, Graph &g) {

	fprintf(out, "VALUE %d\n", g.partial_solution_weight);
	for(auto e : g.partial_solution) {
		e.print(out);
	}
}



void print_graph(FILE* out, const Graph& g) {
	fprintf(out, "|V| = %d, |E| = %d, |R| = %d\n", g.vertex_count, g.edge_count, g.terminal_count);

	for(int i = 0; i < g.vertex_count; i++) {
		fprintf(out, g.terminal_mask[i] ? "* " : "  ");
		fprintf(out, "Inc(%d) = ", i);
		for(auto e : *(g.inc_edges[i])) {
			fprintf(out, "(%d,%d) : %d", boost::source(e,g), boost::target(e,g), e.weight());
			fprintf(out, "<");
			for(auto p : e.d->orig_edges) {
				fprintf(out, "[%d,%d] ", p.first, p.second);
			}
			fprintf(out, ">    ");
		}
		fprintf(out, "\n");
	}

}



#endif // GRAPH_HPP
