#include "Route.h"

#include "common/GeoUtils.h"

#include <QtGlobal>

void Route::addWaypoint(const GeoPoint& p, const RouteSegment& defaultSegment)
{
    if (!waypoints.isEmpty())
        segments.append(segments.isEmpty() ? clamp(defaultSegment) : segments.last());
    waypoints.append(p);
}

bool Route::moveWaypoint(int index, const GeoPoint& p)
{
    if (index < 0 || index >= waypoints.size())
        return false;
    waypoints[index] = p;
    return true;
}

bool Route::removeWaypoint(int index)
{
    if (index < 0 || index >= waypoints.size())
        return false;
    if (!segments.isEmpty())
        segments.removeAt(index < segments.size() ? index : segments.size() - 1);
    waypoints.removeAt(index);
    return true;
}

bool Route::setSegment(int index, const RouteSegment& seg)
{
    if (index < 0 || index >= segments.size())
        return false;
    segments[index] = clamp(seg);
    return true;
}

void Route::clear()
{
    waypoints.clear();
    segments.clear();
}

double Route::segmentLengthKm(int index) const
{
    if (index < 0 || index + 1 >= waypoints.size())
        return 0.0;
    return GeoUtils::distanceKm(waypoints[index], waypoints[index + 1]);
}

double Route::segmentDurationSec(int index) const
{
    if (index < 0 || index >= segments.size() || segments[index].speedMps <= 0)
        return 0.0;
    return segmentLengthKm(index) * 1000.0 / segments[index].speedMps;
}

double Route::totalLengthKm() const
{
    double sum = 0;
    for (int i = 0; i + 1 < waypoints.size(); ++i)
        sum += segmentLengthKm(i);
    return sum;
}

double Route::totalDurationSec() const
{
    double sum = 0;
    for (int i = 0; i < segments.size(); ++i)
        sum += segmentDurationSec(i);
    return sum;
}

bool Route::isValid(QString* reason) const
{
    auto fail = [reason](const QString& r) {
        if (reason)
            *reason = r;
        return false;
    };
    if (waypoints.size() < 2)
        return fail(QStringLiteral("웨이포인트가 2개 이상 필요합니다"));
    if (segments.size() != waypoints.size() - 1)
        return fail(QStringLiteral("구간 수가 웨이포인트 수와 맞지 않습니다"));
    for (int i = 0; i < segments.size(); ++i) {
        const RouteSegment& s = segments[i];
        if (s.speedMps < RouteLimits::MinSpeedMps || s.speedMps > RouteLimits::MaxSpeedMps)
            return fail(QStringLiteral("구간 %1 속력이 범위를 벗어났습니다").arg(i + 1));
        if (s.altitudeKm < RouteLimits::MinAltitudeKm || s.altitudeKm > RouteLimits::MaxAltitudeKm)
            return fail(QStringLiteral("구간 %1 고도가 범위를 벗어났습니다").arg(i + 1));
    }
    return true;
}

RouteSegment Route::clamp(const RouteSegment& seg)
{
    return {qBound(RouteLimits::MinAltitudeKm, seg.altitudeKm, RouteLimits::MaxAltitudeKm),
            qBound(RouteLimits::MinSpeedMps, seg.speedMps, RouteLimits::MaxSpeedMps)};
}
