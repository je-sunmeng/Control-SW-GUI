#pragma once

#include "common/Types.h"

#include <QString>
#include <QVector>

// 구간 i = waypoints[i] → waypoints[endIndex(i)]
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

// 동선: 웨이포인트 목록 + 구간별 고도/속력
//  - 열린 동선(open): 구간 수 = N−1, 마지막 점에서 정지
//  - 루프(closed, 전자전기): 마지막 점 → 시작점 구간이 자동으로 추가되어 구간 수 = N (N ≥ 2), 임무 중 반복
// 불변식: segments.size() == expectedSegmentCount()
struct Route {
    QVector<GeoPoint> waypoints;
    QVector<RouteSegment> segments;
    bool closed = false;

    int waypointCount() const { return int(waypoints.size()); }
    int segmentCount() const { return int(segments.size()); }
    bool isEmpty() const { return waypoints.isEmpty(); }
    int expectedSegmentCount() const;
    int endIndex(int segment) const;   // 구간 끝점 인덱스 (루프의 마지막 구간은 0)

    // 웨이포인트 추가 시 구간 자동 추가: 직전 구간 값 복사, 첫 구간은 defaultSegment
    void addWaypoint(const GeoPoint& p, const RouteSegment& defaultSegment = {});
    bool moveWaypoint(int index, const GeoPoint& p);
    // 웨이포인트 삭제: 해당 점에서 시작하는 구간(열린 동선의 마지막 점이면 직전 구간) 함께 삭제
    bool removeWaypoint(int index);
    bool setSegment(int index, const RouteSegment& seg);
    void setClosed(bool closed);   // 구간 수를 새 형태에 맞게 조정
    void clear();

    double segmentLengthKm(int index) const;
    double segmentDurationSec(int index) const;
    double totalLengthKm() const;
    double totalDurationSec() const;   // 루프면 한 바퀴 시간

    // 웨이포인트 2개 이상, 구간 수 일치, 속력/고도 범위 유효
    bool isValid(QString* reason = nullptr) const;

    static RouteSegment clamp(const RouteSegment& seg);
};
