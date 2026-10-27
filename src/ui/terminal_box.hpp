#pragma once

#include <QWidget>
#include <QLabel>
#include <QAbstractButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QApplication>
#include <QClipboard>
#include <QToolTip>
#include <QEvent>
#include <QMouseEvent>
#include <cmath>
#include "icons.hpp"
#include "high_hz_animator.hpp"

namespace masscan::ui {

class TerminalCopyButton : public QAbstractButton {
    Q_OBJECT
public:
    explicit TerminalCopyButton(QWidget* parent = nullptr) : QAbstractButton(parent) {
        setObjectName("terminalCopyBtn");
        setStyleSheet(
            "QAbstractButton#terminalCopyBtn {"
            "  padding: 0px;"
            "  margin: 0px;"
            "  border: none;"
            "  background: transparent;"
            "  min-width: 28px;"
            "  max-width: 28px;"
            "  min-height: 28px;"
            "  max-height: 28px;"
            "}"
        );
        setFixedSize(28, 28);
        setCursor(Qt::PointingHandCursor);
        setToolTip("Copy command");
        setAttribute(Qt::WA_Hover, true);
        setFocusPolicy(Qt::NoFocus);

        m_hoverAnim = new HighHzAnimator(this);
        m_hoverAnim->setDuration(120);
        m_hoverAnim->onProgress = [this](double p) {
            double start = m_hoverTarget > 0.5 ? 0.0 : 1.0;
            m_hoverProgress = start + (m_hoverTarget - start) * p;
            update();
        };

        m_copyAnim = new HighHzAnimator(this);
        m_copyAnim->setDuration(320);
        m_copyAnim->onProgress = [this](double p) {
            m_copyProgress = p;
            update();
        };
    }

    void setDark(bool dark) {
        if (m_isDark == dark) return;
        m_isDark = dark;
        update();
    }

    void setCopied(bool copied) {
        if (m_copied == copied) return;
        m_copied = copied;
        setToolTip(m_copied ? "Copied!" : "Copy command");
        m_copyProgress = 0.0;
        m_copyAnim->start();
        update();
    }

    bool isCopied() const { return m_copied; }

    QSize sizeHint() const override { return QSize(28, 28); }
    QSize minimumSizeHint() const override { return QSize(28, 28); }

protected:
    void enterEvent(QEnterEvent*) override {
        m_hoverTarget = 1.0;
        m_hoverAnim->start();
    }

