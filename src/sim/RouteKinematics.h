#pragma once

#include "model/Route.h"

struct KinematicState {
    GeoPoint pos;
    double altKm = 0.0;
    double headingDeg = 0.0;   // 진행 방위 (북 0, 시계방향)
    double speedMps = 0.0;     // 정지 시 0
    bool finished = false;     // 마지막 웨이포인트 도달 (이후 정지 유지)
    bool valid = false;        // 웨이포인트 1개 이상
};

// Route + t → 위치/고도/방위 (stateless)
//  - 구간 내 대권 보간, 시간 = 구간 길이 / 구간 속력
//  - t < 0 이면 시작점, 마지막 점 도달 후 정지 (TBD-8: 동선 종료 후 정지로 결정)
namespace RouteKinematics {

KinematicState stateAt(const Route& route, double tSec);

} // namespace RouteKinematics
