#pragma once

#include "LogEntry.h"

#include <QDate>
#include <QFile>
#include <QMutex>
#include <QObject>

// 로그 수집기. 모든 공개 함수는 스레드 안전.
// entryAdded 시그널은 호출 스레드에서 emit 되므로, 다른 스레드의 수신자는 Qt::AutoConnection으로
// 자동 큐잉된다(LogEntry는 메타타입 등록됨).
class Logger : public QObject {
    Q_OBJECT
public:
    explicit Logger(QObject* parent = nullptr);
    ~Logger() override;

    void info(const QString& content, const QString& device = {});
    void error(const QString& content, const QString& device = {});
    void tx(const QString& device, quint32 opcode, quint32 size, const QString& summary,
            const QByteArray& raw = {});
    void rx(const QString& device, quint32 opcode, quint32 size, const QString& summary,
            const QByteArray& raw = {});

    void log(LogEntry entry);

    // 일자별 파일(dir/yyyyMMdd.log) 기록. dir이 비어 있으면 현재 설정 유지.
    bool setFileLoggingEnabled(bool enabled, const QString& dir = {});
    bool isFileLoggingEnabled() const;
    QString logDirectory() const;
    QString currentLogFilePath() const;

    // qDebug/qWarning 등 Qt 메시지를 이 Logger로 전달 (기존 핸들러도 계속 호출)
    void installQtMessageHandler();
    static void uninstallQtMessageHandler();

signals:
    void entryAdded(const LogEntry& entry);

private:
    void writeToFileLocked(const LogEntry& entry);
    bool openFileForDateLocked(const QDate& date);

    mutable QMutex m_mutex;
    bool m_fileEnabled = false;
    QString m_dir;
    QFile m_file;
    QDate m_fileDate;
};
