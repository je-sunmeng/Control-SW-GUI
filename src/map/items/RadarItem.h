#pragma once

#include "MapObjectItem.h"

#include <QColor>
#include <QPainterPath>

class WaypointHandleItem;

// 레이다: 중심 심볼(드래그 가능) + 최대탐지거리 원(레이다 색상, 반투명 채움) + 라벨
class RadarItem : public MapObjectItem {
public:
    RadarItem(ObjectId id, const MapProjection& proj);

    WaypointHandleItem* handle() const { return m_handle; }

    void sync(const ScenarioModel& model) override;
    void setHighlighted(bool on) override;
    void setEditable(bool on) override;

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    static constexpr int CircleSegments = 180;

private:
    WaypointHandleItem* m_handle;
    QPainterPath m_rangePath;
    QColor m_color;
    bool m_highlighted = false;
};
