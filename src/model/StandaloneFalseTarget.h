#pragma once

#include "Route.h"
#include "SimObject.h"

namespace FalseTargetLimits {
inline constexpr double MinDetectAngleDeg = 0.1, MaxDetectAngleDeg = 360.0;
} // namespace FalseTargetLimits

// 단독 허위표적. 타입별 사용 필드:
//  Fixed  : pos
//  Moving : route
//  Random : (없음 — 각 레이다 탐지범위 내 랜덤 생성)
//  Noise  : pos + detectAngleDeg (레이다→잡음표적 방위 ±각/2 섹터 탐지불가)
class StandaloneFalseTarget : public SimObject {
public:
    ObjectType type() const override { return ObjectType::StandaloneFalseTarget; }

    static RouteSegment defaultSegment() { return {3.0, 150.0}; }

    FalseTargetType ftType = FalseTargetType::Fixed;
    GeoPoint pos;
    bool posSet = false;   // Fixed/Noise: 위치 지정 여부
    Route route;
    double detectAngleDeg = 10.0;

    bool usesPosition() const { return ftType == FalseTargetType::Fixed || ftType == FalseTargetType::Noise; }
    bool usesRoute() const { return ftType == FalseTargetType::Moving; }

    bool validate(QString* reason = nullptr) const;
};
