#include "solver_internal.h"

#include <float.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static uint32_t random_next(random_state *state)
{
    /* Defined-width arithmetic makes fixed seeds portable and repeatable. */
    state->value = (state->value * UINT32_C(1664525)) + UINT32_C(1013904223);
    return state->value;
}

static int random_bounded(random_state *state, int upper_bound)
{
    if (state == NULL || upper_bound <= 0) {
        return 0;
    }
    return (int)(random_next(state) % (uint32_t)upper_bound);
}

static void swap_positions(int tour[MAXNUM], int first, int second)
{
    const int temporary = tour[first];
    tour[first] = tour[second];
    tour[second] = temporary;
}

static void initialize_identity_tour(int tour[MAXNUM], int city_count)
{
    for (int i = 0; i < city_count; ++i) {
        tour[i] = i;
    }
    for (int i = city_count; i < MAXNUM; ++i) {
        tour[i] = 0;
    }
}

static void shuffle_tour_tail(int tour[MAXNUM],
                              int city_count,
                              random_state *state)
{
    /* Keep city 0 anchored; rotating a closed tour does not create a new tour. */
    for (int i = city_count - 1; i > 1; --i) {
        const int other = 1 + random_bounded(state, i);
        swap_positions(tour, i, other);
    }
}

static void print_tour(const int tour[MAXNUM], int city_count)
{
    for (int i = 0; i < city_count; ++i) {
        printf("%d -> ", tour[i]);
    }
    printf("%d", tour[0]);
}

static void initialize_result(search_result *result, int city_count)
{
    memset(result, 0, sizeof(*result));
    result->city_count = city_count;
    result->cost = DBL_MAX;
}

static void consider_candidate(search_result *result, const node *candidate)
{
    if (candidate->cost < result->cost) {
        result->cost = candidate->cost;
        result->city_count = candidate->city_count;
        memcpy(result->tour,
               candidate->tour,
               (size_t)candidate->city_count * sizeof(candidate->tour[0]));
    }
}

static node make_candidate(const double cities[MAXNUM][MAXNUM],
                           const int tour[MAXNUM],
                           int city_count)
{
    node candidate = { .tour = { 0 }, .city_count = city_count, .cost = 0.0 };
    memcpy(candidate.tour,
           tour,
           (size_t)city_count * sizeof(candidate.tour[0]));
    candidate.cost = tour_cost(cities, candidate.tour, city_count);
    return candidate;
}

double tour_cost(const double cities[MAXNUM][MAXNUM],
                 const int tour[MAXNUM],
                 int city_count)
{
    if (cities == NULL || tour == NULL ||
        city_count < 2 || city_count > MAXNUM ||
        !tour_is_valid(tour, city_count)) {
        return DBL_MAX;
    }

    double total = 0.0;
    for (int i = 0; i < city_count; ++i) {
        const int next = (i + 1) % city_count;
        total += cities[tour[i]][tour[next]];
    }
    return total;
}

bool tour_is_valid(const int tour[MAXNUM], int city_count)
{
    if (tour == NULL || city_count < 2 || city_count > MAXNUM) {
        return false;
    }

    bool seen[MAXNUM] = { false };
    for (int i = 0; i < city_count; ++i) {
        const int city = tour[i];
        if (city < 0 || city >= city_count || seen[city]) {
            return false;
        }
        seen[city] = true;
    }
    return true;
}

static bool factorial_recursive_checked(int number, uint64_t *result)
{
    if (number <= 1) {
        *result = UINT64_C(1);
        return true;
    }

    uint64_t previous = 0U;
    if (!factorial_recursive_checked(number - 1, &previous)) {
        return false;
    }

    const uint64_t unsigned_number = (uint64_t)number;
    if (previous > UINT64_MAX / unsigned_number) {
        return false;
    }
    *result = previous * unsigned_number;
    return true;
}

bool factorial_checked(int number, uint64_t *result)
{
    if (result == NULL || number < 0) {
        return false;
    }

    /* Retain the original recursive factorial exercise, with overflow checks. */
    return factorial_recursive_checked(number, result);
}

bool next_permutation(int tour[MAXNUM], int city_count)
{
    if (tour == NULL || city_count < 2 || city_count > MAXNUM) {
        return false;
    }

    /* This is the original lexicographic algorithm, restricted to indices
       1..city_count-1 so city 0 remains the anchor. */
    int pivot = city_count - 2;
    while (pivot >= 1 && tour[pivot] >= tour[pivot + 1]) {
        --pivot;
    }
    if (pivot < 1) {
        return false;
    }

    int successor = city_count - 1;
    while (tour[pivot] >= tour[successor]) {
        --successor;
    }
    swap_positions(tour, pivot, successor);

    int left_index = pivot + 1;
    int right_index = city_count - 1;
    while (left_index < right_index) {
        swap_positions(tour, left_index, right_index);
        ++left_index;
        --right_index;
    }
    return true;
}

