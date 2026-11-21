#include "map/MapEditController.h"
#include "map/MapLayerManager.h"
#include "map/MapProjection.h"
#include "map/MapScene.h"
#include "map/MapView.h"
#include "map/items/FalseTargetItem.h"
#include "map/items/RadarItem.h"
#include "map/items/RouteItem.h"
#include "map/items/WaypointHandleItem.h"
#include "model/ScenarioModel.h"
#include "sim/SimulationEngine.h"

#include <QApplication>
#include <QMouseEvent>
#include <QSignalSpy>
#include <QtTest>

#include <memory>

// 지도 편집 상태머신 + Scene/Model 동기화 + 핸들 드래그 (offscreen)
class TstMapEdit : public QObject {
    Q_OBJECT
private slots:
    void init();
    void cleanup();

    void placeRadar();
    void placeRadar_cancel();
    void drawRoute_finishAndBackspace();
    void drawRoute_needsTwoPoints();
    void drawRoute_cancelRemovesNewObject();
    void drawRoute_rightClickFinishes();
    void placePoint_and_cancel();
    void lock_cancelsAndDisablesDrag();
    void scene_syncsItems();
    void highlight();
    void dragRadarHandle_movesModel();
    void editMode_leftClickDoesNotDragHandle();
    void destroySceneBeforeController();
    void runtimeItems_followSnapshots();

private:
    void click(Qt::MouseButton b, const GeoPoint& p)
    {
        emit m_view->mapClicked(p, b, Qt::NoModifier);
    }
    void key(int k) { emit m_view->editKeyPressed(k); }
    void sendMouse(QEvent::Type type, const QPoint& viewportPos, Qt::MouseButton button, Qt::MouseButtons buttons)
    {
        QWidget* vp = m_view->viewport();
        QMouseEvent ev(type, QPointF(viewportPos), QPointF(vp->mapToGlobal(viewportPos)), button, buttons,
                       Qt::NoModifier);
        QApplication::sendEvent(vp, &ev);
    }

    MapProjection m_proj;
    std::unique_ptr<MapLayerManager> m_layers;
    std::unique_ptr<ScenarioModel> m_model;
    std::unique_ptr<MapScene> m_scene;
    std::unique_ptr<MapView> m_view;
    std::unique_ptr<MapEditController> m_edit;
};

void TstMapEdit::init()
{
    m_layers = std::make_unique<MapLayerManager>();
    m_model = std::make_unique<ScenarioModel>();
    m_scene = std::make_unique<MapScene>(*m_layers, m_proj);
    m_scene->setScenario(m_model.get());
    m_view = std::make_unique<MapView>(m_scene.get());
    m_view->resize(900, 700);
    m_view->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_view.get()));
    m_edit = std::make_unique<MapEditController>(*m_model, *m_view, *m_scene);
}

void TstMapEdit::cleanup()
{
    m_edit.reset();
    m_view.reset();
    m_scene.reset();
    m_model.reset();
    m_layers.reset();
}

void TstMapEdit::placeRadar()
{
    QSignalSpy created(m_edit.get(), &MapEditController::objectCreated);
    m_edit->startPlaceRadar();
    QCOMPARE(m_edit->mode(), MapEditController::Mode::PlaceRadar);
    QVERIFY(m_view->isEditMode());
    click(Qt::LeftButton, {38.0, 126.0});
    QCOMPARE(m_model->radars().size(), 1);
    QCOMPARE(m_model->radars().first().pos.lat, 38.0);
    QCOMPARE(m_edit->mode(), MapEditController::Mode::Idle);
    QVERIFY(!m_view->isEditMode());
    QCOMPARE(created.count(), 1);
    click(Qt::LeftButton, {38.0, 126.5});   // Idle: 무시
    QCOMPARE(m_model->radars().size(), 1);
}

