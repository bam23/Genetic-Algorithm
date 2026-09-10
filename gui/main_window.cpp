#include "main_window.hpp"

#include <tsp/tsp.h>

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QSpinBox>
#include <QStatusBar>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

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

QGroupBox *make_result_placeholder(const QString &title, QWidget *parent)
{
    auto *group = new QGroupBox(title, parent);
    auto *layout = new QVBoxLayout(group);
    auto *placeholder = new QLabel(MainWindow::tr("No result yet"), group);
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setMinimumHeight(56);
    layout->addWidget(placeholder);
    return group;
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    build_interface();
    update_dependent_limits();
    update_exact_policy();
    load_graph_data();
}

void MainWindow::build_interface()
{
    setWindowTitle(tr("Traveling Salesman Problem"));
    resize(780, 620);

    auto *central = new QWidget(this);
    auto *root_layout = new QVBoxLayout(central);
    root_layout->setContentsMargins(16, 16, 16, 16);
    root_layout->setSpacing(12);

    auto *intro = new QLabel(
        tr("Configure an exact, evolutionary, or comparison run. Solver "
           "execution will be added in Phase D."),
        central);
    intro->setWordWrap(true);
    root_layout->addWidget(intro);

    graph_status_ = new QLabel(central);
    graph_status_->setWordWrap(true);
    root_layout->addWidget(graph_status_);

    auto *configuration = new QGroupBox(tr("Solver configuration"), central);
    auto *form = new QFormLayout(configuration);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    city_count_ = new QSpinBox(configuration);
    city_count_->setObjectName(QStringLiteral("cityCount"));
    city_count_->setRange(minimum_city_count, TSP_MAX_CITIES);
    city_count_->setValue(8);
    form->addRow(tr("&City count:"), city_count_);

    population_size_ = new QSpinBox(configuration);
    population_size_->setObjectName(QStringLiteral("populationSize"));
    population_size_->setRange(2, TSP_MAX_POPULATION);
    population_size_->setValue(12);
    form->addRow(tr("&Population:"), population_size_);

    generations_ = new QSpinBox(configuration);
    generations_->setObjectName(QStringLiteral("generations"));
    generations_->setRange(0, maximum_generations);
    generations_->setValue(50);
    form->addRow(tr("&Generations:"), generations_);

    elite_count_ = new QSpinBox(configuration);
    elite_count_->setObjectName(QStringLiteral("eliteCount"));
    elite_count_->setMinimum(1);
    elite_count_->setValue(3);
    form->addRow(tr("&Elite count:"), elite_count_);

    mutation_swaps_ = new QSpinBox(configuration);
    mutation_swaps_->setObjectName(QStringLiteral("mutationSwaps"));
    mutation_swaps_->setMinimum(1);
    mutation_swaps_->setValue(1);
    form->addRow(tr("&Mutation swaps:"), mutation_swaps_);

    seed_ = new QDoubleSpinBox(configuration);
    seed_->setObjectName(QStringLiteral("seed"));
    seed_->setDecimals(0);
    seed_->setRange(
        0.0,
        static_cast<double>(std::numeric_limits<std::uint32_t>::max()));
    seed_->setSingleStep(1.0);
    seed_->setValue(12345.0);
    form->addRow(tr("&Seed:"), seed_);

    root_layout->addWidget(configuration);

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

    auto *results = new QGroupBox(tr("Results"), central);
    auto *results_layout = new QGridLayout(results);
    results_layout->addWidget(
        make_result_placeholder(tr("Exact"), results), 0, 0);
    results_layout->addWidget(
        make_result_placeholder(tr("Evolutionary"), results), 0, 1);
    results_layout->addWidget(
        make_result_placeholder(tr("Comparison summary"), results), 1, 0, 1, 2);
    results_layout->setColumnStretch(0, 1);
    results_layout->setColumnStretch(1, 1);
    root_layout->addWidget(results);
    root_layout->addStretch();

    setCentralWidget(central);
    statusBar()->showMessage(
        tr("Idle — no solver execution is available in Phase C."));

    connect(population_size_,
            &QSpinBox::valueChanged,
            this,
            [this](int) { update_dependent_limits(); });
    connect(city_count_,
            &QSpinBox::valueChanged,
            this,
            [this](int) {
                update_dependent_limits();
                update_exact_policy();
            });
    connect(run_exact_,
            &QPushButton::clicked,
            this,
            &MainWindow::show_phase_d_placeholder);
    connect(run_evolutionary_,
            &QPushButton::clicked,
            this,
            &MainWindow::show_phase_d_placeholder);
    connect(run_comparison_,
            &QPushButton::clicked,
            this,
            &MainWindow::show_phase_d_placeholder);

    setTabOrder(city_count_, population_size_);
    setTabOrder(population_size_, generations_);
    setTabOrder(generations_, elite_count_);
    setTabOrder(elite_count_, mutation_swaps_);
    setTabOrder(mutation_swaps_, seed_);
    setTabOrder(seed_, run_exact_);
    setTabOrder(run_exact_, run_evolutionary_);
    setTabOrder(run_evolutionary_, run_comparison_);
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
                tr("Caution: exhaustive search grows factorially. A future "
                   "11-city exact or comparison run will require confirmation."));
            exact_warning_->setStyleSheet(
                QStringLiteral("QLabel { background: #fff4ce; color: #5c4400; "
                               "border: 1px solid #d49b00; padding: 8px; }"));
        } else if (cities == 12) {
            exact_warning_->setText(
                tr("Strong warning: exhaustive search grows factorially. A "
                   "future 12-city exact or comparison run will require "
                   "confirmation."));
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
    const bool graph_available = graph_.has_value();
    const bool exact_available = exact_is_available(city_count_->value());

    run_evolutionary_->setEnabled(graph_available);
    run_exact_->setEnabled(graph_available && exact_available);
    run_comparison_->setEnabled(graph_available && exact_available);

    if (!graph_available) {
        const QString unavailable = tr("Unavailable because graph data failed to load.");
        run_exact_->setToolTip(unavailable);
        run_evolutionary_->setToolTip(unavailable);
        run_comparison_->setToolTip(unavailable);
    } else {
        run_evolutionary_->setToolTip(
            tr("No solver will run until Phase D."));
        if (exact_available) {
            const QString placeholder = tr("No solver will run until Phase D.");
            run_exact_->setToolTip(placeholder);
            run_comparison_->setToolTip(placeholder);
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

void MainWindow::show_phase_d_placeholder()
{
    statusBar()->showMessage(
        tr("No solver was started. Execution will be added in Phase D."),
        5000);
}

} // namespace tsp::desktop
