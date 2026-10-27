#pragma once

#include <QAbstractButton>
#include <QPainter>
#include <QMouseEvent>
#include <QEasingCurve>
#include <cmath>
#include "high_hz_animator.hpp"

namespace masscan::ui {

// Smooth morphing Hamburger (≡) <-> Close (✕) button + Dimmed Drawer Backdrop
class HamburgerButton : public QAbstractButton {
    Q_OBJECT

public:
    explicit HamburgerButton(QWidget* parent = nullptr)
        : QAbstractButton(parent),
          m_anim(this),
          m_curve(QEasingCurve::BezierSpline) {
        setFixedSize(34, 32);
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover, true);

        m_curve.addCubicBezierSegment(QPointF(0.65, 0.0), QPointF(0.35, 1.0), QPointF(1.0, 1.0));
        m_anim.setDuration(280);
        m_anim.onProgress = [this](double p) {
            const qreal eased = m_curve.valueForProgress(p);
            m_progress = m_startVal + (m_targetVal - m_startVal) * eased;
            update();
        };
    }

    void setProgress(qreal progress) {
        m_progress = std::clamp(progress, 0.0, 1.0);
        update();
    }

    void setOpen(bool open, bool animate = true) {
        if (m_open == open) return;
        m_open = open;
        qreal target = m_open ? 1.0 : 0.0;
        if (animate && isVisible()) {
            m_startVal = m_progress;
            m_targetVal = target;
            m_anim.start();
        } else {
            m_anim.stop();
            m_progress = target;
            update();
        }
    }

    [[nodiscard]] bool isOpen() const { return m_open; }

    void setDark(bool dark) {
        if (m_isDark != dark) {
            m_isDark = dark;
            update();
        }
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        const bool hovered = underMouse();
        const bool pressed = isDown();

        QColor bgCol = Qt::transparent;
        QColor borderCol = Qt::transparent;
        if (pressed) {
            bgCol = m_isDark ? QColor("#2a3340") : QColor("#d3dae4");
            borderCol = bgCol;
        } else if (hovered) {
            bgCol = m_isDark ? QColor("#1e2530") : QColor("#eef1f6");
            borderCol = m_isDark ? QColor("#2a3340") : QColor("#d3dae4");
        }

        if (bgCol.alpha() > 0) {
            p.setPen(QPen(borderCol, 1.0));
            p.setBrush(bgCol);
            p.drawRoundedRect(QRectF(0.5, 0.5, width() - 1.0, height() - 1.0), 6.0, 6.0);
        }

        QColor fgCol = m_isDark ? QColor("#e6ebf2") : QColor("#1a2230");
        QPen linePen(fgCol, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(linePen);

        const qreal cx = width() * 0.5;
        const qreal cy = height() * 0.5;
        const qreal halfLen = 7.5;
        const qreal offset = 5.2 * (1.0 - m_progress);
        const qreal angle = 45.0 * m_progress;

        // Top bar: slides to center and rotates +45 deg
        p.save();
        p.translate(cx, cy - offset);
        p.rotate(angle);
        p.drawLine(QPointF(-halfLen, 0.0), QPointF(halfLen, 0.0));
        p.restore();

        // Middle bar: shrinks toward center and fades out
        if (m_progress < 0.99) {
            p.save();
            p.setOpacity(1.0 - m_progress);
            qreal midHalf = halfLen * (1.0 - m_progress * 0.75);
            p.drawLine(QPointF(cx - midHalf, cy), QPointF(cx + midHalf, cy));
            p.restore();
        }

        // Bottom bar: slides to center and rotates -45 deg
        p.save();
        p.translate(cx, cy + offset);
        p.rotate(-angle);
        p.drawLine(QPointF(-halfLen, 0.0), QPointF(halfLen, 0.0));
        p.restore();
    }

private:
    bool m_open{false};
    bool m_isDark{true};
    qreal m_progress{0.0};
    qreal m_startVal{0.0};
    qreal m_targetVal{0.0};
    HighHzAnimator m_anim;
    QEasingCurve m_curve;
};

class DrawerBackdrop : public QWidget {
    Q_OBJECT

public:
    explicit DrawerBackdrop(QWidget* parent = nullptr)
        : QWidget(parent) {
        setAttribute(Qt::WA_StyledBackground, false);
        hide();
    }

