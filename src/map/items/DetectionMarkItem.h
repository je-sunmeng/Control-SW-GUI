#pragma once

#include <QColor>
#include <QGraphicsItem>

// 레이다 탐지 결과 마커 (레이다 색상). 생성 시각부터 lifetime(스캔주기) 동안 서서히 사라짐
class DetectionMarkItem : public QGraphicsItem {
public:
    DetectionMarkItem(const QColor& color, double bornAt, double lifetimeSec, QGraphicsItem* parent = nullptr);

    // 시각 t 기준 투명도 갱신. 수명이 끝났으면 false
    bool updateAt(double t);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    QColor m_color;
    double m_bornAt;
    double m_lifetime;
};
