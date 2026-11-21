#pragma once

#include <QColor>
#include <QGraphicsItem>

// 전자전기 / 아군편대 / 이동표적 / 편대 허위타겟의 현재위치 심볼 (화면 크기 고정, 진행방향 회전)
class PlatformItem : public QGraphicsItem {
public:
    enum class Kind { Jammer, Formation, MovingTarget, FormationFalseTarget };

    PlatformItem(Kind kind, const QColor& color, const QString& label, QGraphicsItem* parent = nullptr);

    void setState(const QPointF& scenePos, double headingDeg, int formationSize = 1);
    void setLabel(const QString& label);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    void drawAircraft(QPainter* p, const QPointF& c, qreal s) const;

    Kind m_kind;
    QColor m_color;
    QString m_label;
    int m_size = 1;
};
