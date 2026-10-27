#include "badge_widget.hpp"
#include "high_hz_animator.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QCursor>
#include <QFontMetrics>
#include <QTimer>
#include <algorithm>
#include <cmath>

namespace masscan::ui {

namespace {

inline double easeOutCubic(double t) {
    double f = 1.0 - t;
    return 1.0 - f * f * f;
}

} // namespace

BadgeWidget::BadgeWidget(const QString& name, QWidget* parent)
    : QWidget(parent),
      m_name(name) {

    setFixedHeight(26);
    setCursor(Qt::PointingHandCursor);
    setToolTip("Click to re-check dependencies");
    setAttribute(Qt::WA_Hover, true);

    m_hoverAnim = new HighHzAnimator(this);
    m_hoverAnim->setDuration(220);
    m_hoverAnim->onProgress = [this](double p) {
        m_hoverProgress = m_hoverStart + (m_hoverTarget - m_hoverStart) * p;
        update();
    };
    m_hoverAnim->onFinished = [this]() {
        if (m_hoverProgress <= 0.001 && m_bobTimer && m_bobTimer->isActive()) {
            m_bobTimer->stop();
            m_bobAngle = 0.0;
        }
    };

    updateTone();
    updateGeometry();
}

BadgeWidget::~BadgeWidget() = default;

void BadgeWidget::setStatus(int state, const QString& info, bool isDark) {
    m_state = state;
    m_info = info;
    m_isDark = isDark;
    updateTone();

    if (m_state != 0) {
        setToolTip("Click to re-check dependencies");
        m_hoverProgress = 0.0;
        if (m_bobTimer && m_bobTimer->isActive()) {
            m_bobTimer->stop();
            m_bobAngle = 0.0;
        }
    }

    updateGeometry();
    update();
}

void BadgeWidget::setMode(int mode) {
    if (m_mode == mode) return;
    m_mode = mode;
    updateGeometry();
    update();
}

void BadgeWidget::setDownloadSupported(bool supported) {
    m_downloadSupported = supported;
    updateGeometry();
    update();
}

void BadgeWidget::startDownloadAnimation() {
    m_downloading = true;
    m_progress = 0.0;
    m_statusText = (m_mode == 0) ? "Downloading 0%" : "0%";
    m_hoverProgress = 0.0;
    if (m_bobTimer && m_bobTimer->isActive()) {
        m_bobTimer->stop();
        m_bobAngle = 0.0;
    }
    update();
}

void BadgeWidget::setDownloadProgress(double progress) {
    m_progress = std::clamp(progress, 0.0, 1.0);
    int pct = static_cast<int>(m_progress * 100);
    if (m_mode == 0) {
        m_statusText = QString("Downloading %1%").arg(pct);
    } else {
        m_statusText = QString("%1%").arg(pct);
    }
    update();
}

void BadgeWidget::setDownloadStatus(const QString& text) {
    m_statusText = text;
    update();
}

void BadgeWidget::setDownloadFinished(bool success) {
    if (success) {
        m_statusText = "Ready!";
        m_progress = 1.0;
        update();
        QTimer::singleShot(800, this, [this]() {
            m_downloading = false;
            m_statusText.clear();
            m_progress = 0.0;
            update();
        });
    } else {
        m_downloading = false;
        m_statusText.clear();
        m_progress = 0.0;
        update();
    }
}

void BadgeWidget::resetDownload() {
    m_downloading = false;
    m_statusText.clear();
    m_progress = 0.0;
    m_hoverProgress = 0.0;
    if (m_bobTimer && m_bobTimer->isActive()) {
        m_bobTimer->stop();
        m_bobAngle = 0.0;
    }
    update();
}

QSize BadgeWidget::sizeHint() const {
    if (m_mode == 2) {
        return QSize(26, 26);
    }

    QFont fontBold("Segoe UI");
    fontBold.setPixelSize(12);
    fontBold.setWeight(QFont::Bold);

    QFont fontRegular("Segoe UI");
    fontRegular.setPixelSize(12);
    fontRegular.setWeight(QFont::Normal);

    QFontMetrics fmBold(fontBold);
    QFontMetrics fmReg(fontRegular);

    int nameW = fmBold.horizontalAdvance(m_name);
    int infoW = fmReg.horizontalAdvance(m_info.isEmpty() ? "not found" : m_info);

    int normalW = 8 + 7 + 6 + nameW + (m_mode == 0 ? (6 + infoW) : 0) + 10;
    int hoverW = 0;
    if (m_downloadSupported && m_state == 0) {
        // 14px icon + 6px gap + "Download" + 18px padding
        hoverW = 14 + 6 + fmBold.horizontalAdvance("Download") + 18;
    }
    int w = std::max(normalW, hoverW);
    return QSize(w, 26);
}

QSize BadgeWidget::minimumSizeHint() const {
    return sizeHint();
}

void BadgeWidget::enterEvent(QEnterEvent* event) {
    QWidget::enterEvent(event);
    if (m_downloadSupported && m_state == 0 && !m_downloading) {
        setToolTip(QString("Click to download %1 automatically").arg(m_name));
        m_hoverStart = m_hoverProgress;
        m_hoverTarget = 1.0;
        m_hoverAnim->setDuration(220);
        m_hoverAnim->start();

        if (!m_bobTimer) {
            m_bobTimer = new QTimer(this);
            connect(m_bobTimer, &QTimer::timeout, this, [this]() {
                m_bobAngle += 0.14;
                if (m_bobAngle >= 6.2831853) m_bobAngle -= 6.2831853;
                update();
            });
        }
        m_bobTimer->start(16);
    }
}

void BadgeWidget::leaveEvent(QEvent* event) {
    QWidget::leaveEvent(event);
    if (m_downloadSupported && m_state == 0 && !m_downloading) {
        setToolTip(QString("Click to download %1 automatically").arg(m_name));
        m_hoverStart = m_hoverProgress;
        m_hoverTarget = 0.0;
        m_hoverAnim->setDuration(180);
        m_hoverAnim->start();
    }
}

void BadgeWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (m_downloading) {
            return;
        }
        if (m_downloadSupported && m_state == 0) {
            emit downloadRequested();
            return;
        }
        emit clicked();
    }
    QWidget::mousePressEvent(event);
}

