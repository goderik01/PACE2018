#ifndef DFS_HPP
#define DFS_HPP
#ifndef GRAPH_HPP
#error "dfs.hpp must be included from graph.hpp!"
#endif

#include "boost/graph/depth_first_search.hpp"

template < typename Backtrack, typename NonTreeEdge >
struct _dfs_visitor : boost::dfs_visitor<> {
	struct Context {
		Vertex source;
		std::vector<Vertex> pred;
		std::vector<Edge> tree_edges;
	};

	Backtrack b;
	NonTreeEdge nt;
	std::shared_ptr<Context> ctx;

	_dfs_visitor(const Graph& g, Vertex source, Backtrack b, NonTreeEdge nt) : b(b), nt(nt), ctx(new Context) {
		ctx->source = source;
		ctx->pred.resize(g.vertex_count, -1);
		ctx->pred[source] = -2;
		ctx->tree_edges.resize(g.vertex_count, null_edge);
	}

	void tree_edge(Edge e, const Graph&) {
		Vertex t = e.target();
		assert(ctx->pred[t] == -1);
		assert(ctx->pred[e.source()] != -1);
		ctx->tree_edges[t] = e;
		ctx->pred[t] = e.source();
	}

	void back_edge(Edge e, const Graph&) {
		if (ctx->tree_edges[e.source()] != e) nt(e);
	}

	void finish_vertex(Vertex v, const Graph&) {
		assert(ctx->pred[v] != -1);
		if (ctx->pred[v] >= 0) b(v, ctx->pred[v], ctx->tree_edges[v]);
	}
};

template < typename B, typename NT >
auto DFS(const Graph& g, Vertex s, B b, NT nt) {
	_dfs_visitor<B, NT> visitor(g, s, b, nt);
	std::vector<char> color_map(g.vertex_count, boost::color_traits<char>::white());
	boost::detail::depth_first_visit_impl(g, s, visitor, &color_map[0],
		boost::detail::nontruth2());
	return visitor.ctx;
}

template < typename T, typename Comp, typename IndexMap, int A = 2 >
struct Heap {
	std::vector<T> data;
	IndexMap map;
	Comp cmp;

	typedef typename IndexMap::value_type Index;
	enum : Index { not_in_heap = std::numeric_limits<Index>::max() };

	Heap(Comp c = Comp()) : cmp(c) {}

	void push(const T& x) {
		if (map[x] == not_in_heap) {
			data.push_back(x);
			map[x] = data.size() - 1;
		}
		decrease(x);
	}

	T pop() {
		T ret = data.front();
		map[ret] = not_in_heap;

		if (data.size() == 1) {
			data.pop_back();
			return ret;
		}

		std::swap(data.front(), data.back());
		data.pop_back();

		T* base = &data[0];
		T* index = base;
		while (true) {
			T* min_i = index;
			T* end = &data[std::min((size_t)(A * (index - base) + A), data.size() - 1)];
			for (T* i = &data[A * (index - base) + 1]; i <= end; i++)
				if (cmp(*i, *min_i)) min_i = i;

			if (min_i == index) {
				map[*index] = index - base;
				return ret;
			}

			map[*min_i] = index - base;
			std::swap(*index, *min_i);
			index = min_i;
		}
	}

	void decrease(T x) {
		size_t index = map[x];
		while (index > 0) {
			size_t anc = (index - 1) / A;
			if (!cmp(data[index], data[anc])) {
				map[x] = index;
				return;
			}

			map[data[anc]] = index;
			std::swap(data[anc], data[index]);
			index = anc;
		}
		map[x] = 0;
	}

	bool empty() const { return data.empty(); }
};


struct Dummy {
	template < typename ... Args > void operator() (Args...) const {}
	template < typename T > Dummy operator[](const T&) const { return Dummy(); }
	template < typename T > Dummy operator= (const T&) const { return Dummy(); }
};
const Dummy dummy;

template < typename DistMap, typename PredMap, typename Heap,
	class VertexPopped = Dummy, class EdgeRelaxed = Dummy, class EdgeNotRelaxed = Dummy >
void Dijkstra(const Graph& g, DistMap& dist, PredMap& pred, Heap& heap,
	VertexPopped vp = {}, EdgeRelaxed er = {}, EdgeNotRelaxed enr = {}) {
	while (!heap.empty()) {
		Vertex v = heap.pop();
		vp(v);

		for (auto e : g.inc_edges[v]) {
			Vertex u = e.target();
			if (dist[v] + e.weight() < dist[u]) {
				dist[u] = dist[v] + e.weight();
				pred[u] = v;
				heap.push(u);
				er(e);
			} else {
				enr(e);
			}
		}
	}
}

#endif

