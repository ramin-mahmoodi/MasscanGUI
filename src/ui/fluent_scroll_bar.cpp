#include "fluent_scroll_bar.hpp"

#include <QScrollArea>
#include <QGuiApplication>
#include <QScreen>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPointer>
#include <QCursor>
#include <algorithm>
#include <cmath>

namespace masscan::ui {

namespace {
// Slint Flickable SCROLL_FILTER_DURATION (800ms): prevents nested child lists (e.g. countryList)
// from hijacking wheel events when they slide underneath a stationary mouse cursor while
// an ancestor ScrollArea (the sidebar) is being scrolled.
QPointer<QAbstractScrollArea> s_activeScrollOwner;
QElapsedTimer s_activeScrollTimer;
QPoint s_activeScrollCursorPos;

bool isAncestorScrollArea(const QWidget* ancestor, const QWidget* child) {
    if (!ancestor || !child || ancestor == child) return false;
    const QWidget* p = child->parentWidget();
    while (p) {
        if (p == ancestor) return true;
        p = p->parentWidget();
    }
    return false;
}
} // namespace

FluentScrollBarOverlay::FluentScrollBarOverlay(QAbstractScrollArea* area)
    : QWidget(area),
      m_area(area) {

    Q_ASSERT(m_area);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setMouseTracking(true);

    // Hide native vertical scrollbar from layout
    m_area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_area->viewport()->installEventFilter(this);
    m_area->installEventFilter(this);

    // Width animation (2px idle -> 6px hover) synced to native high refresh rate
    m_widthAnim = new HighHzAnimator(this);
    m_widthAnim->setDuration(150);
    m_widthAnim->onProgress = [this](double u) {
        const double eased = 2.0 * u - u * u; // OutQuad
        m_currentWidth = m_widthStart + (m_widthTarget - m_widthStart) * eased;
        update();
    };

    // Use a 4ms high-precision multimedia timer tick so integer pixel transitions
    // are always ready before every 90Hz/120Hz/144Hz VSync boundary without beat drift.
    m_smoothTimer.setInterval(4);
    m_smoothTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_smoothTimer, &QTimer::timeout, this, &FluentScrollBarOverlay::onSmoothTimerTick);

    // Listen to scroll range and value changes
    auto* bar = m_area->verticalScrollBar();
    connect(bar, &QScrollBar::valueChanged, this, [this](int) {
        update();
    });
    connect(bar, &QScrollBar::rangeChanged, this, [this](int, int) {
        updateScrollbarGeometry();
        setVisible(maxScroll() > 0);
        update();
    });

    updateScrollbarGeometry();
    setVisible(maxScroll() > 0);
}

void FluentScrollBarOverlay::animateWidthTo(qreal targetWidth) {
    m_widthAnim->stop();
    m_widthStart = m_currentWidth;
    m_widthTarget = targetWidth;
    m_widthAnim->start();
}

void FluentScrollBarOverlay::setDark(bool isDark) {
    m_isDark = isDark;
    update();
}

void FluentScrollBarOverlay::updateScrollbarGeometry() {
    if (!m_area) return;
    QWidget* vp = m_area->viewport();
    QRect vpRect = vp ? vp->geometry() : m_area->rect();
    int h = vpRect.height();
    int w = 14;
    int x = vpRect.right() - w + 1;
    int y = vpRect.top();
    const QRect newGeom(x, y, w, h);
    if (geometry() != newGeom) {
        setGeometry(newGeom);
    }
    raise();
}

int FluentScrollBarOverlay::scrollValue() const {
    return m_area && m_area->verticalScrollBar() ? m_area->verticalScrollBar()->value() : 0;
}

int FluentScrollBarOverlay::maxScroll() const {
    return m_area && m_area->verticalScrollBar() ? m_area->verticalScrollBar()->maximum() : 0;
}

int FluentScrollBarOverlay::pageSize() const {
    return m_area && m_area->verticalScrollBar() ? m_area->verticalScrollBar()->pageStep() : 20;
}

