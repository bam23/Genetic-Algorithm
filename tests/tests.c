#include <tsp/tsp.h>

#include "heap.h"
#include "solver_internal.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int tests_run = 0;
static int tests_failed = 0;

#define CHECK(condition)                                                         \
    do {                                                                         \
        if (!(condition)) {                                                      \
            fprintf(stderr, "  assertion failed at %s:%d: %s\n",              \
                    __FILE__, __LINE__, #condition);                             \
            return false;                                                        \
        }                                                                        \
    } while (false)

static bool nearly_equal(double first, double second)
{
    return fabs(first - second) <= 1.0e-9;
}

static void make_known_graph(tsp_graph *graph)
{
    memset(graph, 0, sizeof(*graph));
    graph->city_count = 4;

    const double known[4][4] = {
        { 0.0, 10.0, 15.0, 20.0 },
        { 10.0, 0.0, 35.0, 25.0 },
        { 15.0, 35.0, 0.0, 30.0 },
        { 20.0, 25.0, 30.0, 0.0 }
    };
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            graph->costs[row][column] = known[row][column];
        }
    }
}

static bool test_graph_load(void)
{
    tsp_graph graph;
    CHECK(tsp_graph_load(&graph, "cities.dat") == TSP_STATUS_OK);
    CHECK(graph.city_count == TSP_MAX_CITIES);
    for (int city = 0; city < graph.city_count; ++city) {
        CHECK(nearly_equal(graph.costs[city][city], 0.0));
    }
    CHECK(tsp_graph_load(NULL, "cities.dat") == TSP_STATUS_INVALID_ARGUMENT);
    return true;
}

static bool test_tour_cost(void)
{
    tsp_graph graph;
    make_known_graph(&graph);
    const int tour[TSP_MAX_CITIES] = { 0, 1, 3, 2 };

    CHECK(nearly_equal(tsp_tour_cost(&graph, tour, 4), 80.0));
    return true;
}

static bool test_heap_ordering(void)
{
    tsp_min_heap heap;
    tsp_heap_init(&heap, 4U);

    const tsp_candidate candidates[] = {
        { .tour = { 0, 1, 2, 3 }, .city_count = 4, .cost = 5.0 },
        { .tour = { 0, 1, 3, 2 }, .city_count = 4, .cost = 1.0 },
        { .tour = { 0, 2, 1, 3 }, .city_count = 4, .cost = 3.0 }
    };
    for (size_t index = 0U; index < 3U; ++index) {
        CHECK(tsp_heap_insert(&heap, &candidates[index]));
    }

    const double expected[] = { 1.0, 3.0, 5.0 };
    for (size_t index = 0U; index < 3U; ++index) {
        tsp_candidate minimum;
        CHECK(tsp_heap_delete_min(&heap, &minimum));
        CHECK(nearly_equal(minimum.cost, expected[index]));
    }
    CHECK(heap.size == 0U);
    return true;
}

static bool test_permutation_validity(void)
{
    int tour[TSP_MAX_CITIES] = { 0, 1, 2, 3 };
    int count = 1;
    CHECK(tsp_tour_is_valid(tour, 4));

    while (tsp_next_permutation(tour, 4)) {
        CHECK(tour[0] == 0);
        CHECK(tsp_tour_is_valid(tour, 4));
        ++count;
    }
    CHECK(count == 6);

    const int duplicate[TSP_MAX_CITIES] = { 0, 1, 1, 3 };
    const int out_of_range[TSP_MAX_CITIES] = { 0, 1, 2, 4 };
    CHECK(!tsp_tour_is_valid(duplicate, 4));
    CHECK(!tsp_tour_is_valid(out_of_range, 4));
    return true;
}

static bool test_bruteforce_search(void)
{
    tsp_graph graph;
    make_known_graph(&graph);
    const tsp_bruteforce_config config = { .city_count = 4 };

    tsp_result result;
    CHECK(tsp_solve_bruteforce(&graph, &config, &result) == TSP_STATUS_OK);
    CHECK(nearly_equal(result.cost, 80.0));
    CHECK(result.candidates_evaluated == UINT64_C(6));
    CHECK(result.algorithm == TSP_ALGORITHM_BRUTE_FORCE);
    CHECK(result.guaranteed_optimal);
    CHECK(tsp_tour_is_valid(result.tour, 4));
    CHECK(nearly_equal(result.cost,
                       tsp_tour_cost(&graph, result.tour, 4)));
    return true;
}

