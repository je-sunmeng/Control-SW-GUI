#pragma once

#include <QGraphicsScene>
#include <QHash>

class MapLayerManager;
class MapProjection;
class GraticuleLayer;

// 지도 Scene: 배경 레이어 아이템 관리 (M3 이후 오브젝트 아이템, M4 이후 런타임 아이템 추가)
class MapScene : public QGraphicsScene {
    Q_OBJECT
public:
    // 지도 데이터 영역 (tools/map_prep 의 BBOX 와 동일)
    static constexpr double MinLat = 29.0, MaxLat = 46.0, MinLon = 116.0, MaxLon = 137.0;
    static constexpr double SceneMarginKm = 400.0;

    MapScene(MapLayerManager& layers, const MapProjection& proj, QObject* parent = nullptr);

    const MapProjection& projection() const { return m_proj; }
    MapLayerManager& layerManager() const { return m_layers; }
    QRectF dataRect() const { return m_dataRect; }

private:
    void onLayerVisibilityChanged(const QString& id, bool visible);

    MapLayerManager& m_layers;
    const MapProjection& m_proj;
    QRectF m_dataRect;
    QHash<QString, QGraphicsItem*> m_layerItems;
};
