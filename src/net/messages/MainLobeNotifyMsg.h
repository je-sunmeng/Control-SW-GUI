#pragma once

#include "common/Types.h"
#include "net/IMessage.h"

#include <QVector>

// 재밍기법발생기 → 제어: 레이다별 MainLobe 탐색 시점 + 탐지결과 (연동모드 빔 동기화·탐지 전시의 원천)
//   radarId(u32) + searchTime(f64, 시뮬레이션 s) + detCount(u8) + [lat(f64), lon(f64)] × detCount
//   (탐지 위치 표현은 TBD-4: 위경도 vs 거리·방위 — 협의 후 변경)
class MainLobeNotifyMsg : public IMessage {
public:
    MessageId id() const override { return MessageId::MainLobeNotify; }
    void writePayload(ByteWriter& w) const override;
    bool readPayload(ByteReader& r) override;
    QString summary() const override;

    quint32 radarId = 0;
    double searchTime = 0.0;
    QVector<GeoPoint> detections;
};
