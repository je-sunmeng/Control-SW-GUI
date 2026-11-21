#pragma once

#include "net/IMessage.h"

// 제어 → 양쪽: 임무종료 (payload 없음)
class MissionStopMsg : public IMessage {
public:
    MessageId id() const override { return MessageId::MissionStop; }
    void writePayload(ByteWriter&) const override {}
    bool readPayload(ByteReader& r) override { return r.ok(); }
    QString summary() const override { return QStringLiteral("MissionStop"); }
};
