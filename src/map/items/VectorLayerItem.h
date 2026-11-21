#pragma once

#include <QGraphicsItem>

class VectorLayer;

// VectorLayer 1개를 그리는 아이템.
//  - 선/외곽선: cosmetic pen (확대해도 픽셀 두께 유지)
//  - 축소 시 단순화 경로 사용, minLod 미만이면 생략
//  - 포인트: 화면 좌표에서 고정 크기 심볼(▲) + 라벨(labelMinLod 이상)
class VectorLayerItem : public QGraphicsItem {
public:
    explicit VectorLayerItem(const VectorLayer& layer, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    void paintPoints(QPainter* painter, double lod) const;

    const VectorLayer& m_layer;
    QRectF m_bounds;
};
