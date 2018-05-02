#include <stdio.h>
#include <signal.h>

#include "read.hpp"
#include "star_contractions.hpp"

volatile sig_atomic_t g_stop_signal = 0;
 
void
signal_handler(int signal)
{
		    g_stop_signal = signal;
}

int main(int , char* []) {

	/* register signal handler */
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	debug_printf("Start\n");
	Graph* g = graph_from_file(stdin);
	debug_printf("Graph loaded\n");
	debug_printf("|V| = %d, |E| = %d, |R| = %d\n", g->vertex_count, g->edge_count, g->terminal_count);

	debug_printf("Calling `contract_till_the_bitter_end`\n");
	contract_till_the_bitter_end(*g);
	//contract_till_the_bitter_end(*g);
	print_solution(stdout, *g);
}