void TstMapEdit::placeRadar_cancel()
{
    m_edit->startPlaceRadar();
    key(Qt::Key_Escape);
    QCOMPARE(m_edit->mode(), MapEditController::Mode::Idle);
    m_edit->startPlaceRadar();
    click(Qt::RightButton, {38, 126});
    QCOMPARE(m_edit->mode(), MapEditController::Mode::Idle);
    QVERIFY(m_model->radars().isEmpty());
}

void TstMapEdit::drawRoute_finishAndBackspace()
{
    const ObjectId j = m_model->createJammer();
    QSignalSpy finished(m_edit.get(), &MapEditController::editFinished);
    m_edit->startDrawRoute(j, true);
    click(Qt::LeftButton, {37.0, 125.0});
    click(Qt::LeftButton, {37.5, 125.5});
    click(Qt::LeftButton, {38.0, 126.0});
    QCOMPARE(m_model->route(j)->waypointCount(), 3);
    key(Qt::Key_Backspace);
    QCOMPARE(m_model->route(j)->waypointCount(), 2);
    key(Qt::Key_Return);
    QCOMPARE(m_edit->mode(), MapEditController::Mode::Idle);
    QCOMPARE(finished.count(), 1);
    QCOMPARE(finished.first().first().toUInt(), j);
    QVERIFY(m_model->route(j)->isValid());
}

void TstMapEdit::drawRoute_needsTwoPoints()
{
    const ObjectId f = m_model->createFormation();
    m_edit->startDrawRoute(f, true);
    click(Qt::LeftButton, {36.5, 126.5});
    key(Qt::Key_Enter);   // 1개뿐 → 계속 그리기
    QCOMPARE(m_edit->mode(), MapEditController::Mode::DrawRoute);
    click(Qt::LeftButton, {37.0, 127.0});
    emit m_view->mapDoubleClicked({37.0, 127.0});
    QCOMPARE(m_edit->mode(), MapEditController::Mode::Idle);
    QCOMPARE(m_model->route(f)->waypointCount(), 2);
}

void TstMapEdit::drawRoute_cancelRemovesNewObject()
{
    const ObjectId j = m_model->createJammer();
    m_edit->startDrawRoute(j, true);
    click(Qt::LeftButton, {37, 125});
    key(Qt::Key_Escape);
    QVERIFY(!m_model->jammer());   // 새로 만든 + 미완성 → 삭제

    // 기존 오브젝트 이어 그리기는 취소해도 삭제하지 않음
    const ObjectId j2 = m_model->createJammer();
    m_edit->startDrawRoute(j2, false);
    key(Qt::Key_Escape);
    QVERIFY(m_model->jammer());
    QCOMPARE(m_model->jammer()->id, j2);
}

void TstMapEdit::drawRoute_rightClickFinishes()
{
    const ObjectId t = m_model->addFalseTarget(FalseTargetType::Moving);
    m_edit->startDrawRoute(t, true);
    click(Qt::LeftButton, {35.0, 125.0});
    click(Qt::LeftButton, {35.5, 125.0});
    click(Qt::RightButton, {0, 0});
    QCOMPARE(m_edit->mode(), MapEditController::Mode::Idle);
    QCOMPARE(m_model->route(t)->waypointCount(), 2);
}

void TstMapEdit::placePoint_and_cancel()
{
    const ObjectId a = m_model->addFalseTarget(FalseTargetType::Noise);
    m_edit->startPlacePoint(a, true);
    QCOMPARE(m_edit->mode(), MapEditController::Mode::PlacePoint);
    click(Qt::LeftButton, {37.8, 128.6});
    QVERIFY(m_model->falseTarget(a)->posSet);
    QCOMPARE(m_model->falseTarget(a)->pos.lon, 128.6);

    const ObjectId b = m_model->addFalseTarget(FalseTargetType::Fixed);
    m_edit->startPlacePoint(b, true);
    key(Qt::Key_Escape);
    QVERIFY(!m_model->falseTarget(b));   // 위치 미지정 + 새로 만듦 → 삭제

    QVERIFY(m_model->falseTarget(a));
    m_edit->startPlacePoint(m_model->addFalseTarget(FalseTargetType::Random));   // 위치형 아님 → 진입 안 함
    QCOMPARE(m_edit->mode(), MapEditController::Mode::Idle);
}

