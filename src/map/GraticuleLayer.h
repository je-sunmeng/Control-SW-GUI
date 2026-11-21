#pragma once

#include <QColor>
#include <QGraphicsItem>

class MapProjection;

// 위경도선. 화면 배율에 따라 간격(10°/5°/2°/1°/0.5°/0.25°/0.1°)을 자동 선택.
// 라벨은 MapView::drawForeground에서 화면 가장자리에 그린다.
class GraticuleLayer : public QGraphicsItem {
public:
    GraticuleLayer(const MapProjection& proj, double minLat, double minLon, double maxLat, double maxLon,
                   QGraphicsItem* parent = nullptr);

    // lod(px/km)에서 격자 간격이 최소 minSpacingPx 이상이 되는 간격(deg)
    static double intervalForLod(double lod, double minSpacingPx = 90.0);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    void setColor(const QColor& c) { m_color = c; update(); }

private:
    const MapProjection& m_proj;
    double m_minLat, m_minLon, m_maxLat, m_maxLon;
    QRectF m_bounds;
    QColor m_color{0x3a, 0x48, 0x58};
};
