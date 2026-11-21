#include "PlatformItem.h"

#include <QFontMetricsF>
#include <QPainter>

namespace {
constexpr qreal kHalf = 18.0;   // 바운딩 반 크기(px)
}

PlatformItem::PlatformItem(Kind kind, const QColor& color, const QString& label, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_kind(kind)
    , m_color(color)
    , m_label(label)
{
    setFlag(ItemIgnoresTransformations, true);
    setAcceptedMouseButtons(Qt::NoButton);
}

void PlatformItem::setState(const QPointF& scenePos, double headingDeg, int formationSize)
{
    setPos(scenePos);
    setRotation(headingDeg);
    if (formationSize != m_size) {
        m_size = formationSize;
        update();
    }
}

void PlatformItem::setLabel(const QString& label)
{
    if (label == m_label)
        return;
    prepareGeometryChange();
    m_label = label;
}

QRectF PlatformItem::boundingRect() const
{
    // 라벨은 회전과 무관하게 오른쪽 위에 그리므로 넉넉히
    return QRectF(-kHalf - 130, -kHalf - 130, 2 * kHalf + 260, 2 * kHalf + 260);
}

void PlatformItem::drawAircraft(QPainter* p, const QPointF& c, qreal s) const
{
    // 기수(−y) 방향 화살촉 모양
    QPainterPath path;
    path.moveTo(c + QPointF(0, -s));
    path.lineTo(c + QPointF(s * 0.75, s * 0.8));
    path.lineTo(c + QPointF(0, s * 0.35));
    path.lineTo(c + QPointF(-s * 0.75, s * 0.8));
    path.closeSubpath();
    p->drawPath(path);
}

void PlatformItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(QPen(m_color.darker(250), 1.2));
    painter->setBrush(m_color);

    switch (m_kind) {
    case Kind::Jammer:
        drawAircraft(painter, {0, 0}, 11);
        // 재밍 표시: 기체 주위 점선 원
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(m_color, 1.0, Qt::DotLine));
        painter->drawEllipse(QPointF(0, 0), 16, 16);
        break;
    case Kind::Formation: {
        // 편대규모만큼 V 대형 (선두 + 좌우 후방)
        static const QPointF offs[] = {{0, -4}, {-10, 6}, {10, 6}, {0, 14}};
        for (int i = 0; i < std::clamp(m_size, 1, 4); ++i)
            drawAircraft(painter, offs[i], 7);
        break;
    }
    case Kind::MovingTarget: {
        QPainterPath d;
        d.moveTo(0, -8);
        d.lineTo(7, 6);
        d.lineTo(-7, 6);
        d.closeSubpath();
        painter->drawPath(d);
        painter->drawLine(QPointF(0, -8), QPointF(0, -14));   // 진행방향
        break;
    }
    case Kind::FormationFalseTarget: {
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(m_color, 1.6));
        QPainterPath d;
        d.moveTo(0, -6);
        d.lineTo(6, 0);
        d.lineTo(0, 6);
        d.lineTo(-6, 0);
        d.closeSubpath();
        painter->drawPath(d);
        break;
    }
    }

    if (m_label.isEmpty())
        return;
    // 라벨은 화면 기준 수평으로
    painter->rotate(-rotation());
    QFont f = painter->font();
    f.setPointSize(8);
    painter->setFont(f);
    const QFontMetricsF fm(f);
    const QRectF box(14, -22, fm.horizontalAdvance(m_label) + 6, fm.height() + 2);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0x0b, 0x14, 0x20, 170));
    painter->drawRoundedRect(box, 2, 2);
    painter->setPen(m_color.lighter(130));
    painter->drawText(box, Qt::AlignCenter, m_label);
}
