#include "solver_adapter.hpp"

#include <tsp/tsp.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace {

using tsp::desktop::AdapterError;
using tsp::desktop::AdapterResult;
using tsp::desktop::Graph;
using tsp::desktop::RunSettings;
using tsp::desktop::SolverResult;

int tests_run = 0;
int tests_failed = 0;

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            std::cerr << "  assertion failed at " << __FILE__ << ':'         \
                      << __LINE__ << ": " << #condition << '\n';             \
            return false;                                                      \
        }                                                                      \
    } while (false)

static_assert(noexcept(tsp::desktop::load_graph(
    std::declval<const std::string &>())));
static_assert(noexcept(tsp::desktop::solve_exact(
    std::declval<const Graph &>(), std::declval<const RunSettings &>())));
static_assert(noexcept(tsp::desktop::solve_evolutionary(
    std::declval<const Graph &>(), std::declval<const RunSettings &>())));
static_assert(std::is_same_v<decltype(AdapterError::status_text), std::string>);

bool nearly_equal(double first, double second)
{
    return std::abs(first - second) <= 1.0e-9;
}

template <typename T>
const T *value(const AdapterResult<T> &result)
{
    return std::get_if<T>(&result);
}

template <typename T>
const AdapterError *error(const AdapterResult<T> &result)
{
    return std::get_if<AdapterError>(&result);
}

RunSettings regression_settings()
{
    RunSettings settings;
    settings.city_count = 5;
    settings.population_size = 8;
    settings.generations = 10;
    settings.elite_count = 2;
    settings.mutation_swaps = 1;
    settings.seed = 12345U;
    return settings;
}

bool test_graph_loading_and_ownership()
{
    auto loaded = tsp::desktop::load_graph("cities.dat");
    const Graph *loaded_graph = value(loaded);
    CHECK(loaded_graph != nullptr);
    CHECK(loaded_graph->city_count() == TSP_MAX_CITIES);

    Graph owned_graph = *loaded_graph;
    loaded = tsp::desktop::load_graph("missing-cities.dat");
    CHECK(owned_graph.city_count() == TSP_MAX_CITIES);

    auto exact = tsp::desktop::solve_exact(owned_graph,
                                            regression_settings());
    CHECK(value(exact) != nullptr);
    return true;
}

bool test_exact_regression_and_owned_result()
{
    auto loaded = tsp::desktop::load_graph("cities.dat");
    const Graph *graph = value(loaded);
    CHECK(graph != nullptr);

    auto outcome = tsp::desktop::solve_exact(*graph, regression_settings());
    const SolverResult *result = value(outcome);
    CHECK(result != nullptr);

    const std::vector<int> expected = { 0, 3, 2, 4, 1 };
    CHECK(result->route == expected);
    CHECK(nearly_equal(result->cost, 1596.2320856626313));
    CHECK(result->candidates_evaluated == std::uint64_t{ 24 });
    CHECK(result->algorithm == TSP_ALGORITHM_BRUTE_FORCE);
    CHECK(result->guaranteed_optimal);
    CHECK(result->elapsed.count() >= 0.0);
    CHECK(result->convergence.empty());

    SolverResult owned = *result;
    outcome = tsp::desktop::solve_exact(*graph, RunSettings{});
    CHECK(owned.route == expected);
    CHECK(nearly_equal(owned.cost, 1596.2320856626313));
    return true;
}

bool test_evolutionary_regression_and_convergence()
{
    auto loaded = tsp::desktop::load_graph("cities.dat");
    const Graph *graph = value(loaded);
    CHECK(graph != nullptr);

    const RunSettings settings = regression_settings();
    auto outcome = tsp::desktop::solve_evolutionary(*graph, settings);
    const SolverResult *result = value(outcome);
    CHECK(result != nullptr);

    const std::vector<int> expected = { 0, 3, 4, 2, 1 };
    CHECK(result->route == expected);
    CHECK(nearly_equal(result->cost, 1598.0174822886754));
    CHECK(result->candidates_evaluated == std::uint64_t{ 68 });
    CHECK(result->algorithm == TSP_ALGORITHM_EVOLUTIONARY);
    CHECK(!result->guaranteed_optimal);
    CHECK(result->elapsed.count() >= 0.0);
    CHECK(result->convergence.size() ==
          static_cast<std::size_t>(settings.generations + 1));
    CHECK(std::isfinite(result->convergence.front()));
    CHECK(nearly_equal(result->convergence.back(), result->cost));
    return true;
}