    void leaveEvent(QEvent*) override {
        m_hoverTarget = 0.0;
        m_hoverAnim->start();
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

        // 1. Draw static box frame (The box does NOT scale or wobble)
        QRectF boxRect(0.5, 0.5, w - 1.0, h - 1.0);
        QPainterPath path;
        path.addRoundedRect(boxRect, 6.0, 6.0);

        // Interpolate colors based on theme, hover, and copied state
        QColor normalBg = m_isDark ? QColor(32, 36, 42) : QColor(255, 255, 255);
        QColor hoverBg  = m_isDark ? QColor(44, 52, 62) : QColor(241, 245, 249);
        QColor copiedBg = QColor(34, 197, 94, 35);

        QColor normalBorder = m_isDark ? QColor(255, 255, 255, 28) : QColor(208, 215, 222);
        QColor hoverBorder  = m_isDark ? QColor(59, 158, 255, 200) : QColor(0, 106, 220, 200);
        QColor copiedBorder = QColor(34, 197, 94, 220);

        QColor normalIcon = m_isDark ? QColor("#8b97a8") : QColor("#57606a");
        QColor hoverIcon  = m_isDark ? QColor("#ffffff") : QColor("#0969da");
        QColor copiedIcon = QColor(34, 197, 94);

        auto lerpColor = [](const QColor& c1, const QColor& c2, double t) {
            t = std::clamp(t, 0.0, 1.0);
            return QColor(
                static_cast<int>(c1.red() + (c2.red() - c1.red()) * t),
                static_cast<int>(c1.green() + (c2.green() - c1.green()) * t),
                static_cast<int>(c1.blue() + (c2.blue() - c1.blue()) * t),
                static_cast<int>(c1.alpha() + (c2.alpha() - c1.alpha()) * t)
            );
        };

        QColor currentBg = lerpColor(normalBg, hoverBg, m_hoverProgress);
        QColor currentBorder = lerpColor(normalBorder, hoverBorder, m_hoverProgress);
        QColor currentIcon = lerpColor(normalIcon, hoverIcon, m_hoverProgress);

        if (m_copied) {
            currentBg = lerpColor(currentBg, copiedBg, std::min(1.0, m_copyProgress * 1.5));
            currentBorder = lerpColor(currentBorder, copiedBorder, std::min(1.0, m_copyProgress * 1.5));
            currentIcon = copiedIcon;
        }

        // Draw static button background and border
        p.fillPath(path, currentBg);
        p.setPen(QPen(currentBorder, 1.0));
        p.drawPath(path);

        // 2. Animations are applied ONLY to the icon inside!
        double iconScale = 1.0 + 0.10 * m_hoverProgress - (m_pressed ? 0.12 : 0.0);

        // Draw Icon with Spring Bounce Animation
        if (m_copied) {
            // Elastic spring overshoot: 0.0 -> 1.25 -> 1.0
            double t = std::clamp(m_copyProgress, 0.0, 1.0);
            double springScale = 1.0;
            if (t < 1.0) {
                springScale = 1.0 + std::sin(t * 3.14159 * 2.0) * 0.28 * (1.0 - t);
                springScale = std::max(0.0, springScale * t);
            }

            p.save();
            p.translate(cx, cy);
            p.scale(iconScale * springScale, iconScale * springScale);
            p.translate(-cx, -cy);

            QPixmap checkPix = Icons::getPixmap(Icons::Check, currentIcon, 15, devicePixelRatioF());
            qreal dpr = checkPix.devicePixelRatio();
            qreal pw = checkPix.width() / dpr;
            qreal ph = checkPix.height() / dpr;
            p.drawPixmap(QRectF((w - pw) / 2.0, (h - ph) / 2.0, pw, ph).toRect(), checkPix);
            p.restore();
        } else {
            p.save();
            p.translate(cx, cy);
            p.scale(iconScale, iconScale);
            p.translate(-cx, -cy);

            QPixmap copyPix = Icons::getPixmap(Icons::Copy, currentIcon, 15, devicePixelRatioF());
            qreal dpr = copyPix.devicePixelRatio();
            qreal pw = copyPix.width() / dpr;
            qreal ph = copyPix.height() / dpr;
            p.drawPixmap(QRectF((w - pw) / 2.0, (h - ph) / 2.0, pw, ph).toRect(), copyPix);
            p.restore();
        }
    }

private:
    bool m_isDark{true};
    bool m_copied{false};
    bool m_pressed{false};
    double m_hoverProgress{0.0};
    double m_hoverTarget{0.0};
    double m_copyProgress{1.0};
    HighHzAnimator* m_hoverAnim{nullptr};
    HighHzAnimator* m_copyAnim{nullptr};
};

class TerminalCodeBox : public QWidget {
    Q_OBJECT
public:
    explicit TerminalCodeBox(QWidget* parent = nullptr) : QWidget(parent) {
        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        // Header (.head: min-height 38px, subtle background, icon + title + copy button)
        auto* header = new QWidget(this);
        header->setFixedHeight(38);
        header->setAttribute(Qt::WA_StyledBackground, false);
        auto* hbLayout = new QHBoxLayout(header);
        hbLayout->setContentsMargins(12, 0, 10, 0);
        hbLayout->setSpacing(8);

        // Terminal icon in header (.title > svg)
        m_iconLabel = new QLabel(header);
        m_iconLabel->setFixedSize(18, 18);
        m_iconLabel->setAlignment(Qt::AlignCenter);

        // Title (.title)
        m_titleLabel = new QLabel("Command preview", header);
        m_titleLabel->setStyleSheet("font-family: \"Segoe UI\", sans-serif; font-size: 13px; font-weight: 600; color: #8b97a8;");

        // Copy button (.copy_toggle: animated, 100% square 28x28, checkmark spring pop)
        m_copyBtn = new TerminalCopyButton(header);

        connect(m_copyBtn, &QAbstractButton::clicked, this, [this]() {
            if (!m_command.trimmed().isEmpty()) {
                QApplication::clipboard()->setText(m_command);
            }
            m_copyBtn->setCopied(true);
            QTimer::singleShot(1400, this, [this]() {
                if (m_copyBtn) m_copyBtn->setCopied(false);
            });
        });

        hbLayout->addWidget(m_iconLabel, 0, Qt::AlignVCenter);
        hbLayout->addWidget(m_titleLabel, 0, Qt::AlignVCenter);
        hbLayout->addStretch(1);
        hbLayout->addWidget(m_copyBtn, 0, Qt::AlignVCenter);

        mainLayout->addWidget(header);

        // Terminal Body (.body: background #0e1117 / #13161c)
        auto* body = new QWidget(this);
        body->setAttribute(Qt::WA_StyledBackground, false);
        auto* bodyLayout = new QVBoxLayout(body);
        bodyLayout->setContentsMargins(12, 10, 12, 12);
        bodyLayout->setSpacing(0);

        m_cmdLabel = new QLabel(body);
        m_cmdLabel->setWordWrap(true);
        m_cmdLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        m_cmdLabel->setStyleSheet("background: transparent; border: none; font-family: \"Consolas\", monospace; font-size: 13px;");
        m_cmdLabel->setTextFormat(Qt::RichText);
        bodyLayout->addWidget(m_cmdLabel);

        mainLayout->addWidget(body, 1);

        // Cursor blink timer (500ms)
        m_blinkTimer = new QTimer(this);
        m_blinkTimer->setInterval(500);
        connect(m_blinkTimer, &QTimer::timeout, this, [this]() {
            m_cursorVisible = !m_cursorVisible;
            updateFormattedText();
        });
        m_blinkTimer->start();

        updateThemeVisuals();
        updateFormattedText();
    }

