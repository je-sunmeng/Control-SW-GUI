#include "DetectionMarkItem.h"

#include <QPainter>
#include <algorithm>
#include <iterator>

DetectionMarkItem::DetectionMarkItem(const QColor& color, double bornAt, double lifetimeSec, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_color(color)
    , m_bornAt(bornAt)
    , m_lifetime(std::max(0.5, lifetimeSec))
{
    setFlag(ItemIgnoresTransformations, true);
    setAcceptedMouseButtons(Qt::NoButton);
}

bool DetectionMarkItem::updateAt(double t)
{
    const double f = (t - m_bornAt) / m_lifetime;
    if (f >= 1.0)
        return false;
    setOpacity(std::clamp(1.0 - f, 0.0, 1.0) * 0.9 + 0.1 * (f < 0.1 ? 1.0 : 0.0));
    return true;
}

QRectF DetectionMarkItem::boundingRect() const
{
    return QRectF(-9, -9, 18, 18);
}

void DetectionMarkItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(QPen(m_color.lighter(140), 1.5));
    painter->setBrush(Qt::NoBrush);
    painter->drawEllipse(QPointF(0, 0), 7, 7);
    painter->setPen(Qt::NoPen);
    painter->setBrush(m_color);
    painter->drawEllipse(QPointF(0, 0), 3.5, 3.5);
}
