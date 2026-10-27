#pragma once

#include <QAbstractButton>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QPixmap>
#include <QEnterEvent>
#include <QCoreApplication>
#include <QLayout>
#include <algorithm>
#include <cmath>
#include "high_hz_animator.hpp"

namespace masscan::ui {

// Exact cubic-bezier(0.65, 0, 0.35, 1) evaluator matching CSS --trans-timing
inline double evalThemeBezier(double x) {
    x = std::clamp(x, 0.0, 1.0);
    double s = x;
    for (int i = 0; i < 8; ++i) {
        const double inv = 1.0 - s;
        const double bx = 3.0 * inv * inv * s * 0.65 + 3.0 * inv * s * s * 0.35 + s * s * s;
        const double dbx = 3.0 * inv * inv * 0.65 + 6.0 * inv * s * (0.35 - 0.65) + 3.0 * s * s * (1.0 - 0.35);
        if (std::abs(dbx) < 1e-6) break;
        s = std::clamp(s - (bx - x) / dbx, 0.0, 1.0);
    }
    const double inv = 1.0 - s;
    return 3.0 * inv * s * s * 1.0 + s * s * s;
}

class ThemeToggleBtn : public QAbstractButton {
    Q_OBJECT

public:
    explicit ThemeToggleBtn(QWidget* parent = nullptr) : QAbstractButton(parent) {
        // 3.0em x 1.5em pill (52x26px to match the 26px header badges)
        setFixedSize(52, 26);
        setCursor(Qt::PointingHandCursor);
        setToolTip("Toggle dark / light theme");

        m_anim = new HighHzAnimator(this);
        m_anim->setDuration(400); // --trans-dur: 0.4s

        m_anim->onProgress = [this](double raw) {
            const double eased = evalThemeBezier(raw);
            m_t = m_startT + (m_targetT - m_startT) * eased;
            m_fadeAlpha = 1.0 - eased;
            if (raw < 1.0) {
                if (m_winOverlay && m_winOverlay->isVisible()) {
                    m_winOverlay->update();
                } else {
                    update();
                }
            }
        };

        m_anim->onFinished = [this]() {
            m_t = m_targetT;
            m_fadeAlpha = 0.0;
            if (m_winOverlay) {
                m_winOverlay->hide();
                m_beforeSnapshot = QPixmap();
                m_afterSnapshot = QPixmap();
            }
            update();
        };
    }

    ~ThemeToggleBtn() override {
        if (m_winOverlay) {
            m_winOverlay->deleteLater();
        }
    }

    // Step 1: Call before applying the new stylesheet
    void captureBeforeTheme(QWidget* topWin) {
        if (!topWin || !topWin->isVisible()) return;
        m_anim->stop();
        if (m_winOverlay) {
            m_winOverlay->hide();
        }
        m_capturingSnapshot = true;
        m_beforeSnapshot = topWin->grab();
        m_capturingSnapshot = false;
    }

    // Step 2: Update target state
    void setDark(bool dark) {
        m_isDark = dark;
        const double target = dark ? 1.0 : 0.0;
        if (!m_initialized || !isVisible()) {
            m_initialized = true;
            m_anim->stop();
            m_t = target;
            m_startT = target;
            m_targetT = target;
            if (m_winOverlay) m_winOverlay->hide();
            update();
            return;
        }
        m_startT = m_t;
        m_targetT = target;
    }

    // Step 3: Call after the new stylesheet & assets are applied to start the 90Hz+ opaque cross-fade
    void startThemeTransition(QWidget* topWin) {
        if (!topWin || !topWin->isVisible() || m_beforeSnapshot.isNull()) {
            m_t = m_targetT;
            update();
            return;
        }
        // Flush any queued Polish/LayoutRequest events from setStyleSheet() synchronously
        // so m_afterSnapshot has the exact final widget positions (zero 1px jump on finish)
        QCoreApplication::sendPostedEvents(nullptr, QEvent::Polish);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::PolishRequest);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
        if (topWin->layout()) {
            topWin->layout()->activate();
        }

        // Grab clean snapshot without static switch baked in (avoids double-composited pill border)
        m_capturingSnapshot = true;
        m_afterSnapshot = topWin->grab();
        m_capturingSnapshot = false;

        if (!m_winOverlay || m_winOverlay->parentWidget() != topWin) {
            if (m_winOverlay) m_winOverlay->deleteLater();
            m_winOverlay = new WindowFadeOverlay(this, topWin);
        }
        m_winOverlay->setGeometry(topWin->rect());
        m_fadeAlpha = 1.0;
        m_winOverlay->raise();
        m_winOverlay->show();
        m_anim->start();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        if (m_capturingSnapshot) {
            return;
        }
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        paintSwitch(p);
    }

private:
    class WindowFadeOverlay : public QWidget {
    public:
        explicit WindowFadeOverlay(ThemeToggleBtn* owner, QWidget* topWin)
            : QWidget(topWin), m_owner(owner) {
            setAttribute(Qt::WA_TransparentForMouseEvents, true);
            setAttribute(Qt::WA_OpaquePaintEvent, true);
            hide();
        }

    protected:
        void paintEvent(QPaintEvent*) override {
            if (m_owner) m_owner->paintWindowFade(this);
        }

    private:
        ThemeToggleBtn* m_owner{nullptr};
    };

