#ifndef TSP_DESKTOP_ROUTE_VIEW_HPP
#define TSP_DESKTOP_ROUTE_VIEW_HPP

#include <QWidget>

#include <cstddef>
#include <vector>

QT_BEGIN_NAMESPACE
class QPaintEvent;
QT_END_NAMESPACE

namespace tsp::desktop {

class RouteView final : public QWidget {
public:
    explicit RouteView(QWidget *parent = nullptr);

    void set_route(std::vector<int> route, std::size_t city_count);
    void clear_route();

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    enum class DisplayState {
        Empty,
        Valid,
        Invalid
    };

    bool route_is_valid() const noexcept;
    void update_accessibility();

    std::vector<int> route_;
    std::size_t city_count_ = 0U;
    DisplayState state_ = DisplayState::Empty;
};

} // namespace tsp::desktop

#endif
