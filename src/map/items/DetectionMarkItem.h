#pragma once

#include <QColor>
#include <QGraphicsItem>

// 레이다 탐지 결과 마커 (레이다 색상). 페이딩 없이 임무가 끝날 때(새로고침)까지 계속 표시
class DetectionMarkItem : public QGraphicsItem {
public:
    explicit DetectionMarkItem(const QColor& color, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    QColor m_color;
};
