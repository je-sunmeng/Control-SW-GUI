#include "RandomGhostItem.h"

#include <QPainter>

RandomGhostItem::RandomGhostItem(const QColor& color, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_color(color)
{
    setFlag(ItemIgnoresTransformations, true);
    setAcceptedMouseButtons(Qt::NoButton);
}

QRectF RandomGhostItem::boundingRect() const
{
    return QRectF(-10, -10, 20, 14);
}

void RandomGhostItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    QColor fill = m_color;
    fill.setAlpha(110);
    painter->setPen(QPen(m_color.lighter(130), 1.2));
    painter->setBrush(fill);
    // 위쪽 반원 (레이다 화면의 고스트 반향 느낌)
    QPainterPath p;
    p.moveTo(-8, 2);
    p.arcTo(QRectF(-8, -6, 16, 16), 180, -180);
    p.closeSubpath();
    painter->drawPath(p);
}
