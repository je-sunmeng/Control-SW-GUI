#include "RandomGhostItem.h"

#include <QPainter>

#include <algorithm>

RandomGhostItem::RandomGhostItem(const QColor& color, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_color(color)
{
    setFlag(ItemIgnoresTransformations, true);
    setAcceptedMouseButtons(Qt::NoButton);
}

void RandomGhostItem::setState(double progress, double fade)
{
    m_progress = std::clamp(progress, 0.0, 1.0);
    setOpacity(std::clamp(fade, 0.0, 1.0) * MaxOpacity);
    update();
}

QRectF RandomGhostItem::boundingRect() const
{
    const qreal r = MaxRadiusPx + 2;
    return QRectF(-r, -r, 2 * r, 2 * r);
}

void RandomGhostItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    const qreal r = MinRadiusPx + (MaxRadiusPx - MinRadiusPx) * m_progress;
    QPen pen(m_color, 1.3, Qt::CustomDashLine);
    pen.setDashPattern({2.0, 2.0});
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawEllipse(QPointF(0, 0), r, r);
    // 중심 작은 점 (블립 위치)
    painter->setPen(Qt::NoPen);
    painter->setBrush(m_color);
    painter->drawEllipse(QPointF(0, 0), 1.4, 1.4);
}
