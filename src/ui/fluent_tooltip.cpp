#include "fluent_tooltip.hpp"
#include "high_hz_animator.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QGuiApplication>
#include <QScreen>
#include <QFontMetrics>
#include <QHelpEvent>
#include <QAbstractItemView>
#include <algorithm>
#include <cmath>

namespace masscan::ui {

FluentToolTip* FluentToolTip::s_instance = nullptr;

FluentToolTip* FluentToolTip::instance() {
    if (!s_instance) {
        s_instance = new FluentToolTip(nullptr);
    }
    return s_instance;
}

FluentToolTip::FluentToolTip(QWidget* parent)
    : QWidget(parent, Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint) {

    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_StyledBackground, false);

    m_inAnim = new HighHzAnimator(this);
    m_inAnim->setDuration(140);
    m_inAnim->onProgress = [this](double p) {
        // easeOutCubic: snappy appearance with silky deceleration
        double eased = 1.0 - std::pow(1.0 - p, 3.0);
        m_opacity = eased;
        int curY = m_targetPos.y() + static_cast<int>(std::round(4.0 * (1.0 - eased)));
        move(m_targetPos.x(), curY);
        update();
    };

    m_outAnim = new HighHzAnimator(this);
    m_outAnim->setDuration(90);
    m_outAnim->onProgress = [this](double p) {
        m_opacity = m_outStartOpacity * (1.0 - p);
        update();
    };
    m_outAnim->onFinished = [this]() {
        hide();
        m_anchor = nullptr;
    };
}

FluentToolTip::~FluentToolTip() {
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

void FluentToolTip::setDark(bool isDark) {
    if (m_isDark != isDark) {
        m_isDark = isDark;
        update();
    }
}

void FluentToolTip::showToolTip(QWidget* anchor, const QString& text, const QPoint& globalPos) {
    if (text.isEmpty()) return;

    if (m_outAnim->isRunning()) {
        m_outAnim->stop();
    }

    m_anchor = anchor;
    m_text = text;
    m_triggerPos = globalPos;

    // Measure text
    QFont font("Segoe UI");
    font.setPixelSize(12);
    font.setWeight(QFont::Medium);
    QFontMetrics fm(font);

    QRect textRect = fm.boundingRect(QRect(0, 0, 360, 1000), Qt::TextWordWrap, m_text);
    const int padH = 10;
    const int padV = 5;
    const int shadowMargin = 6;

    int innerW = textRect.width() + padH * 2;
    int innerH = textRect.height() + padV * 2;
    int totalW = innerW + shadowMargin * 2;
    int totalH = innerH + shadowMargin * 2;

    setFixedSize(totalW, totalH);

    // Position relative to cursor
    int x = globalPos.x() - totalW / 2;
    int y = globalPos.y() + 20;

    QScreen* screen = QGuiApplication::screenAt(globalPos);
    if (!screen) screen = QGuiApplication::primaryScreen();
    QRect avail = screen ? screen->availableGeometry() : QRect(0, 0, 1920, 1080);

    x = std::clamp(x, avail.left() + 6, avail.right() - totalW - 6);
    if (y + totalH > avail.bottom() - 6) {
        // Flip above cursor if overflowing screen bottom
        y = globalPos.y() - totalH - 8;
    }
    m_targetPos = QPoint(x, y);

    m_opacity = 0.0;
    move(m_targetPos.x(), m_targetPos.y() + 4);
    show();
    m_inAnim->start();
}

void FluentToolTip::hideToolTip(bool immediate) {
    if (!isVisible() && !m_inAnim->isRunning()) return;

    if (m_inAnim->isRunning()) {
        m_inAnim->stop();
    }

    if (immediate) {
        m_outAnim->stop();
        m_opacity = 0.0;
        hide();
        m_anchor = nullptr;
        return;
    }

    if (!m_outAnim->isRunning()) {
        m_outStartOpacity = m_opacity > 0.01 ? m_opacity : 1.0;
        m_outAnim->start();
    }
}

void FluentToolTip::paintEvent(QPaintEvent*) {
    if (m_opacity <= 0.001) return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.setOpacity(m_opacity);

    const qreal s = 6.0; // shadow margin
    QRectF innerRect(s, s, width() - 2.0 * s, height() - 2.0 * s);

    // 1. Soft multi-layer drop shadow (zero corner clipping)
    for (int i = 0; i < 3; ++i) {
        qreal spread = (3 - i) * 1.2;
        QRectF sr = innerRect.adjusted(-spread, -spread * 0.4 + 1.2, spread, spread * 0.4 + 1.2);
        int alpha = m_isDark ? (22 - i * 6) : (16 - i * 4);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, alpha));
        p.drawRoundedRect(sr, 6.0 + spread * 0.5, 6.0 + spread * 0.5);
    }

    // 2. Background and 1px crisp border
    QColor bg = m_isDark ? QColor("#1e2530") : QColor("#ffffff");
    QColor border = m_isDark ? QColor("#364254") : QColor("#d3dae4");

    p.setPen(QPen(border, 1.0));
    p.setBrush(bg);
    p.drawRoundedRect(innerRect, 6.0, 6.0);

    // 3. Text
    QColor textCol = m_isDark ? QColor("#e6ebf2") : QColor("#1a2230");
    p.setPen(textCol);
    QFont font("Segoe UI");
    font.setPixelSize(12);
    font.setWeight(QFont::Medium);
    p.setFont(font);

    QRectF textRect = innerRect.adjusted(10.0, 5.0, -10.0, -5.0);
    p.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, m_text);
}

// ---------------- FluentToolTipFilter ----------------
FluentToolTipFilter::FluentToolTipFilter(QObject* parent) : QObject(parent) {}

bool FluentToolTipFilter::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::ToolTip) {
        auto* he = static_cast<QHelpEvent*>(event);
        auto* w = qobject_cast<QWidget*>(watched);
        if (!w) return false;

        QString tip;
        // Check if item view (e.g. QListWidget, QTableWidget item tooltip)
        if (auto* iv = qobject_cast<QAbstractItemView*>(w)) {
            QModelIndex idx = iv->indexAt(he->pos());
            if (idx.isValid()) {
                tip = idx.data(Qt::ToolTipRole).toString();
            }
        }
        if (tip.isEmpty()) {
            QWidget* cur = w;
            while (cur && tip.isEmpty()) {
                tip = cur->toolTip();
                cur = cur->parentWidget();
            }
        }

        if (!tip.isEmpty()) {
            FluentToolTip::instance()->showToolTip(w, tip, he->globalPos());
            return true; // suppress default rectangular Qt tooltip window!
        }
        return false;
    }

    if (event->type() == QEvent::Leave) {
        if (watched == FluentToolTip::instance()->currentAnchor()) {
            FluentToolTip::instance()->hideToolTip();
        }
    } else if (event->type() == QEvent::MouseButtonPress ||
               event->type() == QEvent::MouseButtonDblClick ||
               event->type() == QEvent::Wheel) {
        FluentToolTip::instance()->hideToolTip(true);
    } else if (event->type() == QEvent::WindowDeactivate ||
               event->type() == QEvent::WindowStateChange) {
        FluentToolTip::instance()->hideToolTip(true);
    }

    return QObject::eventFilter(watched, event);
}

} // namespace masscan::ui
