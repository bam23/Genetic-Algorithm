#ifndef FUNCTION_H
#define FUNCTION_H

#include <stdbool.h>
#include <stdint.h>

#include "graph.h"
#include "queue.h"

/* Exhaustive search is intentionally limited to small coursework examples. */
#define MAX_EXACT_CITIES 12

typedef struct random_state {
    uint32_t value;
} random_state;

typedef struct search_result {
    int tour[MAXNUM];
    int city_count;
    double cost;
    uint64_t evaluated_tours;
} search_result;

typedef struct evolution_config {
    int population_size;
    int generations;
    int elite_count;
    int mutation_swaps;
    uint32_t seed;
    bool verbose;
} evolution_config;

double tour_cost(const double cities[MAXNUM][MAXNUM],
                 const int tour[MAXNUM],
                 int city_count);
bool tour_is_valid(const int tour[MAXNUM], int city_count);
bool factorial_checked(int number, uint64_t *result);
bool next_permutation(int tour[MAXNUM], int city_count);

void random_state_init(random_state *state, uint32_t seed);
void mutate_tour(int tour[MAXNUM],
                 int city_count,
                 int swap_count,
                 random_state *state);

bool exhaustive_search(const double cities[MAXNUM][MAXNUM],
                       int city_count,
                       bool verbose,
                       search_result *result);
bool evolutionary_search(const double cities[MAXNUM][MAXNUM],
                         int city_count,
                         const evolution_config *config,
                         search_result *result);

#endif