    void setOpacity(qreal opacity) {
        m_opacity = std::clamp(opacity, 0.0, 1.0);
        update();
    }

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            emit clicked();
            event->accept();
            return;
        }
        QWidget::mousePressEvent(event);
    }

    void paintEvent(QPaintEvent*) override {
        if (m_opacity <= 0.001) return;
        QPainter p(this);
        int alpha = static_cast<int>(std::round(153.0 * m_opacity)); // #00000099 at 1.0
        p.fillRect(rect(), QColor(0, 0, 0, alpha));
    }

private:
    qreal m_opacity{0.0};
};

// Hardware-fast cached pixmap overlay for 0.1ms/frame drawer slide + backdrop fade animation.
// Avoids repainting 150+ child widgets in sidebar and rightColumn on every animation tick.
class DrawerAnimOverlay : public QWidget {
    Q_OBJECT

public:
    explicit DrawerAnimOverlay(QWidget* parent = nullptr)
        : QWidget(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_OpaquePaintEvent, true);
        hide();
    }

    void setPixmaps(const QPixmap& bg, const QPixmap& sidebar, int sideW) {
        m_bgPixmap = bg;
        m_sidebarPixmap = sidebar;
        m_sideW = sideW;
        m_cachedSideW = sideW;
    }

    void setSidebarPixmap(const QPixmap& sidebar, int sideW) {
        m_sidebarPixmap = sidebar;
        m_sideW = sideW;
        m_cachedSideW = sideW;
    }

    bool hasPixmaps() const {
        return !m_bgPixmap.isNull() && !m_sidebarPixmap.isNull();
    }

    bool hasBgPixmap() const {
        return !m_bgPixmap.isNull();
    }

    bool hasSidebarPixmap() const {
        return !m_sidebarPixmap.isNull() && m_cachedSideW == m_sideW;
    }

    void setBgPixmap(const QPixmap& bg) {
        m_bgPixmap = bg;
    }

    void invalidateSidebarPixmap() {
        m_sidebarPixmap = QPixmap();
    }

    void clearPixmaps() {
        m_bgPixmap = QPixmap();
        m_sidebarPixmap = QPixmap();
    }

    void setProgress(qreal progress) {
        m_progress = std::clamp(progress, 0.0, 1.0);
        repaint();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);

        if (!m_bgPixmap.isNull()) {
            p.drawPixmap(0, 0, m_bgPixmap);
        }

        const int alpha = static_cast<int>(std::round(153.0 * m_progress));
        if (alpha > 0) {
            p.fillRect(rect(), QColor(0, 0, 0, alpha));
        }

        if (!m_sidebarPixmap.isNull()) {
            const qreal closedX = -static_cast<qreal>(m_sideW) - 2.0;
            const qreal curX = closedX * (1.0 - m_progress);
            // Force QRasterPaintEngine out of its integer-snapping TxTranslate fast-path
            // so SmoothPixmapTransform performs true subpixel bilinear interpolation on every Hz tick!
            p.save();
            QTransform tf;
            tf.translate(curX, 0.0);
            tf.scale(1.00001, 1.0);
            p.setTransform(tf);
            p.drawPixmap(0, 0, m_sidebarPixmap);
            p.restore();
        }
    }

private:
    QPixmap m_bgPixmap;
    QPixmap m_sidebarPixmap;
    int m_sideW{380};
    int m_cachedSideW{0};
    qreal m_progress{0.0};
};

} // namespace masscan::ui
