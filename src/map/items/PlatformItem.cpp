#include "PlatformItem.h"

#include <QFontMetricsF>
#include <QPainter>
#include <algorithm>
#include <iterator>

namespace {
constexpr qreal kHalf = 34.0;   // 바운딩 반 크기(px)
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

QPainterPath PlatformItem::airlinerPath(qreal s)
{
    // 여객기: 긴 원통형 동체 + 후퇴익 주익 + 수평/수직 꼬리날개 + 엔진 2기
    const qreal u = s / 14.0;
    QPainterPath p;
    p.setFillRule(Qt::WindingFill);
    // 동체 (둥근 기수·꼬리)
    p.addRoundedRect(QRectF(-1.8 * u, -14 * u, 3.6 * u, 27 * u), 1.8 * u, 2.4 * u);
    // 주익 (후퇴각)
    QPolygonF wing{QPointF(-1.5 * u, -3.5 * u), QPointF(-13.5 * u, 3.0 * u), QPointF(-13.5 * u, 4.8 * u),
                   QPointF(-1.5 * u, 1.8 * u),  QPointF(1.5 * u, 1.8 * u),   QPointF(13.5 * u, 4.8 * u),
                   QPointF(13.5 * u, 3.0 * u),  QPointF(1.5 * u, -3.5 * u)};
    p.addPolygon(wing);
    p.closeSubpath();
    // 엔진 (날개 아래)
    p.addRoundedRect(QRectF(-7.2 * u, -0.6 * u, 2.2 * u, 4.2 * u), 1 * u, 1 * u);
    p.addRoundedRect(QRectF(5.0 * u, -0.6 * u, 2.2 * u, 4.2 * u), 1 * u, 1 * u);
    // 수평 꼬리날개
    QPolygonF tail{QPointF(-1.2 * u, 8.5 * u), QPointF(-5.8 * u, 12.2 * u), QPointF(-5.8 * u, 13.4 * u),
                   QPointF(-1.2 * u, 11.8 * u), QPointF(1.2 * u, 11.8 * u),  QPointF(5.8 * u, 13.4 * u),
                   QPointF(5.8 * u, 12.2 * u),  QPointF(1.2 * u, 8.5 * u)};
    p.addPolygon(tail);
    p.closeSubpath();
    return p;
}

QPainterPath PlatformItem::fighterPath(qreal s)
{
    // 전투기: 뾰족한 기수 + 큰 후퇴각 주익(델타형) + 수평 꼬리 + 쌍수직꼬리 느낌의 후미
    const qreal u = s / 12.0;
    QPainterPath p;
    p.setFillRule(Qt::WindingFill);
    QPolygonF body{QPointF(0, -12 * u),        // 기수
                   QPointF(1.4 * u, -7 * u),
                   QPointF(1.8 * u, -2 * u),
                   QPointF(10.5 * u, 4.5 * u),  // 우 주익 끝
                   QPointF(10.5 * u, 6.0 * u),
                   QPointF(2.2 * u, 5.0 * u),
                   QPointF(2.4 * u, 7.5 * u),
                   QPointF(6.2 * u, 10.8 * u),  // 우 수평꼬리
                   QPointF(6.2 * u, 12.0 * u),
                   QPointF(1.2 * u, 11.0 * u),
                   QPointF(0.8 * u, 12.5 * u),  // 노즐
                   QPointF(-0.8 * u, 12.5 * u),
                   QPointF(-1.2 * u, 11.0 * u),
                   QPointF(-6.2 * u, 12.0 * u),
                   QPointF(-6.2 * u, 10.8 * u),
                   QPointF(-2.4 * u, 7.5 * u),
                   QPointF(-2.2 * u, 5.0 * u),
                   QPointF(-10.5 * u, 6.0 * u),
                   QPointF(-10.5 * u, 4.5 * u),
                   QPointF(-1.8 * u, -2 * u),
                   QPointF(-1.4 * u, -7 * u)};
    p.addPolygon(body);
    p.closeSubpath();
    return p;
}

void PlatformItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(QPen(m_color.darker(250), 1.2));
    painter->setBrush(m_color);

    switch (m_kind) {
    case Kind::Jammer:
        // 여객기 형상 (전자전기 플랫폼)
        painter->drawPath(airlinerPath(17));
        // 재밍 표시: 기체 주위 점선 원
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(m_color, 1.0, Qt::DotLine));
        painter->drawEllipse(QPointF(0, 0), 21, 21);
        break;
    case Kind::Formation: {
        // 전투기 형상, 편대규모만큼 V 대형 (선두 + 좌우 후방 + 후미)
        static const QPointF offs[] = {{0, -9}, {-15, 6}, {15, 6}, {0, 20}};
        for (int i = 0; i < std::clamp(m_size, 1, 4); ++i)
            painter->drawPath(fighterPath(11).translated(offs[i]));
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
