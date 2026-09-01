/***************************************************************
 Original tester/driver, retained as the command-line entry point.
***************************************************************/

#include "function.h"
#include "graph.h"

#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

typedef struct program_config {
    int city_count;
    int generations;
    int population_size;
    int elite_percent;
    int mutation_swaps;
    uint32_t seed;
    const char *data_path;
    bool verbose;
    bool print_graph;
} program_config;

static void print_usage(const char *program)
{
    printf("Usage: %s [options]\n", program);
    puts("  --cities N          Cities used by both solvers (3-12)");
    puts("  --generations N     Evolutionary generations (0-100000)");
    puts("  --population N      Population size (2-12)");
    puts("  --elite-percent N   Population retained unchanged (1-100)");
    puts("  --mutation-swaps N  City swaps per generated child (1-N-1)");
    puts("  --seed N            Deterministic 32-bit random seed");
    puts("  --data PATH         Off-diagonal 20-city distance data");
    puts("  --verbose           Print exact candidates and generation progress");
    puts("  --print-graph       Print the selected adjacency matrix");
    puts("  --help              Show this message");
}

static bool parse_int(const char *text, int minimum, int maximum, int *result)
{
    if (text == NULL || result == NULL) {
        return false;
    }

    errno = 0;
    char *end = NULL;
    const long parsed = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' ||
        parsed < minimum || parsed > maximum) {
        return false;
    }

    *result = (int)parsed;
    return true;
}

static bool parse_seed(const char *text, uint32_t *result)
{
    if (text == NULL || result == NULL || text[0] == '-') {
        return false;
    }

    errno = 0;
    char *end = NULL;
    const unsigned long parsed = strtoul(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || parsed > UINT32_MAX) {
        return false;
    }

    *result = (uint32_t)parsed;
    return true;
}

static bool parse_arguments(int argc, char *argv[], program_config *config)
{
    for (int index = 1; index < argc; ++index) {
        const char *option = argv[index];
        if (strcmp(option, "--help") == 0) {
            print_usage(argv[0]);
            exit(EXIT_SUCCESS);
        }
        if (strcmp(option, "--verbose") == 0) {
            config->verbose = true;
            continue;
        }
        if (strcmp(option, "--print-graph") == 0) {
            config->print_graph = true;
            continue;
        }
        if (index + 1 >= argc) {
            fprintf(stderr, "Error: %s requires a value\n", option);
            return false;
        }

        const char *value = argv[++index];
        if (strcmp(option, "--cities") == 0) {
            if (!parse_int(value, 3, MAX_EXACT_CITIES, &config->city_count)) {
                fprintf(stderr, "Error: --cities must be between 3 and %d\n",
                        MAX_EXACT_CITIES);
                return false;
            }
        } else if (strcmp(option, "--generations") == 0) {
            if (!parse_int(value, 0, 100000, &config->generations)) {
                fputs("Error: --generations must be between 0 and 100000\n",
                      stderr);
                return false;
            }
        } else if (strcmp(option, "--population") == 0) {
            if (!parse_int(value, 2, MAX_POPULATION, &config->population_size)) {
                fprintf(stderr, "Error: --population must be between 2 and %d\n",
                        MAX_POPULATION);
                return false;
            }
        } else if (strcmp(option, "--elite-percent") == 0) {
            if (!parse_int(value, 1, 100, &config->elite_percent)) {
                fputs("Error: --elite-percent must be between 1 and 100\n",
                      stderr);
                return false;
            }
        } else if (strcmp(option, "--mutation-swaps") == 0) {
            if (!parse_int(value, 1, MAXNUM - 1, &config->mutation_swaps)) {
                fprintf(stderr,
                        "Error: --mutation-swaps must be between 1 and %d\n",
                        MAXNUM - 1);
                return false;
            }
        } else if (strcmp(option, "--seed") == 0) {
            if (!parse_seed(value, &config->seed)) {
                fputs("Error: --seed must be a 32-bit unsigned integer\n", stderr);
                return false;
            }
        } else if (strcmp(option, "--data") == 0) {
            config->data_path = value;
        } else {
            fprintf(stderr, "Error: unknown option %s\n", option);
            return false;
        }
    }

    if (config->mutation_swaps >= config->city_count) {
        fputs("Error: --mutation-swaps must be less than --cities\n", stderr);
        return false;
    }
    return true;
}

static double current_time_seconds(void)
{
    struct timeval time_value;
    if (gettimeofday(&time_value, NULL) != 0) {
        return 0.0;
    }
    return (double)time_value.tv_sec + ((double)time_value.tv_usec / 1000000.0);
}

static void print_result(const search_result *result)
{
    printf("  Cost: %.6f\n  Tour: ", result->cost);
    for (int index = 0; index < result->city_count; ++index) {
        printf("%d -> ", result->tour[index]);
    }
    printf("%d\n  Evaluated tours: %" PRIu64 "\n",
           result->tour[0],
           result->evaluated_tours);
}

int main(int argc, char *argv[])
{
    program_config config = {
        .city_count = 8,
        .generations = 50,
        .population_size = 12,
        .elite_percent = 25,
        .mutation_swaps = 1,
        .seed = UINT32_C(12345),
        .data_path = "cities.dat",
        .verbose = false,
        .print_graph = false
    };

    if (!parse_arguments(argc, argv, &config)) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    double cities[MAXNUM][MAXNUM];
    if (!graph_load(config.data_path, cities)) {
        return EXIT_FAILURE;
    }
    if (config.print_graph) {
        graph_print(cities, config.city_count);
    }

    search_result exact_result;
    const double exact_start = current_time_seconds();
    if (!exhaustive_search(cities,
                           config.city_count,
                           config.verbose,
                           &exact_result)) {
        fputs("Error: exhaustive search failed\n", stderr);
        return EXIT_FAILURE;
    }
    const double exact_elapsed = current_time_seconds() - exact_start;

    int elite_count = (config.population_size * config.elite_percent) / 100;
    if (elite_count < 1) {
        elite_count = 1;
    }

    const evolution_config evolution = {
        .population_size = config.population_size,
        .generations = config.generations,
        .elite_count = elite_count,
        .mutation_swaps = config.mutation_swaps,
        .seed = config.seed,
        .verbose = config.verbose
    };

    search_result heuristic_result;
    const double heuristic_start = current_time_seconds();
    if (!evolutionary_search(cities,
                             config.city_count,
                             &evolution,
                             &heuristic_result)) {
        fputs("Error: evolutionary search failed\n", stderr);
        return EXIT_FAILURE;
    }
    const double heuristic_elapsed = current_time_seconds() - heuristic_start;

    puts("Traveling Salesman Problem comparison");
    printf("Cities: %d | Population: %d | Generations: %d | Seed: %" PRIu32 "\n\n",
           config.city_count,
           config.population_size,
           config.generations,
           config.seed);

    puts("Exact exhaustive search");
    print_result(&exact_result);
    printf("  Elapsed: %.6f seconds\n\n", exact_elapsed);

    puts("Mutation-and-elitism evolutionary search");
    print_result(&heuristic_result);
    printf("  Elapsed: %.6f seconds\n", heuristic_elapsed);

    const double difference = heuristic_result.cost - exact_result.cost;
    if (fabs(difference) <= 1.0e-9) {
        puts("\nComparison: the heuristic matched the exact optimum for this run.");
    } else {
        printf("\nComparison: heuristic gap = %.6f (%.2f%% above optimum).\n",
               difference,
               (difference / exact_result.cost) * 100.0);
    }

    return EXIT_SUCCESS;
}
