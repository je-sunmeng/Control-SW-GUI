#include "persistence/JsonSerializer.h"
#include "persistence/ObjectFileIO.h"

#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QtTest>

class TstJsonSerializer : public QObject {
    Q_OBJECT
private slots:
    void radar_roundTrip();
    void radar_designExampleParses();
    void formation_roundTrip();
    void falseTarget_typeSpecificFields();
    void fromJson_errors();
    void files_radarsRoundTrip();
    void files_scenarioRoundTrip();
    void files_wrongKindAndVersion();

private:
    QTemporaryDir m_dir;
};

void TstJsonSerializer::radar_roundTrip()
{
    Radar r;
    r.name = QStringLiteral("R1");
    r.pos = {37.25, 126.75};
    r.freqMHz = 3000;
    r.priMs = 2.5;
    r.pwUs = 4;
    r.maxRangeKm = 250;
    r.scanPeriodSec = 6;
    r.scanDir = ScanDirection::CounterClockwise;
    r.beamWidthDeg = 1.5;
    const QJsonObject o = JsonSerializer::toJson(r);
    QCOMPARE(o.value("scanDir").toString(), QStringLiteral("CCW"));
    QVERIFY(!o.contains("id"));

    Radar back;
    QVERIFY(JsonSerializer::fromJson(o, back));
    QCOMPARE(back.name, r.name);
    QCOMPARE(back.pos.lat, r.pos.lat);
    QCOMPARE(back.freqMHz, r.freqMHz);
    QCOMPARE(back.scanDir, r.scanDir);
    QCOMPARE(back.beamWidthDeg, r.beamWidthDeg);
}

void TstJsonSerializer::radar_designExampleParses()
{
    // docs/DESIGN.md 4.2 예시
    const QByteArray json = R"({ "schemaVersion": 1, "kind": "radar",
      "items": [ { "name": "R1", "lat": 37.5, "lon": 127.0, "freqMHz": 9400, "priMs": 1.0,
                   "pwUs": 1.0, "maxRangeKm": 300, "scanPeriodSec": 10,
                   "scanDir": "CW", "beamWidthDeg": 2.0 } ] })";
    const QString path = m_dir.filePath(QStringLiteral("example.radar.json"));
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(json);
    f.close();
    QString err;
    const auto radars = ObjectFileIO::loadRadars(path, &err);
    QVERIFY2(radars, qPrintable(err));
    QCOMPARE(radars->size(), 1);
    QCOMPARE(radars->first().maxRangeKm, 300.0);
}

void TstJsonSerializer::formation_roundTrip()
{
    FriendlyFormation f;
    f.name = QStringLiteral("편대");
    f.formationSize = 3;
    f.route.addWaypoint({36, 126}, {6, 250});
    f.route.addWaypoint({37, 127});
    f.route.addWaypoint({38, 127});
    f.route.setSegment(1, {7, 260});
    f.jamming.resize(2);
    f.jamming.noiseBandwidthMHz = 20;

    FriendlyFormation back;
    QString err;
    QVERIFY2(JsonSerializer::fromJson(JsonSerializer::toJson(f), back, &err), qPrintable(err));
    QCOMPARE(back.name, f.name);
    QCOMPARE(back.formationSize, 3);
    QCOMPARE(back.route.waypointCount(), 3);
    QCOMPARE(back.route.segments, f.route.segments);
    QCOMPARE(back.jamming, f.jamming);
}

void TstJsonSerializer::falseTarget_typeSpecificFields()
{
    StandaloneFalseTarget noise;
    noise.ftType = FalseTargetType::Noise;
    noise.pos = {37, 125};
    noise.posSet = true;
    noise.detectAngleDeg = 15;
    QJsonObject o = JsonSerializer::toJson(noise);
    QCOMPARE(o.value("type").toString(), QStringLiteral("Noise"));
    QVERIFY(o.contains("detectAngleDeg"));
    QVERIFY(!o.contains("route"));
    StandaloneFalseTarget back;
    QVERIFY(JsonSerializer::fromJson(o, back));
    QCOMPARE(back.ftType, FalseTargetType::Noise);
    QVERIFY(back.posSet);
    QCOMPARE(back.detectAngleDeg, 15.0);

    StandaloneFalseTarget fixedUnset;   // 위치 미지정은 lat/lon 저장 안 함
    o = JsonSerializer::toJson(fixedUnset);
    QVERIFY(!o.contains("lat"));
    QVERIFY(JsonSerializer::fromJson(o, back));
    QVERIFY(!back.posSet);

    StandaloneFalseTarget moving;
    moving.ftType = FalseTargetType::Moving;
    moving.route.addWaypoint({35, 125});
    moving.route.addWaypoint({35, 126});
    QVERIFY(JsonSerializer::fromJson(JsonSerializer::toJson(moving), back));
    QCOMPARE(back.route.waypointCount(), 2);
}

