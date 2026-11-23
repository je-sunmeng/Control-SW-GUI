#include "net/ByteReader.h"
#include "net/ByteWriter.h"
#include "net/MessageFactory.h"
#include "net/Opcode.h"
#include "net/ProtocolTable.h"
#include "net/messages/FalseTargetInfoMsg.h"
#include "net/messages/FormationInfoMsg.h"
#include "net/messages/HeartbeatMsg.h"
#include "net/messages/JammerRouteMsg.h"
#include "net/messages/MainLobeNotifyMsg.h"
#include "net/messages/MissionStartMsg.h"
#include "net/messages/MissionStopMsg.h"
#include "net/messages/PositionMsg.h"
#include "net/messages/RadarInfoMsg.h"
#include "net/messages/RawMessage.h"

#include <QtTest>

// 직렬화 왕복 / BigEndian 기대 바이트열 검증 (docs/PROTOCOL_GUIDE.md 4.4)
class TstMessageCodec : public QObject {
    Q_OBJECT
private slots:
    void byteWriter_bigEndian();
    void byteReader_underrun();
    void radarInfo_expectedBytes();
    void radarInfo_roundTrip();
    void jammerRoute_roundTrip();
    void formationInfo_withAndWithoutJamming();
    void falseTargetInfo_allTypes();
    void missionStart_twoForms();
    void position_and_mainLobe_expectedBytes();
    void factory_decode();

private:
    template <typename T>
    static T roundTrip(const T& in)
    {
        T out;
        ByteReader r(in.payload());
        const bool ok = out.readPayload(r);
        if (!ok || !r.finished())
            qWarning("roundTrip failed");
        return out;
    }
};

void TstMessageCodec::byteWriter_bigEndian()
{
    ByteWriter w;
    w.u8(0xAB);
    w.u16(0x1234);
    w.u32(0xDEADBEEF);
    w.i32(-2);
    w.f32(1.0);
    w.f64(-2.5);
    w.str(QStringLiteral("가A"));
    QCOMPARE(w.data().toHex().toUpper(),
             QByteArray("AB1234DEADBEEFFFFFFFFE3F800000C004000000000000") + QByteArray("0004EAB08041"));
}

void TstMessageCodec::byteReader_underrun()
{
    ByteReader r(QByteArray::fromHex("0102"));
    QCOMPARE(r.u8(), quint8(1));
    QVERIFY(!r.finished());
    QCOMPARE(r.u32(), 0u);   // 언더런
    QVERIFY(!r.ok());
    QCOMPARE(r.u8(), quint8(0));   // 이후 읽기도 0
    ByteReader s(QByteArray::fromHex("0003414243"));
    QCOMPARE(s.str(), QStringLiteral("ABC"));
    QVERIFY(s.finished());
}

void TstMessageCodec::radarInfo_expectedBytes()
{
    RadarInfoMsg m;
    Radar r;
    r.id = 1;
    r.pos = {37.5, 127.0};
    r.freqMHz = 9400;
    r.priMs = 1;
    r.pwUs = 1;
    r.maxRangeKm = 300;
    r.scanPeriodSec = 10;
    r.scanDir = ScanDirection::CounterClockwise;
    r.beamWidthDeg = 2;
    m.radars = {r};
    const QByteArray expected = QByteArray::fromHex(
        "01"                    // count
        "00000001"              // id
        "4042C00000000000"      // lat 37.5 (f64)
        "405FC00000000000"      // lon 127.0
        "4612E000"              // freq 9400 (f32)
        "3F800000"              // pri 1
        "3F800000"              // pw 1
        "43960000"              // range 300
        "41200000"              // period 10
        "01"                    // dir CCW
        "40000000");            // beam 2
    QCOMPARE(m.payload().toHex(), expected.toHex());
}

void TstMessageCodec::radarInfo_roundTrip()
{
    RadarInfoMsg m;
    for (int i = 0; i < 4; ++i) {
        Radar r;
        r.id = 10 + i;
        r.pos = {36.0 + i * 0.123456789, 125.0 + i};
        r.maxRangeKm = 250 + i;
        m.radars.append(r);
    }
    const RadarInfoMsg back = roundTrip(m);
    QCOMPARE(back.radars.size(), 4);
    QCOMPARE(back.radars[3].id, 13u);
    QCOMPARE(back.radars[3].pos.lat, m.radars[3].pos.lat);   // f64 무손실
    QCOMPARE(back.radars[3].maxRangeKm, 253.0);
    QVERIFY(back.summary().contains(QStringLiteral("radars=4")));
}

void TstMessageCodec::jammerRoute_roundTrip()
{
    JammerRouteMsg m;
    m.route.addWaypoint({37, 125}, {9, 220});
    m.route.addWaypoint({37.5, 126});
    m.route.addWaypoint({38, 127});
    m.route.setSegment(1, {9.5, 230});
    const JammerRouteMsg back = roundTrip(m);
    QCOMPARE(back.route.waypointCount(), 3);
    QCOMPARE(back.route.segmentCount(), 2);
    QCOMPARE(back.route.segments[1].speedMps, 230.0);
    QCOMPARE(m.payload().size(), 2 + 3 * 24);

    // 루프: 시작점을 마지막에 반복해 송신 → 수신 측에서 루프로 복원
    m.route.setClosed(true);
    QCOMPARE(m.payload().size(), 2 + 4 * 24);
    const JammerRouteMsg loop = roundTrip(m);
    QVERIFY(loop.route.closed);
    QCOMPARE(loop.route.waypointCount(), 3);
    QCOMPARE(loop.route.segmentCount(), 3);
    QCOMPARE(loop.route.segments[2], m.route.segments[2]);
}