void TstMapEdit::lock_cancelsAndDisablesDrag()
{
    const ObjectId r = m_model->addRadar({37.5, 127.0});
    const ObjectId j = m_model->createJammer();
    m_edit->startDrawRoute(j, false);
    m_model->setLocked(true);
    QCOMPARE(m_edit->mode(), MapEditController::Mode::Idle);
    m_edit->startPlaceRadar();
    QCOMPARE(m_edit->mode(), MapEditController::Mode::Idle);

    auto* item = static_cast<RadarItem*>(m_scene->objectItem(r));
    QVERIFY(!(item->handle()->flags() & QGraphicsItem::ItemIsMovable));
    m_model->setLocked(false);
    QVERIFY(item->handle()->flags() & QGraphicsItem::ItemIsMovable);
}

void TstMapEdit::scene_syncsItems()
{
    const ObjectId r = m_model->addRadar({37.5, 127.0});
    const ObjectId j = m_model->createJammer();
    m_model->appendWaypoint(j, {37, 125});
    m_model->appendWaypoint(j, {38, 125});
    const ObjectId fx = m_model->addFalseTarget(FalseTargetType::Fixed);
    const ObjectId rnd = m_model->addFalseTarget(FalseTargetType::Random);

    QVERIFY(dynamic_cast<RadarItem*>(m_scene->objectItem(r)));
    auto* route = dynamic_cast<RouteItem*>(m_scene->objectItem(j));
    QVERIFY(route);
    QCOMPARE(route->handleCount(), 2);
    auto* ft = dynamic_cast<FalseTargetItem*>(m_scene->objectItem(fx));
    QVERIFY(ft);
    QVERIFY(!ft->handle()->isVisible());   // 위치 미지정
    QVERIFY(!m_scene->objectItem(rnd));    // 랜덤은 지도 아이템 없음

    m_model->setPointPosition(fx, {36, 126});
    QVERIFY(ft->handle()->isVisible());

    // 모델 이동 → 핸들 위치 반영
    m_model->moveRadar(r, {36.0, 128.0});
    auto* ri = static_cast<RadarItem*>(m_scene->objectItem(r));
    QVERIFY((ri->handle()->scenePos() - m_proj.toScene({36.0, 128.0})).manhattanLength() < 1e-6);

    m_model->appendWaypoint(j, {38, 126});
    QCOMPARE(route->handleCount(), 3);

    m_model->removeObject(r);
    QVERIFY(!m_scene->objectItem(r));
    m_model->clear();
    QVERIFY(!m_scene->objectItem(j));
}

void TstMapEdit::highlight()
{
    const ObjectId a = m_model->addRadar({37, 127});
    const ObjectId b = m_model->addRadar({38, 127});
    const qreal za = m_scene->objectItem(a)->zValue();
    m_scene->setHighlightedObject(a);
    QVERIFY(m_scene->objectItem(a)->zValue() > za);
    m_scene->setHighlightedObject(b);
    QCOMPARE(m_scene->objectItem(a)->zValue(), za);
    m_model->removeObject(b);
    QCOMPARE(m_scene->highlightedObject(), ObjectId(0));
}

