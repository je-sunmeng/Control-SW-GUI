#pragma once

#include <QColor>
#include <QGraphicsItem>

// 랜덤 허위표적: 레이다 색의 얇은 점선 링이 생겨나 퍼지면서 사라지는 "블립".
//  - 탐지 결과(채운 점)와 모양이 달라 구분이 쉽고, 크기가 작아 지도를 가리지 않음
//  - 탐지 판정 대상 아님 (전시용)
class RandomGhostItem : public QGraphicsItem {
public:
    explicit RandomGhostItem(const QColor& color, QGraphicsItem* parent = nullptr);

    // progress: 수명 진행률 0~1 (링 반경), fade: 투명도 0~1
    void setState(double progress, double fade);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    static constexpr double MaxOpacity = 0.85;
    static constexpr double MinRadiusPx = 3.0;
    static constexpr double MaxRadiusPx = 11.0;

private:
    QColor m_color;
    double m_progress = 0.0;
};
