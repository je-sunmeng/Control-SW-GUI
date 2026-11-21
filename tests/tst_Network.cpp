#include "log/Logger.h"
#include "mission/MissionController.h"
#include "net/FrameDecoder.h"
#include "net/MessageHeader.h"
#include "net/NetworkManager.h"
#include "net/Opcode.h"
#include "net/messages/HeartbeatMsg.h"
#include "net/messages/MainLobeNotifyMsg.h"
#include "sim/SimulationEngine.h"

#include <QSignalSpy>
#include <QTcpSocket>
#include <QtTest>

#include <memory>

// 실제 TCP 로 연동모드 전 과정 검증: 접속·식별 → 임무전송 → 시작 → 위치 주기송신 → MainLobe 수신 → 중지 → 해제
class TstNetwork : public QObject {
    Q_OBJECT
private slots:
    void init();
    void cleanup();
    void linkedMission_endToEnd();
    void standalone_closesServer();
    void duplicateDevice_replacesOld();
    void garbage_isLoggedAsRaw();

private:
    // 모의 장치 클라이언트
    struct Client {
        QTcpSocket sock;
        FrameDecoder decoder{1 << 20};
        QVector<FrameDecoder::Frame> frames;
        void pump()
        {
            frames += decoder.feed(sock.readAll());
        }
        QList<quint16> cmds() const
        {
            QList<quint16> l;
            for (const auto& f : frames)
                l << Opcode::cmd(f.header.opcode);
            return l;
        }
    };

    std::unique_ptr<Client> connectDevice(DeviceCode dev);
    void buildScenario();

    std::unique_ptr<Logger> m_log;
    std::unique_ptr<ProtocolTable> m_table;
    std::unique_ptr<ScenarioModel> m_model;
    std::unique_ptr<SimulationEngine> m_engine;
    std::unique_ptr<NetworkManager> m_net;
    std::unique_ptr<MissionController> m_mission;
    ObjectId m_radar = 0;
};

void TstNetwork::init()
{
    m_log = std::make_unique<Logger>();
    m_table = std::make_unique<ProtocolTable>();
    QVERIFY(m_table->loadFile(ProtocolTable::defaultResourcePath()));
    m_model = std::make_unique<ScenarioModel>();
    m_engine = std::make_unique<SimulationEngine>(*m_model);
    m_engine->setTimerEnabled(false);
    m_net = std::make_unique<NetworkManager>(*m_table, *m_log);
    m_mission = std::make_unique<MissionController>(*m_model, *m_engine, *m_log, m_net.get());
    m_mission->setServerPort(0);   // OS 가 빈 포트 할당
}

void TstNetwork::cleanup()
{
    m_mission.reset();
    m_net.reset();
    m_engine.reset();
    m_model.reset();
    m_table.reset();
    m_log.reset();
}

void TstNetwork::buildScenario()
{
    m_radar = m_model->addRadar({38.0, 126.0});
    const ObjectId j = m_model->createJammer();
    m_model->appendWaypoint(j, {37.0, 125.0});
    m_model->appendWaypoint(j, {37.0, 127.0});
    const ObjectId f = m_model->createFormation();
    m_model->appendWaypoint(f, {36.8, 126.5});
    m_model->appendWaypoint(f, {37.8, 126.5});
    m_model->addFalseTarget(FalseTargetType::Random);
}

std::unique_ptr<TstNetwork::Client> TstNetwork::connectDevice(DeviceCode dev)
{
    auto c = std::make_unique<Client>();
    c->sock.connectToHost(QHostAddress::LocalHost, m_net->port());
    if (!c->sock.waitForConnected(3000))
        return nullptr;
    // 접속 직후 Heartbeat 로 장치 식별
    HeartbeatMsg hb;
    hb.deviceCode = quint8(dev);
    const quint32 op = *m_table->opcodeFor(MessageId::Heartbeat, dev, DeviceCode::ControlSW);
    c->sock.write(MessageHeader::frame(op, hb.payload()));
    c->sock.flush();
    return c;
}

