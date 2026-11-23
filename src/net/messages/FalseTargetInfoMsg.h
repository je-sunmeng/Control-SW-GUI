#pragma once

#include "model/StandaloneFalseTarget.h"
#include "net/IMessage.h"

#include <QVector>

// 제어 → 재밍기법발생기: 단독허위표적 전체
//   count(u8) + [id(u32), type(u8 0=고정 1=이동 2=랜덤 3=잡음), 타입별 필드]
//     고정: lat(f64), lon(f64)
//     이동: route(RouteCodec)
//     랜덤: (없음)
//     잡음: lat(f64), lon(f64), detectAngleDeg(f32)
class FalseTargetInfoMsg : public IMessage {
public:
    MessageId id() const override { return MessageId::FalseTargetInfo; }
    void writePayload(ByteWriter& w) const override;
    bool readPayload(ByteReader& r) override;
    QString summary() const override;

    QVector<StandaloneFalseTarget> targets;
};
