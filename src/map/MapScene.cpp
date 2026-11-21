#include "MapScene.h"

#include <QSet>

#include "GraticuleLayer.h"
#include "MapLayerManager.h"
#include "MapProjection.h"
#include "VectorLayer.h"
#include "common/ColorPalette.h"
#include "items/DetectionMarkItem.h"
#include "items/FalseTargetItem.h"
#include "items/NoiseSectorItem.h"
#include "items/PlatformItem.h"
#include "items/RadarBeamItem.h"
#include "items/RandomGhostItem.h"
#include "items/RadarItem.h"
#include "items/RouteItem.h"
#include "items/VectorLayerItem.h"
#include "items/WaypointHandleItem.h"
#include "model/ScenarioModel.h"
#include "sim/SimSnapshot.h"

namespace {
constexpr qreal kGraticuleZ = 3.0;   // 육지 채움(0,1) 위, 선 레이어(4~) 아래
// 오브젝트는 모든 지도 레이어(최대 11) 위
constexpr qreal kRadarZ = 20.0;
constexpr qreal kRouteZ = 30.0;
constexpr qreal kFalseTargetZ = 35.0;
constexpr qreal kHighlightBoost = 5.0;
// 런타임 아이템
constexpr qreal kNoiseSectorZ = 15.0;
constexpr qreal kBeamZ = 18.0;
constexpr qreal kFormationTargetZ = 38.0;
constexpr qreal kPlatformZ = 40.0;
constexpr qreal kGhostZ = 42.0;
constexpr qreal kMarkZ = 45.0;
constexpr int kMaxMarks = 400;

template <typename Item>
void destroy(QGraphicsScene* scene, Item* item)
{
    scene->removeItem(item);
    delete item;
}
} // namespace

