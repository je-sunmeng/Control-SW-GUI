#include "GeoUtils.h"

#include <QtMath>
#include <cmath>

namespace GeoUtils {

namespace {
double rad(double d) { return qDegreesToRadians(d); }
double deg(double r) { return qRadiansToDegrees(r); }
} // namespace

double distanceKm(const GeoPoint& a, const GeoPoint& b)
{
    const double p1 = rad(a.lat), p2 = rad(b.lat);
    const double dp = p2 - p1, dl = rad(b.lon - a.lon);
    const double h = std::sin(dp / 2) * std::sin(dp / 2)
                   + std::cos(p1) * std::cos(p2) * std::sin(dl / 2) * std::sin(dl / 2);
    return 2.0 * EarthRadiusKm * std::atan2(std::sqrt(h), std::sqrt(1.0 - h));
}

double bearingDeg(const GeoPoint& a, const GeoPoint& b)
{
    const double p1 = rad(a.lat), p2 = rad(b.lat), dl = rad(b.lon - a.lon);
    const double y = std::sin(dl) * std::cos(p2);
    const double x = std::cos(p1) * std::sin(p2) - std::sin(p1) * std::cos(p2) * std::cos(dl);
    return normalizeDeg(deg(std::atan2(y, x)));
}

GeoPoint destination(const GeoPoint& start, double bearing, double distance)
{
    const double d = distance / EarthRadiusKm;
    const double t = rad(bearing);
    const double p1 = rad(start.lat), l1 = rad(start.lon);
    const double p2 = std::asin(std::sin(p1) * std::cos(d) + std::cos(p1) * std::sin(d) * std::cos(t));
    const double l2 = l1 + std::atan2(std::sin(t) * std::sin(d) * std::cos(p1),
                                      std::cos(d) - std::sin(p1) * std::sin(p2));
    double lon = deg(l2);
    lon = std::fmod(lon + 540.0, 360.0) - 180.0;
    return {deg(p2), lon};
}

GeoPoint interpolate(const GeoPoint& a, const GeoPoint& b, double f)
{
    const double total = distanceKm(a, b);
    if (total < 1e-9)
        return a;
    return destination(a, bearingDeg(a, b), total * f);
}

double normalizeDeg(double d)
{
    d = std::fmod(d, 360.0);
    if (d < 0)
        d += 360.0;
    return d;
}

double angleDiffDeg(double a, double b)
{
    double d = normalizeDeg(a - b);
    if (d > 180.0)
        d -= 360.0;
    return d;
}

} // namespace GeoUtils
