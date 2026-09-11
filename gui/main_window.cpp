#include "main_window.hpp"

#include "convergence_view.hpp"
#include "route_view.hpp"

#include <tsp/tsp.h>

#include <QByteArray>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QStatusBar>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <variant>

namespace tsp::desktop {
namespace {

constexpr int minimum_city_count = 3;
constexpr int maximum_generations = 100000;

constexpr std::uint64_t anchored_tour_count(int city_count) noexcept
{
    std::uint64_t count = 1U;
    for (int factor = 2; factor < city_count; ++factor) {
        count *= static_cast<std::uint64_t>(factor);
    }
    return count;
}

constexpr bool exact_is_available(int city_count) noexcept
{
    return city_count <= TSP_MAX_BRUTEFORCE_CITIES;
}

static_assert(anchored_tour_count(11) == UINT64_C(3628800));
static_assert(anchored_tour_count(12) == UINT64_C(39916800));
static_assert(anchored_tour_count(TSP_MAX_CITIES) ==
              UINT64_C(121645100408832000));
static_assert(exact_is_available(12));
static_assert(!exact_is_available(13));

QString formatted_count(std::uint64_t count)
{
    const QLocale english(QLocale::English, QLocale::UnitedStates);
    return english.toString(static_cast<qulonglong>(count));
}

QGroupBox *make_result_group(const QString &title,
                             const QString &label_name,
                             QLabel *&result_label,
                             QWidget *parent)
{
    auto *group = new QGroupBox(title, parent);
    auto *layout = new QVBoxLayout(group);
    result_label = new QLabel(MainWindow::tr("No result yet"), group);
    result_label->setObjectName(label_name);
    result_label->setAlignment(Qt::AlignCenter);
    result_label->setMinimumHeight(88);
    result_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    result_label->setWordWrap(true);
    layout->addWidget(result_label);
    return group;
}

QString formatted_route(const std::vector<int> &route)
{
    QStringList cities;
    for (const int city : route) {
        cities.append(QString::number(city));
    }
    if (!route.empty()) {
        cities.append(QString::number(route.front()));
    }
    return cities.join(QStringLiteral(" → "));
}

QString formatted_result(const SolverResult &result)
{
    const QString optimality = result.guaranteed_optimal
                                   ? MainWindow::tr("Guaranteed optimum")
                                   : MainWindow::tr(
                                         "Heuristic result — not guaranteed optimal");
    return MainWindow::tr(
               "Cost: %1\nRoute: %2\nCandidates evaluated: %3\n"
               "Optimality: %4\nElapsed: %5 seconds")
        .arg(QString::number(result.cost, 'f', 6),
             formatted_route(result.route),
             formatted_count(result.candidates_evaluated),
             optimality,
             QString::number(result.elapsed.count(), 'f', 6));
}

QString formatted_error(const AdapterError &error)
{
    return MainWindow::tr("Error while %1: %2")
        .arg(QString::fromStdString(error.operation),
             QString::fromStdString(error.status_text));
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    build_interface();
    update_dependent_limits();
    update_exact_policy();
    load_graph_data();
    start_worker();
}

MainWindow::~MainWindow()
{
    worker_thread_.quit();
    worker_thread_.wait();
}

void MainWindow::build_interface()
{
    setWindowTitle(tr("Traveling Salesman Problem"));
    resize(1080, 900);

    auto *scroll_area = new QScrollArea(this);
    scroll_area->setObjectName(QStringLiteral("mainScrollArea"));
    scroll_area->setWidgetResizable(true);

    auto *central = new QWidget(scroll_area);
    auto *root_layout = new QVBoxLayout(central);
    root_layout->setContentsMargins(16, 16, 16, 16);
    root_layout->setSpacing(12);

    auto *intro = new QLabel(
        tr("Configure and run the exact or evolutionary solver, or compare "
           "both using the same settings."),
        central);
    intro->setWordWrap(true);
    root_layout->addWidget(intro);

    graph_status_ = new QLabel(central);
    graph_status_->setObjectName(QStringLiteral("graphStatus"));
    graph_status_->setWordWrap(true);
    root_layout->addWidget(graph_status_);

    configuration_group_ =
        new QGroupBox(tr("Solver configuration"), central);
    auto *form = new QFormLayout(configuration_group_);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    city_count_ = new QSpinBox(configuration_group_);
    city_count_->setObjectName(QStringLiteral("cityCount"));
    city_count_->setRange(minimum_city_count, TSP_MAX_CITIES);
    city_count_->setValue(8);
    form->addRow(tr("&City count:"), city_count_);

    population_size_ = new QSpinBox(configuration_group_);
    population_size_->setObjectName(QStringLiteral("populationSize"));
    population_size_->setRange(2, TSP_MAX_POPULATION);
    population_size_->setValue(12);
    form->addRow(tr("&Population:"), population_size_);

    generations_ = new QSpinBox(configuration_group_);
    generations_->setObjectName(QStringLiteral("generations"));
    generations_->setRange(0, maximum_generations);
    generations_->setValue(50);
    form->addRow(tr("&Generations:"), generations_);

    elite_count_ = new QSpinBox(configuration_group_);
    elite_count_->setObjectName(QStringLiteral("eliteCount"));
    elite_count_->setMinimum(1);
    elite_count_->setValue(3);
    form->addRow(tr("&Elite count:"), elite_count_);

    mutation_swaps_ = new QSpinBox(configuration_group_);
    mutation_swaps_->setObjectName(QStringLiteral("mutationSwaps"));
    mutation_swaps_->setMinimum(1);
    mutation_swaps_->setValue(1);
    form->addRow(tr("&Mutation swaps:"), mutation_swaps_);

    seed_ = new QDoubleSpinBox(configuration_group_);
    seed_->setObjectName(QStringLiteral("seed"));
    seed_->setDecimals(0);
    seed_->setRange(
        0.0,
        static_cast<double>(std::numeric_limits<std::uint32_t>::max()));
    seed_->setSingleStep(1.0);
    seed_->setValue(12345.0);
    form->addRow(tr("&Seed:"), seed_);

    root_layout->addWidget(configuration_group_);

    auto *exact_policy = new QGroupBox(tr("Exact search"), central);
    auto *policy_layout = new QVBoxLayout(exact_policy);
    candidate_count_ = new QLabel(exact_policy);
    candidate_count_->setObjectName(QStringLiteral("candidateCount"));
    candidate_count_->setWordWrap(true);
    exact_warning_ = new QLabel(exact_policy);
    exact_warning_->setObjectName(QStringLiteral("exactWarning"));
    exact_warning_->setWordWrap(true);
    policy_layout->addWidget(candidate_count_);
    policy_layout->addWidget(exact_warning_);
    root_layout->addWidget(exact_policy);

    auto *button_layout = new QGridLayout;
    run_exact_ = new QPushButton(tr("Run Exact"), central);
    run_evolutionary_ = new QPushButton(tr("Run Evolutionary"), central);
    run_comparison_ = new QPushButton(tr("Run Comparison"), central);
    run_exact_->setObjectName(QStringLiteral("runExact"));
    run_evolutionary_->setObjectName(QStringLiteral("runEvolutionary"));
    run_comparison_->setObjectName(QStringLiteral("runComparison"));
    button_layout->addWidget(run_exact_, 0, 0);
    button_layout->addWidget(run_evolutionary_, 0, 1);
    button_layout->addWidget(run_comparison_, 0, 2);
    root_layout->addLayout(button_layout);

    busy_label_ = new QLabel(central);
    busy_label_->setObjectName(QStringLiteral("busyStatus"));
    busy_label_->setWordWrap(true);
    busy_label_->setVisible(false);
    root_layout->addWidget(busy_label_);

    progress_bar_ = new QProgressBar(central);
    progress_bar_->setObjectName(QStringLiteral("busyProgress"));
    progress_bar_->setRange(0, 0);
    progress_bar_->setTextVisible(false);
    progress_bar_->setVisible(false);
    root_layout->addWidget(progress_bar_);

    auto *results = new QGroupBox(tr("Results"), central);
    auto *results_layout = new QGridLayout(results);

    auto *route_note = new QLabel(
        tr("Schematic layout; screen distance does not represent route cost."),
        results);
    route_note->setObjectName(QStringLiteral("routeDisclaimer"));
    route_note->setAlignment(Qt::AlignCenter);
    route_note->setWordWrap(true);
    results_layout->addWidget(route_note, 0, 0, 1, 2);

    auto *exact_group =
        make_result_group(tr("Exact"),
                          QStringLiteral("exactResult"),
                          exact_result_label_,
                          results);
    exact_route_view_ = new RouteView(exact_group);
    exact_route_view_->setObjectName(QStringLiteral("exactRouteView"));
    exact_route_view_->setAccessibleName(tr("Exact route visualization"));
    exact_group->layout()->addWidget(exact_route_view_);
    results_layout->addWidget(exact_group, 1, 0);

    auto *evolutionary_group =
        make_result_group(tr("Evolutionary"),
                          QStringLiteral("evolutionaryResult"),
                          evolutionary_result_label_,
                          results);
    evolutionary_route_view_ = new RouteView(evolutionary_group);
    evolutionary_route_view_->setObjectName(
        QStringLiteral("evolutionaryRouteView"));
    evolutionary_route_view_->setAccessibleName(
        tr("Evolutionary route visualization"));
    evolutionary_group->layout()->addWidget(evolutionary_route_view_);
    results_layout->addWidget(evolutionary_group, 1, 1);

    convergence_view_ = new ConvergenceView(results);
    convergence_view_->setObjectName(QStringLiteral("convergenceView"));
    results_layout->addWidget(convergence_view_, 2, 0, 1, 2);
    results_layout->addWidget(
        make_result_group(tr("Comparison summary"),
                          QStringLiteral("comparisonResult"),
                          comparison_result_label_,
                          results),
        3,
        0,
        1,
        2);
    results_layout->setColumnStretch(0, 1);
    results_layout->setColumnStretch(1, 1);
    root_layout->addWidget(results);
    root_layout->addStretch();

    scroll_area->setWidget(central);
    setCentralWidget(scroll_area);
    statusBar()->showMessage(tr("Idle — ready to run a solver."));

    connect(population_size_,
            &QSpinBox::valueChanged,
            this,
            [this](int) {
                update_dependent_limits();
                invalidate_results();
            });
    connect(city_count_,
            &QSpinBox::valueChanged,
            this,
            [this](int) {
                update_dependent_limits();
                update_exact_policy();
                invalidate_results();
            });
    connect(generations_,
            &QSpinBox::valueChanged,
            this,
            [this](int) { invalidate_results(); });
    connect(elite_count_,
            &QSpinBox::valueChanged,
            this,
            [this](int) { invalidate_results(); });
    connect(mutation_swaps_,
            &QSpinBox::valueChanged,
            this,
            [this](int) { invalidate_results(); });
    connect(seed_,
            &QDoubleSpinBox::valueChanged,
            this,
            [this](double) { invalidate_results(); });
    connect(run_exact_, &QPushButton::clicked, this, &MainWindow::run_exact);
    connect(run_evolutionary_,
            &QPushButton::clicked,
            this,
            &MainWindow::run_evolutionary);
    connect(run_comparison_,
            &QPushButton::clicked,
            this,
            &MainWindow::run_comparison);

    setTabOrder(city_count_, population_size_);
    setTabOrder(population_size_, generations_);
    setTabOrder(generations_, elite_count_);
    setTabOrder(elite_count_, mutation_swaps_);
    setTabOrder(mutation_swaps_, seed_);
    setTabOrder(seed_, run_exact_);
    setTabOrder(run_exact_, run_evolutionary_);
    setTabOrder(run_evolutionary_, run_comparison_);
}

void MainWindow::start_worker()
{
    qRegisterMetaType<SolverJobRequest>();
    qRegisterMetaType<SolverJobCompletion>();

    worker_ = new SolverWorker;
    worker_->moveToThread(&worker_thread_);
    connect(this,
            &MainWindow::job_requested,
            worker_,
            &SolverWorker::execute,
            Qt::QueuedConnection);
    connect(worker_,
            &SolverWorker::completed,
            this,
            &MainWindow::handle_job_completed,
            Qt::QueuedConnection);
    connect(&worker_thread_,
            &QThread::finished,
            worker_,
            &QObject::deleteLater);
    worker_thread_.start();
}

void MainWindow::update_dependent_limits()
{
    elite_count_->setMaximum(population_size_->value());
    mutation_swaps_->setMaximum(city_count_->value() - 1);
}

void MainWindow::update_exact_policy()
{
    const int cities = city_count_->value();
    const int factorial_argument = cities - 1;
    candidate_count_->setText(
        tr("Anchored exact tours: %1! = %2 tours")
            .arg(factorial_argument)
            .arg(formatted_count(anchored_tour_count(cities))));

    if (cities <= 10) {
        exact_warning_->clear();
        exact_warning_->setVisible(false);
    } else {
        exact_warning_->setVisible(true);
        if (cities == 11) {
            exact_warning_->setText(
                tr("Caution: exhaustive search grows factorially. An 11-city "
                   "exact or comparison run requires confirmation."));
            exact_warning_->setStyleSheet(
                QStringLiteral("QLabel { background: #fff4ce; color: #5c4400; "
                               "border: 1px solid #d49b00; padding: 8px; }"));
        } else if (cities == 12) {
            exact_warning_->setText(
                tr("Strong warning: exhaustive search grows factorially. A "
                   "12-city exact or comparison run requires confirmation."));
            exact_warning_->setStyleSheet(
                QStringLiteral("QLabel { background: #fde7e9; color: #7a1c1c; "
                               "border: 1px solid #c42b1c; padding: 8px; }"));
        } else {
            exact_warning_->setText(
                tr("Exact and Comparison are unavailable. Exhaustive search "
                   "is supported only through 12 cities."));
            exact_warning_->setStyleSheet(
                QStringLiteral("QLabel { background: #fde7e9; color: #7a1c1c; "
                               "border: 1px solid #c42b1c; padding: 8px; }"));
        }
    }

    update_run_availability();
}

void MainWindow::update_run_availability()
{
    if (is_busy() || pending_close_) {
        run_exact_->setEnabled(false);
        run_evolutionary_->setEnabled(false);
        run_comparison_->setEnabled(false);
        const QString busy = pending_close_
                                 ? tr("The window will close when work finishes.")
                                 : tr("A solver operation is already running.");
        run_exact_->setToolTip(busy);
        run_evolutionary_->setToolTip(busy);
        run_comparison_->setToolTip(busy);
        return;
    }

    const bool graph_available = graph_.has_value();
    const bool exact_available = exact_is_available(city_count_->value());

    run_evolutionary_->setEnabled(graph_available);
    run_exact_->setEnabled(graph_available && exact_available);
    run_comparison_->setEnabled(graph_available && exact_available);

    if (!graph_available) {
        const QString unavailable =
            tr("Unavailable because graph data failed to load.");
        run_exact_->setToolTip(unavailable);
        run_evolutionary_->setToolTip(unavailable);
        run_comparison_->setToolTip(unavailable);
    } else {
        run_evolutionary_->setToolTip(tr("Run the evolutionary solver."));
        if (exact_available) {
            run_exact_->setToolTip(tr("Run the exact solver."));
            run_comparison_->setToolTip(
                tr("Run exact, then evolutionary, using one settings snapshot."));
        } else {
            const QString limit =
                tr("Exhaustive search is supported only through 12 cities.");
            run_exact_->setToolTip(limit);
            run_comparison_->setToolTip(limit);
        }
    }
}

void MainWindow::load_graph_data()
{
    const QString data_path = QDir(QCoreApplication::applicationDirPath())
                                  .filePath(QStringLiteral("cities.dat"));
    const QByteArray encoded_path = QFile::encodeName(data_path);
    auto loaded = load_graph(std::string(
        encoded_path.constData(), static_cast<std::size_t>(encoded_path.size())));

    if (auto *graph = std::get_if<Graph>(&loaded)) {
        graph_.emplace(std::move(*graph));
        graph_status_->setText(
            tr("Graph ready: cities.dat (%1 cities loaded)")
                .arg(graph_->city_count()));
        graph_status_->setStyleSheet(QStringLiteral("QLabel { color: #176b2c; }"));
        graph_status_->setToolTip(QDir::toNativeSeparators(data_path));
        update_run_availability();
        return;
    }

    graph_.reset();
    const auto &error = std::get<AdapterError>(loaded);
    graph_status_->setText(
        tr("Graph error while %1: %2. Expected cities.dat beside the "
           "application executable.")
            .arg(QString::fromStdString(error.operation),
                 QString::fromStdString(error.status_text)));
    graph_status_->setStyleSheet(QStringLiteral("QLabel { color: #a4262c; }"));
    graph_status_->setToolTip(QDir::toNativeSeparators(data_path));
    statusBar()->showMessage(tr("Graph data is unavailable."));
    update_run_availability();
}

void MainWindow::invalidate_results()
{
    if (!is_busy()) {
        clear_results();
    }
}

void MainWindow::clear_results()
{
    exact_result_data_.reset();
    evolutionary_result_data_.reset();
    comparison_.reset();
    active_city_count_ = 0U;
    exact_route_view_->clear_route();
    evolutionary_route_view_->clear_route();
    convergence_view_->clear_history();

    for (auto *label : { exact_result_label_,
                         evolutionary_result_label_,
                         comparison_result_label_ }) {
        label->setText(tr("No result yet"));
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet({});
    }
}

void MainWindow::run_exact()
{
    if (is_busy() || !graph_.has_value() ||
        !exact_is_available(city_count_->value()) ||
        !confirm_expensive_exact(false)) {
        return;
    }

    const RunSettings settings = settings_snapshot();
    clear_results();
    workflow_state_ = WorkflowState::ExactRunning;
    set_busy_message(tr("Running exact solver..."));
    submit_job(SolverOperation::Exact, *graph_, settings);
}

void MainWindow::run_evolutionary()
{
    if (is_busy() || !graph_.has_value()) {
        return;
    }

    const RunSettings settings = settings_snapshot();
    clear_results();
    workflow_state_ = WorkflowState::EvolutionaryRunning;
    set_busy_message(tr("Running evolutionary solver..."));
    submit_job(SolverOperation::Evolutionary, *graph_, settings);
}

void MainWindow::run_comparison()
{
    if (is_busy() || !graph_.has_value() ||
        !exact_is_available(city_count_->value()) ||
        !confirm_expensive_exact(true)) {
        return;
    }

    clear_results();
    comparison_.emplace(
        ComparisonContext{ *graph_, settings_snapshot() });
    workflow_state_ = WorkflowState::ComparisonExactRunning;
    set_busy_message(tr("Running comparison: exact solver..."));
    submit_job(SolverOperation::Exact,
               comparison_->graph,
               comparison_->settings);
}

void MainWindow::submit_job(SolverOperation operation,
                            const Graph &graph,
                            const RunSettings &settings)
{
    active_job_id_ = next_job_id_++;
    active_city_count_ = settings.city_count > 0
                             ? static_cast<std::size_t>(settings.city_count)
                             : 0U;
    emit job_requested(
        SolverJobRequest{ active_job_id_, operation, graph, settings });
}

void MainWindow::handle_job_completed(SolverJobCompletion completion)
{
    if (!is_busy() || completion.job_id != active_job_id_) {
        return;
    }

    const bool expects_exact =
        workflow_state_ == WorkflowState::ExactRunning ||
        workflow_state_ == WorkflowState::ComparisonExactRunning;
    if ((expects_exact && completion.operation != SolverOperation::Exact) ||
        (!expects_exact &&
         completion.operation != SolverOperation::Evolutionary)) {
        return;
    }

    if (auto *error = std::get_if<AdapterError>(&completion.outcome)) {
        if (workflow_state_ == WorkflowState::ExactRunning) {
            show_error(exact_result_label_, exact_route_view_, *error);
        } else if (workflow_state_ == WorkflowState::EvolutionaryRunning) {
            show_error(evolutionary_result_label_,
                       evolutionary_route_view_,
                       *error);
        } else if (workflow_state_ == WorkflowState::ComparisonExactRunning) {
            show_error(exact_result_label_, exact_route_view_, *error);
            comparison_result_label_->setText(
                tr("Comparison stopped because the exact solver failed."));
            comparison_result_label_->setStyleSheet(
                QStringLiteral("QLabel { color: #a4262c; }"));
        } else {
            show_error(evolutionary_result_label_,
                       evolutionary_route_view_,
                       *error);
            comparison_result_label_->setText(
                tr("Comparison incomplete because the evolutionary solver "
                   "failed. The exact result remains valid."));
            comparison_result_label_->setStyleSheet(
                QStringLiteral("QLabel { color: #a4262c; }"));
        }
        statusBar()->showMessage(formatted_error(*error));
        finish_workflow();
        return;
    }

    SolverResult result =
        std::move(std::get<SolverResult>(completion.outcome));
    if (workflow_state_ == WorkflowState::ExactRunning) {
        exact_result_data_.emplace(std::move(result));
        show_result(exact_result_label_,
                    exact_route_view_,
                    *exact_result_data_);
        statusBar()->showMessage(tr("Exact solver finished."));
        finish_workflow();
    } else if (workflow_state_ == WorkflowState::EvolutionaryRunning) {
        evolutionary_result_data_.emplace(std::move(result));
        convergence_view_->set_history(
            evolutionary_result_data_->convergence);
        show_result(evolutionary_result_label_,
                    evolutionary_route_view_,
                    *evolutionary_result_data_);
        statusBar()->showMessage(tr("Evolutionary solver finished."));
        finish_workflow();
    } else if (workflow_state_ == WorkflowState::ComparisonExactRunning) {
        exact_result_data_.emplace(std::move(result));
        show_result(exact_result_label_,
                    exact_route_view_,
                    *exact_result_data_);
        workflow_state_ = WorkflowState::ComparisonEvolutionaryRunning;
        set_busy_message(tr("Running comparison: evolutionary solver..."));
        submit_job(SolverOperation::Evolutionary,
                   comparison_->graph,
                   comparison_->settings);
    } else {
        evolutionary_result_data_.emplace(std::move(result));
        convergence_view_->set_history(
            evolutionary_result_data_->convergence);
        show_result(evolutionary_result_label_,
                    evolutionary_route_view_,
                    *evolutionary_result_data_);
        show_comparison_summary();
        statusBar()->showMessage(tr("Comparison finished."));
        finish_workflow();
    }
}

void MainWindow::finish_workflow()
{
    workflow_state_ = WorkflowState::Idle;
    active_job_id_ = 0U;
    comparison_.reset();
    busy_label_->clear();
    busy_label_->setVisible(false);
    progress_bar_->setVisible(false);

    if (pending_close_) {
        configuration_group_->setEnabled(false);
        update_run_availability();
        statusBar()->showMessage(tr("Work finished. Closing..."));
        QTimer::singleShot(0, this, [this] { close(); });
        return;
    }

    configuration_group_->setEnabled(true);
    update_run_availability();
}

void MainWindow::show_result(QLabel *label,
                             RouteView *route_view,
                             const SolverResult &result)
{
    route_view->set_route(result.route, active_city_count_);
    label->setText(formatted_result(result));
    label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    label->setStyleSheet({});
}

void MainWindow::show_error(QLabel *label,
                            RouteView *route_view,
                            const AdapterError &error)
{
    route_view->clear_route();
    label->setText(formatted_error(error));
    label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    label->setStyleSheet(QStringLiteral("QLabel { color: #a4262c; }"));
}

void MainWindow::show_comparison_summary()
{
    if (!exact_result_data_.has_value() ||
        !evolutionary_result_data_.has_value()) {
        return;
    }

    const double exact_cost = exact_result_data_->cost;
    const double heuristic_cost = evolutionary_result_data_->cost;
    double absolute_gap = heuristic_cost - exact_cost;

    const QString comparison_context =
        tr("Exact is the guaranteed optimum.\n"
           "Evolutionary is heuristic and is not guaranteed optimal.\n");

    if (!std::isfinite(exact_cost) || !std::isfinite(heuristic_cost) ||
        !std::isfinite(absolute_gap)) {
        comparison_result_label_->setText(
            comparison_context +
            tr("Comparison unavailable: solver costs are nonfinite or "
               "produced a nonfinite gap.\nNo numeric gap is reported."));
        comparison_result_label_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        comparison_result_label_->setStyleSheet(
            QStringLiteral("QLabel { color: #a4262c; }"));
        return;
    }

    const double gap_scale =
        std::max({ 1.0, std::abs(exact_cost), std::abs(heuristic_cost) });
    const double gap_tolerance =
        8.0 * std::numeric_limits<double>::epsilon() * gap_scale;

    if (absolute_gap < -gap_tolerance) {
        comparison_result_label_->setText(
            comparison_context +
            tr("Comparison inconsistent: the evolutionary cost is below the "
               "exact guaranteed-optimal cost beyond floating-point "
               "tolerance.\nNo numeric gap is reported."));
        comparison_result_label_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        comparison_result_label_->setStyleSheet(
            QStringLiteral("QLabel { color: #a4262c; }"));
        return;
    }

    if (absolute_gap < 0.0) {
        absolute_gap = 0.0;
    }

    QString percentage_gap = tr("Unavailable");
    if (exact_cost != 0.0) {
        percentage_gap =
            tr("%1%").arg(QString::number(
                (absolute_gap / exact_cost) * 100.0, 'f', 6));
    }

    comparison_result_label_->setText(
        comparison_context +
        tr("Absolute gap: %1\nPercentage gap: %2")
            .arg(QString::number(absolute_gap, 'f', 6), percentage_gap));
    comparison_result_label_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    comparison_result_label_->setStyleSheet({});
}

void MainWindow::set_busy_message(const QString &message)
{
    configuration_group_->setEnabled(false);
    busy_label_->setText(message);
    busy_label_->setVisible(true);
    progress_bar_->setVisible(true);
    statusBar()->showMessage(message);
    update_run_availability();
}

RunSettings MainWindow::settings_snapshot() const
{
    RunSettings settings;
    settings.city_count = city_count_->value();
    settings.population_size = population_size_->value();
    settings.generations = generations_->value();
    settings.elite_count = elite_count_->value();
    settings.mutation_swaps = mutation_swaps_->value();
    settings.seed = static_cast<std::uint32_t>(seed_->value());
    return settings;
}

bool MainWindow::confirm_expensive_exact(bool comparison)
{
    const int cities = city_count_->value();
    if (cities <= 10) {
        return true;
    }

    const int factorial_argument = cities - 1;
    const QString operation = comparison ? tr("comparison") : tr("exact run");
    const QString severity = cities == 12 ? tr("Strong warning")
                                          : tr("Expensive exact search");
    QMessageBox dialog(QMessageBox::Warning,
                       severity,
                       tr("Exhaustive search grows factorially."),
                       QMessageBox::NoButton,
                       this);
    dialog.setInformativeText(
        tr("The exact stage of this %1 will evaluate %2! = %3 anchored "
           "tours. No runtime estimate is available.")
            .arg(operation)
            .arg(factorial_argument)
            .arg(formatted_count(anchored_tour_count(cities))));
    auto *cancel_button =
        dialog.addButton(tr("Cancel"), QMessageBox::RejectRole);
    auto *run_button = dialog.addButton(
        comparison ? tr("Run Comparison") : tr("Run Exact"),
        QMessageBox::AcceptRole);
    cancel_button->setObjectName(QStringLiteral("cancelExpensiveRun"));
    run_button->setObjectName(QStringLiteral("confirmExpensiveRun"));
    dialog.setDefaultButton(cancel_button);
    dialog.setEscapeButton(cancel_button);
    dialog.exec();
    return dialog.clickedButton() == run_button;
}

bool MainWindow::is_busy() const noexcept
{
    return workflow_state_ != WorkflowState::Idle;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!is_busy()) {
        QMainWindow::closeEvent(event);
        return;
    }

    event->ignore();
    if (pending_close_) {
        statusBar()->showMessage(
            tr("The window will close when the active workflow finishes."));
        return;
    }

    QMessageBox dialog(QMessageBox::Question,
                       tr("Solver still running"),
                       tr("There is no cancellation in this release. The "
                          "active solver must finish normally."),
                       QMessageBox::NoButton,
                       this);
    auto *keep_running =
        dialog.addButton(tr("Keep Running"), QMessageBox::RejectRole);
    auto *close_when_finished = dialog.addButton(
        tr("Close When Finished"), QMessageBox::AcceptRole);
    keep_running->setObjectName(QStringLiteral("keepRunning"));
    close_when_finished->setObjectName(
        QStringLiteral("closeWhenFinished"));
    dialog.setDefaultButton(keep_running);
    dialog.setEscapeButton(keep_running);
    dialog.exec();

    if (dialog.clickedButton() == close_when_finished) {
        if (!is_busy()) {
            QTimer::singleShot(0, this, [this] { close(); });
            return;
        }
        pending_close_ = true;
        configuration_group_->setEnabled(false);
        update_run_availability();
        statusBar()->showMessage(
            tr("The window will close when the active workflow finishes."));
    }
}

} // namespace tsp::desktop