MapScene::MapScene(MapLayerManager& layers, const MapProjection& proj, QObject* parent)
    : QGraphicsScene(parent)
    , m_layers(layers)
    , m_proj(proj)
    , m_dataRect(proj.sceneRect(MinLat, MinLon, MaxLat, MaxLon))
{
    // 아이템 수가 적고 이후 동적 아이템이 많아지므로 BSP 인덱스 대신 NoIndex
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

// ---------------------------------------------------------------------------- 오브젝트

void MapScene::setScenario(ScenarioModel* model)
{
    if (m_model)
        disconnect(m_model, nullptr, this, nullptr);
    m_model = model;
    if (m_model) {
        connect(m_model, &ScenarioModel::objectAdded, this, &MapScene::onObjectAdded);
        connect(m_model, &ScenarioModel::objectChanged, this, &MapScene::onObjectChanged);
        connect(m_model, &ScenarioModel::objectRemoved, this, &MapScene::onObjectRemoved);
        connect(m_model, &ScenarioModel::scenarioReset, this, &MapScene::rebuildObjects);
        connect(m_model, &ScenarioModel::lockedChanged, this, [this](bool locked) {
            for (MapObjectItem* item : std::as_const(m_objectItems))
                item->setEditable(!locked);
        });
    }
    rebuildObjects();
}

void MapScene::rebuildObjects()
{
    for (MapObjectItem* item : std::as_const(m_objectItems)) {
        removeItem(item);
        delete item;
    }
    m_objectItems.clear();
    m_highlighted = 0;
    if (!m_model)
        return;
    for (ObjectId id : m_model->objectIds())
        onObjectAdded(id);
}

MapObjectItem* MapScene::createItem(ObjectId id)
{
    const auto type = m_model->objectType(id);
    if (!type)
        return nullptr;
    ScenarioModel* model = m_model;
    const MapProjection& proj = m_proj;
    auto pressed = [this, id] { emit objectClicked(id); };

    switch (*type) {
    case ObjectType::Radar: {
        auto* item = new RadarItem(id, proj);
        item->setZValue(kRadarZ);
        item->handle()->setPressCallback(pressed);
        item->handle()->setMoveCallback([model, &proj, id](const QPointF& p, bool) {
            model->moveRadar(id, proj.toGeo(p));
        });
        return item;
    }
    case ObjectType::Jammer:
    case ObjectType::Formation:
    case ObjectType::StandaloneFalseTarget: {
        if (*type == ObjectType::StandaloneFalseTarget) {
            const StandaloneFalseTarget* t = model->falseTarget(id);
            if (t->ftType == FalseTargetType::Random)
                return nullptr;   // 지도 전시 대상 아님 (M4 에서 고스트로 전시)
            if (t->usesPosition()) {
                auto* item = new FalseTargetItem(id, proj, t->ftType);
                item->setZValue(kFalseTargetZ);
                item->handle()->setPressCallback(pressed);
                item->handle()->setMoveCallback([model, &proj, id](const QPointF& p, bool) {
                    model->setPointPosition(id, proj.toGeo(p));
                });
                return item;
            }
        }
        const QColor color = *type == ObjectType::Jammer      ? ColorPalette::jammer()
                             : *type == ObjectType::Formation ? ColorPalette::formation()
                                                              : ColorPalette::falseTarget();
        auto* item = new RouteItem(id, proj, color,
                                   *type == ObjectType::StandaloneFalseTarget ? Qt::DashLine : Qt::SolidLine);
        item->setZValue(kRouteZ);
        item->setCallbacks(
            [model, &proj, id](int index, const QPointF& p, bool) { model->moveWaypoint(id, index, proj.toGeo(p)); },
            pressed);
        return item;
    }
    }
    return nullptr;
}

void MapScene::onObjectAdded(ObjectId id)
{
    if (!m_model || m_objectItems.contains(id))
        return;
    MapObjectItem* item = createItem(id);
    if (!item)
        return;
    item->setEditable(!m_model->isLocked());
    item->sync(*m_model);
    addItem(item);
    m_objectItems.insert(id, item);
}

void MapScene::onObjectChanged(ObjectId id)
{
    if (MapObjectItem* item = m_objectItems.value(id))
        item->sync(*m_model);
}

void MapScene::onObjectRemoved(ObjectId id)
{
    if (m_highlighted == id)
        m_highlighted = 0;
    if (MapObjectItem* item = m_objectItems.take(id)) {
        removeItem(item);
        delete item;
    }
}

void MapScene::setHighlightedObject(ObjectId id)
{
    if (id == m_highlighted)
        return;
    if (MapObjectItem* old = m_objectItems.value(m_highlighted)) {
        old->setHighlighted(false);
        old->setZValue(old->zValue() - kHighlightBoost);
    }
    m_highlighted = id;
    if (MapObjectItem* item = m_objectItems.value(id)) {
        item->setHighlighted(true);
        item->setZValue(item->zValue() + kHighlightBoost);
    }
}

// ---------------------------------------------------------------------------- 런타임 (임무 중)

int MapScene::runtimeItemCount() const
{
    return int(m_beams.size() + m_platforms.size() + m_formationTargets.size() + m_marks.size() + m_ghosts.size()
               + m_noiseSectors.size());
}

void MapScene::resetRuntimeItems()
{
    for (auto* i : std::as_const(m_beams))
        destroy(this, i);
    for (auto* i : std::as_const(m_platforms))
        destroy(this, i);
    for (auto* i : std::as_const(m_formationTargets))
        destroy(this, i);
    for (auto* i : std::as_const(m_marks))
        destroy(this, i);
    for (auto* i : std::as_const(m_ghosts))
        destroy(this, i);
    for (auto* i : std::as_const(m_noiseSectors))
        destroy(this, i);
    m_beams.clear();
    m_platforms.clear();
    m_formationTargets.clear();
    m_marks.clear();
    m_ghosts.clear();
    m_noiseSectors.clear();
}

void MapScene::applySnapshot(const SimSnapshot& s)
{
    if (s.state == SimState::PreMission) {
        resetRuntimeItems();
        return;
    }
    if (!m_model)
        return;

    // --- 빔 ---
    for (const BeamState& b : s.beams) {
        const Radar* r = m_model->radar(b.radarId);
        if (!r)
            continue;
        RadarBeamItem*& item = m_beams[b.radarId];
        if (!item) {
            item = new RadarBeamItem(ColorPalette::radar(r->colorIndex), r->maxRangeKm, r->beamWidthDeg, r->scanDir);
            item->setZValue(kBeamZ);
            item->setPos(m_proj.toScene(r->pos));
            addItem(item);
        }
        item->setVisible(b.synced);
        item->setAzimuth(b.azimuthDeg);
    }

    // --- 이동체 ---
    QSet<ObjectId> alive;
    for (const PlatformState& p : s.platforms) {
        alive.insert(p.id);
        PlatformItem*& item = m_platforms[p.id];
        if (!item) {
            PlatformItem::Kind kind = PlatformItem::Kind::MovingTarget;
            QColor color = ColorPalette::falseTarget();
            if (p.type == ObjectType::Jammer) {
                kind = PlatformItem::Kind::Jammer;
                color = ColorPalette::jammer();
            } else if (p.type == ObjectType::Formation) {
                kind = PlatformItem::Kind::Formation;
                color = ColorPalette::formation();
            }
            const SimObject* o = m_model->find(p.id);
            item = new PlatformItem(kind, color, o ? o->name : QString());
            item->setZValue(kPlatformZ);
            addItem(item);
        }
        item->setState(m_proj.toScene(p.pos), p.headingDeg, p.formationSize);
    }
    for (auto it = m_platforms.begin(); it != m_platforms.end();) {
        if (!alive.contains(it.key())) {
            destroy(this, it.value());
            it = m_platforms.erase(it);
        } else {
            ++it;
        }
    }

    // --- 편대 허위타겟 ---
    while (m_formationTargets.size() > s.formationFalseTargets.size())
        destroy(this, m_formationTargets.takeLast());
    while (m_formationTargets.size() < s.formationFalseTargets.size()) {
        auto* item = new PlatformItem(PlatformItem::Kind::FormationFalseTarget, ColorPalette::formation().lighter(140),
                                      QStringLiteral("T%1").arg(m_formationTargets.size() + 1));
        item->setZValue(kFormationTargetZ);
        addItem(item);
        m_formationTargets.append(item);
    }
    for (int i = 0; i < s.formationFalseTargets.size(); ++i)
        m_formationTargets[i]->setState(m_proj.toScene(s.formationFalseTargets[i]), 0.0);

    // --- 잡음섹터 (정적: 최초 1회 생성) ---
    for (const NoiseSector& n : s.noiseSectors) {
        const Radar* r = m_model->radar(n.radarId);
        if (!r)
            continue;
        const quint64 key = (quint64(n.radarId) << 32) | n.noiseId;
        NoiseSectorItem*& item = m_noiseSectors[key];
        if (!item) {
            item = new NoiseSectorItem(m_proj, r->pos, r->maxRangeKm, n.centerDeg, n.widthDeg,
                                       ColorPalette::radar(r->colorIndex));
            item->setZValue(kNoiseSectorZ);
            addItem(item);
        } else {
            item->setSector(r->pos, r->maxRangeKm, n.centerDeg, n.widthDeg);
        }
    }

    // --- 탐지 마커: 레이다 색상, 스캔주기 동안 fade ---
    for (const Detection& d : s.detections) {
        const Radar* r = m_model->radar(d.radarId);
        if (!r)
            continue;
        auto* mark = new DetectionMarkItem(ColorPalette::radar(r->colorIndex), d.simTime, r->scanPeriodSec);
        mark->setZValue(kMarkZ);
        mark->setPos(m_proj.toScene(d.pos));
        addItem(mark);
        m_marks.append(mark);
    }
    while (m_marks.size() > kMaxMarks)
        destroy(this, m_marks.takeFirst());
    for (auto it = m_marks.begin(); it != m_marks.end();) {
        if (!(*it)->updateAt(s.simTime)) {
            destroy(this, *it);
            it = m_marks.erase(it);
        } else {
            ++it;
        }
    }

    // --- 랜덤 고스트 ---
    QSet<quint32> ghostKeys;
    for (const GhostState& g : s.ghosts) {
        ghostKeys.insert(g.key);
        RandomGhostItem*& item = m_ghosts[g.key];
        if (!item) {
            const Radar* r = m_model->radar(g.radarId);
            item = new RandomGhostItem(r ? ColorPalette::radar(r->colorIndex).lighter(150) : QColor(Qt::white));
            item->setZValue(kGhostZ);
            item->setPos(m_proj.toScene(g.pos));
            addItem(item);
        }
        item->setOpacity(g.opacityAt(s.simTime) * RandomGhostItem::MaxOpacity);
    }
    for (auto it = m_ghosts.begin(); it != m_ghosts.end();) {
        if (!ghostKeys.contains(it.key())) {
            destroy(this, it.value());
            it = m_ghosts.erase(it);
        } else {
            ++it;
        }
    }
}
