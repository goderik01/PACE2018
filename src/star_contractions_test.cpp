#include "magic_constants.hpp"

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

int main(int argc, char** argv) {
	int seed = argc >= 3 ? atoi(argv[2]) : time(NULL);
	debug_printf("Random seed: %d\n", seed);
	_rand_gen = std::mt19937_64(seed);

	/* register signal handler */
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	debug_printf("Start\n");
	Graph g = graph_from_file(stdin);
	debug_printf("Graph loaded\n");
	debug_printf("|V| = %d, |E| = %d, |R| = %d\n", g.vertex_count, g.edge_count, g.terminal_count);

	debug_printf("First clearance of the input. Calling buy_zero and run_all_heuristics.\n");
	buy_zero(g);
	run_all_heuristics(g);

	g.compress_graph();
	g.save_orig_graph();

	std::vector<Vertex> possible_vertices;
	for (Vertex v = 0; v < g.vertex_count; v++)
		if (g.degrees[v] >= 3) possible_vertices.push_back(v);


	debug_printf("\nCalling `contract_till_the_bitter_end`\n");
	TIMER_BEGIN {
		TIMER(CONST_STAR_CONTRACTIONS_TIME) contract_till_the_bitter_end(g);
	} TIMER_END("contract_till_the_bitter_end: %lg s\n", timer);

	Graph tmp = end_heu(g, possible_vertices);

	print_solution(stdout, tmp);
}

