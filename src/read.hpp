#ifndef READ_HPP
#define READ_HPP

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "graph.hpp"

#define READ_HPP_BUFF_LEN 255
#define PACE_SECTION_END "END\n"
#define PACE_TERMINALS_START "SECTION Terminals\n"

void
read_edges(FILE *fin, Graph *g, size_t nedges, char *buff, size_t buff_len)
{
	unsigned int u, v, weight;

	for (; nedges--; ) {
		fgets(buff, buff_len, fin);
		//fprintf(stderr, "edge: %s", buff);
		if (strcmp(buff, PACE_SECTION_END) != 0) {
			sscanf(buff, "%*c %u %u %u", &u, &v, &weight);
			g->add_edge(u-1, v-1, weight, u, v);
		}
	}
}

void
read_terminals(FILE *fin, Graph *g, char *buff, size_t buff_len)
{
	unsigned int terminal;
	unsigned int nterminals;

	while (true) {
		fgets(buff, buff_len, fin);
		if (strcmp(buff, PACE_TERMINALS_START) == 0)
			break;
	}

	fgets(buff, buff_len, fin);
	sscanf(buff, "%*s %u", &nterminals);

	for (; nterminals--; ) {
		fgets(buff, buff_len, fin);
		if (strcmp(buff, PACE_SECTION_END) != 0) {
			sscanf(buff, "%*c %u", &terminal);
			--terminal;
			g->mark_terminal(terminal);
		}
	}
}

Graph*
graph_from_file(FILE *fin)
{
	unsigned int nvert = 0, nedges = 0;
	char buff[READ_HPP_BUFF_LEN];

	fgets(buff, READ_HPP_BUFF_LEN, fin);
	fgets(buff, READ_HPP_BUFF_LEN, fin);
	sscanf(buff, "%*s %u", &nvert);
	fgets(buff, READ_HPP_BUFF_LEN, fin);
	sscanf(buff, "%*s %u", &nedges);

	//fprintf(stderr, "Graph with %u vertices and %u edges to be created.", nvert, nedges);

	Graph *g = new Graph(nvert);

	//fprintf(stderr, "Graph created; reading edges.\n");
	read_edges(fin, g, nedges, buff, READ_HPP_BUFF_LEN);
	//fprintf(stderr, "Marking terminals.");
	read_terminals(fin, g, buff, READ_HPP_BUFF_LEN);
	//fprintf(stderr, "Done.");

	fclose(fin);
	return (g);
}

#endif // READ_HPP

