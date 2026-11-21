#pragma once

#include "SimSnapshot.h"
#include "model/Radar.h"

#include <QVector>

// 탐지 판정 대상 (편대 허위타겟, 고정/이동 단독 허위표적. 랜덤 고스트는 제외)
struct DetectableTarget {
    QString key;
    GeoPoint pos;
};

// 한 틱의 판정 입력
struct SimContext {
    const QVector<Radar>* radars = nullptr;
    QVector<BeamState> beams;               // radars 와 같은 순서
    QVector<DetectableTarget> targets;
    QVector<NoiseSector> noiseSectors;
};

// 탐지 결과 공급 추상화 (모드별 구현 교체)
class IDetectionSource {
public:
    virtual ~IDetectionSource() = default;
    virtual void reset() = 0;
    virtual QVector<Detection> collect(const SimContext& ctx, double simTime) = 0;   // 틱마다 호출
};