void BadgeWidget::updateTone() {
    QString toneStr;
    if (m_state == 2) {
        toneStr = m_isDark ? "#34c77b" : "#1b9a5a";
    } else if (m_state == 1) {
        toneStr = m_isDark ? "#3b9eff" : "#1f73d6";
    } else {
        toneStr = m_isDark ? "#ef5350" : "#d32f2f";
    }
    m_tone = QColor(toneStr);
}

void BadgeWidget::drawNormalContent(QPainter& p) {
    if (m_mode == 2) {
        p.setPen(Qt::NoPen);
        p.setBrush(m_tone);
        p.drawEllipse(QRectF((width() - 8.0) / 2.0, (height() - 8.0) / 2.0, 8.0, 8.0));
        return;
    }

    // Dot (7x7) optically centered at y = (26 - 7)/2 = 9.5
    p.setPen(Qt::NoPen);
    p.setBrush(m_tone);
    p.drawEllipse(QRectF(8.0, (height() - 7.0) / 2.0, 7.0, 7.0));

    // Name text
    QFont fontBold("Segoe UI");
    fontBold.setPixelSize(12);
    fontBold.setWeight(QFont::Bold);
    p.setFont(fontBold);
    p.setPen(m_isDark ? QColor("#e6ebf2") : QColor("#1a2230"));

    QFontMetrics fmBold(fontBold);
    int nw = fmBold.horizontalAdvance(m_name);
    // Y optical centering: -1.5px
    p.drawText(QRectF(21.0, -1.5, nw + 4, height()), Qt::AlignLeft | Qt::AlignVCenter, m_name);

    if (m_mode == 0) {
        // Info text
        QFont fontRegular("Segoe UI");
        fontRegular.setPixelSize(12);
        fontRegular.setWeight(QFont::Normal);
        p.setFont(fontRegular);
        p.setPen(m_tone);

        qreal infoX = 21.0 + nw + 6.0;
        // Y optical centering: -1.5px
        p.drawText(QRectF(infoX, -1.5, width() - infoX - 8.0, height()), Qt::AlignLeft | Qt::AlignVCenter, m_info);
    }
}

