#ifndef TSP_DESKTOP_SOLVER_TYPES_HPP
#define TSP_DESKTOP_SOLVER_TYPES_HPP

#include <tsp/tsp.h>

#include <chrono>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace tsp::desktop {

struct RunSettings {
    int city_count = 8;
    int population_size = 12;
    int generations = 50;
    int elite_count = 3;
    int mutation_swaps = 1;
    std::uint32_t seed = 12345U;
};

struct SolverResult {
    tsp_algorithm algorithm = TSP_ALGORITHM_BRUTE_FORCE;
    std::vector<int> route;
    double cost = 0.0;
    std::uint64_t candidates_evaluated = 0U;
    bool guaranteed_optimal = false;
    std::chrono::duration<double> elapsed{};
    std::vector<double> convergence;
};

struct AdapterError {
    tsp_status status = TSP_STATUS_INTERNAL_ERROR;
    std::string status_text;
    std::string operation;
};

template <typename T>
using AdapterResult = std::variant<T, AdapterError>;

} // namespace tsp::desktop

#endif
