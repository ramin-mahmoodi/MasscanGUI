#include "badge_widget.hpp"

#include <QPainter>
#include <QMouseEvent>
#include <QCursor>

namespace masscan::ui {

namespace {

class BadgeDot : public QWidget {
public:
    explicit BadgeDot(QWidget* parent = nullptr) : QWidget(parent) {
        setFixedSize(7, 7);
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
    }

    void setTone(const QColor& c) {
        m_tone = c;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(Qt::NoPen);
        p.setBrush(m_tone);
        p.drawEllipse(0.0, 0.0, static_cast<qreal>(width()), static_cast<qreal>(height()));
    }

private:
    QColor m_tone;
};

} // namespace

BadgeWidget::BadgeWidget(const QString& name, QWidget* parent)
    : QWidget(parent),
      m_name(name) {

    setFixedHeight(26);
    setCursor(Qt::PointingHandCursor);
    setToolTip("Click to re-check dependencies");

    m_layout = new QHBoxLayout(this);
    // Exact padding matching app.slint: 8px left, 10px right, 6px spacing
    m_layout->setContentsMargins(8, 0, 10, 0);
    m_layout->setSpacing(6);
    m_layout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_dotWidget = new BadgeDot(this);
    m_layout->addWidget(m_dotWidget);

    m_nameLabel = new QLabel(m_name, this);
    m_nameLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_layout->addWidget(m_nameLabel);

    m_infoLabel = new QLabel(this);
    m_infoLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_layout->addWidget(m_infoLabel);

    updateStyle();
}

void BadgeWidget::setStatus(int state, const QString& info, bool isDark) {
    m_state = state;
    m_info = info;
    m_isDark = isDark;
    m_infoLabel->setText(m_info);
    updateStyle();
}

void BadgeWidget::setMode(int mode) {
    m_mode = mode;
    if (m_mode == 2) {
        // Dot only (26x26 perfect circle)
        m_nameLabel->setVisible(false);
        m_infoLabel->setVisible(false);
        m_dotWidget->setFixedSize(8, 8);
        m_layout->setContentsMargins(0, 0, 0, 0);
        m_layout->setAlignment(Qt::AlignCenter);
        setFixedWidth(26);
    } else if (m_mode == 1) {
        // Dot + Name
        m_nameLabel->setVisible(true);
        m_infoLabel->setVisible(false);
        m_dotWidget->setFixedSize(7, 7);
        m_layout->setContentsMargins(8, 0, 10, 0);
        m_layout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        setMinimumWidth(0);
        setMaximumWidth(QWIDGETSIZE_MAX);
    } else {
        // Full: Dot + Name + Info
        m_nameLabel->setVisible(true);
        m_infoLabel->setVisible(true);
        m_dotWidget->setFixedSize(7, 7);
        m_layout->setContentsMargins(8, 0, 10, 0);
        m_layout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        setMinimumWidth(0);
        setMaximumWidth(QWIDGETSIZE_MAX);
    }
    update();
}

void BadgeWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Exact Theme.panel2 colors from Slint:
    // Dark: #1e2530, Light: #eef1f6
    QColor bg(m_isDark ? "#1e2530" : "#eef1f6");

    // 1px border with tone color
    QRectF r(0.5, 0.5, width() - 1.0, height() - 1.0);
    p.setPen(QPen(m_tone, 1.0));
    p.setBrush(bg);
    p.drawRoundedRect(r, 13.0, 13.0);
}

void BadgeWidget::updateStyle() {
    QString toneStr;
    if (m_state == 2) {
        toneStr = m_isDark ? "#34c77b" : "#1b9a5a";
    } else if (m_state == 1) {
        toneStr = m_isDark ? "#3b9eff" : "#1f73d6";
    } else {
        toneStr = m_isDark ? "#ef5350" : "#d32f2f";
    }
    m_tone = QColor(toneStr);

    static_cast<BadgeDot*>(m_dotWidget)->setTone(m_tone);

    QString textCol = m_isDark ? "#e6ebf2" : "#1a2230";

    m_nameLabel->setStyleSheet(QString(
        "font-family: \"Segoe UI\";"
        "font-size: 12px;"
        "font-weight: 700;"
        "color: %1;"
        "background: transparent;"
        "border: none;"
    ).arg(textCol));

    m_infoLabel->setStyleSheet(QString(
        "font-family: \"Segoe UI\";"
        "font-size: 12px;"
        "font-weight: 400;"
        "color: %1;"
        "background: transparent;"
        "border: none;"
    ).arg(toneStr));

    update();
}

void BadgeWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked();
    }
    QWidget::mousePressEvent(event);
}

} // namespace masscan::ui
