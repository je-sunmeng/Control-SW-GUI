#pragma once

#include "model/Route.h"
#include "net/ByteReader.h"
#include "net/ByteWriter.h"
#include <algorithm>
#include <iterator>

// 동선 공통 인코딩 (임시안, ICD 협의 대상)
//   wpCount(u16) + [lat(f64), lon(f64), altKm(f32), speedMps(f32)] × wpCount
//   웨이포인트 i 의 고도/속력 = 구간 i (i → i+1) 값, 마지막 점은 마지막 구간 값
//   루프 동선(전자전기): 시작점을 마지막에 한 번 더 넣어 닫힌 형태로 송신 (마지막 점 == 시작점)
namespace RouteCodec {

inline void write(ByteWriter& w, const Route& route)
{
    const bool loop = route.closed && route.waypointCount() >= 2;
    const int n = std::min(route.waypointCount(), 0xFFFE);
    w.u16(quint16(n + (loop ? 1 : 0)));
    auto segAt = [&](int i) {
        return route.segments.isEmpty() ? RouteSegment{} : route.segments[std::min(i, route.segmentCount() - 1)];
    };
    for (int i = 0; i < n; ++i) {
        const RouteSegment seg = segAt(i);
        w.f64(route.waypoints[i].lat);
        w.f64(route.waypoints[i].lon);
        w.f32(seg.altitudeKm);
        w.f32(seg.speedMps);
    }
    if (loop) {   // 닫는 점 = 시작점 (값은 마지막 구간)
        const RouteSegment seg = segAt(route.segmentCount() - 1);
        w.f64(route.waypoints[0].lat);
        w.f64(route.waypoints[0].lon);
        w.f32(seg.altitudeKm);
        w.f32(seg.speedMps);
    }
}

inline Route read(ByteReader& r)
{
    Route route;
    const int n = r.u16();
    QVector<RouteSegment> perPoint;
    for (int i = 0; i < n && r.ok(); ++i) {
        const double lat = r.f64(), lon = r.f64();
        const double alt = r.f32(), spd = r.f32();
        route.waypoints.append({lat, lon});
        perPoint.append({alt, spd});
    }
    // 마지막 점 == 시작점 (점 3개 이상) → 루프로 복원
    if (route.waypoints.size() >= 3 && route.waypoints.first().lat == route.waypoints.last().lat
        && route.waypoints.first().lon == route.waypoints.last().lon) {
        route.waypoints.removeLast();
        route.closed = true;
        for (int i = 0; i < route.waypoints.size(); ++i)
            route.segments.append(perPoint[i]);
        return route;
    }
    for (int i = 0; i + 1 < perPoint.size(); ++i)
        route.segments.append(perPoint[i]);
    return route;
}

} // namespace RouteCodec
