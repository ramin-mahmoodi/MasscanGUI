#pragma once

#include <QPushButton>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QElapsedTimer>
#include <QEasingCurve>
#include <QEnterEvent>
#include <QFont>
#include <QFontMetrics>
#include <algorithm>
#include <cmath>

namespace masscan::ui {

enum class PpsState {
    Idle,
    Playing,
    Paused
};

// High-Hz Smooth Animation Helper using Qt::PreciseTimer (250Hz / 4ms tick)
// and nanosecond QElapsedTimer tracking for 60Hz-240Hz stutter-free motion.
class PpsSmoothAnimator : public QObject {
    Q_OBJECT

public:
    PpsSmoothAnimator(QWidget* parentWidget, int durationMs, const QEasingCurve& curve)
        : QObject(parentWidget), m_widget(parentWidget), m_durationMs(std::max(1, durationMs)), m_curve(curve) {
        m_timer.setTimerType(Qt::PreciseTimer);
        connect(&m_timer, &QTimer::timeout, this, &PpsSmoothAnimator::onTick);
    }

    void animateTo(qreal target) {
        if (std::abs(m_current - target) < 0.0005) {
            m_current = m_start = m_target = target;
            return;
        }
        m_start = m_current;
        m_target = target;
        m_elapsed.restart();
        if (!m_timer.isActive()) {
            m_timer.start(4); // 250 FPS precise update
        }
    }

    void snapTo(qreal val) noexcept {
        m_timer.stop();
        m_current = m_start = m_target = val;
        if (m_widget) m_widget->update();
    }

    [[nodiscard]] qreal value() const noexcept { return m_current; }

private:
    void onTick() {
        const qreal ms = m_elapsed.nsecsElapsed() / 1000000.0;
        if (ms >= static_cast<qreal>(m_durationMs)) {
            m_timer.stop();
            m_current = m_target;
        } else {
            const qreal progress = std::clamp(ms / static_cast<qreal>(m_durationMs), 0.0, 1.0);
            const qreal eased = m_curve.valueForProgress(progress);
            m_current = m_start + eased * (m_target - m_start);
        }
        if (m_widget) {
            m_widget->update();
        }
    }

    QWidget* m_widget{nullptr};
    QTimer m_timer;
    QElapsedTimer m_elapsed;
    int m_durationMs{500};
    QEasingCurve m_curve;
    qreal m_start{0.0};
    qreal m_target{0.0};
    qreal m_current{0.0};
};

class PpsMainButton : public QPushButton {
    Q_OBJECT

public:
    explicit PpsMainButton(QWidget* parent = nullptr)
        : QPushButton(parent) {
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover, true);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setFixedHeight(32);
        setFocusPolicy(Qt::NoFocus);
        setStyleSheet("background: transparent; border: none; padding: 0px; margin: 0px;");

        // Hover slide circles: 500ms duration with smooth OutCubic easing
        m_hoverAnim = new PpsSmoothAnimator(this, 500, QEasingCurve(QEasingCurve::OutCubic));

        // Play/Pause icon transition: 300ms smooth InOutCubic easing
        m_iconAnim = new PpsSmoothAnimator(this, 300, QEasingCurve(QEasingCurve::InOutCubic));

        updateLabel();
    }

    void setDarkTheme(bool dark) {
        if (m_isDarkTheme != dark) {
            m_isDarkTheme = dark;
            update();
        }
    }

    void setPpsState(PpsState s) {
        if (m_state == s) return;
        m_state = s;
        updateLabel();

        // Animate Play -> Pause (0.0 -> 1.0) or Pause -> Play (1.0 -> 0.0)
        m_iconAnim->animateTo(m_state == PpsState::Playing ? 1.0 : 0.0);
    }

    [[nodiscard]] PpsState ppsState() const noexcept { return m_state; }

protected:
    void enterEvent(QEnterEvent* event) override {
        QPushButton::enterEvent(event);
        if (!isEnabled()) return;
        m_hoverAnim->animateTo(1.0);
    }

