#ifndef TSP_DESKTOP_CONVERGENCE_VIEW_HPP
#define TSP_DESKTOP_CONVERGENCE_VIEW_HPP

#include <QWidget>

#include <cstddef>
#include <vector>

QT_BEGIN_NAMESPACE
class QPaintEvent;
QT_END_NAMESPACE

namespace tsp::desktop {

class ConvergenceView final : public QWidget {
public:
    explicit ConvergenceView(QWidget *parent = nullptr);

    void set_history(std::vector<double> history);
    void clear_history();

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    struct RenderSample {
        std::size_t generation;
        double value;
    };

    void update_range() noexcept;
    void rebuild_samples(int pixel_width);

    std::vector<double> history_;
    std::vector<RenderSample> render_samples_;
    double minimum_cost_ = 0.0;
    double maximum_cost_ = 0.0;
    int sample_width_ = -1;
    bool range_available_ = false;
};

} // namespace tsp::desktop

#endif
