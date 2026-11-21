#pragma once

#include "common/Constants.h"
#include "common/Types.h"

#include <QPointF>
#include <QRectF>

// 정거원통(Equirectangular) + 기준위도 cos(φ0) 보정 투영.
//  x = (lon - lon0) · k · cos(lat0),  y = -(lat - lat0) · k
//  k = 111.32 → Scene 1 단위 ≈ 1 km (기준위도 부근)
class MapProjection {
public:
    static constexpr double KmPerDeg = 111.32;

    explicit MapProjection(GeoPoint origin = {Defaults::KoreaCenterLat, Defaults::KoreaCenterLon});

    QPointF toScene(const GeoPoint& p) const;
    GeoPoint toGeo(const QPointF& s) const;

    // 위경도 사각 영역 → Scene 사각형
    QRectF sceneRect(double minLat, double minLon, double maxLat, double maxLon) const;

    GeoPoint origin() const { return m_origin; }
    double xScale() const { return m_kx; }   // scene units per deg lon
    double yScale() const { return KmPerDeg; }

private:
    GeoPoint m_origin;
    double m_kx;
};