void TstMessageCodec::formationInfo_withAndWithoutJamming()
{
    FormationInfoMsg m;
    m.formation.route.addWaypoint({36, 126});
    m.formation.route.addWaypoint({37, 126});
    m.formation.formationSize = 3;
    m.formation.jamming.resize(2);
    m.formation.jamming.noiseBandwidthMHz = 20;
    FormationInfoMsg back = roundTrip(m);
    QVERIFY(back.includeJamming);
    QCOMPARE(back.formation.formationSize, 3);
    QCOMPARE(back.formation.jamming.targets.size(), 2);
    QCOMPARE(back.formation.jamming.noiseBandwidthMHz, 20.0);

    m.includeJamming = false;
    back = roundTrip(m);
    QVERIFY(!back.includeJamming);
    QVERIFY(back.formation.jamming.targets.isEmpty());
    QCOMPARE(m.payload().size(), 2 + 2 * 24 + 1 + 1);
}

void TstMessageCodec::falseTargetInfo_allTypes()
{
    FalseTargetInfoMsg m;
    StandaloneFalseTarget a, b, c, d;
    a.id = 1;
    a.ftType = FalseTargetType::Fixed;
    a.pos = {38, 125};
    b.id = 2;
    b.ftType = FalseTargetType::Moving;
    b.route.addWaypoint({37, 124});
    b.route.addWaypoint({38, 124});
    c.id = 3;
    c.ftType = FalseTargetType::Random;
    d.id = 4;
    d.ftType = FalseTargetType::Noise;
    d.pos = {38, 128};
    d.detectAngleDeg = 12;
    m.targets = {a, b, c, d};
    const FalseTargetInfoMsg back = roundTrip(m);
    QCOMPARE(back.targets.size(), 4);
    QCOMPARE(back.targets[0].pos.lon, 125.0);
    QCOMPARE(back.targets[1].route.waypointCount(), 2);
    QCOMPARE(back.targets[2].ftType, FalseTargetType::Random);
    QCOMPARE(back.targets[3].detectAngleDeg, 12.0);

    // 잘못된 타입 값 → 실패
    FalseTargetInfoMsg bad;
    ByteReader r(QByteArray::fromHex("010000000107"));
    QVERIFY(!bad.readPayload(r));
}

void TstMessageCodec::missionStart_twoForms()
{
    MissionStartMsg toAnalysis;
    QVERIFY(toAnalysis.payload().isEmpty());
    MissionStartMsg toSig;
    toSig.withStartPositions = true;
    toSig.formationStart = {36.6, 126.9};
    toSig.jammerStart = {37.05, 125.4};
    QCOMPARE(toSig.payload().size(), 32);
    const MissionStartMsg back = roundTrip(toSig);
    QVERIFY(back.withStartPositions);
    QCOMPARE(back.jammerStart.lon, 125.4);
    QVERIFY(MissionStopMsg().payload().isEmpty());
}

void TstMessageCodec::position_and_mainLobe_expectedBytes()
{
    JammerPositionMsg p;
    p.simTime = 1.0;
    p.pos = {0.0, 0.0};
    p.altKm = 9.0;
    p.headingDeg = 90.0;
    QCOMPARE(p.payload().toHex().toUpper(),
             QByteArray("3FF0000000000000" "0000000000000000" "0000000000000000" "41100000" "42B40000"));

    MainLobeNotifyMsg m;
    m.radarId = 7;
    m.searchTime = 2.5;
    m.detections = {{38.0, 126.0}};
    QCOMPARE(m.payload().toHex().toUpper(),
             QByteArray("00000007" "4004000000000000" "01" "4043000000000000" "405F800000000000"));
    const MainLobeNotifyMsg back = roundTrip(m);
    QCOMPARE(back.radarId, 7u);
    QCOMPARE(back.detections.size(), 1);

    HeartbeatMsg h;
    h.deviceCode = 0xB1;
    QCOMPARE(h.payload(), QByteArray::fromHex("B100"));
}

void TstMessageCodec::factory_decode()
{
    ProtocolTable table;
    QStringList errors;
    QVERIFY2(table.loadFile(ProtocolTable::defaultResourcePath(), &errors), qPrintable(errors.join('\n')));
    MessageFactory f;
    MessageFactory::registerAll(f);
    for (MessageId id : MessageIds::all())
        QVERIFY2(f.isRegistered(id), qPrintable(MessageIds::toString(id)));

    MainLobeNotifyMsg m;
    m.radarId = 3;
    const quint32 op = *table.opcodeFor(MessageId::MainLobeNotify, DeviceCode::SignalGen, DeviceCode::ControlSW);
    auto res = f.decode(op, m.payload(), table);
    QVERIFY(res.known && res.parsed);
    QCOMPARE(res.message->id(), MessageId::MainLobeNotify);

    // 남은 바이트 있음 → 해석 실패 → Raw
    res = f.decode(op, m.payload() + QByteArray("x"), table);
    QVERIFY(res.known && !res.parsed);
    QCOMPARE(res.message->id(), MessageId::Raw);

    // 미등록 Opcode → Raw
    res = f.decode(Opcode::make(0xB1, 0x21, 0x7777), QByteArray("abc"), table);
    QVERIFY(!res.known);
    QCOMPARE(res.message->id(), MessageId::Raw);
    QVERIFY(res.message->summary().contains(QStringLiteral("61 62 63")));
}

QTEST_GUILESS_MAIN(TstMessageCodec)
#include "tst_MessageCodec.moc"
