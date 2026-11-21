#pragma once

#include "IDetectionSource.h"

#include <QHash>

// 단독모드 자체 탐지 판정
//  조건: 거리 ≤ maxRange  AND  빔이 표적 방위를 지나감  AND  잡음섹터에 포함되지 않음
//  빔 통과 판정: 직전 틱 빔 방위 → 현재 빔 방위 사이(스캔 방향)의 호에 표적 방위가 있으면 탐지.
//   → 틱 간격·배속과 무관하게 한 바퀴에 정확히 1회 탐지 (|방위−빔방위| ≤ 빔폭/2 를 통과 시점에 만족)
//  재동기화(MainLobe) 로 빔이 점프한 틱은 판정하지 않는다.
class LocalDetectionSource : public IDetectionSource {
public:
    void reset() override;
    QVector<Detection> collect(const SimContext& ctx, double simTime) override;

    // 잡음섹터 판정: |bearing(radar→target) − center| ≤ width/2
    static bool inNoiseSector(double targetBearing, const NoiseSector& s);
    // 스캔 방향으로 prev → cur 사이(prev 제외, cur 포함)에 az 가 있는지
    static bool sweptOver(double prevAz, double curAz, double az, ScanDirection dir, double maxSweepDeg = 360.0);

private:
    struct BeamMemory {
        double az = 0.0;
        bool valid = false;
    };
    QHash<ObjectId, BeamMemory> m_prev;
};