bool test_zero_generation_history()
{
    auto loaded = tsp::desktop::load_graph("cities.dat");
    const Graph *graph = value(loaded);
    CHECK(graph != nullptr);

    RunSettings settings = regression_settings();
    settings.generations = 0;
    auto outcome = tsp::desktop::solve_evolutionary(*graph, settings);
    const SolverResult *result = value(outcome);
    CHECK(result != nullptr);
    CHECK(result->convergence.size() == 1U);
    CHECK(nearly_equal(result->convergence.front(), result->cost));
    CHECK(result->candidates_evaluated ==
          static_cast<std::uint64_t>(settings.population_size));
    return true;
}

bool test_owned_errors()
{
    auto missing = tsp::desktop::load_graph("missing-cities.dat");
    const AdapterError *missing_error = error(missing);
    CHECK(missing_error != nullptr);
    CHECK(missing_error->status == TSP_STATUS_IO_ERROR);
    CHECK(missing_error->status_text == "input/output error");
    CHECK(missing_error->operation == "load graph");

    const AdapterError owned_error = *missing_error;
    missing = tsp::desktop::load_graph("another-missing-file.dat");
    CHECK(owned_error.status_text == "input/output error");
    CHECK(owned_error.operation == "load graph");

    auto loaded = tsp::desktop::load_graph("cities.dat");
    const Graph *graph = value(loaded);
    CHECK(graph != nullptr);

    RunSettings invalid_exact = regression_settings();
    invalid_exact.city_count = 1;
    const auto exact = tsp::desktop::solve_exact(*graph, invalid_exact);
    const AdapterError *exact_error = error(exact);
    CHECK(exact_error != nullptr);
    CHECK(exact_error->status == TSP_STATUS_INVALID_ARGUMENT);
    CHECK(exact_error->status_text == "invalid argument");
    CHECK(exact_error->operation == "run exact solver");

    RunSettings excessive_exact = regression_settings();
    excessive_exact.city_count = TSP_MAX_BRUTEFORCE_CITIES + 1;
    const auto excessive =
        tsp::desktop::solve_exact(*graph, excessive_exact);
    const AdapterError *excessive_error = error(excessive);
    CHECK(excessive_error != nullptr);
    CHECK(excessive_error->status == TSP_STATUS_LIMIT_EXCEEDED);
    CHECK(excessive_error->status_text == "configured limit exceeded");

    RunSettings invalid_evolutionary = regression_settings();
    invalid_evolutionary.generations = -1;
    const auto evolutionary =
        tsp::desktop::solve_evolutionary(*graph, invalid_evolutionary);
    const AdapterError *evolutionary_error = error(evolutionary);
    CHECK(evolutionary_error != nullptr);
    CHECK(evolutionary_error->status == TSP_STATUS_INVALID_ARGUMENT);
    CHECK(evolutionary_error->status_text == "invalid argument");
    CHECK(evolutionary_error->operation == "run evolutionary solver");
    return true;
}

void run_test(const char *name, bool (*test_function)())
{
    ++tests_run;
    if (test_function()) {
        std::cout << "PASS " << name << '\n';
    } else {
        ++tests_failed;
        std::cout << "FAIL " << name << '\n';
    }
}

} // namespace

int main()
{
    run_test("graph loading and ownership", test_graph_loading_and_ownership);
    run_test("exact regression and owned result",
             test_exact_regression_and_owned_result);
    run_test("evolutionary regression and convergence",
             test_evolutionary_regression_and_convergence);
    run_test("zero-generation convergence history",
             test_zero_generation_history);
    run_test("owned adapter errors", test_owned_errors);

    std::cout << '\n' << tests_run << " tests run, " << tests_failed
              << " failed\n";
    return tests_failed == 0 ? 0 : 1;
}
