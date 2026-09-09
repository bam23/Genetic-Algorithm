#ifndef TSP_DESKTOP_SOLVER_ADAPTER_HPP
#define TSP_DESKTOP_SOLVER_ADAPTER_HPP

#include "solver_types.hpp"

#include <string>

namespace tsp::desktop {

struct GraphAccess;

class Graph final {
public:
    Graph(const Graph &) noexcept = default;
    Graph(Graph &&) noexcept = default;
    Graph &operator=(const Graph &) noexcept = default;
    Graph &operator=(Graph &&) noexcept = default;
    ~Graph() = default;

    int city_count() const noexcept;

private:
    Graph() = default;

    tsp_graph native_{};

    friend struct GraphAccess;
};

AdapterResult<Graph> load_graph(const std::string &path) noexcept;

AdapterResult<SolverResult> solve_exact(const Graph &graph,
                                        const RunSettings &settings) noexcept;

AdapterResult<SolverResult>
solve_evolutionary(const Graph &graph, const RunSettings &settings) noexcept;

} // namespace tsp::desktop

#endif
