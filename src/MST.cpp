#include <stdio.h>
#include <signal.h>

#include "graph.hpp"
#include "read.hpp"
#include "MST.hpp"

volatile sig_atomic_t g_stop_signal = 0;
 
void
signal_handler(int signal)
{
		    g_stop_signal = signal;
}

int main(int , char* argv[]) {

	/* register signal handler */
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	fprintf(stderr, "Start\n");
	Graph* g = graph_from_file(stdin);
	
	//Test Graph
	/*
		Graph* g = new Graph(5);
	Edge *e1, *e2, *e3, *e4, *e5, *e6, *e7, *e8, *e9, *e10;

	e1 = new Edge(0,1,3, 1,2);
	e2 = new Edge(1,2,2, 2,3);
	e3 = new Edge(2,0,70, 3,1);
	e4 = new Edge(0,4,30, 1,5);
	e5 = new Edge(1,4,3, 2,5);
	e6 = new Edge(2,4,20, 3,5);
	e7 = new Edge(0,3,20, 1,4);
	e8 = new Edge(1,3,4, 2,4);
	e9 = new Edge(2,3,20, 3,4);
	e10 = new Edge(3,4,20, 4,5);
	
	g->add_edge(e1);
	g->add_edge(e2);
	g->add_edge(e3);
	g->add_edge(e4);
	g->add_edge(e5);
	g->add_edge(e6);
	g->add_edge(e7);
	g->add_edge(e8);
	g->add_edge(e9);
	g->add_edge(e10);
				
	g->mark_terminal(0);
	g->mark_terminal(2);
	g->mark_terminal(3);
	g->mark_terminal(4);
	*/
	fprintf(stderr, "Graph loaded\n");
	fprintf(stderr, "|V| = %d, |E| = %d, |R| = %d\n", g->vertex_count, g->edge_count, g->terminal_count);
	fprintf(stderr, "Calling `greedy_2approx`\n");
	std::vector<Edge*> result = greedy_2approx(*g);
	//contract_till_the_bitter_end(*g);
	print_solution(stdout, result);
}
