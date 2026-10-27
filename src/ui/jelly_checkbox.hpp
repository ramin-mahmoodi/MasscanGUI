#pragma once

#include <QCheckBox>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <algorithm>
#include <cmath>
#include "high_hz_animator.hpp"

namespace masscan::ui {

class JellyCheckBox : public QCheckBox {
    Q_OBJECT

public:
    explicit JellyCheckBox(const QString& text, QWidget* parent = nullptr)
        : QCheckBox(text, parent) {
        setCursor(Qt::PointingHandCursor);
        setMouseTracking(true);

        m_anim = new HighHzAnimator(this);
        m_anim->onProgress = [this](double p) {
            m_progress = p;
            update();
        };

        connect(this, &QCheckBox::toggled, this, [this](bool checked) {
            m_checking = checked;
            if (!isVisible()) {
                m_anim->stop();
                m_progress = 1.0;
                update();
                return;
            }
            m_anim->stop();
            m_anim->setDuration(checked ? 600 : 180);
            m_progress = 0.0;
            m_anim->start();
        });
    }

    QSize sizeHint() const override {
        QFontMetrics fm(font());
        int w = 3 + 18 + 8 + fm.horizontalAdvance(text()) + 4;
        int h = std::max(24, fm.height() + 6);
        return QSize(w, h);
    }

    QSize minimumSizeHint() const override {
        QFontMetrics fm(font());
        return QSize(40, std::max(24, fm.height() + 6));
    }

protected:
    bool hitButton(const QPoint& pos) const override {
        return rect().contains(pos);
    }

    void enterEvent(QEnterEvent* event) override {
        m_hovered = true;
        update();
        QCheckBox::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override {
        m_hovered = false;
        update();
        QCheckBox::leaveEvent(event);
    }

    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        const bool isDark = palette().color(QPalette::WindowText).lightness() > 128;
        const QColor accentCol    = isDark ? QColor("#3b9eff") : QColor("#1f73d6");
        const QColor uncheckedBg  = isDark
            ? (m_hovered ? QColor("#2c3340") : QColor("#252931"))
            : (m_hovered ? QColor("#eef2f7") : QColor("#ffffff"));
        const QColor uncheckedBdr = m_hovered
            ? accentCol
            : (isDark ? QColor("#8e96a2") : QColor("#c8ccd4"));

        double sx = 1.0;
        double sy = 1.0;
        double bgAlpha = isChecked() ? 1.0 : 0.0;
        double chkProgress = isChecked() ? 1.0 : 0.0;

        if (m_anim->isRunning()) {
            const double t = std::clamp(m_progress, 0.0, 1.0);
            if (m_checking) {
                // 1. Background transition: 0.1s out of 0.6s (t in [0, 0.1667])
                bgAlpha = easeOutCubic(std::clamp(t / 0.1667, 0.0, 1.0));

                // 2. @keyframes jelly (0.6s ease)
                jellyScale(t, sx, sy);

                // 3. .cbx:after transition: 0.3s ease with 0.15s delay (t in [0.25, 0.75])
                chkProgress = easeOutCubic(std::clamp((t - 0.25) / 0.50, 0.0, 1.0));
            } else {
                // Smooth uncheck transition
                bgAlpha = 1.0 - easeOutCubic(t);
                chkProgress = 1.0 - easeOutCubic(std::clamp(t / 0.75, 0.0, 1.0));
            }
        }

        // Indicator center (3px left margin so 1.25x jelly stretch never clips)
        const double boxSize = 18.0;
        const double cx = 3.0 + boxSize * 0.5;
        const double cy = height() * 0.5;

        p.save();
        p.translate(cx, cy);
        p.scale(sx, sy);

        const QRectF boxRect(-boxSize * 0.5, -boxSize * 0.5, boxSize, boxSize);

        // Unchecked base box
        if (bgAlpha < 1.0) {
            p.setOpacity(1.0);
            p.setPen(QPen(uncheckedBdr, 1.0));
            p.setBrush(uncheckedBg);
            p.drawRoundedRect(boxRect.adjusted(0.5, 0.5, -0.5, -0.5), 4.0, 4.0);
        }

        // Checked fill box (border-color: transparent)
        if (bgAlpha > 0.0) {
            p.setOpacity(bgAlpha);
            p.setPen(Qt::NoPen);
            p.setBrush(accentCol);
            p.drawRoundedRect(boxRect, 4.0, 4.0);
        }

        // Checkmark (.cbx:after -> rotate(45deg) scale(chkProgress))
        if (chkProgress > 0.001) {
            p.save();
            p.setOpacity(chkProgress);
            p.translate(0.0, -0.8);
            p.rotate(45.0);
            p.scale(chkProgress, chkProgress);

            const double cw = 4.6;
            const double ch = 9.2;
            QPainterPath checkPath;
            checkPath.moveTo(cw * 0.5, -ch * 0.5);
            checkPath.lineTo(cw * 0.5,  ch * 0.5);
            checkPath.lineTo(-cw * 0.5, ch * 0.5);

            QPen checkPen(Qt::white, 2.0, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
            p.setPen(checkPen);
            p.setBrush(Qt::NoBrush);
            p.drawPath(checkPath);
            p.restore();
        }

        p.restore();

        // Label text
        p.setOpacity(isEnabled() ? 1.0 : 0.55);
        p.setFont(font());
        p.setPen(palette().color(QPalette::WindowText));
        const int textLeft = 3 + static_cast<int>(boxSize) + 8;
        const QRect textRect(textLeft, 0, width() - textLeft, height());
        p.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text());
    }

private:
    static double easeOutCubic(double x) {
        const double inv = 1.0 - x;
        return 1.0 - inv * inv * inv;
    }

    static void jellyScale(double t, double& sx, double& sy) {
        struct Keyframe {
            double t;
            double sx;
            double sy;
        };
        static constexpr Keyframe kFrames[] = {
            {0.00, 1.00, 1.00},
            {0.30, 1.25, 0.75},
            {0.40, 0.75, 1.25},
            {0.50, 1.15, 0.85},
            {0.65, 0.95, 1.05},
            {0.75, 1.05, 0.95},
            {1.00, 1.00, 1.00}
        };
        constexpr int n = sizeof(kFrames) / sizeof(kFrames[0]);
        for (int i = 0; i < n - 1; ++i) {
            if (t >= kFrames[i].t && t <= kFrames[i + 1].t) {
                const double span = kFrames[i + 1].t - kFrames[i].t;
                const double localT = (span > 1e-6) ? (t - kFrames[i].t) / span : 0.0;
                // Smooth cosine ease between keyframes
                const double e = 0.5 - 0.5 * std::cos(localT * 3.14159265358979323846);
                sx = kFrames[i].sx + (kFrames[i + 1].sx - kFrames[i].sx) * e;
                sy = kFrames[i].sy + (kFrames[i + 1].sy - kFrames[i].sy) * e;
                return;
            }
        }
        sx = 1.0;
        sy = 1.0;
    }

    HighHzAnimator* m_anim{nullptr};
    double m_progress{1.0};
    bool m_checking{false};
    bool m_hovered{false};
};

} // namespace masscan::ui
