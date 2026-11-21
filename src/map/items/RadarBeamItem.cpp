#include "RadarBeamItem.h"

#include <QPainter>
#include <QtMath>

#include <cmath>
#include <algorithm>
#include <iterator>

RadarBeamItem::RadarBeamItem(const QColor& color, double maxRangeKm, double beamWidthDeg, ScanDirection dir,
                             QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_color(color)
{
    setAcceptedMouseButtons(Qt::NoButton);
    setBeam(maxRangeKm, beamWidthDeg, dir);
}

double RadarBeamItem::gain(double offsetDeg, double beamWidthDeg)
{
    // sinc²(x), 반전력(−3 dB) 점이 ±빔폭/2 가 되도록 x = 0.886·φ/θ
    const double x = 0.886 * offsetDeg / std::max(1e-6, beamWidthDeg);
    if (std::abs(x) < 1e-9)
        return 1.0;
    const double s = std::sin(M_PI * x) / (M_PI * x);
    return s * s;
}

QPainterPath RadarBeamItem::buildLobePath(double range, double bw)
{
    // 메인로브 + 첫 두 사이드로브 범위 (±3.5θ, 최대 ±180°)
    const double span = std::min(180.0, 3.5 * bw);
    const int samples = 241;
    QPainterPath p(QPointF(0, 0));
    for (int i = 0; i < samples; ++i) {
        const double phi = -span + 2.0 * span * i / (samples - 1);
        const double r = range * gain(phi, bw);
        const double a = qDegreesToRadians(phi);
        p.lineTo(r * std::sin(a), -r * std::cos(a));   // 북쪽(−y) 기준 시계방향
    }
    p.closeSubpath();
    return p;
}

void RadarBeamItem::setBeam(double maxRangeKm, double beamWidthDeg, ScanDirection dir)
{
    prepareGeometryChange();
    m_range = maxRangeKm;
    m_beamWidth = beamWidthDeg;
    m_dir = dir;
    m_lobe = buildLobePath(maxRangeKm, beamWidthDeg);
}

QRectF RadarBeamItem::boundingRect() const
{
    return QRectF(-m_range, -m_range, 2 * m_range, 2 * m_range);
}

void RadarBeamItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);

    // --- 잔상: 스캔 반대쪽으로 AfterglowDeg 만큼 감쇠 부채꼴 ---
    // QConicalGradient 각도: 0 = 3시 방향, 반시계 증가. 로컬 북쪽(−y) = 90°
    {
        QConicalGradient g(QPointF(0, 0), 90.0);
        QColor c0 = m_color, c1 = m_color;
        c0.setAlpha(70);
        c1.setAlpha(0);
        const double frac = AfterglowDeg / 360.0;
        if (m_dir == ScanDirection::Clockwise) {   // 빔 뒤쪽 = 반시계(각도 +) 쪽
            g.setColorAt(0.0, c0);
            g.setColorAt(frac, c1);
            g.setColorAt(1.0, c0);
        } else {                                   // 빔 뒤쪽 = 시계(각도 −, 즉 1.0 직전) 쪽
            g.setColorAt(0.0, c1);
            g.setColorAt(1.0 - frac, c1);
            g.setColorAt(1.0, c0);
        }
        QPainterPath fan(QPointF(0, 0));
        const QRectF circle(-m_range, -m_range, 2 * m_range, 2 * m_range);
        // arcTo: 시작각 90°(북), 스윕 +는 반시계
        fan.arcTo(circle, 90.0, m_dir == ScanDirection::Clockwise ? AfterglowDeg : -AfterglowDeg);
        fan.closeSubpath();
        painter->setBrush(g);
        painter->drawPath(fan);
    }

    // --- 로브: 중심에서 바깥으로 감쇠하는 방사형 그라데이션 ---
    QRadialGradient rg(QPointF(0, 0), m_range);
    QColor inner = m_color.lighter(130), outer = m_color;
    inner.setAlpha(230);
    outer.setAlpha(70);
    rg.setColorAt(0.0, inner);
    rg.setColorAt(1.0, outer);
    painter->setBrush(rg);
    QPen edge(m_color.lighter(150), 1.0);
    edge.setCosmetic(true);
    painter->setPen(edge);
    painter->drawPath(m_lobe);
}
