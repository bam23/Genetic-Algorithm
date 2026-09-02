#include <tsp/tsp.h>

#include "heap.h"
#include "solver_internal.h"

#include <float.h>
#include <string.h>

static uint32_t random_next(tsp_random_state *state)
{
    /* Defined-width arithmetic makes fixed seeds portable and repeatable. */
    state->value = (state->value * UINT32_C(1664525)) + UINT32_C(1013904223);
    return state->value;
}

static int random_bounded(tsp_random_state *state, int upper_bound)
{
    if (state == NULL || upper_bound <= 0) {
        return 0;
    }
    return (int)(random_next(state) % (uint32_t)upper_bound);
}

static void swap_positions(int tour[TSP_MAX_CITIES], int first, int second)
{
    const int temporary = tour[first];
    tour[first] = tour[second];
    tour[second] = temporary;
}

static void initialize_identity_tour(int tour[TSP_MAX_CITIES], int city_count)
{
    for (int i = 0; i < city_count; ++i) {
        tour[i] = i;
    }
    for (int i = city_count; i < TSP_MAX_CITIES; ++i) {
        tour[i] = 0;
    }
}

static void shuffle_tour_tail(int tour[TSP_MAX_CITIES],
                              int city_count,
                              tsp_random_state *state)
{
    /* Keep city 0 anchored; rotating a closed tour does not create a new tour. */
    for (int i = city_count - 1; i > 1; --i) {
        const int other = 1 + random_bounded(state, i);
        swap_positions(tour, i, other);
    }
}

static void initialize_result(tsp_result *result,
                              int city_count,
                              tsp_algorithm algorithm,
                              bool guaranteed_optimal)
{
    memset(result, 0, sizeof(*result));
    result->city_count = city_count;
    result->cost = DBL_MAX;
    result->algorithm = algorithm;
    result->guaranteed_optimal = guaranteed_optimal;
}

static void consider_candidate(tsp_result *result,
                               const tsp_candidate *candidate)
{
    if (candidate->cost < result->cost) {
        result->cost = candidate->cost;
        result->city_count = candidate->city_count;
        memcpy(result->tour,
               candidate->tour,
               (size_t)candidate->city_count * sizeof(candidate->tour[0]));
    }
}

static tsp_candidate make_candidate(const tsp_graph *graph,
                                    const int tour[TSP_MAX_CITIES],
                                    int city_count)
{
    tsp_candidate candidate = {
        .tour = { 0 },
        .city_count = city_count,
        .cost = 0.0
    };
    memcpy(candidate.tour,
           tour,
           (size_t)city_count * sizeof(candidate.tour[0]));
    candidate.cost = tsp_tour_cost(graph, candidate.tour, city_count);
    return candidate;
}

double tsp_tour_cost(const tsp_graph *graph,
                     const int tour[TSP_MAX_CITIES],
                     int city_count)
{
    if (graph == NULL || tour == NULL ||
        city_count < 2 || city_count > TSP_MAX_CITIES ||
        city_count > graph->city_count ||
        !tsp_tour_is_valid(tour, city_count)) {
        return DBL_MAX;
    }

    double total = 0.0;
    for (int i = 0; i < city_count; ++i) {
        const int next = (i + 1) % city_count;
        total += graph->costs[tour[i]][tour[next]];
    }
    return total;
}

