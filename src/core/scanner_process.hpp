#pragma once

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QStringList>
#include <QDateTime>

namespace masscan {

class ScannerProcess : public QObject {
    Q_OBJECT

public:
    enum class State {
        Idle,
        Scanning,
        Paused,
        Completed,
        Error
    };

    explicit ScannerProcess(QObject* parent = nullptr);
    ~ScannerProcess() override;

    bool startScan(const QString& program, const QStringList& args, bool simulate = false);
    void pauseScan();
    void resumeScan(const QString& program = QString());
    void stopScan(bool clearCheckpoint = false);
    void writeCheckpoint();
    void deleteCheckpoint();

    void setCheckpointEnabled(bool enabled) noexcept { m_checkpointEnabled = enabled; }
    [[nodiscard]] bool isCheckpointEnabled() const noexcept { return m_checkpointEnabled; }
    void setCheckpointInterval(int seconds) noexcept {
        m_checkpointIntervalSec = std::max(1, seconds);
        if (m_checkpointTimer && m_checkpointTimer->isActive()) {
            m_checkpointTimer->start(m_checkpointIntervalSec * 1000);
        }
    }
    [[nodiscard]] int checkpointInterval() const noexcept { return m_checkpointIntervalSec; }

    void resetFoundCount() noexcept { m_found = 0; }
    [[nodiscard]] int foundCount() const noexcept { return m_found; }
    static QString lookupServiceName(int port, const QString& proto);

    [[nodiscard]] bool isRunning() const { return m_state == State::Scanning; }
    [[nodiscard]] bool isPaused() const { return m_state == State::Paused; }
    [[nodiscard]] bool canResume() const;
    [[nodiscard]] State state() const { return m_state; }

signals:
    void statusChanged(const QString& stateText);
    void progressUpdated(float pct, double kpps, const QString& eta, int found);
    void portDiscovered(const QString& ip, int port, const QString& proto, const QString& state, const QString& banner, const QString& timeStr);
    void logLineReceived(const QString& line);
    void scanFinished(int exitCode, bool normalExit);
    void scanError(const QString& errorMsg);

private slots:
    void handleReadyReadStandardOutput();
    void handleReadyReadStandardError();
    void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleProcessError(QProcess::ProcessError error);
    void handleSimulationTick();
    void pollOutputFile();

private:
    void setState(State state, const QString& text);
    void parseOutputLine(const QString& line);
    void flushBuffer(QByteArray& buf);

    QProcess* m_process{nullptr};
    QTimer* m_simTimer{nullptr};
    QTimer* m_tailTimer{nullptr};
    QTimer* m_checkpointTimer{nullptr};
    State m_state{State::Idle};
    bool m_simulate{false};
    bool m_canResume{false};
    bool m_userStopped{false};
    bool m_isSuspended{false};
    bool m_checkpointEnabled{true};
    int m_checkpointIntervalSec{5};
    uint64_t m_totalProbes{0};
    uint64_t m_currentResumeIndex{0};
    QString m_baseEchoConf;
    QString m_lastProgram;

    QByteArray m_stdoutBuf;
    QByteArray m_stderrBuf;
    QString m_tailFilePath;
    qint64 m_lastTailPos{0};

    // Metrics
    float m_pct{0.0f};
    double m_kpps{0.0};
    int m_found{0};
    uint64_t m_simTick{0};
};

} // namespace masscan
