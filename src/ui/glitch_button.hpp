#pragma once

#include <QPushButton>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <algorithm>
#include <cmath>
#include "high_hz_animator.hpp"

namespace masscan::ui {

class GlitchButton : public QPushButton {
    Q_OBJECT

public:
    explicit GlitchButton(const QString& text, QWidget* parent = nullptr)
        : QPushButton(text, parent) {
        setCursor(Qt::PointingHandCursor);
        m_anim = new HighHzAnimator(this);
        m_anim->setDuration(1000); // 1s glitch

        m_anim->onProgress = [this](double p) {
            m_progress = p;
            syncOverlay();
        };

        m_anim->onFinished = [this]() {
            if (m_overlay) {
                m_overlay->hide();
            }
            update();
        };
    }

    ~GlitchButton() override {
        if (m_overlay) {
            m_overlay->deleteLater();
        }
    }

    // Trigger glitch manually (e.g. for screenshot verification)
    void triggerGlitchAt(double progress) {
        ensureOverlay();
        m_progress = progress;
        m_manualPreview = true;
        syncOverlay();
    }

protected:
    void enterEvent(QEnterEvent* event) override {
        QPushButton::enterEvent(event);
        if (isEnabled()) {
            m_manualPreview = false;
            ensureOverlay();
            m_anim->stop();
            m_progress = 0.0;
            m_anim->start();
            syncOverlay();
        }
    }

    void leaveEvent(QEvent* event) override {
        QPushButton::leaveEvent(event);
        m_manualPreview = false;
        m_anim->stop();
        if (m_overlay) {
            m_overlay->hide();
        }
        update();
    }

    void hideEvent(QHideEvent* event) override {
        QPushButton::hideEvent(event);
        if (m_overlay) {
            m_overlay->hide();
        }
    }

private:
    class OverlayWidget : public QWidget {
    public:
        explicit OverlayWidget(GlitchButton* owner, QWidget* topWin)
            : QWidget(topWin), m_owner(owner) {
            setAttribute(Qt::WA_TransparentForMouseEvents, true);
            setAttribute(Qt::WA_NoSystemBackground, true);
            setAttribute(Qt::WA_TranslucentBackground, true);
            hide();
        }

    protected:
        void paintEvent(QPaintEvent*) override {
            if (!m_owner) return;
            m_owner->paintGlitchOverlay(this);
        }

    private:
        GlitchButton* m_owner{nullptr};
    };

    static constexpr int kPadX = 32;
    static constexpr int kPadY = 20;

    void ensureOverlay() {
        QWidget* topWin = window();
        if (!topWin) return;
        if (!m_overlay || m_overlay->parentWidget() != topWin) {
            if (m_overlay) m_overlay->deleteLater();
            m_overlay = new OverlayWidget(this, topWin);
        }
    }

    void syncOverlay() {
        if (!m_overlay) return;
        QWidget* topWin = window();
        if (!topWin) return;

        const bool active = m_manualPreview || (m_anim->isRunning() && m_progress < 0.95);
        if (!active || !isEnabled() || !isVisible()) {
            if (m_overlay->isVisible()) {
                m_overlay->hide();
            }
            return;
        }

        const QPoint topLeftInWin = topWin->mapFromGlobal(mapToGlobal(QPoint(-kPadX, -kPadY)));
        const QRect newGeom(topLeftInWin.x(), topLeftInWin.y(), width() + kPadX * 2, height() + kPadY * 2);
        if (m_overlay->geometry() != newGeom) {
            m_overlay->setGeometry(newGeom);
        }
        if (!m_overlay->isVisible()) {
            m_overlay->raise();
            m_overlay->show();
        }
        m_overlay->update();
    }

