

#include <stdlib.h>

#include "graph.hpp"
#include "write.hpp"
#include "boost/graph/dijkstra_shortest_paths_no_color_map.hpp"

#include <iostream>

using namespace boost;



int main(int , char* argv[]) {

//	Graph* g = graph_from_file(stdin);
//
	Graph g(5);
	Edge *e1, *e2, *e3, *e4, *e5, *e6;

	e1 = new Edge(0,1,2, 1,2);
	e2 = new Edge(1,2,4, 2,3);
	e3 = new Edge(2,0,7, 3,1);
	e4 = new Edge(0,4,3, 1,5);
	e5 = new Edge(1,4,5, 2,5);
	e6 = new Edge(2,4,2, 3,5);


	g.add_edge(e1);
	g.add_edge(e2);
	g.add_edge(e3);
	g.add_edge(e4);
	g.add_edge(e5);
	g.add_edge(e6);

	g.mark_terminal(0);
	g.mark_terminal(2);
		
	graph_to_file(stderr, g);
	
	g.contract_edge(e1);
	graph_to_file(stderr, g);
	
//	g.contract_edge(e2);
//	graph_to_file(stderr, g);
	
}
