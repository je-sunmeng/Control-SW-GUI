#include "common/GeoUtils.h"
#include "log/Logger.h"
#include "mission/MissionController.h"
#include "sim/SimulationEngine.h"

#include <QSignalSpy>
#include <QtTest>

#include <memory>

class TstSimulationEngine : public QObject {
    Q_OBJECT
private slots:
    void init();
    void cleanup();

    void start_requiresValidScenario();
    void standalone_mainLobePointsAtJammer();
    void snapshot_contents();
    void detections_overTime();
    void periodicEvents();
    void pause_resume_stop_reset();
    void linked_restrictions();
    void mission_actionsTable();
    void mission_startLocksModel_refreshUnlocks();
    void realTimer_runs();
    void formationArrival_stopsMission();

private:
    void buildScenario();
    std::unique_ptr<ScenarioModel> m_model;
    std::unique_ptr<SimulationEngine> m_engine;
    ObjectId m_radar = 0, m_jammer = 0, m_formation = 0, m_fixed = 0, m_noise = 0;
};

void TstSimulationEngine::init()
{
    m_model = std::make_unique<ScenarioModel>();
    m_engine = std::make_unique<SimulationEngine>(*m_model);
    m_engine->setTimerEnabled(false);
    m_engine->setRandomSeed(42);
}

void TstSimulationEngine::cleanup()
{
    m_engine.reset();
    m_model.reset();
}

void TstSimulationEngine::buildScenario()
{
    m_radar = m_model->addRadar({38.0, 126.0});
    Radar r = *m_model->radar(m_radar);
    r.maxRangeKm = 300;
    r.scanPeriodSec = 10;
    m_model->updateRadar(r);
    m_jammer = m_model->createJammer();
    m_model->appendWaypoint(m_jammer, {37.0, 125.0});
    m_model->appendWaypoint(m_jammer, {37.0, 127.0});
    m_formation = m_model->createFormation();
    m_model->appendWaypoint(m_formation, {36.8, 126.5});
    m_model->appendWaypoint(m_formation, {37.8, 126.5});
    FriendlyFormation f = *m_model->formation();
    f.jamming.resize(2);
    m_model->updateFormation(f);
    m_fixed = m_model->addFalseTarget(FalseTargetType::Fixed);
    m_model->setPointPosition(m_fixed, {38.5, 126.5});
    m_noise = m_model->addFalseTarget(FalseTargetType::Noise);
    m_model->setPointPosition(m_noise, {38.0, 128.0});
    m_model->addFalseTarget(FalseTargetType::Random);
}

void TstSimulationEngine::start_requiresValidScenario()
{
    QString err;
    QVERIFY(!m_engine->start(&err));
    QVERIFY(!err.isEmpty());
    QCOMPARE(m_engine->state(), SimState::PreMission);
    buildScenario();
    QVERIFY2(m_engine->start(&err), qPrintable(err));
    QCOMPARE(m_engine->state(), SimState::Running);
    QVERIFY(!m_engine->start(&err));   // 이미 실행 중
}

void TstSimulationEngine::standalone_mainLobePointsAtJammer()
{
    buildScenario();
    QSignalSpy lobes(m_engine.get(), &SimulationEngine::mainLobeSynced);
    QVERIFY(m_engine->start());
    QCOMPARE(lobes.count(), 1);   // 시작 시점
    const SimSnapshot s0 = m_engine->lastSnapshot();
    QCOMPARE(s0.beams.size(), 1);
    QVERIFY(s0.beams[0].synced);
    const double expect0 = GeoUtils::bearingDeg({38.0, 126.0}, {37.0, 125.0});
    QVERIFY(qAbs(GeoUtils::angleDiffDeg(s0.beams[0].azimuthDeg, expect0)) < 1e-6);

    // 스캔주기마다 재동기화 → 그 시점 전자전기 방향
    const SimSnapshot s1 = m_engine->advanceTo(10.0);
    QCOMPARE(lobes.count(), 2);
    const PlatformState j = s1.platforms.first();
    QCOMPARE(j.type, ObjectType::Jammer);
    QVERIFY(qAbs(GeoUtils::angleDiffDeg(s1.beams[0].azimuthDeg, GeoUtils::bearingDeg({38.0, 126.0}, j.pos))) < 1e-6);

    const SimSnapshot s2 = m_engine->advanceTo(12.5);   // 2.5 s → +90°
    QVERIFY(qAbs(GeoUtils::angleDiffDeg(s2.beams[0].azimuthDeg, s1.beams[0].azimuthDeg + 90.0)) < 1e-6);
}

