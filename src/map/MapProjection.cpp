#include "MapProjection.h"

#include <QtMath>

MapProjection::MapProjection(GeoPoint origin)
    : m_origin(origin)
    , m_kx(KmPerDeg * std::cos(qDegreesToRadians(origin.lat)))
{
}

QPointF MapProjection::toScene(const GeoPoint& p) const
{
    return {(p.lon - m_origin.lon) * m_kx, -(p.lat - m_origin.lat) * KmPerDeg};
}

GeoPoint MapProjection::toGeo(const QPointF& s) const
{
    return {m_origin.lat - s.y() / KmPerDeg, m_origin.lon + s.x() / m_kx};
}

QRectF MapProjection::sceneRect(double minLat, double minLon, double maxLat, double maxLon) const
{
    return QRectF(toScene({maxLat, minLon}), toScene({minLat, maxLon})).normalized();
}
