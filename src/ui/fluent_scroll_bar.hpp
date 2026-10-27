#pragma once

#include <QWidget>
#include <QAbstractScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QElapsedTimer>
#include <QPainter>
#include <QPainterPath>
#include "high_hz_animator.hpp"

namespace masscan::ui {

class FluentScrollBarOverlay : public QWidget {
    Q_OBJECT

public:
    explicit FluentScrollBarOverlay(QAbstractScrollArea* area);
    ~FluentScrollBarOverlay() override = default;

    void setDark(bool isDark);
    void updateScrollbarGeometry();

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onSmoothTimerTick();

private:
    enum class ButtonHover { None, Up, Down };

    QRectF thumbRect() const;
    QRectF upButtonRect() const;
    QRectF downButtonRect() const;
    qreal trackSize() const;
    qreal thumbHeight() const;
    int scrollValue() const;
    int maxScroll() const;
    int pageSize() const;
    void setScrollValue(int val);
    bool handleWheelDelta(int angleDeltaY);
    void animateWidthTo(qreal targetWidth);

    void drawChevronUp(QPainter& p, const QRectF& r, const QColor& color);
    void drawChevronDown(QPainter& p, const QRectF& r, const QColor& color);

    QAbstractScrollArea* m_area{nullptr};
    bool m_isDark{true};
    bool m_hovered{false};
    bool m_dragging{false};
    ButtonHover m_buttonHover{ButtonHover::None};

    qreal m_currentWidth{2.0};
    qreal m_widthStart{2.0};
    qreal m_widthTarget{2.0};
    qreal m_dragStartY{0.0};
    int m_dragStartVal{0};

    // Exact Slint 180ms Constant Deceleration animation (subpixel tracked, integer-target locked)
    double m_animStartPos{0.0};
    double m_animDistance{0.0};
    double m_currentSubpixelPos{0.0};
    int m_targetPos{0};
    QElapsedTimer m_elapsedTimer;
    QTimer m_smoothTimer;

    HighHzAnimator* m_widthAnim{nullptr};
};

} // namespace masscan::ui
