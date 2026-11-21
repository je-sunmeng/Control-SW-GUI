#include "RouteKinematics.h"

#include "common/GeoUtils.h"
#include <algorithm>
#include <iterator>

namespace RouteKinematics {

KinematicState stateAt(const Route& route, double t)
{
    KinematicState s;
    const int n = route.waypointCount();
    if (n == 0)
        return s;
    s.valid = true;
    if (n == 1 || route.segmentCount() == 0) {
        s.pos = route.waypoints.first();
        s.finished = true;
        return s;
    }

    const int segs = std::min(route.segmentCount(), n - 1);
    double elapsed = std::max(0.0, t);
    for (int i = 0; i < segs; ++i) {
        const GeoPoint a = route.waypoints[i], b = route.waypoints[i + 1];
        const RouteSegment& seg = route.segments[i];
        const double dur = route.segmentDurationSec(i);
        if (elapsed <= dur || i == segs - 1) {
            const double f = dur > 0 ? std::min(1.0, elapsed / dur) : 1.0;
            s.pos = GeoUtils::interpolate(a, b, f);
            s.altKm = seg.altitudeKm;
            // 진행 방위: 현재 위치에서 구간 끝점 방향 (대권이므로 구간 중 방위가 변함)
            s.headingDeg = f < 1.0 ? GeoUtils::bearingDeg(s.pos, b)
                                   : GeoUtils::normalizeDeg(GeoUtils::bearingDeg(b, a) + 180.0);
            s.finished = elapsed >= dur && i == segs - 1;
            s.speedMps = s.finished ? 0.0 : seg.speedMps;
            return s;
        }
        elapsed -= dur;
    }
    return s;   // 도달하지 않음
}

} // namespace RouteKinematics