bool tsp_tour_is_valid(const int tour[TSP_MAX_CITIES], int city_count)
{
    if (tour == NULL || city_count < 2 || city_count > TSP_MAX_CITIES) {
        return false;
    }

    bool seen[TSP_MAX_CITIES] = { false };
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

bool tsp_factorial_checked(int number, uint64_t *result)
{
    if (result == NULL || number < 0) {
        return false;
    }

    /* Retain the original recursive factorial exercise, with overflow checks. */
    return factorial_recursive_checked(number, result);
}

bool tsp_next_permutation(int tour[TSP_MAX_CITIES], int city_count)
{
    if (tour == NULL || city_count < 2 || city_count > TSP_MAX_CITIES) {
        return false;
    }

    /* Preserve the original lexicographic algorithm and city 0 anchor. */
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

void tsp_random_state_init(tsp_random_state *state, uint32_t seed)
{
    if (state != NULL) {
        state->value = seed;
    }
}

void tsp_mutate_tour(int tour[TSP_MAX_CITIES],
                     int city_count,
                     int swap_count,
                     tsp_random_state *state)
{
    if (tour == NULL || state == NULL || city_count < 3 ||
        city_count > TSP_MAX_CITIES || swap_count < 1) {
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

tsp_status tsp_solve_bruteforce(const tsp_graph *graph,
                                const tsp_bruteforce_config *config,
                                tsp_result *out_result)
{
    if (graph == NULL || config == NULL || out_result == NULL ||
        config->city_count < 2) {
        return TSP_STATUS_INVALID_ARGUMENT;
    }
    if (config->city_count > TSP_MAX_BRUTEFORCE_CITIES ||
        config->city_count > graph->city_count) {
        return TSP_STATUS_LIMIT_EXCEEDED;
    }

    uint64_t permutation_count = 0U;
    if (!tsp_factorial_checked(config->city_count - 1, &permutation_count)) {
        return TSP_STATUS_ARITHMETIC_OVERFLOW;
    }

    initialize_result(out_result,
                      config->city_count,
                      TSP_ALGORITHM_BRUTE_FORCE,
                      true);
    int tour[TSP_MAX_CITIES] = { 0 };
    initialize_identity_tour(tour, config->city_count);

    for (uint64_t index = 0U; index < permutation_count; ++index) {
        const tsp_candidate candidate =
            make_candidate(graph, tour, config->city_count);
        consider_candidate(out_result, &candidate);
        ++out_result->candidates_evaluated;

        if (index + UINT64_C(1) < permutation_count &&
            !tsp_next_permutation(tour, config->city_count)) {
            return TSP_STATUS_INTERNAL_ERROR;
        }
    }
    return TSP_STATUS_OK;
}

tsp_status tsp_solve_evolutionary(const tsp_graph *graph,
                                  const tsp_evolution_config *config,
                                  tsp_convergence_history *history,
                                  tsp_result *out_result)
{
    if (history != NULL) {
        history->count = 0U;
    }
    if (graph == NULL || config == NULL || out_result == NULL ||
        config->city_count < 3 ||
        config->population_size < 2 ||
        config->generations < 0 ||
        config->elite_count < 1 ||
        config->elite_count > config->population_size ||
        config->mutation_swaps < 1 ||
        config->mutation_swaps >= config->city_count) {
        return TSP_STATUS_INVALID_ARGUMENT;
    }
    if (config->city_count > TSP_MAX_CITIES ||
        config->city_count > graph->city_count ||
        config->population_size > TSP_MAX_POPULATION) {
        return TSP_STATUS_LIMIT_EXCEEDED;
    }

    const size_t required_history =
        (size_t)config->generations + (size_t)1U;
    if (history != NULL) {
        if (history->best_costs == NULL) {
            return TSP_STATUS_INVALID_ARGUMENT;
        }
        if (history->capacity < required_history) {
            return TSP_STATUS_BUFFER_TOO_SMALL;
        }
    }

    initialize_result(out_result,
                      config->city_count,
                      TSP_ALGORITHM_EVOLUTIONARY,
                      false);
    tsp_random_state random;
    tsp_random_state_init(&random, config->seed);

    tsp_min_heap current_population;
    tsp_heap_init(&current_population, (size_t)config->population_size);

    for (int member = 0; member < config->population_size; ++member) {
        int tour[TSP_MAX_CITIES] = { 0 };
        initialize_identity_tour(tour, config->city_count);
        shuffle_tour_tail(tour, config->city_count, &random);

        const tsp_candidate candidate =
            make_candidate(graph, tour, config->city_count);
        if (!tsp_heap_insert(&current_population, &candidate)) {
            return TSP_STATUS_INTERNAL_ERROR;
        }
        consider_candidate(out_result, &candidate);
        ++out_result->candidates_evaluated;
    }

    if (history != NULL) {
        history->best_costs[0] = out_result->cost;
    }

    for (int generation = 1;
         generation <= config->generations;
         ++generation) {
        tsp_candidate ranked[TSP_MAX_POPULATION];
        for (int member = 0; member < config->population_size; ++member) {
            if (!tsp_heap_delete_min(&current_population, &ranked[member])) {
                return TSP_STATUS_INTERNAL_ERROR;
            }
        }

        tsp_min_heap next_population;
        tsp_heap_init(&next_population, (size_t)config->population_size);

        /* Elites retain both their route and its previously calculated cost. */
        for (int elite = 0; elite < config->elite_count; ++elite) {
            if (!tsp_heap_insert(&next_population, &ranked[elite])) {
                return TSP_STATUS_INTERNAL_ERROR;
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
            tsp_candidate child = ranked[parent_index];

            tsp_mutate_tour(child.tour,
                            config->city_count,
                            config->mutation_swaps,
                            &random);
            /* Mutation changes the route, so its old fitness is discarded. */
            child.cost =
                tsp_tour_cost(graph, child.tour, config->city_count);

            if (!tsp_tour_is_valid(child.tour, config->city_count) ||
                !tsp_heap_insert(&next_population, &child)) {
                return TSP_STATUS_INTERNAL_ERROR;
            }
            consider_candidate(out_result, &child);
            ++out_result->candidates_evaluated;
        }

        current_population = next_population;
        if (history != NULL) {
            history->best_costs[(size_t)generation] = out_result->cost;
        }
    }

    if (current_population.size != (size_t)config->population_size ||
        !tsp_tour_is_valid(out_result->tour, out_result->city_count) ||
        out_result->cost !=
            tsp_tour_cost(graph, out_result->tour, out_result->city_count)) {
        return TSP_STATUS_INTERNAL_ERROR;
    }

    if (history != NULL) {
        history->count = required_history;
    }
    return TSP_STATUS_OK;
}