void TstNetwork::linkedMission_endToEnd()
{
    buildScenario();
    QSignalSpy conn(m_net.get(), &NetworkManager::deviceConnectionChanged);
    QVERIFY(m_mission->setMode(RunMode::Linked));
    QVERIFY(m_net->isEnabled());
    QVERIFY(m_net->port() != 0);
    QVERIFY(!m_mission->actions().sendMission);

    auto analysis = connectDevice(DeviceCode::AnalysisSW);
    auto sigGen = connectDevice(DeviceCode::SignalGen);
    QVERIFY(analysis && sigGen);
    QTRY_VERIFY(m_net->isConnected(DeviceCode::AnalysisSW) && m_net->isConnected(DeviceCode::SignalGen));
    QCOMPARE(conn.count(), 2);
    QVERIFY(m_mission->actions().sendMission);

    // 임무전송
    QVERIFY(m_mission->sendMission());
    QTRY_VERIFY((analysis->pump(), analysis->frames.size() >= 3));
    QTRY_VERIFY((sigGen->pump(), sigGen->frames.size() >= 3));
    QCOMPARE(analysis->cmds(), (QList<quint16>{0x0001, 0x0002, 0x0003}));   // Radar, JammerRoute, Formation
    QCOMPARE(sigGen->cmds(), (QList<quint16>{0x0001, 0x0003, 0x0004}));     // Radar, Formation(+교란), FalseTarget
    for (const auto& f : analysis->frames) {
        QCOMPARE(Opcode::src(f.header.opcode), quint8(0x21));
        QCOMPARE(Opcode::dst(f.header.opcode), quint8(0x92));
        QCOMPARE(int(f.header.dataSize), f.payload.size());
    }
    QVERIFY(m_mission->actions().start);

    // 시작 → START + t=0 위치 송신
    analysis->frames.clear();
    sigGen->frames.clear();
    QVERIFY(m_mission->startOrResume());
    QTRY_VERIFY((analysis->pump(), !analysis->frames.isEmpty()));
    QCOMPARE(analysis->frames[0].header.opcode, 0x21920010u);
    QCOMPARE(analysis->frames[0].header.dataSize, 0u);
    QTRY_VERIFY((sigGen->pump(), sigGen->frames.size() >= 3));
    QCOMPARE(sigGen->frames[0].header.opcode, 0x21B10010u);
    QCOMPARE(sigGen->frames[0].header.dataSize, 32u);   // 편대/전자전기 시작위치
    QVERIFY(sigGen->cmds().contains(0x0020));   // JammerPosition
    QVERIFY(sigGen->cmds().contains(0x0021));   // FormationPosition

    // 주기 송신: 3 s 진행 → 전자전기 위치 3회 추가
    sigGen->frames.clear();
    for (int i = 1; i <= 30; ++i)
        m_engine->advanceTo(i / 10.0);
    QTRY_VERIFY((sigGen->pump(), sigGen->cmds().count(0x0020) == 3));

    // MainLobe 수신 → 빔 동기화 + 램프 + 탐지
    QSignalSpy lobes(m_engine.get(), &SimulationEngine::mainLobeSynced);
    MainLobeNotifyMsg ml;
    ml.radarId = m_radar;
    ml.searchTime = 3.0;
    ml.detections = {{37.9, 126.2}, {37.7, 126.4}};
    sigGen->sock.write(MessageHeader::frame(0xB1210100u, ml.payload()));
    sigGen->sock.flush();
    QTRY_COMPARE(lobes.count(), 1);
    const SimSnapshot s = m_engine->advanceTo(3.1);
    QVERIFY(s.beams[0].synced);
    QCOMPARE(s.detections.size(), 2);
    QCOMPARE(s.detections[0].origin, DetectionOrigin::External);

    // 알 수 없는 레이다 → 무시 + 오류 로그
    QSignalSpy logs(m_log.get(), &Logger::entryAdded);
    ml.radarId = 9999;
    sigGen->sock.write(MessageHeader::frame(0xB1210100u, ml.payload()));
    sigGen->sock.flush();
    QTRY_VERIFY(std::any_of(logs.cbegin(), logs.cend(), [](const QList<QVariant>& a) {
        const auto e = a.first().value<LogEntry>();
        return e.type == LogType::Error && e.content.contains(QStringLiteral("알 수 없는 레이다"));
    }));
    QCOMPARE(lobes.count(), 1);

    // 중지 → STOP 양쪽
    analysis->frames.clear();
    sigGen->frames.clear();
    m_mission->stop();
    QTRY_VERIFY((analysis->pump(), analysis->cmds().contains(0x0012)));
    QTRY_VERIFY((sigGen->pump(), sigGen->cmds().contains(0x0012)));

    // 연결 해제 → 상태 반영, 임무전송 플래그 초기화
    sigGen->sock.disconnectFromHost();
    QTRY_VERIFY(!m_net->isConnected(DeviceCode::SignalGen));
    QVERIFY(!m_mission->isDeviceConnected(DeviceCode::SignalGen));

    // Tx/Rx 로그 기록 확인
    bool hasTx = false, hasRx = false;
    for (const auto& a : logs) {
        const auto e = a.first().value<LogEntry>();
        hasTx |= e.type == LogType::Tx;
        hasRx |= e.type == LogType::Rx;
    }
    QVERIFY(hasTx && hasRx);
}

