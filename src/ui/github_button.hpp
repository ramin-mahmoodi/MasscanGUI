#pragma once

#include <QAbstractButton>
#include <QPainter>
#include <QPainterPath>
#include <QEvent>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QFontMetrics>
#include <cmath>
#include "icons.hpp"
#include "high_hz_animator.hpp"

namespace masscan::ui {

class GitHubButton : public QAbstractButton {
    Q_OBJECT

public:
    explicit GitHubButton(QWidget* parent = nullptr) : QAbstractButton(parent) {
        setObjectName("githubStarBtn");
        setStyleSheet(
            "QAbstractButton#githubStarBtn {"
            "  padding: 0px;"
            "  margin: 0px;"
            "  border: none;"
            "  background: transparent;"
            "}"
        );
        setFixedHeight(26);
        setCursor(Qt::PointingHandCursor);
        setToolTip("Star on GitHub (Click to view repository)");
        setAttribute(Qt::WA_Hover, true);
        setFocusPolicy(Qt::NoFocus);

        // Hover animator (160ms background and border ease)
        m_hoverAnim = new HighHzAnimator(this);
        m_hoverAnim->setDuration(160);
        m_hoverAnim->onProgress = [this](double p) {
            m_hoverProgress = m_hoverStart + (m_hoverTarget - m_hoverStart) * p;
            update();
        };

        // Spin animator (500ms matching CSS transition: transform 0.5s, fill 0.5s)
        // Animates 0.0 -> 1.0 on enter (0deg -> 360deg), and 1.0 -> 0.0 on leave (360deg -> 0deg)
        m_spinAnim = new HighHzAnimator(this);
        m_spinAnim->setDuration(500);
        m_spinAnim->onProgress = [this](double p) {
            m_spinProgress = m_spinStart + (m_spinTarget - m_spinStart) * p;
            m_starRotation = m_spinProgress * 360.0;
            m_starScale = 1.0 - 0.10 * std::sin(std::clamp(m_spinProgress, 0.0, 1.0) * 3.14159);
            update();
        };

        updateGeometry();
    }

    void setDark(bool dark) {
        if (m_isDark == dark) return;
        m_isDark = dark;
        update();
    }

    void setMode(int mode) {
        if (m_mode == mode) return;
        m_mode = mode;
        updateGeometry();
        update();
    }

    void setStarred(bool starred) {
        if (m_starred == starred) return;
        m_starred = starred;
        update();
    }

    void toggleStarred() {
        m_starred = !m_starred;
        m_spinStart = m_spinProgress;
        m_spinTarget = 1.0;
        m_spinAnim->start();
        update();
    }

    bool isStarred() const { return m_starred; }

    QSize sizeHint() const override {
        if (m_mode == 2) {
            return QSize(26, 26);
        } else if (m_mode == 1) {
            // Compact ("Star")
            return QSize(88, 26);
        }
        // Full ("Star on GitHub") - 160px ensures zero truncation
        return QSize(160, 26);
    }

    QSize minimumSizeHint() const override {
        return sizeHint();
    }

protected:
    void enterEvent(QEnterEvent*) override {
        m_hoverStart = m_hoverProgress;
        m_hoverTarget = 1.0;
        m_hoverAnim->start();

        m_spinStart = m_spinProgress;
        m_spinTarget = 1.0;
        m_spinAnim->start();
    }

    void leaveEvent(QEvent*) override {
        m_hoverStart = m_hoverProgress;
        m_hoverTarget = 0.0;
        m_hoverAnim->start();

        // Smoothly reverse spin and scale back to initial state (0deg)
        m_spinStart = m_spinProgress;
        m_spinTarget = 0.0;
        m_spinAnim->start();
    }

    void mousePressEvent(QMouseEvent* e) override {
        if (e->button() == Qt::LeftButton) {
            m_pressed = true;
            update();
        }
        QAbstractButton::mousePressEvent(e);
    }

    void mouseReleaseEvent(QMouseEvent* e) override {
        if (e->button() == Qt::LeftButton) {
            m_pressed = false;
            update();
        }
        QAbstractButton::mouseReleaseEvent(e);
    }

    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);

        const double w = width();
        const double h = height();
        const double cx = w / 2.0;
        const double cy = h / 2.0;

        // Button scale on click (matches .compact:active { transform: scale(0.96); })
        double btnScale = m_pressed ? 0.96 : 1.0;

        p.save();
        p.translate(cx, cy);
        p.scale(btnScale, btnScale);
        p.translate(-cx, -cy);

        QRectF boxRect(0.5, 0.5, w - 1.0, h - 1.0);
        QPainterPath path;
        double radius = (m_mode == 2) ? (h / 2.0) : 13.0;
        path.addRoundedRect(boxRect, radius, radius);

        // Color definitions matching user's CSS
        // Dark theme:
        // --btn-bg: #18181b, --btn-bg-hover: #27272a, --btn-bg-active: #323238
        // --btn-border: #3f3f46, --btn-border-hover: #71717a
        // --btn-text: #fafafa, --star-idle: #71717a, --star-active: #facc15
        // Light theme:
        // --btn-bg: #fff, --btn-bg-hover: #f1f1f3, --btn-bg-active: #e6e6ea
        // --btn-border: #d4d4d8, --btn-border-hover: #a1a1aa
        // --btn-text: #18181b, --star-idle: #a1a1aa, --star-active: #f59e0b
        QColor bgNormal = m_isDark ? QColor(0x18, 0x18, 0x1b) : QColor(0xff, 0xff, 0xff);
        QColor bgHover  = m_isDark ? QColor(0x27, 0x27, 0x2a) : QColor(0xf1, 0xf1, 0xf3);
        QColor bgActive = m_isDark ? QColor(0x32, 0x32, 0x38) : QColor(0xe6, 0xe6, 0xea);

