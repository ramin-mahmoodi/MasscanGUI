#pragma once

#include <QWidget>
#include <QPainter>
#include <QPainterPath>
#include <QColor>
#include <QRegion>
#include <QResizeEvent>
#include <QChildEvent>

namespace masscan::ui {

class CardWidget : public QWidget {
    Q_OBJECT

public:
    explicit CardWidget(QWidget* parent = nullptr, int radius = 8)
        : QWidget(parent), m_radius(radius) {
        setAttribute(Qt::WA_StyledBackground, false);
        m_overlay = new BorderOverlay(this, m_radius, m_isDark);
    }

    void setDark(bool dark) {
        if (m_isDark != dark) {
            m_isDark = dark;
            if (m_overlay) m_overlay->setDark(dark);
            update();
        }
    }

    void setRadius(int radius) {
        if (m_radius != radius) {
            m_radius = radius;
            setAttribute(Qt::WA_OpaquePaintEvent, m_radius <= 0);
            if (m_overlay) m_overlay->setRadius(radius);
            updateOverlayMask();
            update();
        }
    }

    [[nodiscard]] bool isDark() const { return m_isDark; }
    [[nodiscard]] int radius() const { return m_radius; }

    void raiseOverlay() {
        if (m_overlay && m_radius > 0) m_overlay->raise();
    }

protected:
    void childEvent(QChildEvent* event) override {
        QWidget::childEvent(event);
        if (event->type() == QEvent::ChildAdded && m_overlay && m_radius > 0) {
            if (event->child() != m_overlay) {
                m_overlay->raise();
            }
        }
    }

    void resizeEvent(QResizeEvent* event) override {
        QWidget::resizeEvent(event);
        updateOverlayMask();
    }

    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        QColor bg = m_isDark ? QColor("#171c24") : QColor("#ffffff");
        QColor border = m_isDark ? QColor("#2a3340") : QColor("#d3dae4");

        if (m_radius <= 0) {
            p.fillRect(rect(), bg);
            p.setPen(QPen(border, 1.0));
            p.drawLine(QPointF(width() - 0.5, 0.0), QPointF(width() - 0.5, height()));
            return;
        }

        p.setRenderHint(QPainter::Antialiasing, true);
        QRectF r(0.5, 0.5, width() - 1.0, height() - 1.0);
        p.setPen(QPen(border, 1.0));
        p.setBrush(bg);
        p.drawRoundedRect(r, m_radius, m_radius);
    }

private:
    void updateOverlayMask() {
        if (!m_overlay) return;
        if (m_radius <= 0) {
            m_overlay->hide();
            return;
        }
        m_overlay->setGeometry(rect());
        const int c = m_radius + 2;
        QRegion borderMask;
        // Exact 1px perimeter so the outer card border is raised above all child widgets and cannot be erased by scrolling
        borderMask += QRect(0, 0, width(), 1);
        borderMask += QRect(0, height() - 1, width(), 1);
        borderMask += QRect(0, 0, 1, height());
        borderMask += QRect(width() - 1, 0, 1, height());
        // 4 corners to clip rectangular children to the card's rounded corners
        borderMask += QRect(0, 0, c, c);
        borderMask += QRect(width() - c, 0, c, c);
        borderMask += QRect(0, height() - c, c, c);
        borderMask += QRect(width() - c, height() - c, c, c);
        m_overlay->setMask(borderMask);
        m_overlay->show();
        m_overlay->raise();
    }

    class BorderOverlay : public QWidget {
    public:
        explicit BorderOverlay(QWidget* parent, int radius, bool isDark)
            : QWidget(parent), m_radius(radius), m_isDark(isDark) {
            setAttribute(Qt::WA_TransparentForMouseEvents, true);
            setAttribute(Qt::WA_NoSystemBackground, true);
            setAttribute(Qt::WA_TranslucentBackground, true);
        }

        void setDark(bool isDark) {
            if (m_isDark != isDark) {
                m_isDark = isDark;
                update();
            }
        }

        void setRadius(int radius) {
            if (m_radius != radius) {
                m_radius = radius;
                update();
            }
        }

    protected:
        void paintEvent(QPaintEvent*) override {
            QPainter p(this);
            p.setRenderHint(QPainter::Antialiasing, true);

            QColor bgOutside = m_isDark ? QColor("#0f1318") : QColor("#f2f4f8");
            QColor border    = m_isDark ? QColor("#2a3340") : QColor("#d3dae4");

            QPainterPath outerRect;
            outerRect.addRect(QRectF(rect()));

            QPainterPath innerRounded;
            QRectF r(0.5, 0.5, width() - 1.0, height() - 1.0);
            innerRounded.addRoundedRect(r, m_radius, m_radius);

            QPainterPath cornersOnly = outerRect.subtracted(innerRounded);
            p.fillPath(cornersOnly, bgOutside);

            p.setPen(QPen(border, 1.0));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(r, m_radius, m_radius);
        }

    private:
        int m_radius{8};
        bool m_isDark{true};
    };

    int m_radius{8};
    bool m_isDark{true};
    BorderOverlay* m_overlay{nullptr};
};

} // namespace masscan::ui
