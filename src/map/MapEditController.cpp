#include "MapEditController.h"

#include "MapProjection.h"
#include "MapScene.h"
#include "MapView.h"
#include "common/GeoUtils.h"
#include "model/ScenarioModel.h"

#include <QGraphicsPathItem>
#include <QPen>

#include <cmath>

MapEditController::MapEditController(ScenarioModel& model, MapView& view, MapScene& scene, QObject* parent)
    : QObject(parent)
    , m_model(model)
    , m_view(view)
    , m_scene(scene)
    , m_sceneGuard(&scene)
{
    m_preview = new QGraphicsPathItem;
    QPen pen(QColor(0xff, 0xff, 0xff, 170), 1.4, Qt::DashLine);
    pen.setCosmetic(true);
    m_preview->setPen(pen);
    m_preview->setZValue(100);
    m_preview->setVisible(false);
    m_preview->setAcceptedMouseButtons(Qt::NoButton);
    m_scene.addItem(m_preview);

    connect(&m_view, &MapView::mapClicked, this, &MapEditController::onMapClicked);
    connect(&m_view, &MapView::mapDoubleClicked, this, &MapEditController::onDoubleClicked);
    connect(&m_view, &MapView::editKeyPressed, this, &MapEditController::onEditKey);
    connect(&m_view, &MapView::cursorGeoChanged, this, &MapEditController::onCursorMoved);
    connect(&m_model, &ScenarioModel::lockedChanged, this, [this](bool locked) {
        if (locked)
            cancel();
    });
    connect(&m_model, &ScenarioModel::objectRemoved, this, [this](ObjectId id) {
        if (id == m_target) {
            m_created = false;
            setMode(Mode::Idle, 0, false);
        }
    });
    connect(&m_model, &ScenarioModel::scenarioReset, this, [this] { setMode(Mode::Idle, 0, false); });
    connect(&m_model, &ScenarioModel::objectChanged, this, [this](ObjectId id) {
        if (id == m_target && m_mode == Mode::DrawRoute) {
            updatePreview(nullptr);
            updateHint();
        }
    });
}

MapEditController::~MapEditController()
{
    // Scene 이 먼저 파괴되었으면(예: MainWindow 자식 삭제 순서) 아이템도 이미 삭제됨 → 접근 금지
    if (m_sceneGuard && m_preview) {
        m_sceneGuard->removeItem(m_preview);
        delete m_preview;
    }
}

void MapEditController::setMode(Mode mode, ObjectId target, bool created)
{
    m_mode = mode;
    m_target = target;
    m_created = created;
    m_view.setEditMode(mode != Mode::Idle);
    if (mode != Mode::Idle)
        m_view.setFocus();
    updatePreview(nullptr);
    updateHint();
    emit modeChanged(mode);
}

void MapEditController::startPlaceRadar()
{
    if (m_model.isLocked())
        return;
    cancel();
    setMode(Mode::PlaceRadar, 0, false);
}

void MapEditController::startDrawRoute(ObjectId id, bool createdForThis)
{
    if (m_model.isLocked() || !m_model.route(id))
        return;
    if (m_target != id)
        cancel();
    setMode(Mode::DrawRoute, id, createdForThis);
    m_scene.setHighlightedObject(id);
}

void MapEditController::startPlacePoint(ObjectId id, bool createdForThis)
{
    const StandaloneFalseTarget* t = m_model.falseTarget(id);
    if (m_model.isLocked() || !t || !t->usesPosition())
        return;
    if (m_target != id)
        cancel();
    setMode(Mode::PlacePoint, id, createdForThis);
    m_scene.setHighlightedObject(id);
}

void MapEditController::finish()
{
    const ObjectId id = m_target;
    const Mode mode = m_mode;
    if (mode == Mode::Idle)
        return;
    if (mode == Mode::DrawRoute) {
        const Route* r = m_model.route(id);
        if (r && r->waypointCount() < 2) {
            // 완료 조건 미충족: 계속 그리기
            emit hintChanged(QStringLiteral("동선은 웨이포인트가 2개 이상 필요합니다. 계속 클릭하세요 (Esc: 취소)"));
            return;
        }
    }
    setMode(Mode::Idle, 0, false);
    if (id)
        emit editFinished(id);
}

