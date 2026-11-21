#pragma once

#include "model/Route.h"
#include "net/IMessage.h"

// 제어 → 분석SW: 전자전기 동선 (RouteCodec 형식)
class JammerRouteMsg : public IMessage {
public:
    MessageId id() const override { return MessageId::JammerRoute; }
    void writePayload(ByteWriter& w) const override;
    bool readPayload(ByteReader& r) override;
    QString summary() const override;

    Route route;
};
