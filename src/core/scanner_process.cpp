#include "scanner_process.hpp"

#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <algorithm>

namespace masscan {

ScannerProcess::ScannerProcess(QObject* parent)
    : QObject(parent),
      m_process(new QProcess(this)),
      m_simTimer(new QTimer(this)),
      m_tailTimer(new QTimer(this)) {

    connect(m_process, &QProcess::readyReadStandardOutput, this, &ScannerProcess::handleReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &ScannerProcess::handleReadyReadStandardError);
    connect(m_process, &QProcess::finished, this, &ScannerProcess::handleProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &ScannerProcess::handleProcessError);

    connect(m_simTimer, &QTimer::timeout, this, &ScannerProcess::handleSimulationTick);
    connect(m_tailTimer, &QTimer::timeout, this, &ScannerProcess::pollOutputFile);
}

ScannerProcess::~ScannerProcess() {
    stopScan();
}

QString ScannerProcess::lookupServiceName(int port, const QString& proto) {
    const QString p = proto.toLower();
    if (p == "tcp") {
        switch (port) {
            case 20: case 21: return "ftp";
            case 22: return "ssh";
            case 23: return "telnet";
            case 25: return "smtp";
            case 53: return "dns";
            case 80: return "http";
            case 110: return "pop3";
            case 143: return "imap";
            case 443: return "https";
            case 465: return "smtps";
            case 587: return "submission";
            case 993: return "imaps";
            case 995: return "pop3s";
            case 1433: return "ms-sql";
            case 1521: return "oracle";
            case 3306: return "mysql";
            case 3389: return "rdp";
            case 5432: return "postgres";
            case 5900: return "vnc";
            case 6379: return "redis";
            case 8000: case 8080: case 8888: return "http-alt";
            case 8443: return "https-alt";
            case 27017: return "mongodb";
            default: break;
        }
    } else if (p == "udp") {
        switch (port) {
            case 53: return "dns";
            case 67: case 68: return "dhcp";
            case 69: return "tftp";
            case 123: return "ntp";
            case 161: case 162: return "snmp";
            case 500: return "ike";
            case 514: return "syslog";
            case 1194: return "openvpn";
            default: break;
        }
    }
    return "unknown";
}

bool ScannerProcess::startScan(const QString& program, const QStringList& args, bool simulate) {
    if (m_state == State::Scanning) {
        return false;
    }

    m_simulate = simulate;
    m_userStopped = false;
    m_pct = 0.0f;
    m_kpps = 0.0;
    m_found = 0;
    m_simTick = 0;
    m_stdoutBuf.clear();
    m_stderrBuf.clear();
    m_tailFilePath.clear();
    m_lastTailPos = 0;

    // Detect if an output file was specified in the arguments (-oD, -oJ, -oL, -oX, -oG, -oB, --output-filename)
    for (int i = 0; i < args.size() - 1; ++i) {
        const QString& a = args.at(i);
        if (a == "-oD" || a == "-oJ" || a == "-oL" || a == "-oX" || a == "-oG" || a == "-oB" || a == "--output-filename") {
            QString rawPath = args.at(i + 1);
            QFileInfo fi(rawPath);
            m_tailFilePath = fi.isAbsolute() ? rawPath : QDir::current().filePath(rawPath);
            break;
        }
    }

    if (!m_tailFilePath.isEmpty()) {
        bool isAppend = args.contains("--append-output");
        if (!isAppend && QFile::exists(m_tailFilePath)) {
            QFile::remove(m_tailFilePath);
            m_lastTailPos = 0;
        } else if (isAppend && QFile::exists(m_tailFilePath)) {
            m_lastTailPos = QFileInfo(m_tailFilePath).size();
        }
    }

    if (m_simulate) {
        setState(State::Scanning, "Simulating Scan...");
        emit logLineReceived(QString("[%1] Simulation scan started...").arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
        m_simTimer->start(100);
        return true;
    }

    setState(State::Scanning, "Scanning...");
    emit logLineReceived(QString("[%1] Starting masscan process: %2 %3")
                             .arg(QDateTime::currentDateTime().toString("hh:mm:ss"), program, args.join(" ")));

    m_process->setWorkingDirectory(QDir::currentPath());
    m_process->setProgram(program);
    m_process->setArguments(args);
    m_process->start();

    if (!m_process->waitForStarted(2000)) {
        setState(State::Error, "Failed to start process");
        emit scanError("Failed to start masscan process. Check permissions or binary path.");
        return false;
    }

    if (!m_tailFilePath.isEmpty()) {
        m_tailTimer->start(200);
    }

    return true;
}

void ScannerProcess::stopScan() {
    if (m_simTimer->isActive()) {
        m_simTimer->stop();
    }

    if (m_tailTimer->isActive()) {
        pollOutputFile();
        m_tailTimer->stop();
    }

    m_userStopped = true;

    if (m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(500)) {
            m_process->kill();
            m_process->waitForFinished(500);
        }
    }

    m_canResume = true;
    setState(State::Paused, "Scan stopped by user");
    emit logLineReceived(QString("[%1] Scan was stopped by user.").arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
}

void ScannerProcess::resumeScan(const QString& program) {
    QStringList args;
    args << "--resume" << "paused.conf";
    startScan(program, args, m_simulate);
}

void ScannerProcess::setState(State state, const QString& text) {
    m_state = state;
    emit statusChanged(text);
}

void ScannerProcess::flushBuffer(QByteArray& buf) {
    int start = 0;
    const int sz = buf.size();
    const char* data = buf.constData();
    for (int i = 0; i < sz; ++i) {
        char ch = data[i];
        if (ch == '\r' || ch == '\n') {
            if (i > start) {
                QString line = QString::fromUtf8(data + start, i - start).trimmed();
                if (!line.isEmpty()) {
                    parseOutputLine(line);
                }
            }
            start = i + 1;
        }
    }
    if (start > 0) {
        buf.remove(0, start);
    }
}

void ScannerProcess::handleReadyReadStandardOutput() {
    m_stdoutBuf.append(m_process->readAllStandardOutput());
    flushBuffer(m_stdoutBuf);
}

void ScannerProcess::handleReadyReadStandardError() {
    m_stderrBuf.append(m_process->readAllStandardError());
    flushBuffer(m_stderrBuf);
}

void ScannerProcess::pollOutputFile() {
    if (m_tailFilePath.isEmpty()) return;
    QFile file(m_tailFilePath);
    if (!file.exists()) return;
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    if (file.size() < m_lastTailPos) {
        m_lastTailPos = 0; // File was recreated/truncated
    }
    if (file.seek(m_lastTailPos)) {
        while (!file.atEnd()) {
            qint64 beforePos = file.pos();
            QByteArray lineBytes = file.readLine();
            if (!lineBytes.endsWith('\n') && !file.atEnd()) {
                // Incomplete line still being written by masscan; rewind and wait for newline
                file.seek(beforePos);
                break;
            }
            QString line = QString::fromUtf8(lineBytes).trimmed();
            if (!line.isEmpty()) {
                parseOutputLine(line);
            }
        }
        m_lastTailPos = file.pos();
    }
}

void ScannerProcess::parseOutputLine(const QString& line) {
    emit logLineReceived(line);

    // 1. Detect open port from standard stdout: "Discovered open port 80/tcp on 192.168.1.1"
    static const QRegularExpression discRegex(R"(Discovered\s+open\s+port\s+(\d+)\/(\w+)\s+on\s+([0-9a-fA-F\.\:]+))",
                                              QRegularExpression::CaseInsensitiveOption);
    auto match = discRegex.match(line);
    if (match.hasMatch()) {
        int port = match.captured(1).toInt();
        QString proto = match.captured(2).toUpper();
        QString ip = match.captured(3);
        QString timeStr = QDateTime::currentDateTime().toString("hh:mm:ss");
        QString svc = lookupServiceName(port, proto);
        m_found++;
        emit portDiscovered(ip, port, proto, "open", svc, timeStr);
        emit progressUpdated(m_pct, m_kpps, "", m_found);
        return;
    }

    // 2. Detect list format (-oL): "open tcp 80 1.1.1.1 1793002162"
    if (line.startsWith("open ", Qt::CaseInsensitive)) {
        QStringList parts = line.split(QRegularExpression(R"(\s+)"), Qt::SkipEmptyParts);
        if (parts.size() >= 4) {
            QString proto = parts[1].toUpper();
            int port = parts[2].toInt();
            QString ip = parts[3];
            QString timeStr = QDateTime::currentDateTime().toString("hh:mm:ss");
            QString svc = lookupServiceName(port, proto);
            m_found++;
            emit portDiscovered(ip, port, proto, "open", svc, timeStr);
            emit progressUpdated(m_pct, m_kpps, "", m_found);
            return;
        }
    }

    // 2b. Detect banner in list format (-oL): "banner tcp 80 1.1.1.1 1793002162 [title] Apache"
    if (line.startsWith("banner ", Qt::CaseInsensitive)) {
        QStringList parts = line.split(QRegularExpression(R"(\s+)"), Qt::SkipEmptyParts);
        if (parts.size() >= 5) {
            QString proto = parts[1].toUpper();
            int port = parts[2].toInt();
            QString ip = parts[3];
            QString banner = parts.mid(5).join(" ");
            if (banner.isEmpty() && parts.size() > 4) banner = parts[4];
            QString timeStr = QDateTime::currentDateTime().toString("hh:mm:ss");
            emit portDiscovered(ip, port, proto, "open", banner, timeStr);
            return;
        }
    }

    // 2c. Detect Grepable format (-oG): "Timestamp: ... Host: 192.168.1.1 () Ports: 80/open/tcp//http//"
    if (line.contains("Host:") && line.contains("Ports:")) {
        static const QRegularExpression grepRegex(R"(Host:\s+([0-9a-fA-F\.\:]+).*?Ports:\s+(\d+)\/(\w+)\/(\w+)\/\/([^\/]*)?)",
                                                  QRegularExpression::CaseInsensitiveOption);
        auto gMatch = grepRegex.match(line);
        if (gMatch.hasMatch()) {
            QString ip = gMatch.captured(1);
            int port = gMatch.captured(2).toInt();
            QString state = gMatch.captured(3);
            QString proto = gMatch.captured(4).toUpper();
            QString banner = gMatch.captured(5).trimmed();
            if (banner.isEmpty()) banner = lookupServiceName(port, proto);
            QString timeStr = QDateTime::currentDateTime().toString("hh:mm:ss");
            m_found++;
            emit portDiscovered(ip, port, proto, state, banner, timeStr);
            emit progressUpdated(m_pct, m_kpps, "", m_found);
            return;
        }
    }

    // 2d. Detect XML format (-oX): <host ...><address addr="192.168.1.1" .../><ports><port protocol="tcp" portid="80"><state state="open" .../></port></ports></host>
    if (line.contains("<host") && line.contains("<address") && line.contains("<port")) {
        static const QRegularExpression xmlRegex(
            "addr=\"([0-9a-fA-F\\.:]+)\".*?protocol=\"(\\w+)\".*?portid=\"(\\d+)\".*?state=\"(\\w+)\"",
            QRegularExpression::CaseInsensitiveOption);
        auto xMatch = xmlRegex.match(line);
        if (xMatch.hasMatch()) {
            QString ip = xMatch.captured(1);
            QString proto = xMatch.captured(2).toUpper();
            int port = xMatch.captured(3).toInt();
            QString state = xMatch.captured(4);
            QString svc = lookupServiceName(port, proto);
            QString timeStr = QDateTime::currentDateTime().toString("hh:mm:ss");
            m_found++;
            emit portDiscovered(ip, port, proto, state, svc, timeStr);
            emit progressUpdated(m_pct, m_kpps, "", m_found);
            return;
        }
    }

    // 2e. Detect Unicornscan format (-oU): "TCP open http[ 80] from 192.168.1.1 ttl 64"
    if (line.contains(" open") && line.contains("from ") && line.contains("[")) {
        static const QRegularExpression uniRegex(R"((\w+)\s+open\s+([a-zA-Z0-9_\-\.]+)?\[\s*(\d+)\]\s+from\s+([0-9a-fA-F\.\:]+))",
                                                 QRegularExpression::CaseInsensitiveOption);
        auto uMatch = uniRegex.match(line);
        if (uMatch.hasMatch()) {
            QString proto = uMatch.captured(1).toUpper();
            QString svc = uMatch.captured(2).trimmed();
            int port = uMatch.captured(3).toInt();
            QString ip = uMatch.captured(4);
            if (svc.isEmpty()) svc = lookupServiceName(port, proto);
            QString timeStr = QDateTime::currentDateTime().toString("hh:mm:ss");
            m_found++;
            emit portDiscovered(ip, port, proto, "open", svc, timeStr);
            emit progressUpdated(m_pct, m_kpps, "", m_found);
            return;
        }
    }

    // 3. Detect NDJSON / JSON format (-oD / -oJ):
    // NDJSON (-oD): {"ip":"1.1.1.1","port":80,"proto":"tcp","rec_type":"status","data":{"status":"open",...}}
    // Multi-port JSON (-oJ): [{"ip":"1.1.1.1","ports":[...]}, ...]
    QString jsonStr = line;
    if (jsonStr.startsWith(',')) {
        jsonStr = jsonStr.mid(1).trimmed();
    }
    if (jsonStr.startsWith('{') && jsonStr.contains("\"ip\"")) {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &err);
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject root = doc.object();
            QString ip = root.value("ip").toString();
            QString timeStr = QDateTime::currentDateTime().toString("hh:mm:ss");

            // Format A: Single-port NDJSON (-oD) where "port" is a direct member of root
            if (root.contains("port")) {
                int port = root.value("port").toInt();
                QString proto = root.value("proto").toString("tcp").toUpper();
                QString status = "open";
                QString svc = lookupServiceName(port, proto);

                if (root.contains("data") && root.value("data").isObject()) {
                    QJsonObject dObj = root.value("data").toObject();
                    if (dObj.contains("status")) {
                        status = dObj.value("status").toString("open");
                    }
                    if (dObj.contains("banner") && !dObj.value("banner").toString().isEmpty()) {
                        svc = dObj.value("banner").toString();
                    } else if (dObj.contains("service") && !dObj.value("service").toString().isEmpty()) {
                        svc = dObj.value("service").toString();
                    }
                }

                bool isBanner = (root.value("rec_type").toString() == "banner");
                if (!isBanner) {
                    m_found++;
                }
                emit portDiscovered(ip, port, proto, status, svc, timeStr);
                emit progressUpdated(m_pct, m_kpps, "", m_found);
                return;
            }

            // Format B: Multi-port JSON (-oJ) where "ports" is an array
            if (root.contains("ports") && root.value("ports").isArray()) {
                QJsonArray portsArr = root.value("ports").toArray();
                for (const auto& pVal : portsArr) {
                    if (pVal.isObject()) {
                        QJsonObject pObj = pVal.toObject();
                        int port = pObj.value("port").toInt();
                        QString proto = pObj.value("proto").toString().toUpper();
                        QString status = pObj.value("status").toString("open");
                        QString svc = lookupServiceName(port, proto);
                        if (pObj.contains("service") && pObj.value("service").isObject()) {
                            QJsonObject sObj = pObj.value("service").toObject();
                            if (sObj.contains("banner") && !sObj.value("banner").toString().isEmpty()) {
                                svc = sObj.value("banner").toString();
                            } else if (sObj.contains("name") && !sObj.value("name").toString().isEmpty()) {
                                svc = sObj.value("name").toString();
                            }
                        } else if (pObj.contains("data") && pObj.value("data").isObject()) {
                            QJsonObject dObj = pObj.value("data").toObject();
                            if (dObj.contains("banner") && !dObj.value("banner").toString().isEmpty()) {
                                svc = dObj.value("banner").toString();
                            }
                        }
                        m_found++;
                        emit portDiscovered(ip, port, proto, status, svc, timeStr);
                    }
                }
                emit progressUpdated(m_pct, m_kpps, "", m_found);
                return;
            }
        }
    }

