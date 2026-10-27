#pragma once

#include <QStackedWidget>
#include <QPainter>
#include <QPixmap>
#include <QResizeEvent>
#include <QLayout>
#include <cmath>
#include <algorithm>
#include "high_hz_animator.hpp"

namespace masscan::ui {

class SlidingStackedWidget : public QStackedWidget {
    Q_OBJECT

public:
    explicit SlidingStackedWidget(QWidget* parent = nullptr)
        : QStackedWidget(parent) {
        setAttribute(Qt::WA_StyledBackground, false);

        m_animator = new HighHzAnimator(this);
        m_animator->setDuration(260); // 260ms smooth responsive slide
        m_animator->onProgress = [this](double p) {
            // Cubic ease-out: 1 - (1 - p)^3
            m_progress = 1.0 - std::pow(1.0 - p, 3.0);
            update();
        };
        m_animator->onFinished = [this]() {
            finishAnimation();
        };
    }

    void setDark(bool dark) {
        m_isDark = dark;
        update();
    }

    void slideTo(int nextIndex) {
        if (nextIndex == currentIndex() || nextIndex < 0 || nextIndex >= count()) {
            return;
        }

        if (m_isAnimating) {
            m_animator->stop();
            finishAnimation();
        }

        if (width() <= 0 || height() <= 0 || !isVisible()) {
            setCurrentIndex(nextIndex);
            return;
        }

        m_prevIndex = currentIndex();
        m_nextIndex = nextIndex;
        // Direction:
        // 1 = forward (0 -> 1): next slides in from right, current exits to left
        // -1 = backward (1 -> 0): next slides in from left, current exits to right
        m_direction = (m_nextIndex > m_prevIndex) ? 1 : -1;

        QWidget* curWidget = widget(m_prevIndex);
        QWidget* nxtWidget = widget(m_nextIndex);

        if (!curWidget || !nxtWidget) {
            setCurrentIndex(nextIndex);
            return;
        }

        qreal dpr = devicePixelRatioF();
        QSize s = size();

        // Ensure target widget is styled, sized, and laid out properly before capturing snapshot
        nxtWidget->ensurePolished();
        nxtWidget->setGeometry(0, 0, s.width(), s.height());
        if (nxtWidget->layout()) {
            nxtWidget->layout()->activate();
        }

        // Capture high-DPI snapshot of current widget
        curWidget->ensurePolished();
        m_currentPixmap = QPixmap(s * dpr);
        m_currentPixmap.setDevicePixelRatio(dpr);
        m_currentPixmap.fill(Qt::transparent);
        curWidget->render(&m_currentPixmap, QPoint(), QRegion(), QWidget::DrawChildren);

        // Capture high-DPI snapshot of next widget
        m_nextPixmap = QPixmap(s * dpr);
        m_nextPixmap.setDevicePixelRatio(dpr);
        m_nextPixmap.fill(Qt::transparent);
        nxtWidget->render(&m_nextPixmap, QPoint(), QRegion(), QWidget::DrawChildren);

        // Hide live child widgets during the 260ms slide to avoid any layout jitter or scrollbar flicker
        curWidget->hide();
        nxtWidget->hide();

        m_progress = 0.0;
        m_isAnimating = true;
        m_animator->start();
    }

protected:
    void resizeEvent(QResizeEvent* event) override {
        QStackedWidget::resizeEvent(event);
        if (m_isAnimating) {
            m_animator->stop();
            finishAnimation();
        }
    }

    void paintEvent(QPaintEvent* event) override {
        if (!m_isAnimating) {
            QStackedWidget::paintEvent(event);
            return;
        }

        QPainter p(this);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);

        QColor bg = m_isDark ? QColor("#171c24") : QColor("#ffffff");
        p.fillRect(rect(), bg);

        const double w = width();
        // Direction == 1 (Next enters from Right):
        // Current: x = -w * progress
        // Next:    x =  w * (1 - progress)
        // Direction == -1 (Next enters from Left):
        // Current: x =  w * progress
        // Next:    x = -w * (1 - progress)
        const double currentX = -m_direction * w * m_progress;
        const double nextX    =  m_direction * w * (1.0 - m_progress);

        p.drawPixmap(QPointF(currentX, 0), m_currentPixmap);
        p.drawPixmap(QPointF(nextX, 0), m_nextPixmap);
    }

private:
    void finishAnimation() {
        m_isAnimating = false;
        setCurrentIndex(m_nextIndex);
        QWidget* active = widget(m_nextIndex);
        if (active) {
            active->setGeometry(0, 0, width(), height());
            active->show();
            active->raise();
        }
        m_currentPixmap = QPixmap();
        m_nextPixmap = QPixmap();
        update();
    }

    HighHzAnimator* m_animator{nullptr};
    QPixmap m_currentPixmap;
    QPixmap m_nextPixmap;
    int m_prevIndex{0};
    int m_nextIndex{0};
    int m_direction{1};
    double m_progress{0.0};
    bool m_isAnimating{false};
    bool m_isDark{true};
};

} // namespace masscan::ui
