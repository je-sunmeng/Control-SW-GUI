#include "WaypointHandleItem.h"

#include <QCursor>
#include <QFontMetricsF>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>

namespace {
constexpr qreal kLabelWidth = 160;
}

WaypointHandleItem::WaypointHandleItem(Shape shape, const QColor& color, qreal sizePx, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_shape(shape)
    , m_color(color)
    , m_size(sizePx)
{
    setFlag(ItemIgnoresTransformations, true);
    setFlag(ItemSendsGeometryChanges, true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setEditable(true);
}

void WaypointHandleItem::setColor(const QColor& c)
{
    m_color = c;
    update();
}

void WaypointHandleItem::setLabel(const QString& label)
{
    if (label == m_label)
        return;
    prepareGeometryChange();
    m_label = label;
}

void WaypointHandleItem::setHighlighted(bool on)
{
    m_highlighted = on;
    update();
}

void WaypointHandleItem::setEditable(bool on)
{
    m_editable = on;
    setFlag(ItemIsMovable, on);
    setCursor(on ? Qt::SizeAllCursor : Qt::ArrowCursor);
}

void WaypointHandleItem::syncPos(const QPointF& scenePos)
{
    if (m_dragging)
        return;
    m_syncing = true;
    setPos(parentItem() ? parentItem()->mapFromScene(scenePos) : scenePos);
    m_syncing = false;
}

QRectF WaypointHandleItem::boundingRect() const
{
    const qreal h = m_size / 2 + 3;
    QRectF r(-h, -h, 2 * h, 2 * h);
    if (!m_label.isEmpty())
        r = r.united(QRectF(h, -10, kLabelWidth, 20));
    return r;
}

QPainterPath WaypointHandleItem::shape() const
{
    QPainterPath p;
    const qreal h = m_size / 2 + 3;   // 클릭 여유
    p.addRect(QRectF(-h, -h, 2 * h, 2 * h));
    return p;
}

QPainterPath WaypointHandleItem::symbolPath() const
{
    const qreal h = m_size / 2;
    QPainterPath p;
    switch (m_shape) {
    case Shape::Square:
        p.addRect(QRectF(-h, -h, 2 * h, 2 * h));
        break;
    case Shape::Circle:
        p.addEllipse(QPointF(0, 0), h, h);
        break;
    case Shape::Diamond:
        p.moveTo(0, -h);
        p.lineTo(h, 0);
        p.lineTo(0, h);
        p.lineTo(-h, 0);
        p.closeSubpath();
        break;
    case Shape::Triangle:
        p.moveTo(0, -h);
        p.lineTo(h, h * 0.8);
        p.lineTo(-h, h * 0.8);
        p.closeSubpath();
        break;
    case Shape::Cross:
        p.addEllipse(QPointF(0, 0), h, h);
        p.moveTo(-h * 0.7, -h * 0.7);
        p.lineTo(h * 0.7, h * 0.7);
        p.moveTo(-h * 0.7, h * 0.7);
        p.lineTo(h * 0.7, -h * 0.7);
        break;
    }
    return p;
}

void WaypointHandleItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    const QColor outline = m_highlighted ? QColor(Qt::white) : m_color.darker(220);
    painter->setPen(QPen(outline, m_highlighted ? 2.0 : 1.2));
    painter->setBrush(m_shape == Shape::Cross ? QBrush(m_color.darker(300)) : QBrush(m_color));
    const QPainterPath p = symbolPath();
    painter->drawPath(p);
    if (m_shape == Shape::Cross) {
        painter->setPen(QPen(m_color, 1.6));
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(p);
    }

    if (!m_label.isEmpty()) {
        QFont f = painter->font();
        f.setPointSize(9);
        f.setBold(m_highlighted);
        painter->setFont(f);
        const qreal x = m_size / 2 + 5;
        const QFontMetricsF fm(f);
        const QRectF box(x - 2, -fm.height() / 2 - 1, fm.horizontalAdvance(m_label) + 4, fm.height() + 2);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0x0b, 0x14, 0x20, 170));
        painter->drawRoundedRect(box, 2, 2);
        painter->setPen(m_highlighted ? QColor(Qt::white) : m_color.lighter(120));
        painter->drawText(box, Qt::AlignCenter, m_label);
    }
}

void WaypointHandleItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_onPress)
        m_onPress();
    if (m_editable && event->button() == Qt::LeftButton)
        m_dragging = true;
    QGraphicsItem::mousePressEvent(event);
}

void WaypointHandleItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItem::mouseReleaseEvent(event);
    if (m_dragging) {
        m_dragging = false;
        if (m_onMove)
            m_onMove(scenePos(), true);
    }
}

QVariant WaypointHandleItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionHasChanged && m_dragging && !m_syncing && m_onMove)
        m_onMove(scenePos(), false);
    return QGraphicsItem::itemChange(change, value);
}
