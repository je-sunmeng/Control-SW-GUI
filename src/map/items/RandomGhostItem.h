#pragma once

#include <QColor>
#include <QGraphicsItem>

// 랜덤 허위표적: 옅은 반원. 투명도는 GhostState::opacityAt 로 fade in/out (탐지 판정 대상 아님)
class RandomGhostItem : public QGraphicsItem {
public:
    explicit RandomGhostItem(const QColor& color, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    static constexpr double MaxOpacity = 0.55;

private:
    QColor m_color;
};
