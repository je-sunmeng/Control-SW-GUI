#pragma once

#include <QByteArray>
#include <QtGlobal>

#include <optional>

// 헤더 8 바이트 (BigEndian): Opcode(32) + DataSize(32, payload 바이트 수)
struct MessageHeader {
    static constexpr int Size = 8;

    quint32 opcode = 0;
    quint32 dataSize = 0;

    QByteArray encode() const;
    static std::optional<MessageHeader> decode(const QByteArray& data);   // 8 바이트 미만이면 nullopt

    // 헤더 + payload 로 프레임 구성 (DataSize 자동 기입)
    static QByteArray frame(quint32 opcode, const QByteArray& payload);
};
