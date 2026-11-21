#pragma once

#include "common/Types.h"

#include <QGraphicsItem>

class ScenarioModel;
class MapProjection;

// 시나리오 오브젝트 1개를 지도에 그리는 아이템의 공통 베이스.
// 아이템은 Scene 원점에 두고, 형상은 지리좌표에서 생성 후 투영한 Scene 좌표로 그린다.
class MapObjectItem : public QGraphicsItem {
public:
    MapObjectItem(ObjectId id, const MapProjection& proj)
        : m_id(id)
        , m_proj(proj)
    {
        setAcceptedMouseButtons(Qt::NoButton);
    }

    ObjectId objectId() const { return m_id; }

    // 모델 값으로 형상/핸들 갱신
    virtual void sync(const ScenarioModel& model) = 0;
    virtual void setHighlighted(bool on) = 0;
    virtual void setEditable(bool on) = 0;

protected:
    ObjectId m_id;
    const MapProjection& m_proj;
};