    // 4. Detect banner: "Banner on [10.0.0.1] port 80/tcp: [Apache]"
    static const QRegularExpression bannerRegex(R"(Banner\s+on\s+\[([^\]]+)\]\s+port\s+(\d+)\/(\w+):\s+\[(.*)\])",
                                                QRegularExpression::CaseInsensitiveOption);
    auto bannerMatch = bannerRegex.match(line);
    if (bannerMatch.hasMatch()) {
        QString ip = bannerMatch.captured(1);
        int port = bannerMatch.captured(2).toInt();
        QString proto = bannerMatch.captured(3).toUpper();
        QString banner = bannerMatch.captured(4);
        QString timeStr = QDateTime::currentDateTime().toString("hh:mm:ss");
        emit portDiscovered(ip, port, proto, "open", banner, timeStr);
        return;
    }

    // 5. Detect masscan status: "rate:  0.00-kpps,  5.20% done, waiting 10-secs, 2-found" OR "found=2"
    static const QRegularExpression statRegex(
        R"(rate:\s*([\d\.]+)-kpps,\s*([\d\.]+)%\s*done,\s*([^,]+),\s*(?:found=)?(\d+)(?:-found)?)",
        QRegularExpression::CaseInsensitiveOption);
    auto statMatch = statRegex.match(line);
    if (statMatch.hasMatch()) {
        m_kpps = statMatch.captured(1).toDouble();
        m_pct = statMatch.captured(2).toFloat();
        QString eta = statMatch.captured(3).trimmed();
        m_found = statMatch.captured(4).toInt();
        emit progressUpdated(m_pct, m_kpps, eta, m_found);
        return;
    }
}