static bool test_mutation_preserves_permutation(void)
{
    int tour[TSP_MAX_CITIES] = { 0, 1, 2, 3, 4, 5 };
    tsp_random_state state;
    tsp_random_state_init(&state, UINT32_C(12345));

    tsp_mutate_tour(tour, 6, 20, &state);
    CHECK(tour[0] == 0);
    CHECK(tsp_tour_is_valid(tour, 6));
    return true;
}

static bool test_deterministic_evolution(void)
{
    tsp_graph graph;
    make_known_graph(&graph);

    const tsp_evolution_config config = {
        .city_count = 4,
        .population_size = 6,
        .generations = 10,
        .elite_count = 2,
        .mutation_swaps = 1,
        .seed = UINT32_C(12345)
    };
    double first_costs[11];
    double second_costs[11];
    tsp_convergence_history first_history = {
        .best_costs = first_costs,
        .capacity = 11U,
        .count = 0U
    };
    tsp_convergence_history second_history = {
        .best_costs = second_costs,
        .capacity = 11U,
        .count = 0U
    };
    tsp_result first;
    tsp_result second;

    CHECK(tsp_solve_evolutionary(&graph,
                                 &config,
                                 &first_history,
                                 &first) == TSP_STATUS_OK);
    CHECK(tsp_solve_evolutionary(&graph,
                                 &config,
                                 &second_history,
                                 &second) == TSP_STATUS_OK);
    CHECK(nearly_equal(first.cost, second.cost));
    CHECK(memcmp(first.tour,
                 second.tour,
                 (size_t)first.city_count * sizeof(first.tour[0])) == 0);
    CHECK(first.candidates_evaluated == second.candidates_evaluated);
    CHECK(first.candidates_evaluated == UINT64_C(46));
    CHECK(first.algorithm == TSP_ALGORITHM_EVOLUTIONARY);
    CHECK(!first.guaranteed_optimal);
    CHECK(first_history.count == 11U);
    CHECK(second_history.count == 11U);
    for (size_t index = 0U; index < first_history.count; ++index) {
        CHECK(nearly_equal(first_costs[index], second_costs[index]));
        if (index > 0U) {
            CHECK(first_costs[index] <= first_costs[index - 1U]);
        }
    }
    CHECK(nearly_equal(first_costs[10], first.cost));
    return true;
}

static bool test_convergence_history_contract(void)
{
    tsp_graph graph;
    make_known_graph(&graph);

    tsp_evolution_config config = {
        .city_count = 4,
        .population_size = 6,
        .generations = 5,
        .elite_count = 1,
        .mutation_swaps = 2,
        .seed = UINT32_C(98765)
    };
    double undersized_values[5] = { -1.0, -1.0, -1.0, -1.0, -1.0 };
    tsp_convergence_history undersized = {
        .best_costs = undersized_values,
        .capacity = 5U,
        .count = 99U
    };
    tsp_result result;

    CHECK(tsp_solve_evolutionary(&graph,
                                 &config,
                                 &undersized,
                                 &result) == TSP_STATUS_BUFFER_TOO_SMALL);
    CHECK(undersized.count == 0U);
    for (size_t index = 0U; index < 5U; ++index) {
        CHECK(nearly_equal(undersized_values[index], -1.0));
    }

    double values[6];
    tsp_convergence_history history = {
        .best_costs = values,
        .capacity = 6U,
        .count = 0U
    };
    CHECK(tsp_solve_evolutionary(&graph,
                                 &config,
                                 &history,
                                 &result) == TSP_STATUS_OK);
    CHECK(history.count == 6U);
    CHECK(nearly_equal(history.best_costs[5], result.cost));

    tsp_evolution_config generation_zero = config;
    generation_zero.generations = 0;
    tsp_result initial_result;
    CHECK(tsp_solve_evolutionary(&graph,
                                 &generation_zero,
                                 NULL,
                                 &initial_result) == TSP_STATUS_OK);
    CHECK(nearly_equal(history.best_costs[0], initial_result.cost));
    return true;
}

