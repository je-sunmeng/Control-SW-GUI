#include "JammingConfig.h"

#include "common/Constants.h"
#include "common/GeoUtils.h"

#include <QtGlobal>

void JammingConfig::resize(int count)
{
    count = qBound(0, count, Limits::MaxFormationFalseTargets);
    const int old = int(targets.size());
    targets.resize(count);
    for (int i = old; i < count; ++i) {
        // 기본: 진행방향 기준 45° 간격으로 번갈아 좌/우, 거리 10 km 단위 증가
        const int k = i / 2 + 1;
        targets[i].angleDeg = GeoUtils::normalizeDeg((i % 2 == 0 ? 1 : -1) * 45.0 * k);
        targets[i].distanceKm = 10.0 * k;
    }
}

void JammingConfig::clampValues()
{
    if (targets.size() > Limits::MaxFormationFalseTargets)
        targets.resize(Limits::MaxFormationFalseTargets);
    for (FalseTargetOffset& t : targets) {
        t.angleDeg = GeoUtils::normalizeDeg(t.angleDeg);
        t.distanceKm = qBound(JammingLimits::MinDistanceKm, t.distanceKm, JammingLimits::MaxDistanceKm);
    }
    noiseBandwidthMHz = qBound(JammingLimits::MinNoiseBandwidthMHz, noiseBandwidthMHz,
                               JammingLimits::MaxNoiseBandwidthMHz);
}
