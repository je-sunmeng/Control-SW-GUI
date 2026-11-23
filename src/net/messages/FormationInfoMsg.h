#pragma once

#include "model/FriendlyFormation.h"
#include "net/IMessage.h"

// 제어 → 분석SW, 재밍기법발생기: 아군편대 동선 (+ 재밍기법발생기에는 교란설정 포함)
//   route(RouteCodec) + size(u8) + hasJamming(u8)
//   [hasJamming=1] ftCount(u8) + [angleDeg(f32), distanceKm(f32)] × ftCount + noiseBandwidthMHz(f32)
class FormationInfoMsg : public IMessage {
public:
    MessageId id() const override { return MessageId::FormationInfo; }
    void writePayload(ByteWriter& w) const override;
    bool readPayload(ByteReader& r) override;
    QString summary() const override;

    FriendlyFormation formation;
    bool includeJamming = true;
};
