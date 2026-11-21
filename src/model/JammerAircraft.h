#pragma once

#include "Route.h"
#include "SimObject.h"

// 전자전기 (시나리오당 1개)
class JammerAircraft : public SimObject {
public:
    ObjectType type() const override { return ObjectType::Jammer; }

    static RouteSegment defaultSegment() { return {9.0, 220.0}; }

    Route route;
};
