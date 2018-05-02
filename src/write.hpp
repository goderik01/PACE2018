#ifndef WRITE_HPP
#define WRITE_HPP

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "graph.hpp"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define PACE_GRAPH_START "SECTION Graph\n"
#define PACE_SECTION_END "END\n"
#define PACE_FILE_END "EOF\n"
#define PACE_TERMINALS_START "SECTION Terminals\n"

int 
count_edges(const Graph& g) {
	int number_of_edges = 0;
		for(int i = 0; i < g.vertex_count; i++){
		std::vector<int> processed;
		 processed.push_back(i); // don't count loops
		for(auto e : *(g.inc_edges[i])) {
			int smaller_end = MIN(boost::source(e,g), boost::target(e,g));
			int bigger_end = MAX(boost::source(e,g), boost::target(e,g));
			if ((smaller_end == i) && (std::find(processed.begin(),processed.end(),bigger_end) == processed.end()))  { // If one of the endpoints is smaller than i, or the edge is processed then we already printed out the edge, I hope ...
				processed.push_back(bigger_end);
				number_of_edges++;
			}
		}
	}
	return number_of_edges;
}

void 
graph_to_file(FILE* out, const Graph& g) {
	std::vector<int> vertex_map;
//	std::vector<bool> processed(g.vertex_count); 
	int defect = 1;
		for(int i = 0; i < g.vertex_count; i++){
//			if ( g.degrees[i] == 0) { // If Vertex has degree 0, we want to skip it. 
//				--defect;
//			}
			vertex_map.push_back(i+defect);
		}
	
	fprintf(out, PACE_GRAPH_START);
	int num_of_edges = count_edges(g); // ideally should be in ------ g.edge_count
	fprintf(out, "Nodes %d\n", vertex_map[g.vertex_count-1]);
	fprintf(out, "Edges %d\n", num_of_edges);
	for(int i = 0; i < g.vertex_count; i++){
//		std::fill(processed.begin(), processed.end(), false); 
//		fprintf(out,"Node %d",i);
		std::vector<int> processed; 
		processed.push_back(i); // do not want to add loops
		for(auto e : *(g.inc_edges[i])) {
			int smaller_end = MIN(boost::source(e,g), boost::target(e,g));
			int bigger_end = MAX(boost::source(e,g), boost::target(e,g));
			if ((smaller_end == i) && (std::find(processed.begin(),processed.end(),bigger_end) == processed.end()))  { // If one of the endpoints is smaller than i, or the edge is processed then we already printed out the edge, I hope ...
				processed.push_back(bigger_end);
				int edge_weight = e->weight();
				for(auto f : *(g.inc_edges[i])) {
				if ( // if we find the same edge with a smaller weight, we update weight
					(smaller_end == MIN(boost::source(f,g), boost::target(f,g))) && 
					(bigger_end == MAX(boost::source(f,g), boost::target(f,g))) &&
					(f->weight() < edge_weight)) {
						edge_weight = f->weight();
					}
				}
				fprintf(out, "E %d %d %d\n", vertex_map[smaller_end], vertex_map[bigger_end],edge_weight);
			}
		}
	}	
	fprintf(out, PACE_SECTION_END);
	fprintf(out,"\n");
	fprintf(out, PACE_TERMINALS_START);
	fprintf(out, "Terminals %d\n",g.terminal_count);
	for(int i = 0; i < g.vertex_count; i++) {
		if (g.terminal_mask[i]) {
			fprintf(out, "T %d\n",vertex_map[i]);
		}
	}
	fprintf(out, PACE_SECTION_END);
	fprintf(out,"\n");
	fprintf(out,PACE_FILE_END);
}

#endif // WRITE_HPP