void TstSimulationEngine::snapshot_contents()
{
    buildScenario();
    QVERIFY(m_engine->start());
    const SimSnapshot s = m_engine->advanceTo(30.0);
    QCOMPARE(s.simTime, 30.0);
    QCOMPARE(s.platforms.size(), 2);   // 전자전기, 편대 (이동표적 없음)
    QCOMPARE(s.formationFalseTargets.size(), 2);
    QCOMPARE(s.noiseSectors.size(), 1);
    QCOMPARE(s.noiseSectors[0].radarId, m_radar);
    QVERIFY(qAbs(GeoUtils::angleDiffDeg(s.noiseSectors[0].centerDeg, GeoUtils::bearingDeg({38, 126}, {38, 128}))) < 1e-9);
    // 편대: 30 s × 250 m/s = 7.5 km 북상
    const PlatformState f = s.platforms[1];
    QVERIFY(qAbs(GeoUtils::distanceKm(f.pos, {36.8, 126.5}) - 7.5) < 0.01);
    QCOMPARE(f.formationSize, 4);
}

void TstSimulationEngine::detections_overTime()
{
    buildScenario();
    QVERIFY(m_engine->start());
    QSet<QString> keys;
    int total = 0;
    for (int i = 1; i <= 1212; ++i) {   // 33 ms 틱 × 40 s
        const SimSnapshot s = m_engine->advanceTo(i * 0.033);
        for (const Detection& d : s.detections) {
            keys.insert(d.targetKey);
            QCOMPARE(d.origin, DetectionOrigin::Local);
            QCOMPARE(d.radarId, m_radar);
            ++total;
        }
        QVERIFY(s.ghosts.size() <= 8);
    }
    QVERIFY(keys.contains(QStringLiteral("FT%1").arg(m_fixed)));
    QVERIFY(keys.contains(QStringLiteral("FFT1")));
    QVERIFY(keys.contains(QStringLiteral("FFT2")));
    QVERIFY2(total >= 3 * 3 && total <= 3 * 5, qPrintable(QString::number(total)));   // 3표적 × ~4바퀴
}

void TstSimulationEngine::periodicEvents()
{
    buildScenario();
    m_model->setDatalinkIntervalSec(0.5);
    m_model->setJammerIntervalSec(2.0);
    QSignalSpy jam(m_engine.get(), &SimulationEngine::jammerPositionDue);
    QSignalSpy form(m_engine.get(), &SimulationEngine::formationPositionDue);
    QVERIFY(m_engine->start());   // t=0 에 1회씩
    for (int i = 1; i <= 50; ++i)   // 0.1 s 간격 (누적 오차 없이)
        m_engine->advanceTo(i / 10.0);
    QCOMPARE(jam.count(), 3);    // 0,2,4 (전자전기 2 s 간격)
    QCOMPARE(form.count(), 11);  // 0,0.5,...,5
}

void TstSimulationEngine::pause_resume_stop_reset()
{
    buildScenario();
    QSignalSpy states(m_engine.get(), &SimulationEngine::stateChanged);
    QSignalSpy snaps(m_engine.get(), &SimulationEngine::snapshotReady);
    QVERIFY(m_engine->start());
    QVERIFY(m_engine->pause());
    QCOMPARE(m_engine->state(), SimState::Paused);
    QVERIFY(!m_engine->pause());
    QVERIFY(m_engine->resume());
    m_engine->stop();
    QCOMPARE(m_engine->state(), SimState::Stopped);
    QVERIFY(!m_engine->resume());
    m_engine->reset();
    QCOMPARE(m_engine->state(), SimState::PreMission);
    QCOMPARE(states.count(), 5);
    QVERIFY(snaps.last().first().value<SimSnapshot>().platforms.isEmpty());   // 전시 초기화 신호
}

void TstSimulationEngine::linked_restrictions()
{
    buildScenario();
    QVERIFY(m_engine->setSpeedFactor(8));
    QVERIFY(m_engine->setMode(RunMode::Linked));
    QCOMPARE(m_engine->speedFactor(), 1);
    QVERIFY(!m_engine->setSpeedFactor(2));
    QSignalSpy lobes(m_engine.get(), &SimulationEngine::mainLobeSynced);
    QVERIFY(m_engine->start());
    QVERIFY(!m_engine->setMode(RunMode::Standalone));   // 임무 중 변경 불가
    QCOMPARE(lobes.count(), 0);                          // 외부 MainLobe 전에는 미동기화
    QVERIFY(!m_engine->lastSnapshot().beams[0].synced);
    QVERIFY(!m_engine->pause());                         // 연동모드 일시정지 불가

    m_engine->onExternalMainLobe(m_radar);
    QCOMPARE(lobes.count(), 1);
    Detection d;
    d.radarId = m_radar;
    d.targetKey = QStringLiteral("EXT");
    d.pos = {38.2, 126.2};
    m_engine->externalSource().push(d);
    const SimSnapshot s = m_engine->advanceTo(1.0);
    QVERIFY(s.beams[0].synced);
    QCOMPARE(s.detections.size(), 1);
    QCOMPARE(s.detections[0].origin, DetectionOrigin::External);
    QVERIFY(m_engine->advanceTo(2.0).detections.isEmpty());   // 자체 판정 없음

    // 연동모드: 최초 MainLobe 이후 수신은 빔을 다시 맞추지 않음 (계속 스캔)
    const double azBefore = m_engine->advanceTo(3.0).beams[0].azimuthDeg;
    m_engine->onExternalMainLobe(m_radar);
    QCOMPARE(lobes.count(), 1);
    QCOMPARE(m_engine->advanceTo(3.0).beams[0].azimuthDeg, azBefore);
}

