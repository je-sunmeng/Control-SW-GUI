#pragma once

#include "net/IMessage.h"

// 미등록 Opcode 수신 / 파싱 실패 시 로그용 (hex dump)
class RawMessage : public IMessage {
public:
    MessageId id() const override { return MessageId::Raw; }
    void writePayload(ByteWriter& w) const override { w.bytes(data); }
    bool readPayload(ByteReader& r) override
    {
        data = r.bytes(r.remaining());
        return r.ok();
    }
    QString summary() const override
    {
        return QStringLiteral("미등록/해석불가 (%1 bytes) %2")
            .arg(data.size())
            .arg(QString::fromLatin1(data.left(16).toHex(' ').toUpper()) + (data.size() > 16 ? QStringLiteral(" …") : QString()));
    }

    quint32 opcode = 0;
    QByteArray data;
};