    void leaveEvent(QEvent* event) override {
        QPushButton::leaveEvent(event);
        m_hoverAnim->animateTo(0.0);
    }

    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);

        const QRectF r = rect();
        const qreal w = r.width();
        const qreal h = r.height();
        constexpr qreal radius = 6.0;

        // Palette from CSS:
        // Light: --pps-green: #0a9f5c, --pps-green-active: teal (#008080), --pps-hover-fg: #1a2030
        // Dark:  --pps-green: mediumspringgreen (#00fa9a), --pps-hover-fg: #0f1a14
        const QColor cGreen = m_isDarkTheme ? QColor("#00fa9a") : QColor("#0a9f5c");
        const QColor cActive = QColor(0, 128, 128); // teal
        const QColor cBase = isDown() ? cActive : cGreen;
        const QColor cHoverFg = m_isDarkTheme ? QColor("#0f1a14") : QColor("#1a2030");

        const qreal hoverP = m_hoverAnim->value();
        const qreal iconP = m_iconAnim->value();

        auto lerpColor = [](const QColor& c1, const QColor& c2, qreal t) {
            return QColor::fromRgbF(
                c1.redF() + t * (c2.redF() - c1.redF()),
                c1.greenF() + t * (c2.greenF() - c1.greenF()),
                c1.blueF() + t * (c2.blueF() - c1.blueF()),
                c1.alphaF() + t * (c2.alphaF() - c1.alphaF())
            );
        };
        const QColor curFg = lerpColor(cBase, cHoverFg, hoverP);

        if (!isEnabled()) {
            p.setOpacity(0.45);
        }

        // 1. Clip path for rounded button (border-radius: 6px)
        QPainterPath clip;
        clip.addRoundedRect(r.adjusted(0.75, 0.75, -0.75, -0.75), radius, radius);
        p.save();
        p.setClipPath(clip);

        // 2. Sliding circles from both sides on hover
        // CSS: width: 110%, ::before left: 0 translateX(-100%) -> translateX(-4.5%)
        //                   ::after right: 0 translateX(100%)  -> translateX(4.5%)
        if (hoverP > 0.001) {
            const qreal circleD = std::max(w * 1.10, h * 2.5);
            const qreal circleR = circleD / 2.0;
            const qreal cy = h / 2.0;

            const qreal leftX = -circleD + hoverP * (circleD * 0.955);
            const qreal rightX = w - hoverP * (circleD * 0.955);

            QColor fillC = isDown() ? cActive : cGreen;
            fillC.setAlphaF(0.5f);
            p.setPen(Qt::NoPen);
            p.setBrush(fillC);

            p.drawEllipse(QRectF(leftX, cy - circleR, circleD, circleD));
            p.drawEllipse(QRectF(rightX, cy - circleR, circleD, circleD));
        }
        p.restore();

        // 3. Draw outline border (1.5px)
        QPen bPen(cBase, 1.5);
        bPen.setJoinStyle(Qt::RoundJoin);
        p.setPen(bPen);
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(r.adjusted(0.75, 0.75, -0.75, -0.75), radius, radius);

        // 4. Content layout: Icon (16x16) + Gap (6px) + Label (13px DemiBold)
        const QString txt = currentLabel();
        QFont f = font();
        f.setFamily("Segoe UI");
        f.setPixelSize(13);
        f.setWeight(QFont::DemiBold);
        p.setFont(f);

        QFontMetrics fm(f);
        const int txtW = fm.horizontalAdvance(txt);

        const qreal iconBoxSize = 16.0;
        const qreal gap = 6.0;
        const qreal totalW = iconBoxSize + gap + txtW;
        const qreal startX = std::max(10.0, (w - totalW) / 2.0);
        const qreal iconCenterX = startX + iconBoxSize / 2.0;
        const qreal centerY = h / 2.0;

        // Vector SVG Icons
        // Play: d="M7 4.5v15l12-7.5z" scaled to ~12x12
        static const auto makePlayPath = []() {
            QPainterPath path;
            path.moveTo(-3.5, -5.0);
            path.lineTo(-3.5, 5.0);
            path.lineTo(5.0, 0.0);
            path.closeSubpath();
            return path;
        };

        // Pause: two bars with rounded corners
        static const auto makePausePath = []() {
            QPainterPath path;
            path.addRoundedRect(QRectF(-4.2, -5.0, 2.8, 10.0), 0.8, 0.8);
            path.addRoundedRect(QRectF(1.4, -5.0, 2.8, 10.0), 0.8, 0.8);
            return path;
        };

        static const QPainterPath playPath = makePlayPath();
        static const QPainterPath pausePath = makePausePath();

        // Play icon (rotates 0 -> 90deg, fades 1 -> 0)
        if (iconP < 0.999) {
            p.save();
            p.translate(iconCenterX, centerY);
            p.rotate(iconP * 90.0);
            QColor ic = curFg;
            qreal op = std::clamp(1.0 - iconP / 0.7, 0.0, 1.0);
            ic.setAlphaF(ic.alphaF() * op);
            QPen pen(ic, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            p.setPen(pen);
            p.setBrush(Qt::NoBrush);
            p.drawPath(playPath);
            p.restore();
        }

        // Pause icon (rotates -90 -> 0deg, fades 0 -> 1)
        if (iconP > 0.001) {
            p.save();
            p.translate(iconCenterX, centerY);
            p.rotate((1.0 - iconP) * -90.0);
            QColor ic = curFg;
            qreal op = std::clamp((iconP - 0.3) / 0.7, 0.0, 1.0);
            ic.setAlphaF(ic.alphaF() * op);
            QPen pen(ic, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            p.setPen(pen);
            p.setBrush(Qt::NoBrush);
            p.drawPath(pausePath);
            p.restore();
        }

        // Draw Label Text
        const qreal textX = startX + iconBoxSize + gap;
        p.setPen(curFg);
        p.drawText(QRectF(textX, 0, w - textX - 4.0, h), Qt::AlignVCenter | Qt::AlignLeft, txt);
    }

private:
    void updateLabel() {
        setText(currentLabel());
        update();
    }

    [[nodiscard]] QString currentLabel() const {
        switch (m_state) {
            case PpsState::Idle:    return "Start Scan";
            case PpsState::Playing: return "Pause Scan";
            case PpsState::Paused:  return "Resume Scan";
        }
        return "Start Scan";
    }

    PpsState m_state{PpsState::Idle};
    bool m_isDarkTheme{true};
    PpsSmoothAnimator* m_hoverAnim{nullptr};
    PpsSmoothAnimator* m_iconAnim{nullptr};
};