void TstSimulationEngine::mission_actionsTable()
{
    Logger log;
    MissionController mc(*m_model, *m_engine, log);
    // 임무 전(단독), 시나리오 미완성 → 시작 불가
    MissionActions a = mc.actions();
    QVERIFY(!a.sendMission && !a.start && !a.pause && !a.stop && a.refresh && a.speed && a.modeChange);
    QVERIFY(!a.sendMissionVisible);   // 단독모드: 임무전송 버튼 숨김
    buildScenario();
    QVERIFY(mc.actions().start);

    QVERIFY(mc.startOrResume());
    a = mc.actions();   // 임무 중(단독)
    QVERIFY(!a.sendMission && !a.start && a.pause && a.stop && a.refresh && a.refreshNeedsConfirm && a.speed && !a.modeChange);
    QVERIFY(mc.pause());
    a = mc.actions();   // 일시정지
    QVERIFY(a.start && a.startIsResume && !a.pause && a.stop && a.speed && !a.modeChange);
    QVERIFY(mc.startOrResume());
    QCOMPARE(mc.state(), SimState::Running);
    mc.stop();
    a = mc.actions();   // 중지
    QVERIFY(!a.start && !a.pause && !a.stop && a.refresh && !a.refreshNeedsConfirm && !a.speed && !a.modeChange);
    mc.refresh();
    QCOMPARE(mc.state(), SimState::PreMission);

    // 연동: 두 장치 연결 시 임무전송 → 시작
    QVERIFY(mc.setMode(RunMode::Linked));
    a = mc.actions();
    QVERIFY(!a.sendMission && !a.start && !a.speed && !a.pauseVisible);
    QVERIFY(a.sendMissionVisible);
    mc.setDeviceConnected(DeviceCode::AnalysisSW, true);
    QVERIFY(!mc.actions().sendMission);
    mc.setDeviceConnected(DeviceCode::SignalGen, true);
    QVERIFY(mc.actions().sendMission);
    QVERIFY(!mc.actions().start);
    QVERIFY(mc.sendMission());
    QVERIFY(mc.actions().start);
    QVERIFY(mc.startOrResume());
    a = mc.actions();   // 임무 중(연동)
    QVERIFY(!a.pause && a.stop && !a.speed && !a.modeChange);
    QVERIFY(!mc.pause());
}

void TstSimulationEngine::mission_startLocksModel_refreshUnlocks()
{
    Logger log;
    MissionController mc(*m_model, *m_engine, log);
    QStringList errors;
    QVERIFY(!mc.startOrResume(&errors));
    QVERIFY(!errors.isEmpty());
    buildScenario();
    QVERIFY(mc.startOrResume(&errors));
    QVERIFY(m_model->isLocked());
    QVERIFY(mc.setSpeed(4));
    QCOMPARE(m_engine->speedFactor(), 4);
    QVERIFY(!mc.setSpeed(3));
    QSignalSpy actions(&mc, &MissionController::actionsChanged);
    mc.refresh();   // 임무 중 → 중지 후 초기화
    QCOMPARE(mc.state(), SimState::PreMission);
    QVERIFY(!m_model->isLocked());
    QVERIFY(actions.count() >= 1);
    QVERIFY(m_model->radar(m_radar));   // 오브젝트 설정 유지
}

void TstSimulationEngine::realTimer_runs()
{
    buildScenario();
    m_engine->setTimerEnabled(true);
    m_engine->setSpeedFactor(8);
    QSignalSpy snaps(m_engine.get(), &SimulationEngine::snapshotReady);
    QVERIFY(m_engine->start());
    QTRY_VERIFY_WITH_TIMEOUT(snaps.count() >= 5, 2000);
    const double t = m_engine->lastSnapshot().simTime;
    QVERIFY2(t > 0.5, qPrintable(QString::number(t)));   // 8배속 → 실시간보다 빠르게 진행
    m_engine->stop();
}

void TstSimulationEngine::formationArrival_stopsMission()
{
    buildScenario();
    Logger log;
    MissionController mc(*m_model, *m_engine, log);
    QSignalSpy done(m_engine.get(), &SimulationEngine::formationRouteCompleted);
    QVERIFY(mc.startOrResume());
    const double arrive = m_model->formation()->route.totalDurationSec();   // 편대 동선 소요시간
    m_engine->advanceTo(arrive - 1.0);
    QCOMPARE(done.count(), 0);
    QCOMPARE(mc.state(), SimState::Running);
    m_engine->advanceTo(arrive + 0.1);
    QCOMPARE(done.count(), 1);
    QCOMPARE(mc.state(), SimState::Stopped);   // 임무 자동 종료
    QCOMPARE(m_engine->lastSnapshot().state, SimState::Stopped);
    m_engine->advanceTo(arrive + 5.0);
    QCOMPARE(done.count(), 1);                 // 1회만
}

QTEST_GUILESS_MAIN(TstSimulationEngine)
#include "tst_SimulationEngine.moc"
