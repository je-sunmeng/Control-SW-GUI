#pragma once

#include "MapObjectItem.h"

class WaypointHandleItem;

// 단독 허위표적 중 위치형(고정/잡음) 심볼. 이동표적은 RouteItem, 랜덤은 지도 아이템 없음.
// M4에서 편대 허위타겟·이동표적 현재위치 전시에도 재사용 예정
class FalseTargetItem : public MapObjectItem {
public:
    // 표적 타입은 생성 후 바뀌지 않으므로 심볼 모양(고정 ▲ / 잡음 ⊗)을 생성 시 결정
    FalseTargetItem(ObjectId id, const MapProjection& proj, FalseTargetType type);

    WaypointHandleItem* handle() const { return m_handle; }

    void sync(const ScenarioModel& model) override;
    void setHighlighted(bool on) override;
    void setEditable(bool on) override;

    QRectF boundingRect() const override { return {}; }
    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override {}

private:
    WaypointHandleItem* m_handle = nullptr;
};
