#pragma once

#include "MessageId.h"
#include "common/Types.h"

#include <QHash>
#include <QStringList>
#include <QVector>

#include <optional>

class Logger;

struct ProtocolEntry {
    MessageId id;
    quint8 src = 0;
    quint8 dst = 0;
    quint16 cmd = 0;
};

// protocol.json 로드: MessageId ↔ (Src, Dst, Cmd) 양방향 조회
//  - 같은 의미의 메시지라도 목적지별 Command Code 가 다를 수 있으므로 (MessageId, Dst) 단위 매핑
//  - 수신: (Src, Dst, Cmd) → MessageId 역조회
//  검증: 알 수 없는 id / 중복 (src,dst,cmd) → 오류(해당 항목 무시), 코드에 있는 메시지 매핑 누락 → 경고
class ProtocolTable {
public:
    static QString defaultResourcePath() { return QStringLiteral(":/protocol/protocol.json"); }
    static QString overrideFileName() { return QStringLiteral("protocol.json"); }

    // 실행파일 옆 protocol.json 이 있으면 우선, 없으면 리소스 기본값. 결과를 Logger 에 기록
    bool loadDefault(Logger* logger, const QString& appDir);
    bool loadFile(const QString& path, QStringList* errors = nullptr, QStringList* warnings = nullptr);
    bool loadJson(const QByteArray& json, QStringList* errors = nullptr, QStringList* warnings = nullptr);

    std::optional<quint16> cmdFor(MessageId id, DeviceCode src, DeviceCode dst) const;
    std::optional<quint32> opcodeFor(MessageId id, DeviceCode src, DeviceCode dst) const;
    std::optional<MessageId> idFor(quint8 src, quint8 dst, quint16 cmd) const;
    std::optional<MessageId> idForOpcode(quint32 opcode) const;

    QString version() const { return m_version; }
    QString sourcePath() const { return m_source; }
    const QVector<ProtocolEntry>& entries() const { return m_entries; }
    int size() const { return int(m_entries.size()); }

private:
    static quint64 key(quint8 src, quint8 dst, quint16 cmd) { return (quint64(src) << 24) | (quint64(dst) << 16) | cmd; }

    QString m_version;
    QString m_source;
    QVector<ProtocolEntry> m_entries;
    QHash<quint64, MessageId> m_byOpcode;
};
