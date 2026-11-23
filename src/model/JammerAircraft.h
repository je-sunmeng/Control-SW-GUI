#pragma once

#include "Route.h"
#include "SimObject.h"

// 전자전기 (시나리오당 1개). 동선은 항상 루프(closed): 마지막 점 → 시작점 자동 연결, 임무 중 반복
class JammerAircraft : public SimObject {
public:
    ObjectType type() const override { return ObjectType::Jammer; }

    static RouteSegment defaultSegment() { return {9.0, 220.0}; }

    Route route;
};
