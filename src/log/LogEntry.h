#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QMetaType>
#include <QString>

enum class LogType : quint8 { Info = 0, Error = 1, Tx = 2, Rx = 3 };
inline constexpr int LogTypeCount = 4;

struct LogEntry {
    QDateTime time;
    LogType type = LogType::Info;
    QString device;        // 장치명 (Info/Error는 출처 모듈명 또는 빈 값)
    quint32 opcode = 0;    // Tx/Rx 전용
    quint32 size = 0;      // Tx/Rx 전용 (DataSize)
    QString content;
    QByteArray raw;        // Tx/Rx 원본 프레임 (hex dump 툴팁용, 선택)

    bool isFrame() const { return type == LogType::Tx || type == LogType::Rx; }
};
Q_DECLARE_METATYPE(LogEntry)

QString toDisplayString(LogType type);   // 정보/오류/송신/수신
QString toFileTag(LogType type);         // INFO/ERROR/TX/RX

// 0x21920010 → "0x2192 0010"
QString formatOpcode(quint32 opcode);

// 16바이트 단위 hex dump (offset | hex | ascii), maxBytes 초과분은 생략 표시
QString hexDump(const QByteArray& data, int maxBytes = 512);

// 파일 기록용 한 줄 포맷
QString formatLogLine(const LogEntry& e);
