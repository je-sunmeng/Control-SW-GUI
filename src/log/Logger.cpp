#include "Logger.h"

#include <QDir>
#include <QMutexLocker>
#include <QTextStream>

#include <atomic>

namespace {
std::atomic<Logger*> g_qtLogger{nullptr};
QtMessageHandler g_prevHandler = nullptr;
thread_local bool t_inHandler = false;

void qtMessageHandler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg)
{
    if (g_prevHandler)
        g_prevHandler(type, ctx, msg);

    Logger* logger = g_qtLogger.load();
    if (!logger || t_inHandler)
        return;
    t_inHandler = true;   // Logger 내부에서 다시 qWarning 등이 발생해도 재귀 방지
    switch (type) {
    case QtWarningMsg:
    case QtCriticalMsg:
    case QtFatalMsg:
        logger->error(msg, QStringLiteral("Qt"));
        break;
    case QtInfoMsg:
        logger->info(msg, QStringLiteral("Qt"));
        break;
    case QtDebugMsg:
        break;   // 디버그 메시지는 로그 패널에 올리지 않음
    }
    t_inHandler = false;
}
} // namespace

Logger::Logger(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<LogEntry>();
}

Logger::~Logger()
{
    Logger* self = this;
    if (g_qtLogger.compare_exchange_strong(self, nullptr))
        qInstallMessageHandler(g_prevHandler);
    QMutexLocker lock(&m_mutex);
    m_file.close();
}

void Logger::info(const QString& content, const QString& device)
{
    log({QDateTime::currentDateTime(), LogType::Info, device, 0, 0, content, {}});
}

void Logger::error(const QString& content, const QString& device)
{
    log({QDateTime::currentDateTime(), LogType::Error, device, 0, 0, content, {}});
}

void Logger::tx(const QString& device, quint32 opcode, quint32 size, const QString& summary,
                const QByteArray& raw)
{
    log({QDateTime::currentDateTime(), LogType::Tx, device, opcode, size, summary, raw});
}

void Logger::rx(const QString& device, quint32 opcode, quint32 size, const QString& summary,
                const QByteArray& raw)
{
    log({QDateTime::currentDateTime(), LogType::Rx, device, opcode, size, summary, raw});
}

void Logger::log(LogEntry entry)
{
    if (!entry.time.isValid())
        entry.time = QDateTime::currentDateTime();
    {
        QMutexLocker lock(&m_mutex);
        if (m_fileEnabled)
            writeToFileLocked(entry);
    }
    emit entryAdded(entry);
}

bool Logger::setFileLoggingEnabled(bool enabled, const QString& dir)
{
    QMutexLocker lock(&m_mutex);
    if (!dir.isEmpty() && dir != m_dir) {
        m_dir = dir;
        m_file.close();
        m_fileDate = {};
    }
    m_fileEnabled = enabled;
    if (!enabled) {
        m_file.close();
        m_fileDate = {};
        return true;
    }
    if (m_dir.isEmpty()) {
        m_fileEnabled = false;
        return false;
    }
    if (!openFileForDateLocked(QDate::currentDate())) {
        m_fileEnabled = false;
        return false;
    }
    return true;
}

bool Logger::isFileLoggingEnabled() const
{
    QMutexLocker lock(&m_mutex);
    return m_fileEnabled;
}

QString Logger::logDirectory() const
{
    QMutexLocker lock(&m_mutex);
    return m_dir;
}

QString Logger::currentLogFilePath() const
{
    QMutexLocker lock(&m_mutex);
    return m_file.isOpen() ? m_file.fileName() : QString();
}

void Logger::installQtMessageHandler()
{
    Logger* expected = nullptr;
    if (g_qtLogger.compare_exchange_strong(expected, this))
        g_prevHandler = qInstallMessageHandler(qtMessageHandler);
}

void Logger::uninstallQtMessageHandler()
{
    if (g_qtLogger.exchange(nullptr))
        qInstallMessageHandler(g_prevHandler);
}

void Logger::writeToFileLocked(const LogEntry& entry)
{
    const QDate date = entry.time.date();
    if (date != m_fileDate && !openFileForDateLocked(date))
        return;
    QTextStream ts(&m_file);
    ts.setEncoding(QStringConverter::Utf8);
    ts << formatLogLine(entry) << '\n';
    ts.flush();
}

bool Logger::openFileForDateLocked(const QDate& date)
{
    m_file.close();
    if (!QDir().mkpath(m_dir))
        return false;
    m_file.setFileName(QDir(m_dir).filePath(date.toString(QStringLiteral("yyyyMMdd")) + QStringLiteral(".log")));
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        return false;
    m_fileDate = date;
    return true;
}
