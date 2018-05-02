

#include <cstdlib>

#include "graph.hpp"
#include "read.hpp"

int
main(int argc, char *argv[])
{
  Graph *g;

  g = graph_from_file(argv[3]);

  //g->contract_edge(new Edge(3,5,1));


  return (EXIT_SUCCESS);
}

