/***************************************************************
 Original tester/driver, retained as the command-line entry point.
***************************************************************/

#include <tsp/tsp.h>

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
            if (!parse_int(value,
                           3,
                           TSP_MAX_BRUTEFORCE_CITIES,
                           &config->city_count)) {
                fprintf(stderr,
                        "Error: --cities must be between 3 and %d\n",
                        TSP_MAX_BRUTEFORCE_CITIES);
                return false;
            }
        } else if (strcmp(option, "--generations") == 0) {
            if (!parse_int(value, 0, 100000, &config->generations)) {
                fputs("Error: --generations must be between 0 and 100000\n",
                      stderr);
                return false;
            }
        } else if (strcmp(option, "--population") == 0) {
            if (!parse_int(value,
                           2,
                           TSP_MAX_POPULATION,
                           &config->population_size)) {
                fprintf(stderr,
                        "Error: --population must be between 2 and %d\n",
                        TSP_MAX_POPULATION);
                return false;
            }
        } else if (strcmp(option, "--elite-percent") == 0) {
            if (!parse_int(value, 1, 100, &config->elite_percent)) {
                fputs("Error: --elite-percent must be between 1 and 100\n",
                      stderr);
                return false;
            }
        } else if (strcmp(option, "--mutation-swaps") == 0) {
            if (!parse_int(value,
                           1,
                           TSP_MAX_CITIES - 1,
                           &config->mutation_swaps)) {
                fprintf(stderr,
                        "Error: --mutation-swaps must be between 1 and %d\n",
                        TSP_MAX_CITIES - 1);
                return false;
            }
        } else if (strcmp(option, "--seed") == 0) {
            if (!parse_seed(value, &config->seed)) {
                fputs("Error: --seed must be a 32-bit unsigned integer\n",
                      stderr);
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

static void print_graph(const tsp_graph *graph, int city_count)
{
    for (int row = 0; row < city_count; ++row) {
        for (int column = 0; column < city_count; ++column) {
            printf("%8.2f ", graph->costs[row][column]);
        }
        putchar('\n');
    }
}

static void print_result(const tsp_result *result)
{
    printf("  Cost: %.6f\n  Tour: ", result->cost);
    for (int index = 0; index < result->city_count; ++index) {
        printf("%d -> ", result->tour[index]);
    }
    printf("%d\n  Evaluated tours: %" PRIu64 "\n",
           result->tour[0],
           result->candidates_evaluated);
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
        .print_graph = false
    };

    if (!parse_arguments(argc, argv, &config)) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    tsp_graph graph;
    tsp_status status = tsp_graph_load(&graph, config.data_path);
    if (status != TSP_STATUS_OK) {
        fprintf(stderr,
                "Error: could not load %s: %s\n",
                config.data_path,
                tsp_status_string(status));
        return EXIT_FAILURE;
    }
    if (config.print_graph) {
        print_graph(&graph, config.city_count);
    }

    const tsp_bruteforce_config brute_force = {
        .city_count = config.city_count
    };
    tsp_result brute_force_result;
    const double brute_force_start = current_time_seconds();
    status = tsp_solve_bruteforce(&graph,
                                  &brute_force,
                                  &brute_force_result);
    if (status != TSP_STATUS_OK) {
        fprintf(stderr,
                "Error: brute-force search failed: %s\n",
                tsp_status_string(status));
        return EXIT_FAILURE;
    }
    const double brute_force_elapsed =
        current_time_seconds() - brute_force_start;

    int elite_count = (config.population_size * config.elite_percent) / 100;
    if (elite_count < 1) {
        elite_count = 1;
    }

    const tsp_evolution_config evolution = {
        .city_count = config.city_count,
        .population_size = config.population_size,
        .generations = config.generations,
        .elite_count = elite_count,
        .mutation_swaps = config.mutation_swaps,
        .seed = config.seed
    };

    tsp_result evolutionary_result;
    const double evolutionary_start = current_time_seconds();
    status = tsp_solve_evolutionary(&graph,
                                    &evolution,
                                    &evolutionary_result);
    if (status != TSP_STATUS_OK) {
        fprintf(stderr,
                "Error: evolutionary search failed: %s\n",
                tsp_status_string(status));
        return EXIT_FAILURE;
    }
    const double evolutionary_elapsed =
        current_time_seconds() - evolutionary_start;

    puts("Traveling Salesman Problem comparison");
    printf("Cities: %d | Population: %d | Generations: %d | Seed: %" PRIu32
           "\n\n",
           config.city_count,
           config.population_size,
           config.generations,
           config.seed);

    puts("Brute-force exhaustive search");
    print_result(&brute_force_result);
    printf("  Elapsed: %.6f seconds\n\n", brute_force_elapsed);

    puts("Mutation-and-elitism evolutionary search");
    print_result(&evolutionary_result);
    printf("  Elapsed: %.6f seconds\n", evolutionary_elapsed);

    const double difference =
        evolutionary_result.cost - brute_force_result.cost;
    if (fabs(difference) <= 1.0e-9) {
        puts("\nComparison: the heuristic matched the brute-force optimum.");
    } else {
        printf("\nComparison: heuristic gap = %.6f (%.2f%% above optimum).\n",
               difference,
               (difference / brute_force_result.cost) * 100.0);
    }

    return EXIT_SUCCESS;
}
