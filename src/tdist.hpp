#ifndef TDIST_HPP
#define TDIST_HPP

#include <vector>

#include "graph.hpp"


struct EarlyTerminate : public std::exception {};

using namespace boost;

struct UnionFind {
	int size;

	int label(int u);
	int root(int u);
	bool find(int u, int v);
	int link(int u, int v);

	int get_size(int u);
	bool is_root(int u);

	std::vector<int> parent;
	std::vector<int> rank;
	std::vector<int> class_size;

	UnionFind(int size);
};


UnionFind::UnionFind(int size) :
	rank(size,0),
	class_size(size,1)
{
	this->size = size;
	parent.resize(size);
	for(int i = 0; i < size; i++) {
		parent[i] = i + size;
	}
}

bool UnionFind::is_root(int u) {
	return parent[u] >= size;
}

int UnionFind::root(int u) {
	std::vector<int> to_compress;
	while( !is_root(u) ) {
		to_compress.push_back(u);
		u = parent[u];
	}
	for(auto v: to_compress) {
		parent[v] = u;
	}
	return u;
}

int UnionFind::get_size(int u) {
	return class_size[root(u)];
}

int UnionFind::label(int u) {
	return parent[root(u)] - size;
}


bool UnionFind::find(int u, int v) {
	return root(u) == root(v);
}

int UnionFind::link(int u, int v) {
	int ur = root(u);
	int vr = root(v);
	if(ur == vr) {
		return ur;
	}

	if( rank[ur] > rank[vr] ) {
		std::swap(ur,vr);
	}

	parent[ur] = vr;

	class_size[vr] += class_size[ur];
	if( rank[ur] == rank[vr] ) {
		rank[vr]++;
	}

	return vr;
}


struct IncrementalBridgeConnComponents {
	int size;

	UnionFind components;
	UnionFind bridge_conn_components; // edge 2-connected components; we follow terminology of Westbrook-Tarjan

	std::vector<int> parent_bc;
	std::vector<Edge> parent_edge;

	Edge get_parent_edge(int u);
	int get_parent_bc(int u);

	int component_label(int u);

	bool same_component(int u, int v);
	bool same_bridge_conn_component(int u, int v);

	std::vector<Edge> link(Edge e);

	void evert(int cu);

	IncrementalBridgeConnComponents(int size);
private:
	int find_lca(int bcu, int bcv);
	std::vector<bool> visited;
};
IncrementalBridgeConnComponents::IncrementalBridgeConnComponents(int size) :
	components(size),
	bridge_conn_components(size),
	parent_bc(size,-1),
	parent_edge(size, null_edge),
	visited(size,false)
{
	this->size = size;
}

void print_parent_tree(IncrementalBridgeConnComponents& inc) {
	for(int i = 0; i < inc.size; i++) {
		int u = i;
		debug_printf("bc %d\n", u);
		debug_printf("Root: ");
		if(u == inc.bridge_conn_components.root(u)) {
			debug_printf("yes\n");
		}
		else {
			debug_printf("no\n");
		}

		/*
		while(u != -1) {
			debug_printf("%d ", u);
			u = inc.parent_bc[u];
		}
		debug_printf("\n");
		*/
		while(u != -1) {
			debug_printf("%d ", u);
			u = inc.get_parent_bc(u);
		}
		debug_printf("\n");
		debug_printf("\n");
	}
}

int IncrementalBridgeConnComponents::component_label(int u) {
	int label = components.label(bridge_conn_components.label(u));
	debug_printf("vertex %d has component label %d\n", u, label);

	return label;
}

Edge IncrementalBridgeConnComponents::get_parent_edge(int bcu) {
	return parent_edge[bridge_conn_components.root(bcu)];
}

int IncrementalBridgeConnComponents::get_parent_bc(int bcu) {
	int pbc = parent_bc[bridge_conn_components.root(bcu)];
	if(pbc != -1) {
		pbc = bridge_conn_components.root(pbc);
	}
	return pbc;
}

bool IncrementalBridgeConnComponents::same_component(int u, int v) {
	return component_label(u) == component_label(v);
}

bool IncrementalBridgeConnComponents::same_bridge_conn_component(int u, int v) {
	return bridge_conn_components.root(u) == bridge_conn_components.root(v);
}