qreal FluentScrollBarOverlay::trackSize() const {
    // 16px offset at top and bottom for chevron buttons
    return std::max<qreal>(0.0, height() - 32.0);
}

qreal FluentScrollBarOverlay::thumbHeight() const {
    if (maxScroll() <= 0) return 0.0;
    qreal ps = pageSize();
    qreal maxS = maxScroll();
    qreal ts = trackSize();
    qreal h = ts * (ps / (maxS + ps));
    return std::max<qreal>(16.0, std::min<qreal>(ts, h));
}

QRectF FluentScrollBarOverlay::thumbRect() const {
    if (maxScroll() <= 0) return QRectF();
    qreal w = m_currentWidth;
    qreal x = width() - 4.0 - w;
    qreal th = thumbHeight();
    qreal ts = trackSize();
    qreal ratio = static_cast<qreal>(scrollValue()) / static_cast<qreal>(maxScroll());
    qreal y = 16.0 + (ts - th) * ratio;
    return QRectF(x, y, w, th);
}

QRectF FluentScrollBarOverlay::upButtonRect() const {
    return QRectF(0, 0, width(), 16);
}

QRectF FluentScrollBarOverlay::downButtonRect() const {
    return QRectF(0, height() - 16, width(), 16);
}

void FluentScrollBarOverlay::setScrollValue(int val) {
    if (m_area && m_area->verticalScrollBar()) {
        val = std::clamp(val, 0, maxScroll());
        if (m_area->verticalScrollBar()->value() != val) {
            m_area->verticalScrollBar()->setValue(val);
        }
    }
}

bool FluentScrollBarOverlay::handleWheelDelta(int angleDeltaY) {
    const int maxS = maxScroll();
    if (maxS <= 0 || angleDeltaY == 0) return false;

    const QPoint globalCursor = QCursor::pos();

    // Slint flickable.rs SCROLL_FILTER_DURATION (800ms):
    // If an ancestor scroll area (e.g. sidebar) is actively scrolling and the user hasn't moved
    // the mouse cursor (> 3px), do NOT let this child list hijack the wheel event as it slides under the cursor.
    if (s_activeScrollOwner && isAncestorScrollArea(s_activeScrollOwner, m_area)) {
        if (s_activeScrollTimer.isValid() && s_activeScrollTimer.elapsed() < 800 &&
            (globalCursor - s_activeScrollCursorPos).manhattanLength() <= 3) {
            return false;
        }
    }

    const int curVal = scrollValue();
    if (m_smoothTimer.isActive()) {
        // Advance m_currentSubpixelPos to the exact current nanosecond timestamp BEFORE restarting
        // m_elapsedTimer so zero motion/velocity is lost between consecutive wheel notches!
        const double elapsed = m_elapsedTimer.nsecsElapsed() / 1000000.0;
        const double u = std::clamp(elapsed / 180.0, 0.0, 1.0);
        const double eased = 2.0 * u - u * u;
        m_currentSubpixelPos = m_animStartPos + m_animDistance * eased;
    } else {
        m_currentSubpixelPos = static_cast<double>(curVal);
        m_targetPos = curVal;
    }

    // If target is already at the boundary in the scroll direction, hand off immediately to parent scroll area
    if ((angleDeltaY > 0 && m_targetPos <= 0) || (angleDeltaY < 0 && m_targetPos >= maxS)) {
        return false;
    }

    // Exact Slint winitwindowadapter.rs line 1472: LineDelta * 60px per 120 notch
    const int delta = qRound((static_cast<double>(angleDeltaY) / 120.0) * 60.0);
    const int newTarget = std::clamp(m_targetPos - delta, 0, maxS);
    if (newTarget == m_targetPos && !m_smoothTimer.isActive()) {
        return false;
    }

    s_activeScrollOwner = m_area;
    s_activeScrollTimer.restart();
    s_activeScrollCursorPos = globalCursor;

    m_targetPos = newTarget;
    m_animStartPos = m_currentSubpixelPos;
    m_animDistance = static_cast<double>(m_targetPos) - m_animStartPos;
    m_elapsedTimer.restart();

    if (std::abs(m_animDistance) >= 0.5) {
        if (!m_smoothTimer.isActive()) {
            m_smoothTimer.start();
        }
    } else {
        m_currentSubpixelPos = static_cast<double>(m_targetPos);
        setScrollValue(m_targetPos);
        m_smoothTimer.stop();
    }
    return true;
}

