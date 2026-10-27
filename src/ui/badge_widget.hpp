#pragma once

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>

namespace masscan::ui {

class BadgeWidget : public QWidget {
    Q_OBJECT

public:
    explicit BadgeWidget(const QString& name, QWidget* parent = nullptr);

    void setStatus(int state, const QString& info, bool isDark);
    void setMode(int mode); // 0 = full (dot + name + info), 1 = compact (dot + name), 2 = dot only

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    void updateStyle();

    QString m_name;
    QString m_info;
    int m_state{0}; // 0 = missing, 1 = next to app, 2 = installed
    int m_mode{0};  // 0, 1, 2
    bool m_isDark{true};

    QLabel* m_nameLabel{nullptr};
    QLabel* m_infoLabel{nullptr};
    QWidget* m_dotWidget{nullptr};
    QHBoxLayout* m_layout{nullptr};
    QColor m_tone;
};

} // namespace masscan::ui