std::vector<Edge> IncrementalBridgeConnComponents::link(Edge e) {
	int u = e.source();
	int v = e.target();
	std::vector<Edge> removed_bridges;
	if( same_bridge_conn_component(u,v) ) { // nothing to do
		debug_printf("Edge (%d,%d) is in bridge component\n", u,v);
		return removed_bridges;
	}
	else {
		if (! same_component(u,v) ) { // join components and add new bridge
			debug_printf("Edge (%d,%d) is joining two components\n", u,v);

			int bcu = bridge_conn_components.root(u);
			int bcv = bridge_conn_components.root(v);

			int cu = component_label(u);
			int cv = component_label(v);

			if(components.get_size(cu) > components.get_size(cv)) {
				std::swap(cu,cv);
				std::swap(bcu,bcv);
				std::swap(u,v);
			}
			//print_parent_tree(*this);
			debug_printf("Everting...");
			evert(bcu);
			debug_printf("Done\n");
			//print_parent_tree(*this);

			parent_bc[bcu] = bcv;
			parent_edge[bcu] = e;

			components.link(cu,cv);
		}
		else { // condense bridge components
			debug_printf("Edge (%d,%d) is condensing bridge_components\n", u,v);
			int bcu = bridge_conn_components.root(u);
			int bcv = bridge_conn_components.root(v);

			
			debug_printf("finding lca... ");
			int bcz = find_lca(bcu, bcv);	
			debug_printf("Done\n");

			std::vector<int> to_link;
			for(int x : {bcu, bcv}) {
				int bcx = x;
				while(bcx != bcz) {
					debug_printf("%d ", bcx);
					removed_bridges.push_back(get_parent_edge(bcx));
					to_link.push_back(bcx);
					bcx = get_parent_bc(bcx);
				}
			}
			int bczp = get_parent_bc(bcz);
			Edge bcze = get_parent_edge(bcz);
			int new_bcz = bcz;
			for(auto bcx : to_link) {
				new_bcz = bridge_conn_components.link(bcx,bcz);
			}
			parent_bc[new_bcz] = bczp;
			parent_edge[new_bcz] = bcze;
		}
	}

	return removed_bridges;
}

int IncrementalBridgeConnComponents::find_lca(int bcu, int bcv) {
	int bcu_start = bcu;
	int bcv_start = bcv;
	int lca = -1;
	while(true) {
		debug_printf("bcu = %d, bcv = %d\n", bcu, bcv);
		debug_printf("parent_bc[bcu] = %d, parent_bc[bcv] = %d\n", parent_bc[bcu], parent_bc[bcv]);
		if(get_parent_bc(bcu) != -1) {
			visited[bcu] = true;
			bcu = get_parent_bc(bcu); 
			if(visited[bcu]) {
				lca = bcu;
				break;
			}
		}
		if(get_parent_bc(bcv) != -1) {
			visited[bcv] = true;
			bcv = get_parent_bc(bcv);
			if(visited[bcv]) {
				lca = bcv;
				break;
			}
		}

		if(bcu == bcv) { // since root does not get marked
			lca = bcu;
			break;
		}
	}

	debug_printf("lca is = %d\n", lca);

	for(int x : {bcu_start, bcv_start}) {
		int y = x;
		while((parent_bc[y] != -1) && visited[y]) {
			debug_printf("%d %d\n", y, parent_bc[y]);
			if( parent_bc[y] == -1) {
				debug_printf("dafuq...\n");
			}
			visited[y] = false;
			y = parent_bc[y];
		}
		visited[y] = false;
	}

	std::fill(visited.begin(), visited.end(), false);

	for(int i = 0; i < size; i++) {
		assert( !visited[i] );
	}
	return lca;
}

void IncrementalBridgeConnComponents::evert(int bcu) {
	bcu = bridge_conn_components.root(bcu);
	int curr = bcu;
	Edge e = get_parent_edge(curr);
	//int par = bridge_conn_components.root(parent_bc[curr]);
	int par = get_parent_bc(curr);
	while(par != -1) {
		//int pp = bridge_conn_components.root(parent_bc[par]);
		int pp = get_parent_bc(par);
		Edge pe = get_parent_edge(par);
		parent_bc[par] = curr;
		parent_edge[par] = e;

		e = pe;
		curr = par;
		par = pp;
	}
	parent_bc[bcu] = -1;
	parent_edge[bcu] = null_edge;
}


