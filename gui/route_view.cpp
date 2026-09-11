#include "route_view.hpp"

#include <tsp/tsp.h>

#include <QFontMetrics>
#include <QPainter>
#include <QPaintEvent>
#include <QPalette>
#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <QSizePolicy>
#include <QString>
#include <QStringList>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

namespace tsp::desktop {
namespace {

constexpr double pi = 3.14159265358979323846;

void draw_directed_edge(QPainter &painter,
                        const QPointF &from,
                        const QPointF &to,
                        double node_radius,
                        const QColor &color)
{
    const double delta_x = to.x() - from.x();
    const double delta_y = to.y() - from.y();
    const double center_distance = std::hypot(delta_x, delta_y);
    const double endpoint_trim = node_radius + 2.0;

    if (!std::isfinite(center_distance) ||
        center_distance <= endpoint_trim * 2.0 + 1.0) {
        return;
    }

    const double unit_x = delta_x / center_distance;
    const double unit_y = delta_y / center_distance;
    const QPointF start(from.x() + unit_x * endpoint_trim,
                        from.y() + unit_y * endpoint_trim);
    const QPointF tip(to.x() - unit_x * endpoint_trim,
                      to.y() - unit_y * endpoint_trim);
    const double trimmed_length = center_distance - endpoint_trim * 2.0;

    QPen edge_pen(color);
    edge_pen.setWidthF(1.8);
    edge_pen.setCapStyle(Qt::RoundCap);
    painter.setPen(edge_pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawLine(start, tip);

    constexpr double minimum_arrow_length = 6.0;
    if (trimmed_length < minimum_arrow_length) {
        return;
    }

    const double arrow_length =
        std::clamp(trimmed_length * 0.16, minimum_arrow_length, 11.0);
    const double arrow_half_width = arrow_length * 0.48;
    const QPointF arrow_base(tip.x() - unit_x * arrow_length,
                             tip.y() - unit_y * arrow_length);
    const QPointF perpendicular(-unit_y * arrow_half_width,
                                unit_x * arrow_half_width);

    QPolygonF arrow;
    arrow << tip << arrow_base + perpendicular << arrow_base - perpendicular;
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawPolygon(arrow);
}

} // namespace

RouteView::RouteView(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setAccessibleName(tr("Route visualization"));
    update_accessibility();
}

void RouteView::set_route(std::vector<int> route, std::size_t city_count)
{
    route_ = std::move(route);
    city_count_ = city_count;
    state_ = route_is_valid() ? DisplayState::Valid : DisplayState::Invalid;
    update_accessibility();
    update();
}

void RouteView::clear_route()
{
    route_.clear();
    city_count_ = 0U;
    state_ = DisplayState::Empty;
    update_accessibility();
    update();
}

QSize RouteView::sizeHint() const
{
    return { 420, 280 };
}

QSize RouteView::minimumSizeHint() const
{
    return { 220, 200 };
}

bool RouteView::route_is_valid() const noexcept
{
    if (city_count_ == 0U ||
        city_count_ > static_cast<std::size_t>(TSP_MAX_CITIES) ||
        route_.size() != city_count_ || route_.front() != 0) {
        return false;
    }

    for (std::size_t route_index = 0U; route_index < city_count_;
         ++route_index) {
        const int city = route_[route_index];
        if (city < 0 || static_cast<std::size_t>(city) >= city_count_) {
            return false;
        }
        for (std::size_t earlier = 0U; earlier < route_index; ++earlier) {
            if (route_[earlier] == city) {
                return false;
            }
        }
    }

    return true;
}

void RouteView::update_accessibility()
{
    if (state_ == DisplayState::Empty) {
        setAccessibleDescription(
            tr("Schematic directed route view. City 0 is the start. Screen "
               "distance does not represent route cost. No route is "
               "available yet."));
        return;
    }
    if (state_ == DisplayState::Invalid) {
        setAccessibleDescription(
            tr("Schematic directed route view unavailable because the route "
               "data is invalid."));
        return;
    }

    QStringList route_labels;
    route_labels.reserve(static_cast<qsizetype>(city_count_ + 1U));
    for (std::size_t index = 0U; index < city_count_; ++index) {
        route_labels.append(QString::number(route_[index]));
    }
    route_labels.append(QStringLiteral("0"));
    setAccessibleDescription(
        tr("Schematic directed route through %1 cities. City 0 is the start. "
           "Route order: %2. Screen distance does not represent route cost.")
            .arg(static_cast<qulonglong>(city_count_))
            .arg(route_labels.join(QStringLiteral(" to "))));
}

void RouteView::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), palette().brush(QPalette::Base));

    if (state_ == DisplayState::Empty) {
        painter.setPen(palette().color(QPalette::PlaceholderText));
        painter.drawText(rect(),
                         Qt::AlignCenter | Qt::TextWordWrap,
                         tr("No route yet"));
        return;
    }
    if (state_ == DisplayState::Invalid) {
        painter.setPen(palette().color(QPalette::Text));
        painter.drawText(rect(),
                         Qt::AlignCenter | Qt::TextWordWrap,
                         tr("Route visualization unavailable\nInvalid route "
                            "data"));
        return;
    }
    if (width() < 140 || height() < 130) {
        painter.setPen(palette().color(QPalette::Text));
        painter.drawText(rect(),
                         Qt::AlignCenter | Qt::TextWordWrap,
                         tr("Resize to view route"));
        return;
    }

    const QFontMetrics metrics = painter.fontMetrics();
    const QString largest_label =
        QString::number(static_cast<qulonglong>(city_count_ - 1U));
    const double label_radius =
        static_cast<double>(metrics.horizontalAdvance(largest_label)) / 2.0 +
        5.0;
    const double node_radius = std::clamp(label_radius, 11.0, 18.0);
    const QRectF canvas = QRectF(rect()).adjusted(12.0, 12.0, -12.0, -12.0);
    const double layout_radius =
        std::min(canvas.width(), canvas.height()) / 2.0 - node_radius - 8.0;
    if (!std::isfinite(layout_radius) || layout_radius <= 1.0) {
        painter.setPen(palette().color(QPalette::Text));
        painter.drawText(rect(),
                         Qt::AlignCenter | Qt::TextWordWrap,
                         tr("Resize to view route"));
        return;
    }

    const QPointF center = canvas.center();
    std::vector<QPointF> positions;
    positions.reserve(city_count_);
    for (std::size_t city = 0U; city < city_count_; ++city) {
        const double fraction = static_cast<double>(city) /
                                static_cast<double>(city_count_);
        const double angle = -pi / 2.0 + 2.0 * pi * fraction;
        positions.emplace_back(center.x() + layout_radius * std::cos(angle),
                               center.y() + layout_radius * std::sin(angle));
    }

    if (city_count_ > 1U) {
        const QColor edge_color = palette().color(QPalette::Highlight);
        for (std::size_t order = 0U; order < city_count_; ++order) {
            const std::size_t next_order = (order + 1U) % city_count_;
            const auto from_city =
                static_cast<std::size_t>(route_[order]);
            const auto to_city =
                static_cast<std::size_t>(route_[next_order]);
            draw_directed_edge(painter,
                               positions[from_city],
                               positions[to_city],
                               node_radius,
                               edge_color);
        }
    }

    QPen node_pen(palette().color(QPalette::Text));
    node_pen.setWidthF(1.5);
    for (std::size_t city = 0U; city < city_count_; ++city) {
        const QPointF &position = positions[city];
        const QRectF node_rect(position.x() - node_radius,
                               position.y() - node_radius,
                               node_radius * 2.0,
                               node_radius * 2.0);

        if (city == 0U) {
            QPen start_pen(palette().color(QPalette::Text));
            start_pen.setWidthF(2.4);
            painter.setPen(start_pen);
            painter.setBrush(palette().brush(QPalette::Highlight));
            painter.drawRoundedRect(node_rect, 3.0, 3.0);
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(node_rect.adjusted(3.5, 3.5, -3.5, -3.5),
                                    2.0,
                                    2.0);
            painter.setPen(palette().color(QPalette::HighlightedText));
        } else {
            painter.setPen(node_pen);
            painter.setBrush(palette().brush(QPalette::Base));
            painter.drawEllipse(node_rect);
            painter.setPen(palette().color(QPalette::Text));
        }

        painter.drawText(node_rect,
                         Qt::AlignCenter,
                         QString::number(static_cast<qulonglong>(city)));
    }
}

} // namespace tsp::desktop
