#include "RadarItem.h"

#include "WaypointHandleItem.h"
#include "common/ColorPalette.h"
#include "common/GeoUtils.h"
#include "map/MapProjection.h"
#include "model/ScenarioModel.h"

#include <QPainter>

RadarItem::RadarItem(ObjectId id, const MapProjection& proj)
    : MapObjectItem(id, proj)
    , m_handle(new WaypointHandleItem(WaypointHandleItem::Shape::Diamond, Qt::white, 14, this))
{
}

void RadarItem::sync(const ScenarioModel& model)
{
    const Radar* r = model.radar(m_id);
    if (!r)
        return;
    m_color = ColorPalette::radar(r->colorIndex);
    m_handle->setColor(m_color);
    m_handle->setLabel(r->name);
    m_handle->syncPos(m_proj.toScene(r->pos));

    // 최대탐지거리 원: 지리좌표에서 방위별 목적지점 생성 후 투영 (투영 왜곡 반영)
    QPainterPath p;
    for (int i = 0; i <= CircleSegments; ++i) {
        const QPointF s = m_proj.toScene(GeoUtils::destination(r->pos, 360.0 * i / CircleSegments, r->maxRangeKm));
        i == 0 ? p.moveTo(s) : p.lineTo(s);
    }
    p.closeSubpath();
    prepareGeometryChange();
    m_rangePath = p;
}

void RadarItem::setHighlighted(bool on)
{
    m_highlighted = on;
    m_handle->setHighlighted(on);
    update();
}

void RadarItem::setEditable(bool on)
{
    m_handle->setEditable(on);
}

QRectF RadarItem::boundingRect() const
{
    return m_rangePath.boundingRect().adjusted(-1, -1, 1, 1);
}

void RadarItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    QColor stroke = m_color;
    stroke.setAlpha(m_highlighted ? 230 : 150);
    QColor fill = m_color;
    fill.setAlpha(m_highlighted ? 40 : 22);
    QPen pen(stroke, m_highlighted ? 2.0 : 1.2, m_highlighted ? Qt::SolidLine : Qt::DashLine);
    pen.setCosmetic(true);
    painter->setPen(pen);
    painter->setBrush(fill);
    painter->drawPath(m_rangePath);
}