void TstMapEdit::dragRadarHandle_movesModel()
{
    const ObjectId r = m_model->addRadar({37.5, 127.5});
    m_view->centerOn(m_proj.toScene({37.5, 127.5}));
    QSignalSpy clicked(m_scene.get(), &MapScene::objectClicked);
    QSignalSpy changed(m_model.get(), &ScenarioModel::objectChanged);

    const QPoint start = m_view->mapFromScene(m_proj.toScene({37.5, 127.5}));
    const QPoint end = start + QPoint(60, 40);
    sendMouse(QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
    QCOMPARE(clicked.count(), 1);
    QCOMPARE(clicked.first().first().toUInt(), r);
    for (int i = 1; i <= 4; ++i)
        sendMouse(QEvent::MouseMove, start + (end - start) * i / 4, Qt::NoButton, Qt::LeftButton);
    QVERIFY(changed.count() >= 1);   // 드래그 중 실시간 반영
    sendMouse(QEvent::MouseButtonRelease, end, Qt::LeftButton, Qt::NoButton);

    const GeoPoint expected = m_proj.toGeo(m_view->mapToScene(end));
    const GeoPoint got = m_model->radar(r)->pos;
    QVERIFY2(qAbs(got.lat - expected.lat) < 0.02 && qAbs(got.lon - expected.lon) < 0.02,
             qPrintable(QStringLiteral("got %1,%2 expected %3,%4").arg(got.lat).arg(got.lon).arg(expected.lat).arg(expected.lon)));
}

void TstMapEdit::editMode_leftClickDoesNotDragHandle()
{
    const ObjectId r = m_model->addRadar({37.5, 127.5});
    const ObjectId j = m_model->createJammer();
    m_view->centerOn(m_proj.toScene({37.5, 127.5}));
    m_edit->startDrawRoute(j, false);

    const QPoint p = m_view->mapFromScene(m_proj.toScene({37.5, 127.5}));
    sendMouse(QEvent::MouseButtonPress, p, Qt::LeftButton, Qt::LeftButton);
    sendMouse(QEvent::MouseMove, p + QPoint(50, 0), Qt::NoButton, Qt::LeftButton);
    sendMouse(QEvent::MouseButtonRelease, p + QPoint(50, 0), Qt::LeftButton, Qt::NoButton);
    QCOMPARE(m_model->radar(r)->pos.lat, 37.5);           // 레이다는 그대로
    QCOMPARE(m_model->route(j)->waypointCount(), 1);      // 클릭은 웨이포인트로
}

void TstMapEdit::destroySceneBeforeController()
{
    // MainWindow 종료 시 자식 삭제 순서(Scene → Controller) 재현: 크래시 없어야 함
    m_view.reset();
    m_scene.reset();
    m_edit.reset();
}

void TstMapEdit::runtimeItems_followSnapshots()
{
    const ObjectId r = m_model->addRadar({38.0, 126.0});
    const ObjectId j = m_model->createJammer();
    m_model->appendWaypoint(j, {37.0, 125.0});
    m_model->appendWaypoint(j, {37.0, 127.0});
    const ObjectId n = m_model->addFalseTarget(FalseTargetType::Noise);
    m_model->setPointPosition(n, {38.0, 128.0});
    m_model->addFalseTarget(FalseTargetType::Random);
    Q_UNUSED(r);

    SimulationEngine engine(*m_model);
    engine.setTimerEnabled(false);
    engine.setRandomSeed(7);
    connect(&engine, &SimulationEngine::snapshotReady, m_scene.get(), &MapScene::applySnapshot);
    QVERIFY(engine.start());
    for (int i = 1; i <= 100; ++i)
        engine.advanceTo(i * 0.1);
    // 빔 1 + 전자전기 1 + 잡음섹터 1 + (고스트 0~n)
    QVERIFY2(m_scene->runtimeItemCount() >= 3, qPrintable(QString::number(m_scene->runtimeItemCount())));
    engine.stop();
    QVERIFY(m_scene->runtimeItemCount() >= 3);   // 중지 후에도 마지막 상태 유지
    engine.reset();                              // 새로고침 → 전시 초기화
    QCOMPARE(m_scene->runtimeItemCount(), 0);
}

QTEST_MAIN(TstMapEdit)
#include "tst_MapEdit.moc"