        QColor borderNormal = m_isDark ? QColor(0x3f, 0x3f, 0x46) : QColor(0xd4, 0xd4, 0xd8);
        QColor borderHover  = m_isDark ? QColor(0x71, 0x71, 0x7a) : QColor(0xa1, 0xa1, 0xaa);

        QColor textColor = m_isDark ? QColor("#fafafa") : QColor("#18181b");

        QColor starIdle   = m_isDark ? QColor("#71717a") : QColor("#a1a1aa");
        QColor starActive = m_isDark ? QColor("#facc15") : QColor("#f59e0b");

        auto lerpColor = [](const QColor& c1, const QColor& c2, double t) {
            t = std::clamp(t, 0.0, 1.0);
            return QColor(
                static_cast<int>(c1.red() + (c2.red() - c1.red()) * t),
                static_cast<int>(c1.green() + (c2.green() - c1.green()) * t),
                static_cast<int>(c1.blue() + (c2.blue() - c1.blue()) * t),
                static_cast<int>(c1.alpha() + (c2.alpha() - c1.alpha()) * t)
            );
        };

        QColor curBg = m_pressed ? bgActive : lerpColor(bgNormal, bgHover, m_hoverProgress);
        QColor curBorder = lerpColor(borderNormal, borderHover, m_hoverProgress);

        p.fillPath(path, curBg);
        p.setPen(QPen(curBorder, 1.0));
        p.drawPath(path);

        qreal dpr = devicePixelRatioF();

        // ================= MODE 2: Icon Only (Circle) =================
        if (m_mode == 2) {
            // Spins Octocat icon 360deg and scales to 0.9 on hover matching:
            // .circle:hover .gh { transform: scale(0.9) rotate(360deg); }
            p.save();
            p.translate(cx, cy);
            p.rotate(m_starRotation);
            p.scale(m_starScale, m_starScale);
            p.translate(-cx, -cy);

            QPixmap ghPix = Icons::getFilledPixmap(Icons::GitHub, textColor, 16, dpr, 24.0, 24.0);
            qreal pw = ghPix.width() / ghPix.devicePixelRatio();
            qreal ph = ghPix.height() / ghPix.devicePixelRatio();
            QRectF ghRect((w - pw) / 2.0, (h - ph) / 2.0, pw, ph);
            p.drawPixmap(ghRect.toRect(), ghPix);
            p.restore();

            p.restore();
            return;
        }

        // ================= MODE 0 & 1: Pill (Full or Compact) =================
        double leftPad = (m_mode == 1) ? 9.0 : 12.0;
        QPixmap ghPix = Icons::getFilledPixmap(Icons::GitHub, textColor, 15, dpr, 24.0, 24.0);
        qreal ghW = ghPix.width() / ghPix.devicePixelRatio();
        qreal ghH = ghPix.height() / ghPix.devicePixelRatio();
        QRectF ghRect(leftPad, (h - ghH) / 2.0, ghW, ghH);
        p.drawPixmap(ghRect.toRect(), ghPix);

        // 2. Draw Text ("Star on GitHub" or "Star")
        QString labelText = (m_mode == 1) ? "Star" : "Star on GitHub";
        QFont font("Segoe UI", 9, QFont::Bold);
        font.setPixelSize(12);
        p.setFont(font);
        p.setPen(textColor);

        double textX = leftPad + ghW + ((m_mode == 1) ? 6.0 : 8.0);
        double starPad = (m_mode == 1) ? 9.0 : 12.0;
        double starSize = 14.0;
        double textW = w - textX - starPad - starSize - ((m_mode == 1) ? 6.0 : 8.0);

        QRectF textRect(textX, -1.5, textW, h);
        p.drawText(textRect, Qt::AlignCenter, labelText);

        // 3. Draw Star Icon (.star with 360deg rotation and active color transition)
        double starCenterX = w - starPad - (starSize / 2.0);
        double starCenterY = h / 2.0;

        // Star color: active gold if starred OR during spin
        double starColorT = (m_starred) ? 1.0 : std::clamp(m_spinProgress, 0.0, 1.0);
        QColor curStarColor = lerpColor(starIdle, starActive, starColorT);

        QPixmap starPix = Icons::getFilledPixmap(Icons::Star, curStarColor, 14, dpr, 48.0, 48.0);
        qreal stW = starPix.width() / starPix.devicePixelRatio();
        qreal stH = starPix.height() / starPix.devicePixelRatio();

        p.save();
        p.translate(starCenterX, starCenterY);
        p.rotate(m_starRotation);
        p.scale(m_starScale, m_starScale);
        p.translate(-starCenterX, -starCenterY);

        QRectF starRect(starCenterX - (stW / 2.0), starCenterY - (stH / 2.0), stW, stH);
        p.drawPixmap(starRect.toRect(), starPix);
        p.restore();

        p.restore();
    }

private:
    bool m_isDark{true};
    bool m_starred{false};
    bool m_pressed{false};
    int m_mode{0}; // 0 = Full, 1 = Compact, 2 = Icon
    double m_hoverProgress{0.0};
    double m_hoverStart{0.0};
    double m_hoverTarget{0.0};

    double m_spinProgress{0.0};
    double m_spinStart{0.0};
    double m_spinTarget{0.0};
    double m_starRotation{0.0};
    double m_starScale{1.0};

    HighHzAnimator* m_hoverAnim{nullptr};
    HighHzAnimator* m_spinAnim{nullptr};
};

} // namespace masscan::ui
