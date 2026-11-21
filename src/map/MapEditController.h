#pragma once

#include "common/Types.h"

#include <QObject>
#include <QPointer>

class QGraphicsPathItem;
class MapScene;
class MapView;
class ScenarioModel;

// 지도 편집 상태머신
//   Idle ─(탐지기 추가)→ PlaceRadar ─click→ addRadar → Idle
//   Idle ─(편대/전자전기/이동표적 추가)→ DrawRoute ─click→ appendWaypoint (반복)
//                                     ─더블클릭/Enter/우클릭→ 완료 → Idle
//                                     ─Backspace→ 마지막 점 삭제
//                                     ─Esc→ 취소(새로 만든 오브젝트이고 웨이포인트<2 이면 삭제)
//   Idle ─(고정/잡음 표적 추가)→ PlacePoint ─click→ setPointPosition → Idle
//   Idle ─ 핸들 드래그 → moveRadar / moveWaypoint (MapScene 에서 처리, 임무 중 금지)
class MapEditController : public QObject {
    Q_OBJECT
public:
    enum class Mode { Idle, PlaceRadar, DrawRoute, PlacePoint };
    Q_ENUM(Mode)

    MapEditController(ScenarioModel& model, MapView& view, MapScene& scene, QObject* parent = nullptr);
    ~MapEditController() override;

    Mode mode() const { return m_mode; }
    ObjectId target() const { return m_target; }

public slots:
    void startPlaceRadar();
    // createdForThis: 이번 편집을 위해 새로 만든 오브젝트 (취소 시 삭제 대상)
    void startDrawRoute(ObjectId id, bool createdForThis = false);
    void startPlacePoint(ObjectId id, bool createdForThis = false);
    void finish();
    void cancel();

signals:
    void modeChanged(MapEditController::Mode mode);
    void hintChanged(const QString& hint);   // 빈 문자열 = 안내 없음
    void objectCreated(ObjectId id);         // 레이다 배치 완료 → 선택 요청
    void editFinished(ObjectId id);          // 동선/위치 편집 완료 → 선택 요청

private:
    void setMode(Mode mode, ObjectId target, bool created);
    void onMapClicked(const GeoPoint& pos, Qt::MouseButton button, Qt::KeyboardModifiers mods);
    void onDoubleClicked(const GeoPoint& pos);
    void onEditKey(int key);
    void onCursorMoved(const GeoPoint& pos);
    void updatePreview(const GeoPoint* cursor);
    void updateHint();

    ScenarioModel& m_model;
    MapView& m_view;
    MapScene& m_scene;
    Mode m_mode = Mode::Idle;
    ObjectId m_target = 0;
    bool m_created = false;
    QPointer<MapScene> m_sceneGuard;           // Scene 이 먼저 파괴되었는지 확인용
    QGraphicsPathItem* m_preview = nullptr;   // 마지막 점 → 커서 점선 (Scene 소유)
    GeoPoint m_cursor;
    bool m_hasCursor = false;
};