void print_union_find(UnionFind& f) {
	int n = f.size;

	std::vector<int> size(n,0);
	std::vector<std::vector<int>> label_list(n);

	for(int i = 0; i < n; i++) {
		label_list[i].clear();
	}

	for(int i = 0; i < n; i++) {
		int l = f.label(i);
		label_list[l].push_back(i);
		size[l]++;
	}

	for(int i = 0; i < n; i++) {
		if( label_list[i].size() == 0) {
			continue;
		}
		debug_printf("Label: %d\n", i);
		debug_printf("   Size: %d\n", size[i]);
		debug_printf("   Rank: %d\n", f.rank[i]);
		debug_printf("   Members: ");
		for(auto x : label_list[i]) {
			debug_printf("%d ", x);
		}
		debug_printf("\n");
	}
	debug_printf("\n");
}


bool test_edge(Graph& g, Edge e, int threshold) {
	Weight orig_weight = e.weight();
	e.edge_data()->weight = threshold + 1;

	std::vector<Weight> dist(num_vertices(g));
	Dummy pred_map;

	const auto cmp = [&](Vertex a, Vertex b) { return dist[a] < dist[b]; };
	Heap<Vertex, decltype(cmp), std::vector<unsigned>, 4> heap{cmp};
	heap.map.assign(g.vertex_count, heap.not_in_heap);

	bool success = true;

	threshold -= orig_weight;

	for(Vertex v : { e.source(), e.target() } ) {
		std::fill(dist.begin(), dist.end(), std::numeric_limits<Weight>::max());
		bool found_terminal = false;
		Weight term_dist = 0;
		while(!heap.empty()) heap.pop();
		heap.push(v);
		dist[v] = 0;
		try {
			Dijkstra(g, dist, pred_map, heap,
				[&](Vertex v){
					if( dist[v] > threshold ) {
						debug_printf("  terminal not found within %d distance (already reached %d)   ", threshold, dist[v]);
						throw EarlyTerminate();
					}
					if (g.is_terminal(v)) {
						found_terminal = true;
						debug_printf("  terminal found within %d distance    ", dist[v]);
						term_dist = dist[v];
						throw EarlyTerminate();
					}
				}
			);
		}
		catch (EarlyTerminate e) {}

		if(!found_terminal) {
			success = false;
			break;
		}

		threshold -= term_dist;
	}
	e.edge_data()->weight = orig_weight;

	if(success) {
		g.mark_terminal(e.source());
		g.mark_terminal(e.target());
	}
	return success;
}


void terminal_distance_test(Graph& g) {
	IncrementalBridgeConnComponents inc(num_vertices(g));

	std::vector<Edge> sorted_edges(num_edges(g), null_edge);

	std::partial_sort_copy(
			g.edge_list.begin(), g.edge_list.end(),
			sorted_edges.begin(), sorted_edges.end(),
			[&](Edge e, Edge f) {
				return e.weight() < f.weight();
			});
	
	std::vector<Edge> to_buy;

	Weight last_weight = -1;
	for(auto it = sorted_edges.begin(); it != sorted_edges.end(); ++it) {
		Edge e = *it;
		if( last_weight != e.weight() ) {
			auto jt = it;
			while(jt != sorted_edges.end() && jt->weight() == e.weight() ) {
				Edge f = *jt;
				if ( inc.components.root(f.source()) != inc.components.root(f.target()) &&
					g.is_terminal(f.source()) && g.is_terminal(f.target()) ) {
					debug_printf("Edge (%d,%d) connects two terminal... Marking for buying...", f.source(), f.target());
					to_buy.push_back(f);
					inc.link(f);
					debug_printf("Done\n");
				}
				++jt;
			}
			last_weight = e.weight();
		}

		std::vector<Edge> removed_bridges = inc.link(e);
		for(auto f: removed_bridges ) {
			debug_printf("Testing edge (%d,%d) weight %d against threshold %d...", f.source(), f.target(), f.weight(), e.weight());
			if(test_edge(g, f, e.weight())) {
				debug_printf("Marking for buying...");
				to_buy.push_back(f);
			}
			debug_printf("Done\n");
		}
	}
	print_union_find(inc.components);
	print_union_find(inc.bridge_conn_components);

	for(auto e : to_buy) {
		g.buy_edge(e);
	}
}

#endif