void FluentScrollBarOverlay::onSmoothTimerTick() {
    const double elapsed = m_elapsedTimer.nsecsElapsed() / 1000000.0;
    const double u = std::clamp(elapsed / 180.0, 0.0, 1.0);

    // Exact Slint Constant Deceleration: OutQuad (2u - u^2)
    const double eased = 2.0 * u - u * u;
    m_currentSubpixelPos = m_animStartPos + m_animDistance * eased;
    const int nextVal = static_cast<int>(std::round(m_currentSubpixelPos));

    // Stop as soon as integer rounding reaches m_targetPos so there are zero dead tail frames
    if (u >= 1.0 || nextVal == m_targetPos) {
        m_currentSubpixelPos = static_cast<double>(m_targetPos);
        setScrollValue(m_targetPos);
        m_smoothTimer.stop();
    } else {
        setScrollValue(nextVal);
    }
}

void FluentScrollBarOverlay::enterEvent(QEnterEvent*) {
    m_hovered = true;
    animateWidthTo(6.0);
}

void FluentScrollBarOverlay::leaveEvent(QEvent*) {
    if (m_dragging) {
        return;
    }
    m_hovered = false;
    m_buttonHover = ButtonHover::None;
    animateWidthTo(2.0);
    update();
}

void FluentScrollBarOverlay::mouseMoveEvent(QMouseEvent* event) {
    qreal my = event->position().y();

    if (m_dragging) {
        qreal dy = my - m_dragStartY;
        qreal ts = trackSize();
        qreal th = thumbHeight();
        qreal avail = ts - th;
        if (avail > 0.0) {
            int newScroll = m_dragStartVal + qRound(dy * maxScroll() / avail);
            newScroll = std::clamp(newScroll, 0, maxScroll());
            m_currentSubpixelPos = static_cast<double>(newScroll);
            m_targetPos = newScroll;
            setScrollValue(newScroll);
        }
        return;
    }

    ButtonHover prev = m_buttonHover;
    if (upButtonRect().contains(event->position())) {
        m_buttonHover = ButtonHover::Up;
    } else if (downButtonRect().contains(event->position())) {
        m_buttonHover = ButtonHover::Down;
    } else {
        m_buttonHover = ButtonHover::None;
    }

    if (prev != m_buttonHover) {
        update();
    }
}

void FluentScrollBarOverlay::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) return;

    QPointF pt = event->position();
    if (upButtonRect().contains(pt)) {
        handleWheelDelta(120);
    } else if (downButtonRect().contains(pt)) {
        handleWheelDelta(-120);
    } else if (thumbRect().contains(pt)) {
        m_dragging = true;
        grabMouse();
        m_smoothTimer.stop();
        m_dragStartY = pt.y();
        m_dragStartVal = scrollValue();
        m_currentSubpixelPos = static_cast<double>(m_dragStartVal);
        m_targetPos = m_dragStartVal;
    } else {
        // Track jump
        QRectF tr = thumbRect();
        int currentPos = scrollValue();
        m_currentSubpixelPos = static_cast<double>(currentPos);
        m_targetPos = std::clamp(currentPos + (pt.y() < tr.top() ? -pageSize() : pageSize()), 0, maxScroll());
        m_animStartPos = m_currentSubpixelPos;
        m_animDistance = static_cast<double>(m_targetPos) - m_animStartPos;
        m_elapsedTimer.restart();
        if (std::abs(m_animDistance) >= 0.5) {
            if (!m_smoothTimer.isActive()) {
                m_smoothTimer.start();
            }
        }
    }
}

