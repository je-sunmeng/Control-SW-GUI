#pragma once

#include <QColor>
#include <QGraphicsItem>

#include <functional>

// 화면 크기 고정(ItemIgnoresTransformations) 드래그 핸들.
// 레이다 심볼, 동선 웨이포인트, 고정/잡음 표적 위치에 공용으로 사용한다.
//  - 사용자가 드래그하는 동안 moveCallback(scenePos, false), 놓을 때 (scenePos, true)
//  - 누르면 pressCallback() (오브젝트 선택용)
class WaypointHandleItem : public QGraphicsItem {
public:
    enum class Shape { Square, Circle, Diamond, Triangle, Cross };
    using MoveCallback = std::function<void(const QPointF& scenePos, bool finished)>;
    using PressCallback = std::function<void()>;

    WaypointHandleItem(Shape shape, const QColor& color, qreal sizePx, QGraphicsItem* parent = nullptr);

    void setColor(const QColor& c);
    void setLabel(const QString& label);
    void setHighlighted(bool on);
    void setEditable(bool on);
    bool isDragging() const { return m_dragging; }

    void setMoveCallback(MoveCallback cb) { m_onMove = std::move(cb); }
    void setPressCallback(PressCallback cb) { m_onPress = std::move(cb); }

    // 모델 → 아이템 위치 반영 (사용자가 드래그 중이면 무시)
    void syncPos(const QPointF& scenePos);

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    QPainterPath symbolPath() const;

    Shape m_shape;
    QColor m_color;
    qreal m_size;
    QString m_label;
    bool m_highlighted = false;
    bool m_editable = true;
    bool m_dragging = false;
    bool m_syncing = false;
    MoveCallback m_onMove;
    PressCallback m_onPress;
};
