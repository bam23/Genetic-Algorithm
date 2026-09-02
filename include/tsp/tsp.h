#ifndef TSP_TSP_H
#define TSP_TSP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TSP_MAX_CITIES 20
#define TSP_MAX_BRUTEFORCE_CITIES 12
#define TSP_MAX_POPULATION 12

typedef enum tsp_status {
    TSP_STATUS_OK = 0,
    TSP_STATUS_INVALID_ARGUMENT,
    TSP_STATUS_IO_ERROR,
    TSP_STATUS_INVALID_DATA,
    TSP_STATUS_LIMIT_EXCEEDED,
    TSP_STATUS_ARITHMETIC_OVERFLOW,
    TSP_STATUS_INTERNAL_ERROR
} tsp_status;

typedef enum tsp_algorithm {
    TSP_ALGORITHM_BRUTE_FORCE = 0,
    TSP_ALGORITHM_EVOLUTIONARY
} tsp_algorithm;

typedef struct tsp_graph {
    double costs[TSP_MAX_CITIES][TSP_MAX_CITIES];
    int city_count;
} tsp_graph;

typedef struct tsp_bruteforce_config {
    int city_count;
} tsp_bruteforce_config;

typedef struct tsp_evolution_config {
    int city_count;
    int population_size;
    int generations;
    int elite_count;
    int mutation_swaps;
    uint32_t seed;
} tsp_evolution_config;

/*
 * Solver output only. The caller retains the corresponding config when it
 * needs to reproduce or display a run; the library does not duplicate the
 * complete input configuration in each result.
 */
typedef struct tsp_result {
    int tour[TSP_MAX_CITIES];
    int city_count;
    double cost;
    uint64_t candidates_evaluated;
    tsp_algorithm algorithm;
    bool guaranteed_optimal;
} tsp_result;

/* Load the coursework's fixed 20-city off-diagonal matrix format. */
tsp_status tsp_graph_load(tsp_graph *out_graph, const char *path);

tsp_status tsp_solve_bruteforce(const tsp_graph *graph,
                                const tsp_bruteforce_config *config,
                                tsp_result *out_result);

tsp_status tsp_solve_evolutionary(const tsp_graph *graph,
                                  const tsp_evolution_config *config,
                                  tsp_result *out_result);

const char *tsp_status_string(tsp_status status);

#ifdef __cplusplus
}
#endif

#endif
