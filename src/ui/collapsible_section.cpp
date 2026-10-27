#include "collapsible_section.hpp"
#include "icons.hpp"
#include "high_hz_animator.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QCursor>
#include <cmath>
#include <algorithm>

namespace masscan::ui {

// ---------------- ChevronWidget ----------------
class ChevronWidget : public QWidget {
public:
    explicit ChevronWidget(QWidget* parent = nullptr) : QWidget(parent) {
        setFixedSize(14, 14);
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setAttribute(Qt::WA_StyledBackground, false);
    }

    void setAngle(double angle) {
        if (std::abs(m_angle - angle) < 0.1) return;
        m_angle = angle;
        update();
    }

    void setDark(bool isDark) {
        if (m_isDark == isDark) return;
        m_isDark = isDark;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);

        QColor mutedCol = m_isDark ? QColor("#8b97a8") : QColor("#5b6a7e");
        QPixmap pix = Icons::getPixmap(Icons::ChevRight, mutedCol, 14, devicePixelRatioF());

        p.save();
        p.translate(width() / 2.0, height() / 2.0);
        p.rotate(m_angle);
        p.translate(-width() / 2.0, -height() / 2.0);

        qreal dpr = pix.devicePixelRatio();
        qreal pw = pix.width() / dpr;
        qreal ph = pix.height() / dpr;
        p.drawPixmap(QRectF((width() - pw) / 2.0, (height() - ph) / 2.0, pw, ph).toRect(), pix);
        p.restore();
    }

private:
    double m_angle{90.0}; // 0 = right (collapsed), 90 = down (expanded)
    bool m_isDark{true};
};

// ---------------- ContentContainer ----------------
CollapsibleSection::ContentContainer::ContentContainer(CollapsibleSection* section)
    : QWidget(section), m_section(section) {
    setAttribute(Qt::WA_StyledBackground, false);
}

QSize CollapsibleSection::ContentContainer::sizeHint() const {
    if (m_section && m_section->m_contentLayout) {
        return m_section->m_contentLayout->sizeHint();
    }
    if (m_section && m_section->m_innerWidget) {
        return m_section->m_innerWidget->sizeHint();
    }
    return QWidget::sizeHint();
}

QSize CollapsibleSection::ContentContainer::minimumSizeHint() const {
    if (m_section && m_section->m_contentLayout) {
        return m_section->m_contentLayout->minimumSize();
    }
    if (m_section && m_section->m_innerWidget) {
        return m_section->m_innerWidget->minimumSizeHint();
    }
    return QWidget::minimumSizeHint();
}

void CollapsibleSection::ContentContainer::paintEvent(QPaintEvent*) {
    // Content is clipped by this widget's geometry; background is painted by CollapsibleSection
}

void CollapsibleSection::ContentContainer::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (m_section) {
        m_section->onContentContainerResized(event->size());
    }
}

// ---------------- CollapsibleSection ----------------
CollapsibleSection::CollapsibleSection(const QString& title, const char* iconPath, QWidget* parent, bool expanded)
    : QWidget(parent),
      m_expanded(expanded),
      m_iconPath(iconPath) {

    m_animator = new HighHzAnimator(this);
    m_animator->onProgress = [this](double p) {
        updateAnimationStep(p);
    };
    m_animator->onFinished = [this]() {
        finishAnimation();
    };

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // Header (34px height)
    m_headerWidget = new QWidget(this);
    m_headerWidget->setFixedHeight(34);
    m_headerWidget->setCursor(Qt::PointingHandCursor);
    m_headerWidget->setAttribute(Qt::WA_StyledBackground, false);

    auto* headerLayout = new QHBoxLayout(m_headerWidget);
    headerLayout->setContentsMargins(10, 0, 10, 0);
    headerLayout->setSpacing(8);

    if (m_iconPath) {
        m_iconLabel = new QLabel(m_headerWidget);
        m_iconLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        m_iconLabel->setAttribute(Qt::WA_StyledBackground, false);
        headerLayout->addWidget(m_iconLabel);
    }

    m_titleLabel = new QLabel(title, m_headerWidget);
    m_titleLabel->setObjectName("sectionTitle");
    m_titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    QFont titleFont("Segoe UI");
    titleFont.setPixelSize(14);
    titleFont.setWeight(QFont::Bold);
    m_titleLabel->setFont(titleFont);
    headerLayout->addWidget(m_titleLabel);

    headerLayout->addStretch(1);

    m_toggleIcon = new ChevronWidget(m_headerWidget);
    headerLayout->addWidget(m_toggleIcon);

    m_headerWidget->installEventFilter(this);
    m_mainLayout->addWidget(m_headerWidget);

    // Content container (clipping viewport)
    m_contentWidget = new ContentContainer(this);
    m_contentWidget->setAttribute(Qt::WA_StyledBackground, false);
    m_mainLayout->addWidget(m_contentWidget);

    // Inner widget (holds the layout, never squished during animation)
    m_innerWidget = new QWidget(m_contentWidget);
    m_innerWidget->setAttribute(Qt::WA_StyledBackground, false);

    // Initial state without animation
    m_animProgress = m_expanded ? 1.0 : 0.0;
    m_toggleIcon->setAngle(m_animProgress * 90.0);
    if (m_expanded) {
        m_contentWidget->setVisible(true);
        m_contentWidget->setMaximumHeight(QWIDGETSIZE_MAX);
        m_contentWidget->setMinimumHeight(0);
        setMaximumHeight(QWIDGETSIZE_MAX);
        setMinimumHeight(0);
    } else {
        m_contentWidget->setVisible(false);
        m_contentWidget->setFixedHeight(0);
        setFixedHeight(34);
    }

    applyTheme(true);
}

