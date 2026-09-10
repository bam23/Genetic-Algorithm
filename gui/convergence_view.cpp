#include "convergence_view.hpp"

#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPaintEvent>
#include <QRectF>
#include <QSizePolicy>
#include <QString>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <utility>

namespace tsp::desktop {
namespace {

QString formatted_cost(double value)
{
    return QString::number(value, 'g', 7);
}

} // namespace

ConvergenceView::ConvergenceView(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setAccessibleName(tr("Evolutionary convergence chart"));
    setAccessibleDescription(
        tr("Best route cost found so far over generations. The horizontal "
           "axis is generation, the vertical axis is best route cost, and "
           "lower values are better. No convergence data is available yet."));
}

void ConvergenceView::set_history(std::vector<double> history)
{
    history_ = std::move(history);
    update_range();
    render_samples_.clear();
    sample_width_ = -1;

    if (history_.empty()) {
        setAccessibleDescription(
            tr("Best route cost found so far over generations. The horizontal "
               "axis is generation, the vertical axis is best route cost, "
               "and lower values are better. No convergence data is "
               "available yet."));
    } else if (!range_available_) {
        setAccessibleDescription(
            tr("Convergence chart unavailable because the history contains "
               "a nonfinite value."));
    } else {
        const QString history_summary =
            history_.size() == 1U
                ? tr("1 point represents generation 0.")
                : tr("%1 points represent generation 0 through generation %2.")
                      .arg(static_cast<qulonglong>(history_.size()))
                      .arg(static_cast<qulonglong>(history_.size() - 1U));
        setAccessibleDescription(
            tr("Best route cost found so far over generations. The horizontal "
               "axis is generation, the vertical axis is best route cost, "
               "and lower values are better. %1 Final best cost: %2.")
                .arg(history_summary, formatted_cost(history_.back())));
    }
    update();
}

void ConvergenceView::clear_history()
{
    history_.clear();
    render_samples_.clear();
    minimum_cost_ = 0.0;
    maximum_cost_ = 0.0;
    sample_width_ = -1;
    range_available_ = false;
    setAccessibleDescription(
        tr("Best route cost found so far over generations. The horizontal "
           "axis is generation, the vertical axis is best route cost, and "
           "lower values are better. No convergence data is available yet."));
    update();
}

QSize ConvergenceView::sizeHint() const
{
    return { 560, 260 };
}

QSize ConvergenceView::minimumSizeHint() const
{
    return { 260, 180 };
}

void ConvergenceView::update_range() noexcept
{
    range_available_ = !history_.empty();
    if (!range_available_) {
        minimum_cost_ = 0.0;
        maximum_cost_ = 0.0;
        return;
    }

    minimum_cost_ = history_.front();
    maximum_cost_ = history_.front();
    for (const double value : history_) {
        if (!std::isfinite(value)) {
            range_available_ = false;
            return;
        }
        minimum_cost_ = std::min(minimum_cost_, value);
        maximum_cost_ = std::max(maximum_cost_, value);
    }
}

void ConvergenceView::rebuild_samples(int pixel_width)
{
    render_samples_.clear();
    sample_width_ = pixel_width;

    if (!range_available_ || pixel_width <= 0) {
        return;
    }

    const std::size_t point_count = history_.size();
    const std::size_t bucket_count =
        std::min(point_count, static_cast<std::size_t>(pixel_width));

    const auto append_sample = [this](std::size_t generation) {
        if (render_samples_.empty() ||
            render_samples_.back().generation != generation) {
            render_samples_.push_back(
                RenderSample{ generation, history_[generation] });
        }
    };

    if (point_count <= bucket_count * 2U) {
        render_samples_.reserve(point_count);
        for (std::size_t generation = 0U; generation < point_count;
             ++generation) {
            append_sample(generation);
        }
        return;
    }

    render_samples_.reserve(bucket_count * 2U + 2U);
    append_sample(0U);

    for (std::size_t bucket = 0U; bucket < bucket_count; ++bucket) {
        const std::size_t begin = (bucket * point_count) / bucket_count;
        const std::size_t end = ((bucket + 1U) * point_count) / bucket_count;

        std::size_t minimum_generation = begin;
        std::size_t maximum_generation = begin;
        for (std::size_t generation = begin + 1U; generation < end;
             ++generation) {
            if (history_[generation] < history_[minimum_generation]) {
                minimum_generation = generation;
            }
            if (history_[generation] > history_[maximum_generation]) {
                maximum_generation = generation;
            }
        }

        if (minimum_generation < maximum_generation) {
            append_sample(minimum_generation);
            append_sample(maximum_generation);
        } else {
            append_sample(maximum_generation);
            append_sample(minimum_generation);
        }
    }

    append_sample(point_count - 1U);
}

void ConvergenceView::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), palette().brush(QPalette::Base));
    painter.setPen(palette().color(QPalette::Text));

    const QFontMetrics metrics = painter.fontMetrics();
    if (history_.empty()) {
        painter.setPen(palette().color(QPalette::PlaceholderText));
        painter.drawText(rect(),
                         Qt::AlignCenter | Qt::TextWordWrap,
                         tr("No convergence data yet"));
        return;
    }

    if (!range_available_) {
        painter.drawText(
            rect(),
            Qt::AlignCenter | Qt::TextWordWrap,
            tr("Convergence chart unavailable because the history contains "
               "a nonfinite value."));
        return;
    }

    if (width() < 120 || height() < 100) {
        painter.drawText(rect(),
                         Qt::AlignCenter | Qt::TextWordWrap,
                         tr("Resize to view convergence"));
        return;
    }

    const QString minimum_label = formatted_cost(minimum_cost_);
    const QString maximum_label = formatted_cost(maximum_cost_);
    const int widest_value =
        std::max(metrics.horizontalAdvance(minimum_label),
                 metrics.horizontalAdvance(maximum_label));
    const int left_margin = std::max(72, widest_value + 18);
    const int right_margin = 18;
    const int top_margin = metrics.height() * 2 + 14;
    const int bottom_margin = metrics.height() * 2 + 20;
    const QRectF plot_area(
        static_cast<double>(left_margin),
        static_cast<double>(top_margin),
        static_cast<double>(width() - left_margin - right_margin),
        static_cast<double>(height() - top_margin - bottom_margin));

    if (plot_area.width() < 2.0 || plot_area.height() < 2.0) {
        painter.drawText(rect(),
                         Qt::AlignCenter | Qt::TextWordWrap,
                         tr("Resize to view convergence"));
        return;
    }

    painter.drawText(
        QRectF(0.0,
               2.0,
               static_cast<double>(width()),
               static_cast<double>(top_margin - 4)),
        Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap,
        tr("Best route cost found so far over generations — lower is better"));

    const double range_scale =
        std::max({ 1.0, std::abs(minimum_cost_), std::abs(maximum_cost_) });
    const double normalized_minimum = minimum_cost_ / range_scale;
    const double normalized_maximum = maximum_cost_ / range_scale;
    const double normalized_range = normalized_maximum - normalized_minimum;
    const bool flat_range =
        normalized_range <=
        8.0 * std::numeric_limits<double>::epsilon();

    const auto y_for_value = [&](double value) {
        if (flat_range) {
            return plot_area.center().y();
        }
        const double fraction =
            ((value / range_scale) - normalized_minimum) / normalized_range;
        return plot_area.bottom() - fraction * plot_area.height();
    };

    const std::size_t final_generation = history_.size() - 1U;
    const auto x_for_generation = [&](std::size_t generation) {
        if (final_generation == 0U) {
            return plot_area.center().x();
        }
        const double fraction = static_cast<double>(generation) /
                                static_cast<double>(final_generation);
        return plot_area.left() + fraction * plot_area.width();
    };

    QPen grid_pen(palette().color(QPalette::Midlight));
    grid_pen.setWidthF(1.0);
    painter.setPen(grid_pen);
    if (flat_range) {
        const double y = plot_area.center().y();
        painter.drawLine(QPointF(plot_area.left(), y),
                         QPointF(plot_area.right(), y));
    } else {
        constexpr int tick_count = 4;
        for (int tick = 0; tick <= tick_count; ++tick) {
            const double fraction = static_cast<double>(tick) /
                                    static_cast<double>(tick_count);
            const double y = plot_area.top() + fraction * plot_area.height();
            painter.drawLine(QPointF(plot_area.left(), y),
                             QPointF(plot_area.right(), y));

            const double tick_value =
                (1.0 - fraction) * maximum_cost_ +
                fraction * minimum_cost_;
            painter.setPen(palette().color(QPalette::Text));
            painter.drawText(
                QRectF(0.0,
                       y - static_cast<double>(metrics.height()) / 2.0,
                       static_cast<double>(left_margin - 8),
                       static_cast<double>(metrics.height())),
                Qt::AlignRight | Qt::AlignVCenter,
                formatted_cost(tick_value));
            painter.setPen(grid_pen);
        }
    }

    if (flat_range) {
        painter.setPen(palette().color(QPalette::Text));
        painter.drawText(
            QRectF(0.0,
                   plot_area.center().y() -
                       static_cast<double>(metrics.height()) / 2.0,
                   static_cast<double>(left_margin - 8),
                   static_cast<double>(metrics.height())),
            Qt::AlignRight | Qt::AlignVCenter,
            formatted_cost(minimum_cost_));
    }

    QPen axis_pen(palette().color(QPalette::Text));
    axis_pen.setWidthF(1.0);
    painter.setPen(axis_pen);
    painter.drawLine(plot_area.bottomLeft(), plot_area.bottomRight());
    painter.drawLine(plot_area.topLeft(), plot_area.bottomLeft());

    const int plot_pixel_width =
        std::max(1, static_cast<int>(std::floor(plot_area.width())));
    if (sample_width_ != plot_pixel_width) {
        rebuild_samples(plot_pixel_width);
    }

    QPainterPath path;
    bool begins_path = true;
    for (const RenderSample &sample : render_samples_) {
        const QPointF point(x_for_generation(sample.generation),
                            y_for_value(sample.value));
        if (begins_path) {
            path.moveTo(point);
            begins_path = false;
        } else {
            path.lineTo(point);
        }
    }

    QPen line_pen(palette().color(QPalette::Highlight));
    line_pen.setWidthF(2.0);
    painter.save();
    painter.setClipRect(plot_area.adjusted(-2.0, -2.0, 2.0, 2.0));
    painter.setPen(line_pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);
    painter.restore();

    const QPointF initial_point(x_for_generation(0U),
                                y_for_value(history_.front()));
    const QPointF final_point(x_for_generation(final_generation),
                              y_for_value(history_.back()));
    painter.setPen(line_pen);
    painter.setBrush(palette().brush(QPalette::Highlight));
    painter.drawEllipse(initial_point, 3.5, 3.5);
    painter.drawEllipse(final_point, 4.0, 4.0);

    painter.setPen(palette().color(QPalette::Text));
    if (final_generation == 0U) {
        painter.drawText(
            QRectF(plot_area.center().x() - 24.0,
                   plot_area.bottom() + 4.0,
                   48.0,
                   static_cast<double>(metrics.height())),
            Qt::AlignHCenter | Qt::AlignTop,
            QStringLiteral("0"));
    } else {
        painter.drawText(
            QRectF(plot_area.left(),
                   plot_area.bottom() + 4.0,
                   48.0,
                   static_cast<double>(metrics.height())),
            Qt::AlignLeft | Qt::AlignTop,
            QStringLiteral("0"));
        painter.drawText(
            QRectF(plot_area.right() - 96.0,
                   plot_area.bottom() + 4.0,
                   96.0,
                   static_cast<double>(metrics.height())),
            Qt::AlignRight | Qt::AlignTop,
            QString::number(static_cast<qulonglong>(final_generation)));
    }

    painter.drawText(
        QRectF(plot_area.left(),
               plot_area.bottom() + static_cast<double>(metrics.height()) + 4.0,
               plot_area.width(),
               static_cast<double>(metrics.height())),
        Qt::AlignHCenter | Qt::AlignTop,
        tr("Generation"));

    painter.save();
    painter.translate(14.0, plot_area.center().y());
    painter.rotate(-90.0);
    painter.drawText(
        QRectF(-plot_area.height() / 2.0,
               -static_cast<double>(metrics.height()),
               plot_area.height(),
               static_cast<double>(metrics.height())),
        Qt::AlignCenter,
        tr("Best route cost"));
    painter.restore();

    const QString final_label =
        tr("Final: %1").arg(formatted_cost(history_.back()));
    const double annotation_width =
        static_cast<double>(metrics.horizontalAdvance(final_label) + 12);
    const double annotation_height =
        static_cast<double>(metrics.height() + 8);
    double annotation_x = final_point.x() + 8.0;
    if (annotation_x + annotation_width > plot_area.right()) {
        annotation_x = final_point.x() - annotation_width - 8.0;
    }
    annotation_x = std::max(plot_area.left(), annotation_x);

    double annotation_y = final_point.y() - annotation_height - 8.0;
    if (annotation_y < plot_area.top()) {
        annotation_y = final_point.y() + 8.0;
    }
    annotation_y = std::min(annotation_y,
                            plot_area.bottom() - annotation_height);

    const QRectF annotation(annotation_x,
                            annotation_y,
                            annotation_width,
                            annotation_height);
    painter.fillRect(annotation, palette().brush(QPalette::Highlight));
    painter.setBrush(Qt::NoBrush);
    painter.setPen(palette().color(QPalette::Mid));
    painter.drawRect(annotation);
    painter.setPen(palette().color(QPalette::HighlightedText));
    painter.drawText(annotation, Qt::AlignCenter, final_label);
}

} // namespace tsp::desktop
