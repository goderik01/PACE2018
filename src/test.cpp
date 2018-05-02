

#include <stdlib.h>

#include "graph.hpp"
// #include "read.hpp"
// #include "boost/graph/dijkstra_shortest_paths_no_color_map.hpp"

// #include <iostream>

using namespace boost;



int main(int , char* []) {

//	Graph* g = graph_from_file(stdin);
//
	Graph g(3);

	Edge e1, e2, e3;

	e1 = g.add_edge(0,1,1, 1,2);
	e2 = g.add_edge(1,2,4, 2,3);
	e3 = g.add_edge(2,0,10, 3,1);

	print_graph(stderr, g);

	g.add_edge(2,0,11,3,1);
	print_graph(stderr, g);

	g.add_edge(2,0,9,3,1);
	print_graph(stderr, g);


	g.contract_edge(e1);
	print_graph(stderr, g);

	g.contract_edge(e2);
	print_graph(stderr, g);

	g.contract_edge(e3);
	print_graph(stderr, g);
}