void CollapsibleSection::onContentContainerResized(const QSize& sz) {
    if (!m_innerWidget) return;
    int h = (m_animator && m_animator->isRunning()) ? m_fullContentHeight : sz.height();
    m_innerWidget->setGeometry(0, 0, sz.width(), std::max(h, m_fullContentHeight));
}

void CollapsibleSection::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (m_contentWidget && m_innerWidget) {
        int w = m_contentWidget->width();
        int h = (m_animator && m_animator->isRunning()) ? m_fullContentHeight : m_contentWidget->height();
        m_innerWidget->setGeometry(0, 0, w, std::max(h, m_fullContentHeight));
    }
}

QSize CollapsibleSection::sizeHint() const {
    if (m_animProgress <= 0.001) {
        return QSize(280, 34);
    }
    if (m_animator && m_animator->isRunning()) {
        return QSize(280, 34 + m_contentWidget->height());
    }
    int innerH = m_innerWidget ? m_innerWidget->sizeHint().height() : 0;
    return QSize(280, 34 + innerH);
}

QSize CollapsibleSection::minimumSizeHint() const {
    if (m_animProgress <= 0.001) {
        return QSize(60, 34);
    }
    return QSize(60, 34 + (m_contentWidget ? m_contentWidget->minimumHeight() : 0));
}

bool CollapsibleSection::hasHeightForWidth() const {
    if (m_animProgress <= 0.001) return false;
    return m_innerWidget && m_innerWidget->layout() && m_innerWidget->layout()->hasHeightForWidth();
}

int CollapsibleSection::heightForWidth(int w) const {
    if (m_animProgress <= 0.001) return 34;
    if (m_animator && m_animator->isRunning()) {
        return 34 + m_contentWidget->height();
    }
    int innerH = m_innerWidget ? m_innerWidget->heightForWidth(w) : 0;
    return 34 + innerH;
}

void CollapsibleSection::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    QColor bgPanel2 = m_isDark ? (m_headerHovered ? QColor("#242c38") : QColor("#1e2530"))
                               : (m_headerHovered ? QColor("#e5e9f0") : QColor("#eef1f6"));
    QColor bgPanel  = m_isDark ? QColor("#171c24") : QColor("#ffffff");
    QColor border   = m_isDark ? QColor("#2a3340") : QColor("#d3dae4");

    QRectF r(0.5, 0.5, width() - 1.0, height() - 1.0);

    if (m_animProgress <= 0.001) {
        p.setPen(QPen(border, 1.0));
        p.setBrush(bgPanel2);
        p.drawRoundedRect(r, 6.0, 6.0);
        return;
    }

    // Content background (rounded rect, zero QPainterPath clip overhead)
    p.setPen(Qt::NoPen);
    p.setBrush(bgPanel);
    p.drawRoundedRect(r, 6.0, 6.0);

    // Header background (y = 0 to 34, fast rectangular clip)
    p.setClipRect(QRect(0, 0, width(), 34));
    p.setBrush(bgPanel2);
    p.drawRoundedRect(r, 6.0, 6.0);
    p.setClipping(false);

    // 1px separator at y = 34
    QColor sepColor = border;
    sepColor.setAlpha(static_cast<int>(sepColor.alpha() * std::clamp(m_animProgress * 1.5, 0.0, 1.0)));
    p.setPen(QPen(sepColor, 1.0));
    p.drawLine(QPointF(1.0, 34.0), QPointF(width() - 1.0, 34.0));

    // Outer 1px rounded border
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(r, 6.0, 6.0);
}

void CollapsibleSection::applyTheme(bool isDark) {
    m_isDark = isDark;
    if (m_contentWidget) m_contentWidget->setDark(isDark);
    if (m_toggleIcon) m_toggleIcon->setDark(isDark);
    updateHeaderIcons();
    update();
}

void CollapsibleSection::setContentLayout(QLayout* layout) {
    m_contentLayout = layout;
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);
    m_innerWidget->setLayout(layout);

    if (m_expanded) {
        int naturalH = layout->sizeHint().height();
        if (naturalH <= 0) naturalH = m_innerWidget->sizeHint().height();
        if (naturalH <= 0) naturalH = 120;
        m_fullContentHeight = naturalH;
        m_contentWidget->setFixedHeight(m_fullContentHeight);
        m_innerWidget->setGeometry(0, 0, m_contentWidget->width(), m_fullContentHeight);
        setFixedHeight(34 + m_fullContentHeight);
        updateGeometry();
        if (parentWidget() && parentWidget()->layout()) {
            parentWidget()->layout()->activate();
        }
    }
}

