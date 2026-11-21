#pragma once

#include "model/Route.h"
#include "net/ByteReader.h"
#include "net/ByteWriter.h"
#include <algorithm>
#include <iterator>

// 동선 공통 인코딩 (임시안, ICD 협의 대상)
//   wpCount(u16) + [lat(f64), lon(f64), altKm(f32), speedMps(f32)] × wpCount
//   웨이포인트 i 의 고도/속력 = 구간 i (i → i+1) 값, 마지막 점은 마지막 구간 값
namespace RouteCodec {

inline void write(ByteWriter& w, const Route& route)
{
    const int n = std::min(route.waypointCount(), 0xFFFF);
    w.u16(quint16(n));
    for (int i = 0; i < n; ++i) {
        const RouteSegment seg = route.segments.isEmpty() ? RouteSegment{}
                                                          : route.segments[std::min(i, route.segmentCount() - 1)];
        w.f64(route.waypoints[i].lat);
        w.f64(route.waypoints[i].lon);
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
    for (int i = 0; i + 1 < perPoint.size(); ++i)
        route.segments.append(perPoint[i]);
    return route;
}

} // namespace RouteCodec
