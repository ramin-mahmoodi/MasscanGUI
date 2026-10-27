#pragma once

#include <QObject>
#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>
#include <QScreen>
#include <QGuiApplication>
#include <algorithm>
#include <cmath>
#include <functional>

namespace masscan::ui {

// Frame-accurate animator synced to the monitor's native refresh rate (90Hz / 120Hz / 144Hz+)
// using Qt::PreciseTimer (1ms multimedia timer on Windows) + QElapsedTimer nanosecond tracking.
class HighHzAnimator : public QObject {
    Q_OBJECT

public:
    explicit HighHzAnimator(QWidget* targetWidget)
        : QObject(targetWidget), m_widget(targetWidget) {
        m_timer.setTimerType(Qt::PreciseTimer);
        connect(&m_timer, &QTimer::timeout, this, &HighHzAnimator::onTick);
    }

    void setDuration(int durationMs) {
        m_durationMs = std::max(1, durationMs);
    }

    bool isRunning() const {
        return m_timer.isActive();
    }

    void start() {
        // Always tick at 4ms (250Hz) with Qt::PreciseTimer (matching smooth scroll)
        // so every monitor VBlank (60Hz/90Hz/120Hz/144Hz/165Hz/240Hz) gets a fresh frame
        // without QTimer integer-ms phase drift against DWM VSync.
        m_elapsed.restart();
        m_timer.start(4);
    }

    void stop() {
        m_timer.stop();
    }

    std::function<void(double)> onProgress;
    std::function<void()> onFinished;

private:
    void onTick() {
        const double elapsedMs = m_elapsed.nsecsElapsed() / 1000000.0;
        if (elapsedMs >= static_cast<double>(m_durationMs)) {
            m_timer.stop();
            if (onProgress) {
                onProgress(1.0);
            }
            if (onFinished) {
                onFinished();
            }
            return;
        }
        const double p = std::clamp(elapsedMs / static_cast<double>(m_durationMs), 0.0, 1.0);
        if (onProgress) {
            onProgress(p);
        }
    }

    QWidget* m_widget{nullptr};
    QTimer m_timer;
    QElapsedTimer m_elapsed;
    int m_durationMs{400};
};

} // namespace masscan::ui
