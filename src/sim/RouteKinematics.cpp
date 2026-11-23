#include "RouteKinematics.h"

#include "common/GeoUtils.h"

#include <algorithm>
#include <cmath>

namespace RouteKinematics {

KinematicState stateAt(const Route& route, double t)
{
    KinematicState s;
    const int n = route.waypointCount();
    if (n == 0)
        return s;
    s.valid = true;
    const int segs = std::min(route.segmentCount(), route.expectedSegmentCount());
    if (n == 1 || segs == 0) {
        s.pos = route.waypoints.first();
        s.finished = true;
        return s;
    }

    double elapsed = std::max(0.0, t);
    if (route.closed) {
        // 루프: 한 바퀴 시간으로 순환 (끝나지 않음)
        const double lap = route.totalDurationSec();
        if (lap > 0)
            elapsed = std::fmod(elapsed, lap);
    }
    for (int i = 0; i < segs; ++i) {
        const GeoPoint a = route.waypoints[i], b = route.waypoints[route.endIndex(i)];
        const RouteSegment& seg = route.segments[i];
        const double dur = route.segmentDurationSec(i);
        if (elapsed <= dur || i == segs - 1) {
            const double f = dur > 0 ? std::min(1.0, elapsed / dur) : 1.0;
            s.pos = GeoUtils::interpolate(a, b, f);
            s.altKm = seg.altitudeKm;
            // 진행 방위: 현재 위치에서 구간 끝점 방향 (대권이므로 구간 중 방위가 변함)
            s.headingDeg = f < 1.0 ? GeoUtils::bearingDeg(s.pos, b)
                                   : GeoUtils::normalizeDeg(GeoUtils::bearingDeg(b, a) + 180.0);
            s.finished = !route.closed && elapsed >= dur && i == segs - 1;
            s.speedMps = s.finished ? 0.0 : seg.speedMps;
            return s;
        }
        elapsed -= dur;
    }
    return s;
}

} // namespace RouteKinematics