void ScannerProcess::handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (m_tailTimer && m_tailTimer->isActive()) {
        pollOutputFile();
        m_tailTimer->stop();
    }

    if (!m_stdoutBuf.isEmpty()) {
        QString line = QString::fromUtf8(m_stdoutBuf).trimmed();
        if (!line.isEmpty()) parseOutputLine(line);
        m_stdoutBuf.clear();
    }
    if (!m_stderrBuf.isEmpty()) {
        QString line = QString::fromUtf8(m_stderrBuf).trimmed();
        if (!line.isEmpty()) parseOutputLine(line);
        m_stderrBuf.clear();
    }

    if (m_userStopped) {
        m_userStopped = false;
        m_canResume = true;
        setState(State::Paused, "Scan stopped by user");
        emit scanFinished(exitCode, true);
        return;
    }

    if (m_state == State::Scanning) {
        if (exitCode == 0) {
            m_pct = 100.0f;
            emit progressUpdated(100.0f, 0.0, "00:00:00", m_found);
        }
        setState(State::Completed, exitCode == 0 ? "Scan complete" : "Scan finished with exit code " + QString::number(exitCode));
        emit logLineReceived(QString("[%1] Process finished with exit code %2")
                                 .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                                 .arg(exitCode));
        emit scanFinished(exitCode, exitStatus == QProcess::NormalExit);
    }
}