void FluentScrollBarOverlay::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (m_dragging) {
            m_dragging = false;
            releaseMouse();
            if (!rect().contains(mapFromGlobal(QCursor::pos()))) {
                m_hovered = false;
                m_buttonHover = ButtonHover::None;
                animateWidthTo(2.0);
                update();
            }
        }
    }
}

void FluentScrollBarOverlay::wheelEvent(QWheelEvent* event) {
    int dy = event->angleDelta().y();
    if (dy != 0 && handleWheelDelta(dy)) {
        event->accept();
    } else {
        event->ignore();
    }
}

bool FluentScrollBarOverlay::eventFilter(QObject* watched, QEvent* event) {
    if (!m_area) return false;

    if (watched == m_area->viewport() || watched == m_area) {
        if (event->type() == QEvent::Resize || event->type() == QEvent::Show) {
            updateScrollbarGeometry();
            setVisible(maxScroll() > 0);
        } else if (event->type() == QEvent::Wheel) {
            // Only handle on viewport so wheel events aren't processed twice on both viewport and m_area
            if (watched == m_area->viewport()) {
                auto* we = static_cast<QWheelEvent*>(event);
                int dy = we->angleDelta().y();
                if (dy != 0 && handleWheelDelta(dy)) {
                    we->accept();
                    return true;
                }
            }
        }
    }
    return false;
}

void FluentScrollBarOverlay::drawChevronUp(QPainter& p, const QRectF& r, const QColor& color) {
    p.save();
    p.setPen(Qt::NoPen);
    p.setBrush(color);

    // 8x6 filled triangle with soft rounded corners
    qreal cx = r.center().x();
    qreal cy = r.center().y();
    QPainterPath path;
    path.moveTo(cx, cy - 2.5);
    path.lineTo(cx + 3.5, cy + 2.5);
    path.lineTo(cx - 3.5, cy + 2.5);
    path.closeSubpath();
    p.drawPath(path);

    p.restore();
}

void FluentScrollBarOverlay::drawChevronDown(QPainter& p, const QRectF& r, const QColor& color) {
    p.save();
    p.setPen(Qt::NoPen);
    p.setBrush(color);

    qreal cx = r.center().x();
    qreal cy = r.center().y();
    QPainterPath path;
    path.moveTo(cx, cy + 2.5);
    path.lineTo(cx + 3.5, cy - 2.5);
    path.lineTo(cx - 3.5, cy - 2.5);
    path.closeSubpath();
    p.drawPath(path);

    p.restore();
}

void FluentScrollBarOverlay::paintEvent(QPaintEvent*) {
    if (maxScroll() <= 0) {
        return;
    }

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Hover background track (7px radius)
    if (m_hovered || m_currentWidth > 2.5) {
        QColor trackBg = m_isDark ? QColor("#2C2C2C") : QColor("#f0f0f0");
        p.setPen(Qt::NoPen);
        p.setBrush(trackBg);
        p.drawRoundedRect(rect(), 7.0, 7.0);

        // Buttons
        QColor btnNormal = m_isDark ? QColor(255, 255, 255, 75) : QColor(0, 0, 0, 95);
        QColor btnHover = m_isDark ? QColor("#e6ebf2") : QColor("#1a2230");

        drawChevronUp(p, upButtonRect(), m_buttonHover == ButtonHover::Up ? btnHover : btnNormal);
        drawChevronDown(p, downButtonRect(), m_buttonHover == ButtonHover::Down ? btnHover : btnNormal);
    }

    // Thumb pill (always drawn as long as maxScroll > 0)
    QRectF tr = thumbRect();
    if (!tr.isEmpty()) {
        QColor thumbCol;
        if (m_isDark) {
            thumbCol = m_hovered ? QColor(255, 255, 255, 140) : QColor(255, 255, 255, 55);
        } else {
            thumbCol = m_hovered ? QColor(0, 0, 0, 160) : QColor(0, 0, 0, 85);
        }
        p.setPen(Qt::NoPen);
        p.setBrush(thumbCol);
        p.drawRoundedRect(tr, tr.width() / 2.0, tr.width() / 2.0);
    }
}

} // namespace masscan::ui