static bool test_heuristic_fitness_matches_route(void)
{
    tsp_graph graph;
    make_known_graph(&graph);

    const tsp_evolution_config config = {
        .city_count = 4,
        .population_size = 6,
        .generations = 5,
        .elite_count = 1,
        .mutation_swaps = 2,
        .seed = UINT32_C(98765)
    };
    tsp_result result;

    CHECK(tsp_solve_evolutionary(&graph,
                                 &config,
                                 NULL,
                                 &result) == TSP_STATUS_OK);
    CHECK(tsp_tour_is_valid(result.tour, result.city_count));
    CHECK(nearly_equal(result.cost,
                       tsp_tour_cost(&graph,
                                     result.tour,
                                     result.city_count)));
    return true;
}

static bool test_factorial_overflow_protection(void)
{
    uint64_t result = 0U;
    CHECK(tsp_factorial_checked(12, &result));
    CHECK(result == UINT64_C(479001600));
    CHECK(!tsp_factorial_checked(21, &result));
    return true;
}

static bool test_v1_regression_example(void)
{
    tsp_graph graph;
    CHECK(tsp_graph_load(&graph, "cities.dat") == TSP_STATUS_OK);

    const tsp_bruteforce_config brute_force = { .city_count = 5 };
    tsp_result brute_force_result;
    CHECK(tsp_solve_bruteforce(&graph,
                               &brute_force,
                               &brute_force_result) == TSP_STATUS_OK);
    const int expected_brute_force[] = { 0, 3, 2, 4, 1 };
    CHECK(nearly_equal(brute_force_result.cost, 1596.2320856626313));
    CHECK(brute_force_result.candidates_evaluated == UINT64_C(24));
    CHECK(memcmp(brute_force_result.tour,
                 expected_brute_force,
                 sizeof(expected_brute_force)) == 0);

    const tsp_evolution_config evolution = {
        .city_count = 5,
        .population_size = 8,
        .generations = 10,
        .elite_count = 2,
        .mutation_swaps = 1,
        .seed = UINT32_C(12345)
    };
    tsp_result evolutionary_result;
    CHECK(tsp_solve_evolutionary(&graph,
                                 &evolution,
                                 NULL,
                                 &evolutionary_result) == TSP_STATUS_OK);
    const int expected_evolutionary[] = { 0, 3, 4, 2, 1 };
    CHECK(nearly_equal(evolutionary_result.cost, 1598.0174822886754));
    CHECK(evolutionary_result.candidates_evaluated == UINT64_C(68));
    CHECK(memcmp(evolutionary_result.tour,
                 expected_evolutionary,
                 sizeof(expected_evolutionary)) == 0);
    return true;
}

static void run_test(const char *name, bool (*test_function)(void))
{
    ++tests_run;
    if (test_function()) {
        printf("PASS %s\n", name);
    } else {
        ++tests_failed;
        printf("FAIL %s\n", name);
    }
}

int main(void)
{
    run_test("graph loading through public API", test_graph_load);
    run_test("tour cost calculation", test_tour_cost);
    run_test("binary min-heap ordering", test_heap_ordering);
    run_test("permutation validity", test_permutation_validity);
    run_test("brute-force solver correctness", test_bruteforce_search);
    run_test("mutation preserves permutation", test_mutation_preserves_permutation);
    run_test("fixed-seed deterministic evolution", test_deterministic_evolution);
    run_test("convergence history contract", test_convergence_history_contract);
    run_test("heuristic route and fitness agree",
             test_heuristic_fitness_matches_route);
    run_test("recursive factorial overflow protection",
             test_factorial_overflow_protection);
    run_test("v1.0.0 deterministic example regression",
             test_v1_regression_example);

    printf("\n%d tests run, %d failed\n", tests_run, tests_failed);
    return tests_failed == 0 ? 0 : 1;
}
