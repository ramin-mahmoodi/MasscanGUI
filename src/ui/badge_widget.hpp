#pragma once

#include <QWidget>
#include <QString>
#include <QColor>
#include <QEnterEvent>

namespace masscan::ui {

class HighHzAnimator;

class BadgeWidget : public QWidget {
    Q_OBJECT

public:
    explicit BadgeWidget(const QString& name, QWidget* parent = nullptr);
    ~BadgeWidget() override;

    void setStatus(int state, const QString& info, bool isDark);
    void setMode(int mode); // 0 = full (dot + name + info), 1 = compact (dot + name), 2 = dot only
    void setDownloadSupported(bool supported);

    // Download & in-place progress bar support
    void startDownloadAnimation();
    void setDownloadProgress(double progress); // 0.0 to 1.0
    void setDownloadStatus(const QString& text);
    void setDownloadFinished(bool success);
    void resetDownload();

    bool isDownloading() const { return m_downloading; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void clicked();
    void downloadRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void updateTone();
    void drawNormalContent(QPainter& p);
    void drawDownloadContent(QPainter& p);
    void drawDownloadIcon(QPainter& p, qreal x, qreal y, qreal size, const QColor& color);
    void drawProgressBar(QPainter& p);

    QString m_name;
    QString m_info;
    int m_state{0}; // 0 = missing, 1 = next to app, 2 = installed
    int m_mode{0};  // 0, 1, 2
    bool m_isDark{true};
    bool m_downloadSupported{false};
    bool m_downloading{false};
    double m_progress{0.0};
    QString m_statusText;

    QColor m_tone;

    // Hover animation
    HighHzAnimator* m_hoverAnim{nullptr};
    double m_hoverProgress{0.0};
    double m_hoverStart{0.0};
    double m_hoverTarget{0.0};

    // Arrow bobbing animation
    QTimer* m_bobTimer{nullptr};
    double m_bobAngle{0.0};
};

} // namespace masscan::ui