    void paintWindowFade(QWidget* overlay) {
        QPainter p(overlay);
        // 1. Fast SIMD blit of the new theme snapshot (opaque)
        if (!m_afterSnapshot.isNull()) {
            p.setOpacity(1.0);
            p.drawPixmap(0, 0, m_afterSnapshot);
        }
        // 2. Fast SIMD alpha-blend of the old theme snapshot (no clipPath so AVX2 fast-path is used)
        if (!m_beforeSnapshot.isNull() && m_fadeAlpha > 0.001) {
            p.setOpacity(m_fadeAlpha);
            p.drawPixmap(0, 0, m_beforeSnapshot);
        }
        // 3. Draw the live morphing switch on top at its exact global->overlay mapped position
        p.setOpacity(1.0);
        p.setRenderHint(QPainter::Antialiasing, true);
        const QPoint btnPos = overlay->mapFromGlobal(mapToGlobal(QPoint(0, 0)));
        p.translate(btnPos);
        paintSwitch(p);
    }

    void paintSwitch(QPainter& p) {
        const double t = std::clamp(m_t, 0.0, 1.0);
        const double em = std::min(width() / 3.0, height() / 1.5);

        // Colors from morphing-theme-switch (--hue: 223)
        // hsl(223, 10%, 10%) = #17181c, hsl(223, 10%, 90%) = #e3e4e8
        const QColor colDark(23, 24, 28);
        const QColor colLight(227, 228, 232);

        // 1. Outer pill (.switch__input): 3em x 1.5em, radius 0.75em
        const QColor trackBg = lerpColor(colDark, colLight, t);
        const QRectF pillRect(0.0, 0.0, 3.0 * em, 1.5 * em);
        p.setPen(Qt::NoPen);
        p.setBrush(trackBg);
        p.drawRoundedRect(pillRect, 0.75 * em, 0.75 * em);

        // 2. Sliding circular icon container (.switch__icon)
        // top: 0.125em, left: 0.125em + 1.5em * t, size: 1.25em x 1.25em
        const double knobRadius = 0.625 * em;
        const double cx = (0.125 + 0.625 + 1.5 * t) * em;
        const double cy = (0.125 + 0.625) * em;

        p.save();
        p.translate(cx, cy);

        // Clip to the 1.25em circle (overflow: hidden on .switch__icon)
        QPainterPath knobClip;
        knobClip.addEllipse(QPointF(0.0, 0.0), knobRadius, knobRadius);
        p.setClipPath(knobClip);

        // .switch__icon + .switch__icon-part--1 background: hsl(var(--hue), 10%, 90%)
        p.setPen(Qt::NoPen);
        p.setBrush(colLight);
        p.drawEllipse(QPointF(0.0, 0.0), knobRadius, knobRadius);

        // .switch__icon-part--2: 0.5em dark circle (crescent cutout)
        const double c2x = 0.1875 * t * em;
        const double c2y = -0.1875 * t * em;
        const double s2  = 0.2 + (1.0 - 0.2) * t;
        const double r2  = 0.25 * s2 * em;
        p.setBrush(colDark);
        p.drawEllipse(QPointF(c2x, c2y), r2, r2);

        // .switch__icon-part--3: 1.25em circle with inset dark ring (OddEvenFill)
        const double s3 = 0.25 + (1.0 - 0.25) * t;
        const double outerR3 = 0.625 * s3 * em;
        const double innerR3 = (0.375 * t) * s3 * em;

        QPainterPath part3;
        part3.setFillRule(Qt::OddEvenFill);
        part3.addEllipse(QPointF(0.0, 0.0), outerR3, outerR3);
        if (innerR3 > 0.001) {
            part3.addEllipse(QPointF(0.0, 0.0), innerR3, innerR3);
        }
        p.setBrush(colDark);
        p.drawPath(part3);

        // .switch__icon-part--4 .. 11: 8 sun rays (angles 0, 45, ..., 315)
        const double rayScale = 1.0 - t;
        if (rayScale > 0.001) {
            const double rayDist = (0.25 + (0.625 - 0.25) * t) * em;
            const double rayW = 0.125 * em;
            const double rayH = 0.1875 * em;
            const double rayR = 0.0625 * em;

            p.setBrush(colDark);
            for (int i = 0; i < 8; ++i) {
                p.save();
                p.rotate(i * 45.0);
                p.translate(0.0, rayDist);
                p.scale(rayScale, rayScale);
                p.drawRoundedRect(QRectF(-rayW * 0.5, 0.0, rayW, rayH), rayR, rayR);
                p.restore();
            }
        }

        p.restore();
    }

    static QColor lerpColor(const QColor& a, const QColor& b, double t) {
        return QColor(
            static_cast<int>(std::round(a.red()   + (b.red()   - a.red())   * t)),
            static_cast<int>(std::round(a.green() + (b.green() - a.green()) * t)),
            static_cast<int>(std::round(a.blue()  + (b.blue()  - a.blue())  * t))
        );
    }

    HighHzAnimator* m_anim{nullptr};
    QPointer<WindowFadeOverlay> m_winOverlay;
    QPixmap m_beforeSnapshot;
    QPixmap m_afterSnapshot;
    double m_fadeAlpha{0.0};
    double m_t{1.0};
    double m_startT{1.0};
    double m_targetT{1.0};
    bool m_isDark{true};
    bool m_initialized{false};
    bool m_capturingSnapshot{false};
};

} // namespace masscan::ui