void TstNetwork::standalone_closesServer()
{
    QVERIFY(m_mission->setMode(RunMode::Linked));
    auto c = connectDevice(DeviceCode::AnalysisSW);
    QVERIFY(c);
    QTRY_VERIFY(m_net->isConnected(DeviceCode::AnalysisSW));
    QSignalSpy conn(m_net.get(), &NetworkManager::deviceConnectionChanged);
    QVERIFY(m_mission->setMode(RunMode::Standalone));
    QVERIFY(!m_net->isEnabled());
    QVERIFY(!m_net->isConnected(DeviceCode::AnalysisSW));
    QCOMPARE(conn.count(), 1);
    QTRY_COMPARE(c->sock.state(), QAbstractSocket::UnconnectedState);
    HeartbeatMsg hb;
    QVERIFY(!m_net->send(DeviceCode::AnalysisSW, hb));   // 비활성 → 송신 무시
}

void TstNetwork::duplicateDevice_replacesOld()
{
    QVERIFY(m_mission->setMode(RunMode::Linked));
    auto first = connectDevice(DeviceCode::SignalGen);
    QTRY_VERIFY(m_net->isConnected(DeviceCode::SignalGen));
    auto second = connectDevice(DeviceCode::SignalGen);
    QTRY_COMPARE(first->sock.state(), QAbstractSocket::UnconnectedState);
    QVERIFY(m_net->isConnected(DeviceCode::SignalGen));
    HeartbeatMsg hb;
    QVERIFY(m_net->send(DeviceCode::SignalGen, hb));
    QTRY_VERIFY((second->pump(), !second->frames.isEmpty()));
}

void TstNetwork::garbage_isLoggedAsRaw()
{
    QVERIFY(m_mission->setMode(RunMode::Linked));
    auto c = connectDevice(DeviceCode::SignalGen);
    QTRY_VERIFY(m_net->isConnected(DeviceCode::SignalGen));
    QSignalSpy logs(m_log.get(), &Logger::entryAdded);
    QSignalSpy msgs(m_net.get(), &NetworkManager::messageReceived);
    c->sock.write(MessageHeader::frame(0xB1217777u, QByteArray("??")));
    c->sock.flush();
    QTRY_VERIFY(std::any_of(logs.cbegin(), logs.cend(), [](const QList<QVariant>& a) {
        const auto e = a.first().value<LogEntry>();
        return e.type == LogType::Rx && e.content.contains(QStringLiteral("미등록"));
    }));
    QCOMPARE(msgs.count(), 0);
}

QTEST_GUILESS_MAIN(TstNetwork)
#include "tst_Network.moc"
