#include "MockDevice.h"
#include "common/GeoUtils.h"
#include "log/Logger.h"
#include "mission/MissionController.h"
#include "net/NetworkManager.h"
#include "sim/SimulationEngine.h"

#include <QSignalSpy>
#include <QtTest>

#include <memory>

// 모의 클라이언트 ↔ 제어SW 종단 간: 임무전송 내용 수신, MainLobe 주기 송신, 탐지결과 전시
class TstMockDevice : public QObject {
    Q_OBJECT
private slots:
    void init();
    void cleanup();
    void endToEnd_linkedMission();
    void detections_respectRangeAndNoise();

private:
    std::unique_ptr<Logger> m_log;
    std::unique_ptr<ProtocolTable> m_table;
    std::unique_ptr<ScenarioModel> m_model;
    std::unique_ptr<SimulationEngine> m_engine;
    std::unique_ptr<NetworkManager> m_net;
    std::unique_ptr<MissionController> m_mission;
    std::unique_ptr<MockDevice> m_analysis;
    std::unique_ptr<MockDevice> m_sig;
};

void TstMockDevice::init()
{
    m_log = std::make_unique<Logger>();
    m_table = std::make_unique<ProtocolTable>();
    QVERIFY(m_table->loadFile(ProtocolTable::defaultResourcePath()));
    m_model = std::make_unique<ScenarioModel>();
    m_engine = std::make_unique<SimulationEngine>(*m_model);
    m_engine->setTimerEnabled(false);
    m_net = std::make_unique<NetworkManager>(*m_table, *m_log);
    m_mission = std::make_unique<MissionController>(*m_model, *m_engine, *m_log, m_net.get());
    m_mission->setServerPort(0);

    // 시나리오: 레이다 2, 전자전기, 편대(허위타겟 2), 고정 표적 1(R1 범위 내), 잡음 표적
    const ObjectId r1 = m_model->addRadar({38.0, 126.0});
    const ObjectId r2 = m_model->addRadar({39.0, 127.5});
    Q_UNUSED(r1);
    Q_UNUSED(r2);
    const ObjectId j = m_model->createJammer();
    m_model->appendWaypoint(j, {37.0, 125.0});
    m_model->appendWaypoint(j, {37.0, 127.0});
    const ObjectId f = m_model->createFormation();
    m_model->appendWaypoint(f, {37.2, 126.5});
    m_model->appendWaypoint(f, {38.0, 126.5});
    FriendlyFormation ff = *m_model->formation();
    ff.jamming.resize(2);
    m_model->updateFormation(ff);
    const ObjectId fx = m_model->addFalseTarget(FalseTargetType::Fixed);
    m_model->setPointPosition(fx, {38.3, 126.3});

    QVERIFY(m_mission->setMode(RunMode::Linked));
    m_analysis = std::make_unique<MockDevice>(DeviceCode::AnalysisSW, *m_table);
    m_sig = std::make_unique<MockDevice>(DeviceCode::SignalGen, *m_table);
    m_sig->setManualTime(true);
    m_analysis->connectTo(QStringLiteral("127.0.0.1"), m_net->port());
    m_sig->connectTo(QStringLiteral("127.0.0.1"), m_net->port());
    QTRY_VERIFY(m_net->isConnected(DeviceCode::AnalysisSW) && m_net->isConnected(DeviceCode::SignalGen));
}

void TstMockDevice::cleanup()
{
    m_analysis.reset();
    m_sig.reset();
    m_mission.reset();
    m_net.reset();
    m_engine.reset();
    m_model.reset();
    m_table.reset();
    m_log.reset();
}

void TstMockDevice::endToEnd_linkedMission()
{
    QSignalSpy sigMsgs(m_sig.get(), &MockDevice::messageReceived);
    QSignalSpy anaMsgs(m_analysis.get(), &MockDevice::messageReceived);
    QVERIFY(m_mission->sendMission());
    QTRY_COMPARE(m_sig->radars().size(), 2);
    QTRY_COMPARE(anaMsgs.count(), 3);   // RadarInfo, JammerRoute, FormationInfo
    QTRY_COMPARE(sigMsgs.count(), 3);   // RadarInfo, FormationInfo(+교란), FalseTargetInfo
    QCOMPARE(anaMsgs.at(1).first().value<MessageId>(), MessageId::JammerRoute);
    QCOMPARE(sigMsgs.at(2).first().value<MessageId>(), MessageId::FalseTargetInfo);

    QVERIFY(m_mission->startOrResume());
    QTRY_VERIFY(m_sig->missionRunning());
    QTRY_COMPARE(anaMsgs.count(), 4);   // + MissionStart
    m_mission->stop();
    QTRY_VERIFY(!m_sig->missionRunning());
}

void TstMockDevice::detections_respectRangeAndNoise()
{
    // R1 → 고정표적 방향만 가리는 잡음표적 (R1 기준 ±2°)
    const GeoPoint r1 = m_model->radars()[0].pos;
    const GeoPoint fixedPos{38.3, 126.3};
    const ObjectId noise = m_model->addFalseTarget(FalseTargetType::Noise);
    m_model->setPointPosition(noise, GeoUtils::destination(r1, GeoUtils::bearingDeg(r1, fixedPos), 20.0));
    StandaloneFalseTarget nt = *m_model->falseTarget(noise);
    nt.detectAngleDeg = 4.0;
    m_model->updateFalseTarget(nt);

    QVERIFY(m_mission->sendMission());
    QTRY_COMPARE(m_sig->radars().size(), 2);
    QVERIFY(m_mission->startOrResume());
    QTRY_VERIFY(m_sig->missionRunning());

    QSignalSpy lobes(m_engine.get(), &SimulationEngine::mainLobeSynced);
    QSignalSpy sent(m_sig.get(), &MockDevice::mainLobeSent);
    m_sig->advanceTo(2.0);   // 두 레이다 첫 MainLobe (0.5 s, 1.2 s)
    QCOMPARE(sent.count(), 2);
    QTRY_COMPARE(lobes.count(), 2);

    // R1(38N 126E, 300 km): 편대 허위타겟 2 (고정표적은 잡음섹터에 가려짐)
    const auto r1Sent = sent.first();
    QCOMPARE(r1Sent.at(0).toUInt(), m_model->radars()[0].id);
    QCOMPARE(r1Sent.at(1).toInt(), 2);
    // R2(39N 127.5E): 잡음섹터 방향이 달라 고정표적 포함 3
    QCOMPARE(sent.at(1).at(1).toInt(), 3);

    const SimSnapshot s = m_engine->advanceTo(0.1);
    QVERIFY(s.beams[0].synced && s.beams[1].synced);
    QCOMPARE(s.detections.size(), sent.at(0).at(1).toInt() + sent.at(1).at(1).toInt());
    for (const Detection& d : s.detections)
        QCOMPARE(d.origin, DetectionOrigin::External);

    m_mission->stop();
    QTRY_VERIFY(!m_sig->missionRunning());
}

QTEST_GUILESS_MAIN(TstMockDevice)
#include "tst_MockDevice.moc"
