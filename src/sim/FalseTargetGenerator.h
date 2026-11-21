#pragma once

#include "SimSnapshot.h"
#include "model/JammingConfig.h"
#include "model/Radar.h"

#include <QHash>
#include <QRandomGenerator>
#include <QVector>

// 편대 허위타겟 위치 / 랜덤 고스트 생성
class FalseTargetGenerator {
public:
    static constexpr double MinSpawnIntervalSec = 0.5;
    static constexpr double MaxSpawnIntervalSec = 2.5;
    static constexpr double MinLifetimeSec = 1.0;
    static constexpr double MaxLifetimeSec = 3.0;
    static constexpr double MinGhostRangeKm = 5.0;

    // 편대 허위타겟: destination(편대위치, 진행방위 + 이격각도(시계방향), 이격거리) — 편대와 함께 강체 이동
    static QVector<GeoPoint> formationTargets(const GeoPoint& formationPos, double headingDeg, const JammingConfig& cfg);

    explicit FalseTargetGenerator(quint32 seed = 0);

    // 시드 재설정 + 상태 초기화 (seed 0 = 매번 다른 난수)
    void reset(quint32 seed = 0);

    // 시뮬레이션 시간 t 까지 고스트 생성/소멸. enabled=false 면 모두 제거
    void updateGhosts(double t, const QVector<Radar>& radars, bool enabled);
    const QVector<GhostState>& ghosts() const { return m_ghosts; }

private:
    double uniform(double a, double b);

    QRandomGenerator m_rng;
    QHash<ObjectId, double> m_nextSpawn;
    QVector<GhostState> m_ghosts;
    quint32 m_nextKey = 1;
};