class PpsIconButton : public QPushButton {
    Q_OBJECT

public:
    enum class Kind {
        Stop,
        Folder
    };

    explicit PpsIconButton(Kind kind, QWidget* parent = nullptr)
        : QPushButton(parent), m_kind(kind) {
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover, true);
        setFixedSize(32, 32);
        setFocusPolicy(Qt::NoFocus);
        setToolTip(kind == Kind::Stop ? "Stop Scan" : "Open Folder");
        setStyleSheet("background: transparent; border: none; padding: 0px; margin: 0px;");

        // Hover slide circles: 500ms duration with smooth OutCubic easing
        m_hoverAnim = new PpsSmoothAnimator(this, 500, QEasingCurve(QEasingCurve::OutCubic));
    }

    void setDarkTheme(bool dark) {
        if (m_isDarkTheme != dark) {
            m_isDarkTheme = dark;
            update();
        }
    }

protected:
    void enterEvent(QEnterEvent* event) override {
        QPushButton::enterEvent(event);
        if (!isEnabled()) return;
        m_hoverAnim->animateTo(1.0);
    }

    void leaveEvent(QEvent* event) override {
        QPushButton::leaveEvent(event);
        m_hoverAnim->animateTo(0.0);
    }

    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);

        const QRectF r = rect();
        const qreal w = r.width();
        const qreal h = r.height();
        constexpr qreal radius = 6.0;

        // Colors from CSS:
        // Stop:   --pps-red: #e5484d, --pps-red-active: #a61e24, hover fg: #ffffff
        // Folder: --pps-amber: #d97706 (light) / #fbbf24 (dark), active: #92400e, hover fg: #1a2030 / #0f1a14
        QColor cBase;
        QColor cActive;
        QColor cHoverFg;

        if (m_kind == Kind::Stop) {
            cBase = QColor("#e5484d");
            cActive = QColor("#a61e24");
            cHoverFg = QColor("#ffffff");
        } else {
            cBase = m_isDarkTheme ? QColor("#fbbf24") : QColor("#d97706");
            cActive = QColor("#92400e");
            cHoverFg = m_isDarkTheme ? QColor("#0f1a14") : QColor("#1a2030");
        }

        const QColor cCurrent = isDown() ? cActive : cBase;
        const qreal hoverP = m_hoverAnim->value();

        auto lerpColor = [](const QColor& c1, const QColor& c2, qreal t) {
            return QColor::fromRgbF(
                c1.redF() + t * (c2.redF() - c1.redF()),
                c1.greenF() + t * (c2.greenF() - c1.greenF()),
                c1.blueF() + t * (c2.blueF() - c1.blueF()),
                c1.alphaF() + t * (c2.alphaF() - c1.alphaF())
            );
        };
        const QColor curFg = lerpColor(cCurrent, cHoverFg, hoverP);

        if (!isEnabled()) {
            p.setOpacity(0.45);
        }

        // 1. Clip path for rounded border (radius 6px)
        QPainterPath clip;
        clip.addRoundedRect(r.adjusted(0.75, 0.75, -0.75, -0.75), radius, radius);
        p.save();
        p.setClipPath(clip);

        // 2. Sliding circles from both sides on hover (width 160%)
        // CSS: ::before translateX(-100%) -> translateX(-18.75%)
        //      ::after  translateX(100%)  -> translateX(18.75%)
        if (hoverP > 0.001) {
            const qreal circleD = w * 1.6;
            const qreal circleR = circleD / 2.0;
            const qreal cy = h / 2.0;

            const qreal leftX = -circleD + hoverP * (circleD * 0.8125);
            const qreal rightX = w - hoverP * (circleD * 0.8125);

            QColor fillC = isDown() ? cActive : cBase;
            fillC.setAlphaF(0.5f);
            p.setPen(Qt::NoPen);
            p.setBrush(fillC);

            p.drawEllipse(QRectF(leftX, cy - circleR, circleD, circleD));
            p.drawEllipse(QRectF(rightX, cy - circleR, circleD, circleD));
        }
        p.restore();

        // 3. Draw outline border (1.5px)
        QPen bPen(cCurrent, 1.5);
        bPen.setJoinStyle(Qt::RoundJoin);
        p.setPen(bPen);
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(r.adjusted(0.75, 0.75, -0.75, -0.75), radius, radius);

        // 4. Draw SVG Icon centered
        const qreal cx = w / 2.0;
        const qreal cy = h / 2.0;

        p.save();
        p.translate(cx, cy);

        QPen iconPen(curFg, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(iconPen);
        p.setBrush(Qt::NoBrush);

        if (m_kind == Kind::Stop) {
            // Stop icon: rect x="6" y="6" width="12" height="12" rx="1.5" scaled to ~10x10
            p.drawRoundedRect(QRectF(-4.75, -4.75, 9.5, 9.5), 1.2, 1.2);
        } else {
            // Folder icon: scaled to ~13x10
            QPainterPath fp;
            fp.moveTo(-6.5, -2.5);
            fp.lineTo(-6.5, -4.0);
            fp.arcTo(QRectF(-6.5, -5.0, 2.0, 2.0), 180, -90);
            fp.lineTo(-2.2, -5.0);
            fp.lineTo(-0.8, -3.0);
            fp.lineTo(5.5, -3.0);
            fp.arcTo(QRectF(4.5, -3.0, 2.0, 2.0), 90, -90);
            fp.lineTo(6.5, 4.0);
            fp.arcTo(QRectF(4.5, 3.0, 2.0, 2.0), 0, -90);
            fp.lineTo(-5.5, 5.0);
            fp.arcTo(QRectF(-6.5, 3.0, 2.0, 2.0), 270, -90);
            fp.closeSubpath();
            p.drawPath(fp);
        }
        p.restore();
    }

private:
    Kind m_kind;
    bool m_isDarkTheme{true};
    PpsSmoothAnimator* m_hoverAnim{nullptr};
};

} // namespace masscan::ui