void MapEditController::cancel()
{
    if (m_mode == Mode::Idle)
        return;
    const ObjectId id = m_target;
    const bool created = m_created;
    const Mode mode = m_mode;
    setMode(Mode::Idle, 0, false);
    if (!created || !id)
        return;
    // 새로 만든 오브젝트인데 미완성이면 삭제
    if (mode == Mode::DrawRoute) {
        const Route* r = m_model.route(id);
        if (r && r->waypointCount() < 2)
            m_model.removeObject(id);
        else
            emit editFinished(id);
    } else if (mode == Mode::PlacePoint) {
        const StandaloneFalseTarget* t = m_model.falseTarget(id);
        if (t && !t->posSet)
            m_model.removeObject(id);
    }
}

void MapEditController::onMapClicked(const GeoPoint& pos, Qt::MouseButton button, Qt::KeyboardModifiers)
{
    switch (m_mode) {
    case Mode::Idle:
        break;
    case Mode::PlaceRadar:
        if (button == Qt::LeftButton) {
            const ObjectId id = m_model.addRadar(pos);
            setMode(Mode::Idle, 0, false);
            if (id)
                emit objectCreated(id);
        } else if (button == Qt::RightButton) {
            cancel();
        }
        break;
    case Mode::DrawRoute:
        if (button == Qt::LeftButton)
            m_model.appendWaypoint(m_target, pos);
        else if (button == Qt::RightButton)
            finish();
        break;
    case Mode::PlacePoint:
        if (button == Qt::LeftButton) {
            const ObjectId id = m_target;
            m_model.setPointPosition(id, pos);
            setMode(Mode::Idle, 0, false);
            emit editFinished(id);
        } else if (button == Qt::RightButton) {
            cancel();
        }
        break;
    }
}

void MapEditController::onDoubleClicked(const GeoPoint&)
{
    // 더블클릭의 첫 클릭으로 점이 이미 추가되었으므로 완료만 처리
    if (m_mode == Mode::DrawRoute)
        finish();
}

void MapEditController::onEditKey(int key)
{
    switch (key) {
    case Qt::Key_Escape:
        cancel();
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        finish();
        break;
    case Qt::Key_Backspace:
        if (m_mode == Mode::DrawRoute) {
            if (const Route* r = m_model.route(m_target); r && r->waypointCount() > 0)
                m_model.removeWaypoint(m_target, r->waypointCount() - 1);
        }
        break;
    default:
        break;
    }
}

void MapEditController::onCursorMoved(const GeoPoint& pos)
{
    if (m_mode == Mode::DrawRoute)
        updatePreview(&pos);
}

void MapEditController::updatePreview(const GeoPoint* cursor)
{
    if (cursor) {
        m_cursor = *cursor;
        m_hasCursor = true;
    }
    const Route* r = m_mode == Mode::DrawRoute ? m_model.route(m_target) : nullptr;
    if (!r || r->isEmpty() || !m_hasCursor) {
        m_preview->setVisible(false);
        return;
    }
    const GeoPoint a = r->waypoints.last();
    const MapProjection& proj = m_scene.projection();
    QPainterPath p(proj.toScene(a));
    const double len = GeoUtils::distanceKm(a, m_cursor);
    const int n = std::max(1, int(std::ceil(len / 20.0)));
    for (int k = 1; k <= n; ++k)
        p.lineTo(proj.toScene(GeoUtils::interpolate(a, m_cursor, double(k) / n)));
    m_preview->setPath(p);
    m_preview->setVisible(true);
}

void MapEditController::updateHint()
{
    QString hint;
    switch (m_mode) {
    case Mode::Idle:
        break;
    case Mode::PlaceRadar:
        hint = QStringLiteral("지도를 클릭해 레이다 위치를 지정하세요 (Esc/우클릭: 취소)");
        break;
    case Mode::DrawRoute: {
        const SimObject* o = m_model.find(m_target);
        const Route* r = m_model.route(m_target);
        hint = QStringLiteral("[%1] 클릭으로 웨이포인트 추가 (%2개) — 더블클릭/Enter/우클릭: 완료, Backspace: 마지막 점 삭제, Esc: 취소")
                   .arg(o ? o->name : QString())
                   .arg(r ? r->waypointCount() : 0);
        break;
    }
    case Mode::PlacePoint: {
        const SimObject* o = m_model.find(m_target);
        hint = QStringLiteral("[%1] 지도를 클릭해 위치를 지정하세요 (Esc/우클릭: 취소)").arg(o ? o->name : QString());
        break;
    }
    }
    emit hintChanged(hint);
}
