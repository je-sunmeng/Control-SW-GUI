#include "DetectionMarkItem.h"

#include <QPainter>
#include <algorithm>
#include <iterator>

DetectionMarkItem::DetectionMarkItem(const QColor& color, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_color(color)
{
    setFlag(ItemIgnoresTransformations, true);
    setAcceptedMouseButtons(Qt::NoButton);
}

QRectF DetectionMarkItem::boundingRect() const
{
    return QRectF(-6, -6, 12, 12);
}

void DetectionMarkItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    // 계속 표시되므로 누적돼도 지도를 가리지 않게 작게 (테두리 점)
    painter->setPen(QPen(m_color.darker(180), 1.0));
    painter->setBrush(m_color);
    painter->drawEllipse(QPointF(0, 0), 3.2, 3.2);
}
