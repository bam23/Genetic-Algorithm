#include "function.h"
#include "queue.h"

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

static void make_known_graph(double cities[MAXNUM][MAXNUM])
{
    memset(cities, 0, (size_t)MAXNUM * (size_t)MAXNUM * sizeof(cities[0][0]));

    const double known[4][4] = {
        { 0.0, 10.0, 15.0, 20.0 },
        { 10.0, 0.0, 35.0, 25.0 },
        { 15.0, 35.0, 0.0, 30.0 },
        { 20.0, 25.0, 30.0, 0.0 }
    };
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            cities[row][column] = known[row][column];
        }
    }
}

static bool test_tour_cost(void)
{
    double cities[MAXNUM][MAXNUM];
    make_known_graph(cities);
    const int tour[MAXNUM] = { 0, 1, 3, 2 };

    CHECK(nearly_equal(tour_cost(cities, tour, 4), 80.0));
    return true;
}

static bool test_heap_ordering(void)
{
    min_heap heap;
    heap_init(&heap, 4U);

    const node candidates[] = {
        { .tour = { 0, 1, 2, 3 }, .city_count = 4, .cost = 5.0 },
        { .tour = { 0, 1, 3, 2 }, .city_count = 4, .cost = 1.0 },
        { .tour = { 0, 2, 1, 3 }, .city_count = 4, .cost = 3.0 }
    };
    for (size_t index = 0U; index < 3U; ++index) {
        CHECK(heap_insert(&heap, &candidates[index]));
    }

    const double expected[] = { 1.0, 3.0, 5.0 };
    for (size_t index = 0U; index < 3U; ++index) {
        node minimum;
        CHECK(heap_delete_min(&heap, &minimum));
        CHECK(nearly_equal(minimum.cost, expected[index]));
    }
    CHECK(heap.size == 0U);
    return true;
}

static bool test_permutation_validity(void)
{
    int tour[MAXNUM] = { 0, 1, 2, 3 };
    int count = 1;
    CHECK(tour_is_valid(tour, 4));

    while (next_permutation(tour, 4)) {
        CHECK(tour[0] == 0);
        CHECK(tour_is_valid(tour, 4));
        ++count;
    }
    CHECK(count == 6);

    const int duplicate[MAXNUM] = { 0, 1, 1, 3 };
    const int out_of_range[MAXNUM] = { 0, 1, 2, 4 };
    CHECK(!tour_is_valid(duplicate, 4));
    CHECK(!tour_is_valid(out_of_range, 4));
    return true;
}

static bool test_exhaustive_search(void)
{
    double cities[MAXNUM][MAXNUM];
    make_known_graph(cities);

    search_result result;
    CHECK(exhaustive_search(cities, 4, false, &result));
    CHECK(nearly_equal(result.cost, 80.0));
    CHECK(result.evaluated_tours == UINT64_C(6));
    CHECK(tour_is_valid(result.tour, 4));
    CHECK(nearly_equal(result.cost, tour_cost(cities, result.tour, 4)));
    return true;
}

static bool test_mutation_preserves_permutation(void)
{
    int tour[MAXNUM] = { 0, 1, 2, 3, 4, 5 };
    random_state state;
    random_state_init(&state, UINT32_C(12345));

    mutate_tour(tour, 6, 20, &state);
    CHECK(tour[0] == 0);
    CHECK(tour_is_valid(tour, 6));
    return true;
}

static bool test_deterministic_evolution(void)
{
    double cities[MAXNUM][MAXNUM];
    make_known_graph(cities);

    const evolution_config config = {
        .population_size = 6,
        .generations = 10,
        .elite_count = 2,
        .mutation_swaps = 1,
        .seed = UINT32_C(12345),
        .verbose = false
    };
    search_result first;
    search_result second;

    CHECK(evolutionary_search(cities, 4, &config, &first));
    CHECK(evolutionary_search(cities, 4, &config, &second));
    CHECK(nearly_equal(first.cost, second.cost));
    CHECK(memcmp(first.tour,
                 second.tour,
                 (size_t)first.city_count * sizeof(first.tour[0])) == 0);
    CHECK(first.evaluated_tours == second.evaluated_tours);
    CHECK(first.evaluated_tours == UINT64_C(46));
    return true;
}

static bool test_heuristic_fitness_matches_route(void)
{
    double cities[MAXNUM][MAXNUM];
    make_known_graph(cities);

    const evolution_config config = {
        .population_size = 6,
        .generations = 5,
        .elite_count = 1,
        .mutation_swaps = 2,
        .seed = UINT32_C(98765),
        .verbose = false
    };
    search_result result;

    CHECK(evolutionary_search(cities, 4, &config, &result));
    CHECK(tour_is_valid(result.tour, result.city_count));
    CHECK(nearly_equal(result.cost,
                       tour_cost(cities, result.tour, result.city_count)));
    return true;
}

static bool test_factorial_overflow_protection(void)
{
    uint64_t result = 0U;
    CHECK(factorial_checked(12, &result));
    CHECK(result == UINT64_C(479001600));
    CHECK(!factorial_checked(21, &result));
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
    run_test("tour cost calculation", test_tour_cost);
    run_test("binary min-heap ordering", test_heap_ordering);
    run_test("permutation validity", test_permutation_validity);
    run_test("exhaustive solver correctness", test_exhaustive_search);
    run_test("mutation preserves permutation", test_mutation_preserves_permutation);
    run_test("fixed-seed deterministic evolution", test_deterministic_evolution);
    run_test("heuristic fitness matches route", test_heuristic_fitness_matches_route);
    run_test("factorial overflow protection", test_factorial_overflow_protection);

    printf("\n%d tests run, %d failed\n", tests_run, tests_failed);
    return tests_failed == 0 ? 0 : 1;
}