void CollapsibleSection::setExpanded(bool expanded, bool animate) {
    if (m_expanded == expanded && (!m_animator || !m_animator->isRunning())) {
        return;
    }

    m_expanded = expanded;
    emit toggled(m_expanded);

    if (!animate) {
        if (m_animator && m_animator->isRunning()) {
            m_animator->stop();
        }
        m_animProgress = m_expanded ? 1.0 : 0.0;
        if (m_toggleIcon) {
            m_toggleIcon->setAngle(m_animProgress * 90.0);
        }
        if (m_expanded) {
            m_contentWidget->setVisible(true);
            m_contentWidget->setMaximumHeight(QWIDGETSIZE_MAX);
            m_contentWidget->setMinimumHeight(0);
            setMaximumHeight(QWIDGETSIZE_MAX);
            setMinimumHeight(0);
        } else {
            m_contentWidget->setVisible(false);
            m_contentWidget->setFixedHeight(0);
            setFixedHeight(34);
        }
        updateGeometry();
        if (parentWidget() && parentWidget()->layout()) {
            parentWidget()->layout()->activate();
        }
        update();
        return;
    }

    m_startProgress = m_animProgress;
    m_targetProgress = m_expanded ? 1.0 : 0.0;

    if (m_expanded) {
        m_contentWidget->setVisible(true);
        m_innerWidget->setVisible(true);
        if (m_contentLayout) {
            m_contentLayout->activate();
        }
        int naturalH = 0;
        if (m_contentLayout) {
            naturalH = m_contentLayout->sizeHint().height();
        }
        if (naturalH <= 0 && m_innerWidget) {
            naturalH = m_innerWidget->sizeHint().height();
        }
        int curW = width() > 20 ? width() : 280;
        if (m_contentLayout && m_contentLayout->hasHeightForWidth()) {
            naturalH = std::max(naturalH, m_contentLayout->totalHeightForWidth(curW));
        }
        if (naturalH <= 0) {
            naturalH = 120;
        }
        m_fullContentHeight = naturalH;
        m_innerWidget->setGeometry(0, 0, m_contentWidget->width(), m_fullContentHeight);
    } else {
        if (m_contentWidget->height() > 0) {
            m_fullContentHeight = m_contentWidget->height();
        }
    }

    int duration = std::clamp(static_cast<int>(240.0 * std::abs(m_targetProgress - m_startProgress)), 120, 240);
    m_animator->setDuration(duration);
    m_animator->start();
}

void CollapsibleSection::updateAnimationStep(double p) {
    // easeOutCubic: snappy, immediate response with silky smooth deceleration
    double eased = 1.0 - std::pow(1.0 - p, 3.0);
    m_animProgress = m_startProgress + (m_targetProgress - m_startProgress) * eased;

    if (m_toggleIcon) {
        m_toggleIcon->setAngle(m_animProgress * 90.0);
    }

    int currentH = static_cast<int>(std::round(m_fullContentHeight * m_animProgress));
    currentH = std::max(0, currentH);

    m_contentWidget->setFixedHeight(currentH);
    if (m_innerWidget) {
        m_innerWidget->setGeometry(0, 0, m_contentWidget->width(), m_fullContentHeight);
    }
    setFixedHeight(34 + currentH);

    updateGeometry();
    if (parentWidget() && parentWidget()->layout()) {
        parentWidget()->layout()->activate();
    }
    update();
}

void CollapsibleSection::finishAnimation() {
    m_animProgress = m_targetProgress;
    if (m_toggleIcon) {
        m_toggleIcon->setAngle(m_animProgress * 90.0);
    }

    if (m_expanded) {
        int naturalH = m_contentLayout ? m_contentLayout->sizeHint().height() : m_fullContentHeight;
        if (naturalH <= 0) naturalH = m_fullContentHeight;
        m_contentWidget->setFixedHeight(naturalH);
        setFixedHeight(34 + naturalH);
        if (m_innerWidget) {
            m_innerWidget->setGeometry(0, 0, m_contentWidget->width(), naturalH);
        }
    } else {
        m_contentWidget->setVisible(false);
        m_contentWidget->setFixedHeight(0);
        setFixedHeight(34);
    }

    updateGeometry();
    if (parentWidget() && parentWidget()->layout()) {
        parentWidget()->layout()->activate();
    }
    update();
}

void CollapsibleSection::updateHeaderIcons() {
    qreal dpr = devicePixelRatioF();
    QColor accentCol = m_isDark ? QColor("#3b9eff") : QColor("#1f73d6");

    if (m_iconPath && m_iconLabel) {
        m_iconLabel->setPixmap(Icons::getPixmap(m_iconPath, accentCol, 16, dpr));
    }
    if (m_toggleIcon) {
        m_toggleIcon->setDark(m_isDark);
    }
}

bool CollapsibleSection::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_headerWidget) {
        if (event->type() == QEvent::Enter) {
            m_headerHovered = true;
            update();
        } else if (event->type() == QEvent::Leave) {
            m_headerHovered = false;
            update();
        } else if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                setExpanded(!m_expanded, true);
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

} // namespace masscan::ui
