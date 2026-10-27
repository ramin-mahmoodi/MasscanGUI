#pragma once

#include <QWidget>
#include <QEvent>
#include <QPoint>
#include <QString>

namespace masscan::ui {

class HighHzAnimator;

class FluentToolTip : public QWidget {
    Q_OBJECT

public:
    static FluentToolTip* instance();

    void setDark(bool isDark);
    void showToolTip(QWidget* anchor, const QString& text, const QPoint& globalPos);
    void hideToolTip(bool immediate = false);

    [[nodiscard]] QWidget* currentAnchor() const { return m_anchor; }
    [[nodiscard]] bool isDark() const { return m_isDark; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    explicit FluentToolTip(QWidget* parent = nullptr);
    ~FluentToolTip() override;

    QString m_text;
    QWidget* m_anchor{nullptr};
    QPoint m_targetPos;
    QPoint m_triggerPos;

    bool m_isDark{true};
    double m_opacity{0.0};
    double m_outStartOpacity{1.0};

    HighHzAnimator* m_inAnim{nullptr};
    HighHzAnimator* m_outAnim{nullptr};

    static FluentToolTip* s_instance;
};

class FluentToolTipFilter : public QObject {
    Q_OBJECT

public:
    explicit FluentToolTipFilter(QObject* parent = nullptr);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
};

} // namespace masscan::ui
