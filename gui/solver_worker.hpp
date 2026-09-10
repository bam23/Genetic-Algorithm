#ifndef TSP_DESKTOP_SOLVER_WORKER_HPP
#define TSP_DESKTOP_SOLVER_WORKER_HPP

#include "solver_adapter.hpp"

#include <QMetaType>
#include <QObject>

#include <cstdint>

namespace tsp::desktop {

enum class SolverOperation {
    Exact,
    Evolutionary
};

struct SolverJobRequest {
    std::uint64_t job_id;
    SolverOperation operation;
    Graph graph;
    RunSettings settings;
};

struct SolverJobCompletion {
    std::uint64_t job_id;
    SolverOperation operation;
    AdapterResult<SolverResult> outcome;
};

class SolverWorker final : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;

public slots:
    void execute(SolverJobRequest request);

signals:
    void completed(SolverJobCompletion completion);
};

} // namespace tsp::desktop

Q_DECLARE_METATYPE(tsp::desktop::SolverJobRequest)
Q_DECLARE_METATYPE(tsp::desktop::SolverJobCompletion)

#endif
