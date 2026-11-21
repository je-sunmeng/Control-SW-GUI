#pragma once

#include "common/Types.h"

#include <QGraphicsScene>
#include <QHash>

class MapLayerManager;
class MapProjection;
class MapObjectItem;
class ScenarioModel;
class RadarBeamItem;
class PlatformItem;
class DetectionMarkItem;
class RandomGhostItem;
class NoiseSectorItem;
struct SimSnapshot;

// 지도 Scene
//  - 배경 레이어 아이템 (MapLayerManager 표시 여부 연동)
//  - ScenarioModel 시그널 구독 → 오브젝트별 아이템 생성/갱신/삭제
//  - 핸들 드래그 → ScenarioModel 편집 API 호출 (실시간 반영, 잠금 시 드래그 불가)
class MapScene : public QGraphicsScene {
    Q_OBJECT
public:
    // 지도 데이터 영역 (tools/map_prep 의 BBOX 와 동일)
    static constexpr double MinLat = 29.0, MaxLat = 46.0, MinLon = 116.0, MaxLon = 137.0;
    static constexpr double SceneMarginKm = 400.0;

    MapScene(MapLayerManager& layers, const MapProjection& proj, QObject* parent = nullptr);

    void setScenario(ScenarioModel* model);

    const MapProjection& projection() const { return m_proj; }
    MapLayerManager& layerManager() const { return m_layers; }
    QRectF dataRect() const { return m_dataRect; }

    MapObjectItem* objectItem(ObjectId id) const { return m_objectItems.value(id); }
    ObjectId highlightedObject() const { return m_highlighted; }

    int runtimeItemCount() const;   // 테스트/진단용

public slots:
    void setHighlightedObject(ObjectId id);
    // 시뮬레이션 틱 결과 전시: 이동체/빔/허위타겟/탐지마커/고스트/잡음섹터
    void applySnapshot(const SimSnapshot& snapshot);
    // 임무 후 동적 아이템 제거
    void resetRuntimeItems();

signals:
    // 지도에서 오브젝트 핸들을 눌렀을 때
    void objectClicked(ObjectId id);

private:
    void onLayerVisibilityChanged(const QString& id, bool visible);
    void onObjectAdded(ObjectId id);
    void onObjectChanged(ObjectId id);
    void onObjectRemoved(ObjectId id);
    void rebuildObjects();
    MapObjectItem* createItem(ObjectId id);

    MapLayerManager& m_layers;
    const MapProjection& m_proj;
    QRectF m_dataRect;
    QHash<QString, QGraphicsItem*> m_layerItems;

    ScenarioModel* m_model = nullptr;
    QHash<ObjectId, MapObjectItem*> m_objectItems;
    ObjectId m_highlighted = 0;

    // 런타임(임무 중) 아이템
    QHash<ObjectId, RadarBeamItem*> m_beams;
    QHash<ObjectId, PlatformItem*> m_platforms;
    QVector<PlatformItem*> m_formationTargets;
    QList<DetectionMarkItem*> m_marks;
    QHash<quint32, RandomGhostItem*> m_ghosts;
    QHash<quint64, NoiseSectorItem*> m_noiseSectors;   // (radarId << 32) | noiseId
};
