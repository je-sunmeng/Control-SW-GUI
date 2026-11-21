#pragma once

#include "ByteReader.h"
#include "ByteWriter.h"
#include "MessageId.h"

#include <QSharedPointer>

// 메시지 인터페이스. payload 만 담당 (헤더/DataSize 는 NetworkManager 가 기입)
// 규칙: docs/PROTOCOL_GUIDE.md 3장 (write/read 필드 순서 동일, ByteWriter/Reader 만 사용)
class IMessage {
public:
    virtual ~IMessage() = default;
    virtual MessageId id() const = 0;
    virtual void writePayload(ByteWriter& w) const = 0;
    virtual bool readPayload(ByteReader& r) = 0;   // 언더런/남은 바이트 → false
    virtual QString summary() const = 0;           // 로그 '내용' 컬럼

    QByteArray payload() const
    {
        ByteWriter w;
        writePayload(w);
        return w.data();
    }
};

using MessagePtr = QSharedPointer<IMessage>;
Q_DECLARE_METATYPE(MessagePtr)
