#ifndef GRAPH_H
#define GRAPH_H

#include <stdbool.h>

#define MAXNUM 20

/* Load the fixed 20-city off-diagonal matrix used by the coursework. */
bool graph_load(const char *path, double cities[MAXNUM][MAXNUM]);
void graph_print(const double cities[MAXNUM][MAXNUM], int city_count);

#endif
