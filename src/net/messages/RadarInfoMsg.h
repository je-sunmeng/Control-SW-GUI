#pragma once

#include "model/Radar.h"
#include "net/IMessage.h"

#include <QVector>

// 제어 → 분석SW, 재밍기법발생기: 레이다 전체 정보
//   count(u8) + [id(u32), lat(f64), lon(f64), freqMHz(f32), priMs(f32), pwUs(f32),
//                maxRangeKm(f32), scanPeriodSec(f32), scanDir(u8 0=CW 1=CCW), beamWidthDeg(f32)]
class RadarInfoMsg : public IMessage {
public:
    MessageId id() const override { return MessageId::RadarInfo; }
    void writePayload(ByteWriter& w) const override;
    bool readPayload(ByteReader& r) override;
    QString summary() const override;

    QVector<Radar> radars;
};
