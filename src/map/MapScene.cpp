#include "MapScene.h"

#include "GraticuleLayer.h"
#include "MapLayerManager.h"
#include "MapProjection.h"
#include "VectorLayer.h"
#include "items/VectorLayerItem.h"

namespace {
constexpr qreal kGraticuleZ = 3.0;   // 육지 채움(0,1) 위, 선 레이어(4~) 아래
}

MapScene::MapScene(MapLayerManager& layers, const MapProjection& proj, QObject* parent)
    : QGraphicsScene(parent)
    , m_layers(layers)
    , m_proj(proj)
    , m_dataRect(proj.sceneRect(MinLat, MinLon, MaxLat, MaxLon))
{
    // 정적 배경 위주이므로 BSP 인덱스 대신 NoIndex (아이템 수가 적고 이후 동적 아이템이 많아짐)
    setItemIndexMethod(QGraphicsScene::NoIndex);
    setSceneRect(m_dataRect.adjusted(-SceneMarginKm, -SceneMarginKm, SceneMarginKm, SceneMarginKm));

    for (const QString& id : layers.layerIds()) {
        QGraphicsItem* item = nullptr;
        if (VectorLayer* vl = layers.layer(id)) {
            item = new VectorLayerItem(*vl);
        } else if (id == MapLayerManager::GraticuleId) {
            item = new GraticuleLayer(proj, MinLat, MinLon, MaxLat, MaxLon);
            item->setZValue(kGraticuleZ);
        }
        if (!item)
            continue;
        item->setVisible(layers.isVisible(id));
        addItem(item);
        m_layerItems.insert(id, item);
    }
    connect(&layers, &MapLayerManager::layerVisibilityChanged, this, &MapScene::onLayerVisibilityChanged);
}

void MapScene::onLayerVisibilityChanged(const QString& id, bool visible)
{
    if (QGraphicsItem* item = m_layerItems.value(id))
        item->setVisible(visible);
}
