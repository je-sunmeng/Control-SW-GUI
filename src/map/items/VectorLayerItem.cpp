#include "VectorLayerItem.h"

#include "map/VectorLayer.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>

namespace {
constexpr qreal kPointMarginKm = 120.0;   // 포인트 라벨이 경계 밖으로 나가는 여유
}

VectorLayerItem::VectorLayerItem(const VectorLayer& layer, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_layer(layer)
{
    m_bounds = layer.boundingRect();
    if (!layer.points().isEmpty())
        m_bounds.adjust(-kPointMarginKm, -kPointMarginKm, kPointMarginKm, kPointMarginKm);
    setZValue(layer.zValue());
    setFlag(ItemUsesExtendedStyleOption, true);
    setAcceptedMouseButtons(Qt::NoButton);
}

QRectF VectorLayerItem::boundingRect() const
{
    return m_bounds;
}

void VectorLayerItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*)
{
    const double lod = QStyleOptionGraphicsItem::levelOfDetailFromTransform(painter->worldTransform());
    if (lod < m_layer.minLod())
        return;

    const LayerStyle st = m_layer.style();
    const QPainterPath& path = m_layer.pathForLod(lod);
    if (!path.isEmpty() && path.intersects(option->exposedRect.isNull() ? m_bounds : option->exposedRect)) {
        painter->setRenderHint(QPainter::Antialiasing, true);
        if (st.stroke.isValid()) {
            QPen pen(st.stroke, st.width, st.penStyle, Qt::RoundCap, Qt::RoundJoin);
            pen.setCosmetic(true);
            painter->setPen(pen);
        } else {
            painter->setPen(Qt::NoPen);
        }
        painter->setBrush(m_layer.hasArea() && st.fill.isValid() ? QBrush(st.fill) : Qt::NoBrush);
        painter->drawPath(path);
    }

    if (!m_layer.points().isEmpty())
        paintPoints(painter, lod);
}

void VectorLayerItem::paintPoints(QPainter* painter, double lod) const
{
    const LayerStyle st = m_layer.style();
    const QTransform xf = painter->worldTransform();
    const bool showLabels = lod >= m_layer.labelMinLod();

    painter->save();
    painter->resetTransform();
    painter->setRenderHint(QPainter::Antialiasing, true);
    QFont font = painter->font();
    font.setPointSize(st.labelPointSize);
    painter->setFont(font);

    const QColor c = st.stroke.isValid() ? st.stroke : st.labelColor;
    const qreal s = 4.5;   // 심볼 반 크기(px)
    painter->setPen(QPen(c.darker(150), 1));
    painter->setBrush(c);
    for (const VectorLayer::PointFeature& p : m_layer.points()) {
        const QPointF d = xf.map(p.scenePos);
        const QPolygonF tri{QPointF(d.x(), d.y() - s), QPointF(d.x() - s, d.y() + s * 0.8),
                            QPointF(d.x() + s, d.y() + s * 0.8)};
        painter->drawPolygon(tri);
    }
    if (showLabels) {
        painter->setPen(st.labelColor.isValid() ? st.labelColor : c);
        for (const VectorLayer::PointFeature& p : m_layer.points()) {
            if (p.label.isEmpty())
                continue;
            const QPointF d = xf.map(p.scenePos);
            painter->drawText(QPointF(d.x() + s + 3, d.y() + s * 0.8), p.label);
        }
    }
    painter->restore();
}
