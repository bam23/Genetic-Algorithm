#ifndef TSP_DESKTOP_MAIN_WINDOW_HPP
#define TSP_DESKTOP_MAIN_WINDOW_HPP

#include "solver_worker.hpp"

#include <QMainWindow>
#include <QThread>

#include <cstdint>
#include <optional>

QT_BEGIN_NAMESPACE
class QCloseEvent;
class QDoubleSpinBox;
class QGroupBox;
class QLabel;
class QProgressBar;
class QPushButton;
class QSpinBox;
class QString;
class QWidget;
QT_END_NAMESPACE

namespace tsp::desktop {

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

signals:
    void job_requested(SolverJobRequest request);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    enum class WorkflowState {
        Idle,
        ExactRunning,
        EvolutionaryRunning,
        ComparisonExactRunning,
        ComparisonEvolutionaryRunning
    };

    struct ComparisonContext {
        Graph graph;
        RunSettings settings;
    };

    void build_interface();
    void start_worker();
    void update_dependent_limits();
    void update_exact_policy();
    void update_run_availability();
    void load_graph_data();
    void invalidate_results();
    void clear_results();
    void run_exact();
    void run_evolutionary();
    void run_comparison();
    void submit_job(SolverOperation operation,
                    const Graph &graph,
                    const RunSettings &settings);
    void handle_job_completed(SolverJobCompletion completion);
    void finish_workflow();
    void show_result(QLabel *label, const SolverResult &result);
    void show_error(QLabel *label, const AdapterError &error);
    void show_comparison_summary();
    void set_busy_message(const QString &message);
    RunSettings settings_snapshot() const;
    bool confirm_expensive_exact(bool comparison);
    bool is_busy() const noexcept;

    std::optional<Graph> graph_;
    std::optional<SolverResult> exact_result_data_;
    std::optional<SolverResult> evolutionary_result_data_;
    std::optional<ComparisonContext> comparison_;
    SolverWorker *worker_ = nullptr;
    QThread worker_thread_;
    WorkflowState workflow_state_ = WorkflowState::Idle;
    std::uint64_t next_job_id_ = 1U;
    std::uint64_t active_job_id_ = 0U;
    bool pending_close_ = false;
    QGroupBox *configuration_group_ = nullptr;
    QSpinBox *city_count_ = nullptr;
    QSpinBox *population_size_ = nullptr;
    QSpinBox *generations_ = nullptr;
    QSpinBox *elite_count_ = nullptr;
    QSpinBox *mutation_swaps_ = nullptr;
    QDoubleSpinBox *seed_ = nullptr;
    QLabel *graph_status_ = nullptr;
    QLabel *candidate_count_ = nullptr;
    QLabel *exact_warning_ = nullptr;
    QLabel *busy_label_ = nullptr;
    QProgressBar *progress_bar_ = nullptr;
    QLabel *exact_result_label_ = nullptr;
    QLabel *evolutionary_result_label_ = nullptr;
    QLabel *comparison_result_label_ = nullptr;
    QPushButton *run_exact_ = nullptr;
    QPushButton *run_evolutionary_ = nullptr;
    QPushButton *run_comparison_ = nullptr;
};

} // namespace tsp::desktop

#endif
