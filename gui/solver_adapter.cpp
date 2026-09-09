#include "solver_adapter.hpp"

#include <chrono>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

namespace tsp::desktop {

struct GraphAccess {
    static Graph from_native(const tsp_graph &native) noexcept
    {
        Graph graph;
        graph.native_ = native;
        return graph;
    }

    static const tsp_graph *native(const Graph &graph) noexcept
    {
        return &graph.native_;
    }
};

namespace {

constexpr const char *load_graph_operation = "load graph";
constexpr const char *exact_operation = "run exact solver";
constexpr const char *evolutionary_operation = "run evolutionary solver";

static_assert(std::is_nothrow_default_constructible_v<AdapterError>);
static_assert(std::is_nothrow_move_constructible_v<AdapterError>);
static_assert(std::is_nothrow_constructible_v<AdapterResult<Graph>,
                                              AdapterError &&>);
static_assert(std::is_nothrow_constructible_v<AdapterResult<SolverResult>,
                                              AdapterError &&>);
static_assert(std::is_nothrow_move_constructible_v<AdapterResult<Graph>>);
static_assert(
    std::is_nothrow_move_constructible_v<AdapterResult<SolverResult>>);

template <typename T>
AdapterResult<T> status_error(tsp_status status, const char *operation)
{
    AdapterError error;
    error.status = status;
    error.status_text = tsp_status_string(status);
    error.operation = operation;
    return error;
}

template <typename T>
AdapterResult<T> adapter_exception(const char *operation) noexcept
{
    AdapterError error;
    error.status = TSP_STATUS_INTERNAL_ERROR;
    try {
        error.status_text = "adapter allocation or conversion failure";
        error.operation = operation;
    } catch (...) {
        // Default-constructed strings keep the failure object returnable even
        // when no further allocation is possible.
    }
    return error;
}

AdapterResult<SolverResult>
copy_result(const tsp_result &native,
            std::chrono::duration<double> elapsed,
            std::vector<double> convergence,
            const char *operation)
{
    if (native.city_count < 2 || native.city_count > TSP_MAX_CITIES) {
        return status_error<SolverResult>(TSP_STATUS_INTERNAL_ERROR,
                                          operation);
    }

    SolverResult result;
    result.algorithm = native.algorithm;
    result.route.assign(native.tour, native.tour + native.city_count);
    result.cost = native.cost;
    result.candidates_evaluated = native.candidates_evaluated;
    result.guaranteed_optimal = native.guaranteed_optimal;
    result.elapsed = elapsed;
    result.convergence = std::move(convergence);
    return result;
}

} // namespace

int Graph::city_count() const noexcept
{
    return native_.city_count;
}

AdapterResult<Graph> load_graph(const std::string &path) noexcept
{
    try {
        tsp_graph native{};
        const tsp_status status = tsp_graph_load(&native, path.c_str());
        if (status != TSP_STATUS_OK) {
            return status_error<Graph>(status, load_graph_operation);
        }
        return GraphAccess::from_native(native);
    } catch (...) {
        return adapter_exception<Graph>(load_graph_operation);
    }
}

AdapterResult<SolverResult> solve_exact(const Graph &graph,
                                        const RunSettings &settings) noexcept
{
    try {
        const tsp_bruteforce_config config = { settings.city_count };
        tsp_result native_result{};

        const auto start = std::chrono::steady_clock::now();
        const tsp_status status = tsp_solve_bruteforce(
            GraphAccess::native(graph), &config, &native_result);
        const auto elapsed = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - start);

        if (status != TSP_STATUS_OK) {
            return status_error<SolverResult>(status, exact_operation);
        }
        return copy_result(native_result, elapsed, {}, exact_operation);
    } catch (...) {
        return adapter_exception<SolverResult>(exact_operation);
    }
}

AdapterResult<SolverResult>
solve_evolutionary(const Graph &graph, const RunSettings &settings) noexcept
{
    try {
        const tsp_evolution_config config = {
            settings.city_count,
            settings.population_size,
            settings.generations,
            settings.elite_count,
            settings.mutation_swaps,
            settings.seed
        };
        tsp_result native_result{};

        if (settings.generations < 0) {
            const tsp_status status = tsp_solve_evolutionary(
                GraphAccess::native(graph), &config, nullptr, &native_result);
            if (status != TSP_STATUS_OK) {
                return status_error<SolverResult>(status,
                                                  evolutionary_operation);
            }
            return status_error<SolverResult>(TSP_STATUS_INTERNAL_ERROR,
                                              evolutionary_operation);
        }

        const std::size_t history_capacity =
            static_cast<std::size_t>(settings.generations) + 1U;
        std::vector<double> history_values(history_capacity);
        tsp_convergence_history history = {
            history_values.data(), history_values.size(), 0U
        };

        const auto start = std::chrono::steady_clock::now();
        const tsp_status status = tsp_solve_evolutionary(
            GraphAccess::native(graph), &config, &history, &native_result);
        const auto elapsed = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - start);

        if (status != TSP_STATUS_OK) {
            return status_error<SolverResult>(status, evolutionary_operation);
        }
        if (history.count != history_values.size()) {
            return status_error<SolverResult>(TSP_STATUS_INTERNAL_ERROR,
                                              evolutionary_operation);
        }
        return copy_result(native_result,
                           elapsed,
                           std::move(history_values),
                           evolutionary_operation);
    } catch (...) {
        return adapter_exception<SolverResult>(evolutionary_operation);
    }
}

} // namespace tsp::desktop
