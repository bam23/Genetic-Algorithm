#ifndef TSP_SOLVER_INTERNAL_H
#define TSP_SOLVER_INTERNAL_H

#include <stdbool.h>
#include <stdint.h>

#include <tsp/tsp.h>

typedef struct tsp_random_state {
    uint32_t value;
} tsp_random_state;

double tsp_tour_cost(const tsp_graph *graph,
                     const int tour[TSP_MAX_CITIES],
                     int city_count);
bool tsp_tour_is_valid(const int tour[TSP_MAX_CITIES], int city_count);
bool tsp_factorial_checked(int number, uint64_t *result);
bool tsp_next_permutation(int tour[TSP_MAX_CITIES], int city_count);

void tsp_random_state_init(tsp_random_state *state, uint32_t seed);
void tsp_mutate_tour(int tour[TSP_MAX_CITIES],
                     int city_count,
                     int swap_count,
                     tsp_random_state *state);

#endif
