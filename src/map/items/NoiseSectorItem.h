#pragma once

#include "common/Types.h"

#include <QColor>
#include <QGraphicsItem>
#include <QPainterPath>

class MapProjection;

// 잡음 허위표적: 레이다별 탐지불가 섹터 (레이다→잡음표적 방위 ±탐지각/2, 최대탐지거리까지)
// 지리좌표에서 생성 후 투영. 어둡게 + 해칭
class NoiseSectorItem : public QGraphicsItem {
public:
    NoiseSectorItem(const MapProjection& proj, const GeoPoint& radarPos, double rangeKm, double centerDeg,
                    double widthDeg, const QColor& radarColor, QGraphicsItem* parent = nullptr);

    void setSector(const GeoPoint& radarPos, double rangeKm, double centerDeg, double widthDeg);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    const MapProjection& m_proj;
    QColor m_color;
    QPainterPath m_path;
    GeoPoint m_radarPos;
    double m_range = 0, m_center = 0, m_width = 0;
};