void ScannerProcess::handleProcessError(QProcess::ProcessError error) {
    if (m_userStopped) {
        return; // Normal user-requested stop; ignore artificial crash signal
    }
    QString errStr = m_process->errorString();
    setState(State::Error, "Process error: " + errStr);
    emit scanError(errStr);
}

void ScannerProcess::handleSimulationTick() {
    if (m_state != State::Scanning) return;

    m_simTick++;
    m_pct += 0.5f;
    m_kpps = 2.5;

    if (m_simTick % 5 == 0) {
        m_found++;
        QString ip = QString("10.0.0.%1").arg((m_simTick % 254) + 1);
        int port = (m_simTick % 2 == 0) ? 80 : 443;
        QString banner = (port == 80) ? "HTTP/1.1 200 OK (nginx/1.24)" : "TLSv1.3 (Cloudflare)";
        QString timeStr = QDateTime::currentDateTime().toString("hh:mm:ss");
        QString svc = lookupServiceName(port, "tcp");
        emit portDiscovered(ip, port, "TCP", "open", svc, timeStr);
        emit logLineReceived(QString("[%1] Discovered open port on %2:%3/tcp").arg(timeStr, ip).arg(port));
    }

    double remainingSecs = (100.0f - m_pct) * 0.2;
    int secs = std::max(0, static_cast<int>(remainingSecs));
    QString eta = QString("%1m %2s").arg(secs / 60).arg(secs % 60);

    emit progressUpdated(std::min(100.0f, m_pct), m_kpps, eta, m_found);

    if (m_pct >= 100.0f) {
        m_simTimer->stop();
        setState(State::Completed, "Scan complete (simulation)");
        emit logLineReceived(QString("[%1] Simulation scan completed.").arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
        emit scanFinished(0, true);
    }
}

} // namespace masscan