void random_state_init(random_state *state, uint32_t seed)
{
    if (state != NULL) {
        state->value = seed;
    }
}

void mutate_tour(int tour[MAXNUM],
                 int city_count,
                 int swap_count,
                 random_state *state)
{
    if (tour == NULL || state == NULL || city_count < 3 ||
        city_count > MAXNUM || swap_count < 1) {
        return;
    }

    for (int mutation = 0; mutation < swap_count; ++mutation) {
        const int first = 1 + random_bounded(state, city_count - 1);
        int second = 1 + random_bounded(state, city_count - 1);
        while (second == first) {
            second = 1 + random_bounded(state, city_count - 1);
        }
        swap_positions(tour, first, second);
    }
}

bool exhaustive_search(const double cities[MAXNUM][MAXNUM],
                       int city_count,
                       bool verbose,
                       search_result *result)
{
    if (cities == NULL || result == NULL ||
        city_count < 2 || city_count > MAX_EXACT_CITIES) {
        return false;
    }

    uint64_t permutation_count = 0U;
    if (!factorial_checked(city_count - 1, &permutation_count)) {
        return false;
    }

    initialize_result(result, city_count);
    int tour[MAXNUM] = { 0 };
    initialize_identity_tour(tour, city_count);

    for (uint64_t index = 0U; index < permutation_count; ++index) {
        const node candidate = make_candidate(cities, tour, city_count);
        consider_candidate(result, &candidate);
        ++result->evaluated_tours;

        if (verbose) {
            printf("Exact candidate %" PRIu64 ": ", index + UINT64_C(1));
            print_tour(candidate.tour, city_count);
            printf("  cost=%.6f\n", candidate.cost);
        }

        if (index + UINT64_C(1) < permutation_count &&
            !next_permutation(tour, city_count)) {
            return false;
        }
    }
    return true;
}

bool evolutionary_search(const double cities[MAXNUM][MAXNUM],
                         int city_count,
                         const evolution_config *config,
                         search_result *result)
{
    if (cities == NULL || config == NULL || result == NULL ||
        city_count < 3 || city_count > MAXNUM ||
        config->population_size < 2 ||
        config->population_size > MAX_POPULATION ||
        config->generations < 0 ||
        config->elite_count < 1 ||
        config->elite_count > config->population_size ||
        config->mutation_swaps < 1 ||
        config->mutation_swaps >= city_count) {
        return false;
    }

    initialize_result(result, city_count);
    random_state random;
    random_state_init(&random, config->seed);

    min_heap current_population;
    heap_init(&current_population, (size_t)config->population_size);

    for (int member = 0; member < config->population_size; ++member) {
        int tour[MAXNUM] = { 0 };
        initialize_identity_tour(tour, city_count);
        shuffle_tour_tail(tour, city_count, &random);

        const node candidate = make_candidate(cities, tour, city_count);
        if (!heap_insert(&current_population, &candidate)) {
            return false;
        }
        consider_candidate(result, &candidate);
        ++result->evaluated_tours;
    }

    if (config->verbose) {
        const node *initial_best = heap_peek_min(&current_population);
        printf("Generation 0 best: %.6f\n", initial_best->cost);
    }

    for (int generation = 1; generation <= config->generations; ++generation) {
        node ranked[MAX_POPULATION];
        for (int member = 0; member < config->population_size; ++member) {
            if (!heap_delete_min(&current_population, &ranked[member])) {
                return false;
            }
        }

        min_heap next_population;
        heap_init(&next_population, (size_t)config->population_size);

        /* Elites retain both their route and the cost already calculated for it. */
        for (int elite = 0; elite < config->elite_count; ++elite) {
            if (!heap_insert(&next_population, &ranked[elite])) {
                return false;
            }
        }

        int parent_pool = config->population_size / 2;
        if (parent_pool < config->elite_count) {
            parent_pool = config->elite_count;
        }

        for (int member = config->elite_count;
             member < config->population_size;
             ++member) {
            const int parent_index = random_bounded(&random, parent_pool);
            node child = ranked[parent_index];

            mutate_tour(child.tour,
                        city_count,
                        config->mutation_swaps,
                        &random);
            /* Mutation changes the route, so its old fitness is discarded. */
            child.cost = tour_cost(cities, child.tour, city_count);

            if (!tour_is_valid(child.tour, city_count) ||
                !heap_insert(&next_population, &child)) {
                return false;
            }
            consider_candidate(result, &child);
            ++result->evaluated_tours;
        }

        current_population = next_population;
        if (config->verbose) {
            const node *generation_best = heap_peek_min(&current_population);
            printf("Generation %d best: %.6f\n",
                   generation,
                   generation_best->cost);
        }
    }

    return current_population.size == (size_t)config->population_size &&
           tour_is_valid(result->tour, result->city_count) &&
           result->cost == tour_cost(cities, result->tour, result->city_count);
}
