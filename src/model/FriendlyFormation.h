#pragma once

#include "JammingConfig.h"
#include "Route.h"
#include "SimObject.h"

#include "common/Constants.h"

// 아군편대 (시나리오당 최대 1개)
class FriendlyFormation : public SimObject {
public:
    ObjectType type() const override { return ObjectType::Formation; }

    static RouteSegment defaultSegment() { return {6.0, 250.0}; }

    Route route;
    int formationSize = Limits::MaxFormationSize;   // 1~4
    JammingConfig jamming;
};
