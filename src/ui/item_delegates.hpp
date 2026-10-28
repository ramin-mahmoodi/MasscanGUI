#pragma once

#include <QStyledItemDelegate>
#include <QAbstractItemView>
#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QEvent>
#include <set>
#include <string>

namespace masscan::ui {

// User roles for item data
enum ItemRoles {
    RoleCode = Qt::UserRole,
    RoleLabel = Qt::UserRole + 1,
    RoleInfo = Qt::UserRole + 2
};

class CountryItemDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit CountryItemDelegate(const std::set<std::string>* pickedCodes, QAbstractItemView* view = nullptr)
        : QStyledItemDelegate(view), m_pickedCodes(pickedCodes), m_view(view) {
        if (m_view) {
            m_view->setMouseTracking(true);
            if (m_view->viewport()) {
                m_view->viewport()->setMouseTracking(true);
                m_view->viewport()->setAttribute(Qt::WA_Hover, true);
                m_view->viewport()->installEventFilter(this);
            }
        }
    }

    void setDark(bool isDark) {
        m_isDark = isDark;
    }

    bool eventFilter(QObject* watched, QEvent* event) override {
        if (m_view && watched == m_view->viewport()) {
            if (event->type() == QEvent::MouseMove) {
                auto* me = static_cast<QMouseEvent*>(event);
                QModelIndex idx = m_view->indexAt(me->pos());
                int newRow = idx.isValid() ? idx.row() : -1;
                if (newRow != m_hoveredRow) {
                    m_hoveredRow = newRow;
                    m_view->viewport()->update();
                }
            } else if (event->type() == QEvent::Leave) {
                if (m_hoveredRow != -1) {
                    m_hoveredRow = -1;
                    m_view->viewport()->update();
                }
            }
        }
        return QStyledItemDelegate::eventFilter(watched, event);
    }

    QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const override {
        return QSize(0, 40);
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        std::string code = index.data(RoleCode).toString().toStdString();
        bool isPicked = m_pickedCodes && (m_pickedCodes->find(code) != m_pickedCodes->end());
        bool isHovered = (index.row() == m_hoveredRow) || (option.state & QStyle::State_MouseOver);

        if (isPicked || isHovered) {
            painter->save();
            if (const auto* view = qobject_cast<const QAbstractItemView*>(option.widget)) {
                if (const auto* vp = view->viewport()) {
                    if (option.rect.top() < 6 || option.rect.bottom() > vp->height() - 6) {
                        QPainterPath clipPath;
                        clipPath.addRoundedRect(QRectF(vp->rect()), 5.0, 5.0);
                        painter->setClipPath(clipPath);
                    }
                }
            }
            if (isPicked) {
                QColor selBg = isHovered 
                    ? (m_isDark ? QColor("#4fa8ff") : QColor("#2b7de0"))
                    : (m_isDark ? QColor("#3b9eff") : QColor("#1f73d6"));
                painter->fillRect(option.rect, selBg);
            } else {
                QColor hovBg = m_isDark ? QColor("#2a3340") : QColor("#d3dae4");
                painter->fillRect(option.rect, hovBg);
            }
            painter->restore();
        }

        QString label = index.data(RoleLabel).toString();
        QString info = index.data(RoleInfo).toString();
        if (label.isEmpty()) {
            QString full = index.data(Qt::DisplayRole).toString();
            int nl = full.indexOf('\n');
            if (nl >= 0) {
                label = full.left(nl);
                info = full.mid(nl + 1);
            } else {
                label = full;
            }
        }

        // Title Line (13px, Segoe UI, Normal weight)
        QFont titleFont("Segoe UI");
        titleFont.setPixelSize(13);
        titleFont.setWeight(QFont::Normal);
        painter->setFont(titleFont);

        QColor titleColor;
        if (isPicked) {
            titleColor = Qt::white;
        } else {
            titleColor = m_isDark ? QColor("#e6ebf2") : QColor("#1a2230");
        }
        painter->setPen(titleColor);

        QRect textRect1(option.rect.left() + 8, option.rect.top() + 3, option.rect.width() - 16, 18);
        QFontMetrics fm1(titleFont);
        QString elidedTitle = fm1.elidedText(label, Qt::ElideRight, textRect1.width());
        painter->drawText(textRect1, Qt::AlignLeft | Qt::AlignVCenter, elidedTitle);

        // Subtitle Line (12px, Segoe UI, Normal weight)
        QFont infoFont("Segoe UI");
        infoFont.setPixelSize(12);
        infoFont.setWeight(QFont::Normal);
        painter->setFont(infoFont);

        QColor infoColor;
        if (isPicked) {
            infoColor = QColor("#dbeaff");
        } else {
            infoColor = m_isDark ? QColor("#8b97a8") : QColor("#5b6a7e");
        }
        painter->setPen(infoColor);

        QRect textRect2(option.rect.left() + 8, option.rect.top() + 21, option.rect.width() - 16, 16);
        QFontMetrics fm2(infoFont);
        QString elidedInfo = fm2.elidedText(info, Qt::ElideRight, textRect2.width());
        painter->drawText(textRect2, Qt::AlignLeft | Qt::AlignVCenter, elidedInfo);

        painter->restore();
    }

private:
    const std::set<std::string>* m_pickedCodes{nullptr};
    QAbstractItemView* m_view{nullptr};
    int m_hoveredRow{-1};
    bool m_isDark{true};
};

class TargetItemDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit TargetItemDelegate(QAbstractItemView* view = nullptr)
        : QStyledItemDelegate(view), m_view(view) {
        if (m_view) {
            m_view->setMouseTracking(true);
            if (m_view->viewport()) {
                m_view->viewport()->setMouseTracking(true);
                m_view->viewport()->setAttribute(Qt::WA_Hover, true);
                m_view->viewport()->installEventFilter(this);
            }
        }
    }

    void setDark(bool isDark) {
        m_isDark = isDark;
    }

    bool eventFilter(QObject* watched, QEvent* event) override {
        if (m_view && watched == m_view->viewport()) {
            if (event->type() == QEvent::MouseMove) {
                auto* me = static_cast<QMouseEvent*>(event);
                QModelIndex idx = m_view->indexAt(me->pos());
                int newRow = idx.isValid() ? idx.row() : -1;
                if (newRow != m_hoveredRow) {
                    m_hoveredRow = newRow;
                    m_view->viewport()->update();
                }
            } else if (event->type() == QEvent::Leave) {
                if (m_hoveredRow != -1) {
                    m_hoveredRow = -1;
                    m_view->viewport()->update();
                }
            }
        }
        return QStyledItemDelegate::eventFilter(watched, event);
    }

    QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const override {
        return QSize(0, 26);
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        bool isSelected = (option.state & QStyle::State_Selected);
        bool isHovered = (index.row() == m_hoveredRow) || (option.state & QStyle::State_MouseOver);

        if (isSelected || isHovered) {
            painter->save();
            if (const auto* view = qobject_cast<const QAbstractItemView*>(option.widget)) {
                if (const auto* vp = view->viewport()) {
                    if (option.rect.top() < 6 || option.rect.bottom() > vp->height() - 6) {
                        QPainterPath clipPath;
                        clipPath.addRoundedRect(QRectF(vp->rect()), 5.0, 5.0);
                        painter->setClipPath(clipPath);
                    }
                }
            }
            if (isSelected) {
                QColor selBg = isHovered 
                    ? (m_isDark ? QColor("#4fa8ff") : QColor("#2b7de0"))
                    : (m_isDark ? QColor("#3b9eff") : QColor("#1f73d6"));
                painter->fillRect(option.rect, selBg);
            } else {
                QColor hovBg = m_isDark ? QColor("#2a3340") : QColor("#d3dae4");
                painter->fillRect(option.rect, hovBg);
            }
            painter->restore();
        }

        QString label = index.data(RoleLabel).toString();
        QString info = index.data(RoleInfo).toString();
        if (label.isEmpty()) {
            label = index.data(Qt::DisplayRole).toString();
        }

        // Info width (right side, 12px, Segoe UI Normal)
        QFont infoFont("Segoe UI");
        infoFont.setPixelSize(12);
        infoFont.setWeight(QFont::Normal);
        QFontMetrics fmInfo(infoFont);
        int infoWidth = !info.isEmpty() ? fmInfo.horizontalAdvance(info) + 8 : 0;

        // Label (left side, 13px, Segoe UI Normal)
        QFont labelFont("Segoe UI");
        labelFont.setPixelSize(13);
        labelFont.setWeight(QFont::Normal);
        painter->setFont(labelFont);

        QColor labelColor = isSelected ? Qt::white : (m_isDark ? QColor("#e6ebf2") : QColor("#1a2230"));
        painter->setPen(labelColor);

        int leftW = option.rect.width() - 16 - infoWidth;
        if (leftW < 0) leftW = 0;
        QRect labelRect(option.rect.left() + 8, option.rect.top(), leftW, option.rect.height());
        QFontMetrics fmLabel(labelFont);
        QString elidedLabel = fmLabel.elidedText(label, Qt::ElideRight, labelRect.width());
        painter->drawText(labelRect, Qt::AlignLeft | Qt::AlignVCenter, elidedLabel);

        // Info (right side, 10px)
        if (!info.isEmpty()) {
            painter->setFont(infoFont);
            QColor infoColor = isSelected ? QColor("#dbeaff") : (m_isDark ? QColor("#8b97a8") : QColor("#5b6a7e"));
            painter->setPen(infoColor);
            QRect infoRect(option.rect.right() - 8 - infoWidth, option.rect.top(), infoWidth, option.rect.height());
            painter->drawText(infoRect, Qt::AlignRight | Qt::AlignVCenter, info);
        }

        painter->restore();
    }

private:
    QAbstractItemView* m_view{nullptr};
    int m_hoveredRow{-1};
    bool m_isDark{true};
};

} // namespace masscan::ui
