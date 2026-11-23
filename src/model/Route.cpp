#include "Route.h"

#include "common/GeoUtils.h"

#include <QtGlobal>

int Route::expectedSegmentCount() const
{
    const int n = waypointCount();
    if (n < 2)
        return 0;
    return closed ? n : n - 1;
}

int Route::endIndex(int segment) const
{
    const int n = waypointCount();
    return n == 0 ? 0 : (segment + 1) % n;
}

void Route::addWaypoint(const GeoPoint& p, const RouteSegment& defaultSegment)
{
    if (!waypoints.isEmpty()) {
        const RouteSegment seg = segments.isEmpty() ? clamp(defaultSegment) : segments.last();
        segments.append(seg);
        // 루프: 두 번째 점이 생기는 순간 되돌아오는 구간도 생성
        if (closed && waypoints.size() == 1)
            segments.append(seg);
    }
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
    if (closed) {
        // 루프: 제거 점에서 시작하는 구간 삭제 (직전 구간이 다음 점으로 이어짐). 점이 1개 이하가 되면 구간 없음
        if (waypoints.size() <= 2)
            segments.clear();
        else if (index < segments.size())
            segments.removeAt(index);
    } else if (!segments.isEmpty()) {
        segments.removeAt(index < segments.size() ? index : segments.size() - 1);
    }
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

void Route::setClosed(bool c)
{
    if (closed == c)
        return;
    closed = c;
    const int want = expectedSegmentCount();
    while (segments.size() > want)
        segments.removeLast();
    while (segments.size() < want)
        segments.append(segments.isEmpty() ? RouteSegment{} : segments.last());
}

void Route::clear()
{
    waypoints.clear();
    segments.clear();
}

double Route::segmentLengthKm(int index) const
{
    if (index < 0 || index >= expectedSegmentCount())
        return 0.0;
    return GeoUtils::distanceKm(waypoints[index], waypoints[endIndex(index)]);
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
    for (int i = 0; i < expectedSegmentCount(); ++i)
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
    if (segments.size() != expectedSegmentCount())
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
