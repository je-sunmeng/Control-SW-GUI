#pragma once

#include "common/Types.h"

#include <QString>
#include <QVector>

// 구간 i = waypoints[i] → waypoints[i+1]
struct RouteSegment {
    double altitudeKm = 5.0;
    double speedMps = 200.0;

    bool operator==(const RouteSegment& o) const { return altitudeKm == o.altitudeKm && speedMps == o.speedMps; }
    bool operator!=(const RouteSegment& o) const { return !(*this == o); }
};

namespace RouteLimits {
inline constexpr double MinAltitudeKm = 0.0;
inline constexpr double MaxAltitudeKm = 30.0;
inline constexpr double MinSpeedMps = 1.0;
inline constexpr double MaxSpeedMps = 1500.0;
} // namespace RouteLimits

// 동선: 웨이포인트 목록 + 구간별 고도/속력. 불변식: segments.size() == max(0, waypoints.size()-1)
struct Route {
    QVector<GeoPoint> waypoints;
    QVector<RouteSegment> segments;

    int waypointCount() const { return int(waypoints.size()); }
    int segmentCount() const { return int(segments.size()); }
    bool isEmpty() const { return waypoints.isEmpty(); }

    // 웨이포인트 추가 시 구간 자동 추가: 직전 구간 값 복사, 첫 구간은 defaultSegment
    void addWaypoint(const GeoPoint& p, const RouteSegment& defaultSegment = {});
    bool moveWaypoint(int index, const GeoPoint& p);
    // 웨이포인트 삭제: 해당 점에서 시작하는 구간(마지막 점이면 직전 구간) 함께 삭제
    bool removeWaypoint(int index);
    bool setSegment(int index, const RouteSegment& seg);
    void clear();

    double segmentLengthKm(int index) const;
    double segmentDurationSec(int index) const;
    double totalLengthKm() const;
    double totalDurationSec() const;

    // 웨이포인트 2개 이상, 구간 수 일치, 속력/고도 범위 유효
    bool isValid(QString* reason = nullptr) const;

    static RouteSegment clamp(const RouteSegment& seg);
};
