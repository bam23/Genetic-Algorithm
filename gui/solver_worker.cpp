#include "solver_worker.hpp"

#include <utility>

namespace tsp::desktop {

void SolverWorker::execute(SolverJobRequest request)
{
    AdapterResult<SolverResult> outcome =
        request.operation == SolverOperation::Exact
            ? solve_exact(request.graph, request.settings)
            : solve_evolutionary(request.graph, request.settings);

    emit completed(SolverJobCompletion{
        request.job_id, request.operation, std::move(outcome)
    });
}

} // namespace tsp::desktop
