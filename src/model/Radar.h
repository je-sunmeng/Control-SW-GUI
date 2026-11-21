#pragma once

#include "SimObject.h"

namespace RadarLimits {
inline constexpr double MinFreqMHz = 1.0, MaxFreqMHz = 100000.0;
inline constexpr double MinPriMs = 0.001, MaxPriMs = 100.0;
inline constexpr double MinPwUs = 0.01, MaxPwUs = 10000.0;
inline constexpr double MinRangeKm = 1.0, MaxRangeKm = 2000.0;
inline constexpr double MinScanPeriodSec = 0.1, MaxScanPeriodSec = 600.0;
inline constexpr double MinBeamWidthDeg = 0.1, MaxBeamWidthDeg = 90.0;
} // namespace RadarLimits

class Radar : public SimObject {
public:
    ObjectType type() const override { return ObjectType::Radar; }

    GeoPoint pos;
    double freqMHz = 9400.0;
    double priMs = 1.0;
    double pwUs = 1.0;
    double maxRangeKm = 300.0;
    double scanPeriodSec = 10.0;
    ScanDirection scanDir = ScanDirection::Clockwise;
    double beamWidthDeg = 2.0;
    int colorIndex = 0;   // 0~3

    // 범위를 벗어난 값은 경계값으로 보정
    void clampValues();
};
