#include "NoiseSectorItem.h"

#include "common/GeoUtils.h"
#include "map/MapProjection.h"

#include <QPainter>

#include <cmath>

NoiseSectorItem::NoiseSectorItem(const MapProjection& proj, const GeoPoint& radarPos, double rangeKm, double centerDeg,
                                 double widthDeg, const QColor& radarColor, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_proj(proj)
    , m_color(radarColor)
{
    setAcceptedMouseButtons(Qt::NoButton);
    setSector(radarPos, rangeKm, centerDeg, widthDeg);
}

void NoiseSectorItem::setSector(const GeoPoint& radarPos, double rangeKm, double centerDeg, double widthDeg)
{
    if (radarPos.lat == m_radarPos.lat && radarPos.lon == m_radarPos.lon && rangeKm == m_range
        && centerDeg == m_center && widthDeg == m_width && !m_path.isEmpty())
        return;
    m_radarPos = radarPos;
    m_range = rangeKm;
    m_center = centerDeg;
    m_width = widthDeg;

    QPainterPath p(m_proj.toScene(radarPos));
    const int n = std::max(4, int(std::ceil(widthDeg / 1.0)));
    for (int i = 0; i <= n; ++i) {
        const double brg = centerDeg - widthDeg / 2 + widthDeg * i / n;
        p.lineTo(m_proj.toScene(GeoUtils::destination(radarPos, brg, rangeKm)));
    }
    p.closeSubpath();
    prepareGeometryChange();
    m_path = p;
}

QRectF NoiseSectorItem::boundingRect() const
{
    return m_path.boundingRect();
}

void NoiseSectorItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 90));
    painter->drawPath(m_path);

    // 해칭: 화면 기준 패턴 (확대해도 간격 유지)
    QColor hatch = m_color;
    hatch.setAlpha(110);
    QBrush b(hatch, Qt::BDiagPattern);
    b.setTransform(painter->worldTransform().inverted());
    painter->setBrush(b);
    painter->drawPath(m_path);

    QColor edge = m_color;
    edge.setAlpha(170);
    QPen pen(edge, 1.0, Qt::DashLine);
    pen.setCosmetic(true);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(m_path);
}