void TstJsonSerializer::fromJson_errors()
{
    Radar r;
    QString err;
    QVERIFY(!JsonSerializer::fromJson(QJsonObject{{"lat", 37}}, r, &err));
    QVERIFY(!err.isEmpty());
    QVERIFY(!JsonSerializer::fromJson(QJsonObject{{"lat", 95}, {"lon", 127}}, r, &err));
    QVERIFY(!JsonSerializer::fromJson(QJsonObject{{"lat", 37}, {"lon", 127}, {"freqMHz", "fast"}}, r, &err));
    QVERIFY(err.contains("freqMHz"));
    QVERIFY(!JsonSerializer::fromJson(QJsonObject{{"lat", 37}, {"lon", 127}, {"scanDir", "UP"}}, r, &err));

    StandaloneFalseTarget t;
    QVERIFY(!JsonSerializer::fromJson(QJsonObject{{"type", "Bogus"}}, t, &err));

    FriendlyFormation f;
    QVERIFY(!JsonSerializer::fromJson(QJsonObject{{"formationSize", 9}}, f, &err));

    // 구간 누락 → 직전/기본값으로 채움
    Route route;
    const QJsonObject ro = QJsonDocument::fromJson(
        R"({"waypoints":[{"lat":37,"lon":127},{"lat":37,"lon":128},{"lat":38,"lon":128}],
            "segments":[{"altitudeKm":3,"speedMps":111}]})").object();
    QVERIFY(JsonSerializer::fromJson(ro, route, &err));
    QCOMPARE(route.segmentCount(), 2);
    QCOMPARE(route.segments[1].speedMps, 111.0);
}

void TstJsonSerializer::files_radarsRoundTrip()
{
    QVector<Radar> radars(2);
    radars[0].name = QStringLiteral("A");
    radars[1].name = QStringLiteral("B");
    radars[1].pos = {38, 128};
    const QString path = m_dir.filePath(QStringLiteral("r.radar.json"));
    QString err;
    QVERIFY2(ObjectFileIO::saveRadars(path, radars, &err), qPrintable(err));
    const auto back = ObjectFileIO::loadRadars(path, &err);
    QVERIFY2(back, qPrintable(err));
    QCOMPARE(back->size(), 2);
    QCOMPARE(back->at(1).name, QStringLiteral("B"));
    QCOMPARE(back->at(1).pos.lon, 128.0);
}

void TstJsonSerializer::files_scenarioRoundTrip()
{
    ScenarioModel m;
    m.addRadar({37, 127});
    const ObjectId j = m.createJammer();
    m.appendWaypoint(j, {37, 124});
    m.appendWaypoint(j, {38, 124});
    const ObjectId f = m.createFormation();
    m.appendWaypoint(f, {36, 126});
    m.appendWaypoint(f, {37, 127});
    const ObjectId ft = m.addFalseTarget(FalseTargetType::Fixed);
    m.setPointPosition(ft, {36.5, 126.5});
    m.setDatalinkIntervalSec(0.5);
    m.setJammerIntervalSec(3.0);

    const QString path = m_dir.filePath(QStringLiteral("s.scenario.json"));
    QString err;
    QVERIFY2(ObjectFileIO::saveScenario(path, m.snapshot(), &err), qPrintable(err));
    const auto d = ObjectFileIO::loadScenario(path, &err);
    QVERIFY2(d, qPrintable(err));

    ScenarioModel m2;
    QVERIFY(m2.replaceAll(*d));
    QCOMPARE(m2.radars().size(), 1);
    QVERIFY(m2.jammer());
    QCOMPARE(m2.jammer()->route.waypointCount(), 2);
    QVERIFY(m2.jammer()->route.closed);
    QCOMPARE(m2.jammer()->route.segmentCount(), 2);   // 루프 저장/복원
    QVERIFY(m2.formation());
    QCOMPARE(m2.falseTargets().size(), 1);
    QVERIFY(m2.falseTargets().first().posSet);
    QCOMPARE(m2.datalinkIntervalSec(), 0.5);
    QCOMPARE(m2.jammerIntervalSec(), 3.0);
    QVERIFY(m2.validate());
}

void TstJsonSerializer::files_wrongKindAndVersion()
{
    const QString path = m_dir.filePath(QStringLiteral("x.radar.json"));
    QString err;
    QVERIFY(ObjectFileIO::saveRadars(path, {}, &err));
    QVERIFY(!ObjectFileIO::loadFormation(path, &err));
    QVERIFY(err.contains(QStringLiteral("종류")));

    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(R"({"schemaVersion": 99, "kind": "radar", "items": []})");
    f.close();
    QVERIFY(!ObjectFileIO::loadRadars(path, &err));
    QVERIFY(err.contains(QStringLiteral("schemaVersion")));

    QVERIFY(!ObjectFileIO::loadRadars(m_dir.filePath(QStringLiteral("missing.json")), &err));
}

QTEST_GUILESS_MAIN(TstJsonSerializer)
#include "tst_JsonSerializer.moc"
