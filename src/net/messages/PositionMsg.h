#pragma once

#include "common/Types.h"
#include "net/IMessage.h"

// 제어 → 신호발생: 현재위치 (전자전기 1 s 주기 / 아군편대 데이터링크 간격)
//   simTime(f64, s) + lat(f64) + lon(f64) + altKm(f32) + headingDeg(f32)
class PositionMsgBase : public IMessage {
public:
    void writePayload(ByteWriter& w) const override;
    bool readPayload(ByteReader& r) override;
    QString summary() const override;

    double simTime = 0.0;
    GeoPoint pos;
    double altKm = 0.0;
    double headingDeg = 0.0;
};

class JammerPositionMsg : public PositionMsgBase {
public:
    MessageId id() const override { return MessageId::JammerPosition; }
};

class FormationPositionMsg : public PositionMsgBase {
public:
    MessageId id() const override { return MessageId::FormationPosition; }
};