    void paintGlitchOverlay(QWidget* overlay) {
        struct GlitchStep {
            int sliceIdx; // 1..5
            double dx;
            double dy;
        };

        // Exact keyframes from @keyframes glitch (0% .. 100%)
        static constexpr GlitchStep kSteps[] = {
            {1, -20.0, -10.0}, // 0%
            {3,  10.0,  10.0}, // 10%
            {1, -10.0,  10.0}, // 20%
            {3,   0.0,   5.0}, // 30%
            {2,  -5.0,   0.0}, // 40%
            {3,   5.0,   0.0}, // 50%
            {4,   5.0,  10.0}, // 60%
            {2, -10.0,  10.0}, // 70%
            {5,  20.0, -10.0}, // 80%
            {1, -10.0,   0.0}, // 90%
            {1,   0.0,   0.0}  // 100%
        };

        const double t = std::clamp(m_progress, 0.0, 1.0);
        if (t >= 0.98 && !m_manualPreview) {
            return;
        }

        // animation-timing-function: steps(2, end) across the 10 keyframe intervals
        const int seg = std::clamp(static_cast<int>(t * 10.0), 0, 9);
        const double localT = (t * 10.0) - seg;
        const bool secondHalf = (localT >= 0.5);

        const GlitchStep& s0 = kSteps[seg];
        const GlitchStep& s1 = kSteps[seg + 1];

        const int sliceIdx = secondHalf ? s1.sliceIdx : s0.sliceIdx;
        const double dx = secondHalf ? (s0.dx + s1.dx) * 0.5 : s0.dx;
        const double dy = secondHalf ? (s0.dy + s1.dy) * 0.5 : s0.dy;

        // Slice definitions (--slice-1 .. --slice-5): inset(top% -6px bottom% 0)
        double topPct = 0.50;
        double botPct = 0.50;
        switch (sliceIdx) {
            case 1: topPct = 0.72; botPct = 0.00; break; // --slice-1: inset(80% -6px 0 0)
            case 2: topPct = 0.45; botPct = 0.25; break; // --slice-2: inset(50% -6px 30% 0)
            case 3: topPct = 0.08; botPct = 0.72; break; // --slice-3: inset(10% -6px 85% 0)
            case 4: topPct = 0.35; botPct = 0.38; break; // --slice-4: inset(40% -6px 43% 0)
            case 5: topPct = 0.72; botPct = 0.05; break; // --slice-5: inset(80% -6px 5% 0)
            default: break;
        }

        const double w = width();
        const double h = height();

        // Colors matched to the blue primary button (#3b9eff / #52abff)
        const QColor btnBlue("#3b9eff");
        const QColor btnHoverBlue("#52abff");
        const QColor neonIceBlue("#8ce8ff");
        const QColor deepElectricBlue("#1559b8");

        QPainter p(overlay);
        p.setRenderHint(QPainter::Antialiasing, true);

        // Button origin inside the overlay widget is (kPadX, kPadY)
        // Apply transform: translate(dx, dy) to button::after!
        p.save();
        p.translate(kPadX + dx, kPadY + dy);

        // Clip to --slice-k: inset(top% -6px bottom% 0) in button::after's local space
        // Notice right inset is -6px -> width is w + 6.0!
        const double sliceTop = h * topPct;
        const double sliceBot = h * (1.0 - botPct);
        const double sliceH = std::max(4.0, sliceBot - sliceTop);
        const QRectF clipSlice(0.0, sliceTop, w + 6.0, sliceH);
        p.setClipRect(clipSlice);

        // Draw the shifted button::after body (with 6px right box-shadow overhang in electric ice-blue)
        QPainterPath shadowPath;
        shadowPath.addRoundedRect(QRectF(6.0, 0.0, w, h), 6.0, 6.0);
        p.fillPath(shadowPath, neonIceBlue);

        QPainterPath bodyPath;
        bodyPath.addRoundedRect(QRectF(0.0, 0.0, w, h), 6.0, 6.0);
        p.fillPath(bodyPath, btnHoverBlue);

        // Top/bottom slice tear highlight line in ice-blue
        p.fillRect(QRectF(0.0, sliceTop, w + 6.0, 1.5), neonIceBlue);
        p.fillRect(QRectF(0.0, sliceBot - 1.5, w + 6.0, 1.5), deepElectricBlue);

        // Draw text & icon inside the slice with blue-toned chromatic aberration shadow
        drawButtonContent(p, -2.5, -1.5, neonIceBlue, false);
        drawButtonContent(p,  2.5,  1.5, deepElectricBlue, false);
        drawButtonContent(p,  0.0,  0.0, Qt::white, true);

        p.restore();

        // When slice-1/3/5 is at top or bottom, also project a subtle middle text-slice tear
        // so the "Start Scan" text itself visibly glitches out of the button frame too
        if (sliceIdx == 1 || sliceIdx == 3 || sliceIdx == 5) {
            p.save();
            const double midDx = -dx * 0.65;
            const double midDy = dy * 0.35;
            p.translate(kPadX + midDx, kPadY + midDy);

            const QRectF midSlice(0.0, h * 0.28, w + 6.0, h * 0.44);
            p.setClipRect(midSlice);

            QPainterPath midShadow;
            midShadow.addRoundedRect(QRectF(4.0, 0.0, w, h), 6.0, 6.0);
            p.fillPath(midShadow, neonIceBlue);

            QPainterPath midBody;
            midBody.addRoundedRect(QRectF(0.0, 0.0, w, h), 6.0, 6.0);
            p.fillPath(midBody, btnBlue);

            drawButtonContent(p, -2.0, -1.0, neonIceBlue, false);
            drawButtonContent(p,  2.0,  1.0, deepElectricBlue, false);
            drawButtonContent(p,  0.0,  0.0, Qt::white, true);
            p.restore();
        }
    }

    void drawButtonContent(QPainter& p, double ox, double oy, const QColor& col, bool drawIconPixmap) {
        p.save();
        p.translate(ox, oy);
        p.setFont(font());
        p.setPen(col);

        const QFontMetrics fm(font());
        const QString label = text();
        const int iconW = icon().isNull() ? 0 : iconSize().width();
        const int spacing = icon().isNull() ? 0 : 6;
        const int totalW = iconW + spacing + fm.horizontalAdvance(label);
        const int startX = (width() - totalW) / 2;

        if (drawIconPixmap && !icon().isNull()) {
            const int iconY = (height() - iconSize().height()) / 2;
            icon().paint(&p, QRect(startX, iconY, iconSize().width(), iconSize().height()));
        }

        const QRect textRect(startX + iconW + spacing, 0, fm.horizontalAdvance(label) + 4, height());
        p.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, label);
        p.restore();
    }

    HighHzAnimator* m_anim{nullptr};
    QPointer<OverlayWidget> m_overlay;
    double m_progress{0.0};
    bool m_manualPreview{false};
};

} // namespace masscan::ui