    void setDark(bool dark) {
        if (m_isDark == dark) return;
        m_isDark = dark;
        if (m_copyBtn) m_copyBtn->setDark(dark);
        updateThemeVisuals();
        updateFormattedText(true);
        update();
    }

    void setCommand(const QString& cmd) {
        if (m_command == cmd) return;
        m_command = cmd;
        updateFormattedText();
    }

    QString command() const { return m_command; }
    TerminalCopyButton* copyButton() const { return m_copyBtn; }
    QLabel* commandLabel() const { return m_cmdLabel; }

    bool hasHeightForWidth() const override { return true; }

    int heightForWidth(int w) const override {
        int innerW = std::max(60, w - 24);
        int labelH = m_cmdLabel ? m_cmdLabel->heightForWidth(innerW) : 20;
        if (labelH <= 0) {
            labelH = m_cmdLabel ? m_cmdLabel->sizeHint().height() : 20;
        }
        labelH = std::max(18, labelH);
        return 38 + 22 + labelH;
    }

    QSize sizeHint() const override {
        int labelH = m_cmdLabel ? m_cmdLabel->sizeHint().height() : 20;
        return QSize(300, 38 + 22 + std::max(18, labelH));
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        QRectF r = rect();
        QRectF boxRect = r.adjusted(0.5, 0.5, -0.5, -0.5);

        QPainterPath path;
        path.addRoundedRect(boxRect, 8.0, 8.0);

        // Terminal body background
        QColor bodyBg = m_isDark ? QColor(0x0e, 0x11, 0x17) : QColor(0xf8, 0xfa, 0xfc);
        p.fillPath(path, bodyBg);

        // Terminal header background
        p.save();
        p.setClipPath(path);
        QRectF headerRect(boxRect.left(), boxRect.top(), boxRect.width(), 38.0);
        QColor headerBg = m_isDark ? QColor(0x16, 0x1b, 0x22) : QColor(0xee, 0xf2, 0xf6);
        p.fillRect(headerRect, headerBg);

        // Divider under header
        QColor divCol = m_isDark ? QColor(255, 255, 255, 20) : QColor(0xd0, 0xd7, 0xde);
        p.setPen(QPen(divCol, 1.0));
        p.drawLine(QPointF(boxRect.left(), boxRect.top() + 38.0), QPointF(boxRect.right(), boxRect.top() + 38.0));
        p.restore();

        // Outer border
        QColor borderCol = m_isDark ? QColor(255, 255, 255, 26) : QColor(0xd0, 0xd7, 0xde);
        p.setPen(QPen(borderCol, 1.0));
        p.drawPath(path);
    }

private:
    void updateThemeVisuals() {
        QColor accentCol = m_isDark ? QColor("#3b9eff") : QColor("#006adc");
        if (m_iconLabel) {
            m_iconLabel->setPixmap(Icons::getPixmap(Icons::TerminalBox, accentCol, 16, devicePixelRatioF()));
        }
        if (m_titleLabel) {
            m_titleLabel->setStyleSheet(QString(
                "font-family: \"Segoe UI\", sans-serif; font-size: 13px; font-weight: 600; color: %1;"
            ).arg(m_isDark ? "#8b97a8" : "#475569"));
        }
    }

