#include "GraticuleLayer.h"

#include "MapProjection.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include <cmath>

GraticuleLayer::GraticuleLayer(const MapProjection& proj, double minLat, double minLon, double maxLat,
                               double maxLon, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_proj(proj)
    , m_minLat(minLat)
    , m_minLon(minLon)
    , m_maxLat(maxLat)
    , m_maxLon(maxLon)
    , m_bounds(proj.sceneRect(minLat, minLon, maxLat, maxLon))
{
    setFlag(ItemUsesExtendedStyleOption, true);
    setAcceptedMouseButtons(Qt::NoButton);
}

double GraticuleLayer::intervalForLod(double lod, double minSpacingPx)
{
    static constexpr double kSteps[] = {0.1, 0.25, 0.5, 1.0, 2.0, 5.0, 10.0};
    const double pxPerDeg = lod * MapProjection::KmPerDeg;
    for (double s : kSteps)
        if (s * pxPerDeg >= minSpacingPx)
            return s;
    return kSteps[std::size(kSteps) - 1];
}

QRectF GraticuleLayer::boundingRect() const
{
    return m_bounds;
}

void GraticuleLayer::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*)
{
    const double lod = QStyleOptionGraphicsItem::levelOfDetailFromTransform(painter->worldTransform());
    const double step = intervalForLod(lod);

    const QRectF r = (option->exposedRect.isNull() ? m_bounds : option->exposedRect).intersected(m_bounds);
    if (r.isEmpty())
        return;
    const GeoPoint tl = m_proj.toGeo(r.topLeft());
    const GeoPoint br = m_proj.toGeo(r.bottomRight());

    QPen pen(m_color, 1.0, Qt::DotLine);
    pen.setCosmetic(true);
    painter->setPen(pen);
    painter->setRenderHint(QPainter::Antialiasing, false);

    for (double lon = std::ceil(tl.lon / step) * step; lon <= br.lon + 1e-9; lon += step) {
        painter->drawLine(m_proj.toScene({std::max(br.lat, m_minLat), lon}),
                          m_proj.toScene({std::min(tl.lat, m_maxLat), lon}));
    }
    for (double lat = std::ceil(br.lat / step) * step; lat <= tl.lat + 1e-9; lat += step) {
        painter->drawLine(m_proj.toScene({lat, std::max(tl.lon, m_minLon)}),
                          m_proj.toScene({lat, std::min(br.lon, m_maxLon)}));
    }
}
