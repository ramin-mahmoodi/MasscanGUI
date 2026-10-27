#pragma once

#include <QComboBox>
#include <QAbstractItemView>
#include <QFrame>
#include <QPoint>
#include <QWheelEvent>

namespace masscan::ui {

class FluentComboBox : public QComboBox {
public:
    explicit FluentComboBox(QWidget* parent = nullptr) : QComboBox(parent) {
        setFocusPolicy(Qt::StrongFocus);
        setCursor(Qt::PointingHandCursor);
        if (auto* v = view()) {
            v->setCursor(Qt::PointingHandCursor);
            if (auto* c = qobject_cast<QFrame*>(v->parentWidget())) {
                c->setAttribute(Qt::WA_TranslucentBackground, true);
                c->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
                c->setFrameShape(QFrame::NoFrame);
                c->setLineWidth(0);
            }
        }
    }

    void showPopup() override {
        if (auto* v = view()) {
            if (auto* c = qobject_cast<QFrame*>(v->parentWidget())) {
                c->setAttribute(Qt::WA_TranslucentBackground, true);
                c->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
                c->setFrameShape(QFrame::NoFrame);
                c->setLineWidth(0);
            }
        }
        QComboBox::showPopup();
        if (auto* v = view()) {
            if (auto* c = v->parentWidget()) {
                QPoint globalBottom = mapToGlobal(QPoint(0, height()));
                QPoint p = c->pos();
                if (p.y() >= globalBottom.y() - 2) {
                    c->move(p.x(), p.y() + 4);
                } else {
                    c->move(p.x(), p.y() - 4);
                }
            }
        }
    }

protected:
    void wheelEvent(QWheelEvent* event) override {
        // Do not scroll or change selection when the mouse hovers over the closed combo box.
        // Ignore the event so it propagates to parent scroll area to scroll the page smoothly.
        event->ignore();
    }
};

} // namespace masscan::ui