void BadgeWidget::drawDownloadIcon(QPainter& p, qreal x, qreal y, qreal size, const QColor& color) {
    qreal s = size / 24.0;

    // SVG Tray: M21 15 v4 a2 2 0 0 1 -2 2 H5 a2 2 0 0 1 -2 -2 v-4
    QPainterPath tray;
    tray.moveTo(21.0, 15.0);
    tray.lineTo(21.0, 19.0);
    tray.arcTo(QRectF(17.0, 17.0, 4.0, 4.0), 0.0, -90.0);
    tray.lineTo(5.0, 21.0);
    tray.arcTo(QRectF(3.0, 17.0, 4.0, 4.0), 270.0, -90.0);
    tray.lineTo(3.0, 15.0);

    QTransform tTray;
    tTray.translate(x, y);
    tTray.scale(s, s);
    QPainterPath scaledTray = tTray.map(tray);

    // SVG Arrow: line (12, 3) -> (12, 15), polyline (7, 10) -> (12, 15) -> (17, 10)
    QPainterPath arrow;
    arrow.moveTo(12.0, 3.0);
    arrow.lineTo(12.0, 15.0);
    arrow.moveTo(7.0, 10.0);
    arrow.lineTo(12.0, 15.0);
    arrow.lineTo(17.0, 10.0);

    // Floating bobbing offset matching CSS @keyframes docs (up to -2px)
    qreal bobY = -2.0 * std::abs(std::sin(m_bobAngle));

    QTransform tArrow;
    tArrow.translate(x, y + bobY);
    tArrow.scale(s, s);
    QPainterPath scaledArrow = tArrow.map(arrow);

    QPen pen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.strokePath(scaledTray, pen);
    p.strokePath(scaledArrow, pen);
}

void BadgeWidget::drawDownloadContent(QPainter& p) {
    QFont font("Segoe UI");
    font.setPixelSize(12);
    font.setWeight(QFont::Bold);
    p.setFont(font);

    QColor fgColor = m_isDark ? QColor("#092011") : QColor("#ffffff");

    if (m_mode == 2) {
        qreal iconX = (width() - 14.0) / 2.0;
        qreal iconY = (height() - 14.0) / 2.0;
        drawDownloadIcon(p, iconX, iconY, 14.0, fgColor);
        return;
    }

    QFontMetrics fm(font);
    int textW = fm.horizontalAdvance("Download");
    qreal totalW = 14.0 + 6.0 + textW;
    qreal startX = (width() - totalW) / 2.0;
    qreal iconY = (height() - 14.0) / 2.0;

    // Draw floating download icon
    drawDownloadIcon(p, startX, iconY, 14.0, fgColor);

    // Draw "Download" text with Y optical centering: -1.5px
    p.setPen(fgColor);
    QRectF textRect(startX + 14.0 + 6.0, -1.5, textW + 4, height());
    p.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, "Download");
}

void BadgeWidget::drawProgressBar(QPainter& p) {
    QRectF r(0.5, 0.5, width() - 1.0, height() - 1.0);
    QPainterPath clipPath;
    clipPath.addRoundedRect(r, 13.0, 13.0);

    p.save();
    p.setClipPath(clipPath);

    // Track background
    QColor trackBg(m_isDark ? "#141922" : "#e8edf5");
    p.fillRect(rect(), trackBg);

    // Smoothly filled gradient progress bar
    qreal fillW = width() * std::clamp(m_progress, 0.0, 1.0);
    if (fillW > 0.5) {
        QLinearGradient grad(0, 0, width(), 0);
        if (m_isDark) {
            grad.setColorAt(0.0, QColor("#0fd850")); // Uiverse emerald
            grad.setColorAt(0.5, QColor("#10b981")); // Mint/emerald
            grad.setColorAt(1.0, QColor("#3b9eff")); // Cyan/accent blue
        } else {
            grad.setColorAt(0.0, QColor("#059669")); // Deep emerald
            grad.setColorAt(0.5, QColor("#10b981")); // Mint
            grad.setColorAt(1.0, QColor("#1f73d6")); // Primary blue
        }
        p.fillRect(QRectF(0, 0, fillW, height()), grad);

        // Specular highlight line on leading edge
        if (fillW < width() - 2.0) {
            p.setPen(QPen(QColor(255, 255, 255, 180), 1.5));
            p.drawLine(QPointF(fillW - 0.75, 2.0), QPointF(fillW - 0.75, height() - 2.0));
        }
    }

    // Text: bold centered with Y optical centering (-1.5px)
    QFont font("Segoe UI");
    font.setPixelSize(11);
    font.setWeight(QFont::Bold);
    p.setFont(font);

    QRectF textRect(0, -1.5, width(), height());

    // 1. Dual-clip: Text over the unfilled track (from fillW to width())
    if (fillW < width()) {
        p.save();
        p.setClipRect(QRectF(fillW, 0, width() - fillW, height()));
        if (m_isDark) {
            p.setPen(QColor(0, 0, 0, 160));
            p.drawText(textRect.translated(0.5, 0.5), Qt::AlignCenter, m_statusText);
            p.setPen(QColor("#cbd5e1"));
            p.drawText(textRect, Qt::AlignCenter, m_statusText);
        } else {
            p.setPen(QColor(255, 255, 255, 180));
            p.drawText(textRect.translated(0.5, 0.5), Qt::AlignCenter, m_statusText);
            p.setPen(QColor("#0f172a")); // Crisp dark slate on light track
            p.drawText(textRect, Qt::AlignCenter, m_statusText);
        }
        p.restore();
    }

    // 2. Dual-clip: Text over the filled gradient bar (from 0 to fillW)
    if (fillW > 0.5) {
        p.save();
        p.setClipRect(QRectF(0, 0, fillW, height()));
        p.setPen(QColor(0, 0, 0, 120));
        p.drawText(textRect.translated(0.5, 0.5), Qt::AlignCenter, m_statusText);
        p.setPen(QColor("#ffffff")); // Crisp white on emerald/blue fill
        p.drawText(textRect, Qt::AlignCenter, m_statusText);
        p.restore();
    }

    p.restore();

    // Border
    QColor borderCol(m_isDark ? "#10b981" : "#059669");
    p.setPen(QPen(borderCol, 1.0));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(r, 13.0, 13.0);
}

void BadgeWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    QRectF r(0.5, 0.5, width() - 1.0, height() - 1.0);
    QPainterPath clipPath;
    clipPath.addRoundedRect(r, 13.0, 13.0);

    if (m_downloading) {
        drawProgressBar(p);
        return;
    }

    if (m_downloadSupported && m_state == 0 && m_hoverProgress > 0.001) {
        p.save();
        p.setClipPath(clipPath);

        double eased = easeOutCubic(m_hoverProgress);
        qreal h = height();

        // Layer 1: Normal badge sliding UP and fading out
        qreal normalOpacity = std::clamp(1.0 - eased * 1.2, 0.0, 1.0);
        if (normalOpacity > 0.01) {
            p.save();
            p.setOpacity(normalOpacity);
            p.translate(0, -eased * h);

            QColor bg(m_isDark ? "#1e2530" : "#eef1f6");
            p.fillRect(rect(), bg);
            p.setPen(QPen(m_tone, 1.0));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(r, 13.0, 13.0);
            drawNormalContent(p);

            p.restore();
        }

        // Layer 2: Download button sliding UP from below into view
        qreal downloadOpacity = std::clamp(eased * 1.2, 0.0, 1.0);
        if (downloadOpacity > 0.01) {
            p.save();
            p.setOpacity(downloadOpacity);
            p.translate(0, (1.0 - eased) * h);

            QLinearGradient dlGrad(0, 0, width(), 0);
            if (m_isDark) {
                dlGrad.setColorAt(0.0, QColor("#0fd850"));
                dlGrad.setColorAt(1.0, QColor("#01e056"));
            } else {
                dlGrad.setColorAt(0.0, QColor("#10b981"));
                dlGrad.setColorAt(1.0, QColor("#059669"));
            }
            p.fillRect(rect(), dlGrad);
            p.setPen(QPen(m_isDark ? QColor("#01e056") : QColor("#059669"), 1.0));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(r, 13.0, 13.0);
            drawDownloadContent(p);

            p.restore();
        }

        p.restore();

        // Outer border
        QColor borderCol = (eased >= 0.5) ? (m_isDark ? QColor("#01e056") : QColor("#059669")) : m_tone;
        p.setPen(QPen(borderCol, 1.0));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(r, 13.0, 13.0);
        return;
    }

    // Default static state (state != 0 or unhovered)
    QColor bg(m_isDark ? "#1e2530" : "#eef1f6");
    p.setPen(QPen(m_tone, 1.0));
    p.setBrush(bg);
    p.drawRoundedRect(r, 13.0, 13.0);

    drawNormalContent(p);
}

} // namespace masscan::ui
