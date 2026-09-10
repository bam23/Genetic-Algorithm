#ifndef TSP_DESKTOP_MAIN_WINDOW_HPP
#define TSP_DESKTOP_MAIN_WINDOW_HPP

#include "solver_adapter.hpp"

#include <QMainWindow>

#include <optional>

QT_BEGIN_NAMESPACE
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QWidget;
QT_END_NAMESPACE

namespace tsp::desktop {

class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void build_interface();
    void update_dependent_limits();
    void update_exact_policy();
    void update_run_availability();
    void load_graph_data();
    void show_phase_d_placeholder();

    std::optional<Graph> graph_;
    QSpinBox *city_count_ = nullptr;
    QSpinBox *population_size_ = nullptr;
    QSpinBox *generations_ = nullptr;
    QSpinBox *elite_count_ = nullptr;
    QSpinBox *mutation_swaps_ = nullptr;
    QDoubleSpinBox *seed_ = nullptr;
    QLabel *graph_status_ = nullptr;
    QLabel *candidate_count_ = nullptr;
    QLabel *exact_warning_ = nullptr;
    QPushButton *run_exact_ = nullptr;
    QPushButton *run_evolutionary_ = nullptr;
    QPushButton *run_comparison_ = nullptr;
};

} // namespace tsp::desktop

#endif
