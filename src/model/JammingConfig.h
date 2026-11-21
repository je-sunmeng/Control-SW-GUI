#pragma once

#include <QVector>

// 편대 허위타겟 1개의 이격 (편대 이동방향 기준 시계방향 각도, 이격거리는 임무 중 고정)
struct FalseTargetOffset {
    double angleDeg = 0.0;
    double distanceKm = 10.0;

    bool operator==(const FalseTargetOffset& o) const { return angleDeg == o.angleDeg && distanceKm == o.distanceKm; }
};

namespace JammingLimits {
inline constexpr double MinDistanceKm = 0.0, MaxDistanceKm = 500.0;
inline constexpr double MinNoiseBandwidthMHz = 0.0, MaxNoiseBandwidthMHz = 10000.0;
} // namespace JammingLimits

// 교란설정: 허위타겟 0~8개 + 잡음신호폭
struct JammingConfig {
    QVector<FalseTargetOffset> targets;
    double noiseBandwidthMHz = 10.0;

    // 개수 변경 (늘어나면 기본값으로 채움: 각도는 등간격)
    void resize(int count);
    // 범위 보정 (개수 0~8, 각도 [0,360), 거리/대역폭 범위)
    void clampValues();

    bool operator==(const JammingConfig& o) const
    {
        return targets == o.targets && noiseBandwidthMHz == o.noiseBandwidthMHz;
    }
};