    void updateFormattedText(bool force = false) {
        if (!m_cmdLabel) return;
        if (!force && (m_cmdLabel->hasSelectedText() || QApplication::mouseButtons() != Qt::NoButton)) {
            return;
        }

        QString promptCol = m_isDark ? "#64748b" : "#6e7781";
        QString binCol    = m_isDark ? "#e34ba9" : "#cf222e";
        QString flagCol   = m_isDark ? "#38bdf8" : "#0969da";
        QString valCol    = m_isDark ? "#f1f5f9" : "#1f2328";
        QString cursorCol = m_cursorVisible ? binCol : "transparent";

        if (m_command.trimmed().isEmpty()) {
            QString html = QString(
                "<span style=\"color: %1; font-weight: 500;\">-&nbsp;</span>"
                "<span style=\"color: %2; font-weight: bold; font-family: 'Consolas', monospace;\">|</span>"
            ).arg(promptCol, cursorCol);
            m_cmdLabel->setText(html);
            return;
        }

        QStringList tokens = m_command.split(' ', Qt::SkipEmptyParts);
        QString formatted;
        for (int i = 0; i < tokens.size(); ++i) {
            if (i > 0) formatted += " ";
            QString tok = tokens[i].toHtmlEscaped();
            if (i == 0) {
                formatted += QString("<span style=\"color: %1; font-weight: 600;\">%2</span>").arg(binCol, tok);
            } else if (tok.startsWith('-')) {
                formatted += QString("<span style=\"color: %1;\">%2</span>").arg(flagCol, tok);
            } else {
                formatted += QString("<span style=\"color: %1;\">%2</span>").arg(valCol, tok);
            }
        }

        QString html = QString(
            "<span style=\"color: %1; font-weight: 500;\">-&nbsp;</span>"
            "%2"
            "<span style=\"color: %3; font-weight: bold; font-family: 'Consolas', monospace;\">|</span>"
        ).arg(promptCol, formatted, cursorCol);

        m_cmdLabel->setText(html);
    }

    QString m_command;
    QLabel* m_iconLabel{nullptr};
    QLabel* m_titleLabel{nullptr};
    QLabel* m_cmdLabel{nullptr};
    TerminalCopyButton* m_copyBtn{nullptr};
    QTimer* m_blinkTimer{nullptr};
    bool m_cursorVisible{true};
    bool m_isDark{true};
};

} // namespace masscan::ui
