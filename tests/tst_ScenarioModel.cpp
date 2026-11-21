#include "log/Logger.h"
#include "model/ScenarioModel.h"

#include <QSignalSpy>
#include <QtTest>

class TstScenarioModel : public QObject {
    Q_OBJECT
private slots:
    void radar_limitAndColors();
    void radar_updateKeepsColor();
    void jammerAndFormation_single();
    void formation_constraints();
    void route_editing();
    void falseTargets_namingAndPosition();
    void remove_and_clear();
    void lock_rejectsEdits();
    void replace_reissuesIds();
    void validate_messages();
    void datalink_clamped();
    void rename();
};

void TstScenarioModel::rename()
{
    ScenarioModel m;
    const ObjectId j = m.createJammer();
    QSignalSpy changed(&m, &ScenarioModel::objectChanged);
    QVERIFY(m.renameObject(j, QStringLiteral("  EA-18  ")));
    QCOMPARE(m.find(j)->name, QStringLiteral("EA-18"));
    QVERIFY(!m.renameObject(j, QStringLiteral("   ")));
    QVERIFY(!m.renameObject(12345, QStringLiteral("x")));
    QCOMPARE(changed.count(), 1);
}

void TstScenarioModel::radar_limitAndColors()
{
    ScenarioModel m;
    QSignalSpy added(&m, &ScenarioModel::objectAdded);
    QVector<ObjectId> ids;
    for (int i = 0; i < 4; ++i)
        ids << m.addRadar({37.0 + i * 0.1, 127.0});
    QCOMPARE(added.count(), 4);
    QCOMPARE(m.radars().size(), 4);
    QCOMPARE(m.radar(ids[2])->name, QStringLiteral("R3"));
    QCOMPARE(m.radar(ids[2])->colorIndex, 2);

    Logger log;
    QSignalSpy logSpy(&log, &Logger::entryAdded);
    m.setLogger(&log);
    QCOMPARE(m.addRadar({36, 127}), ObjectId(0));   // 5번째 거부 + 로그
    QCOMPARE(logSpy.count(), 1);

    // 삭제 후 빈 색상 재사용
    QVERIFY(m.removeObject(ids[1]));
    const ObjectId again = m.addRadar({36, 127});
    QCOMPARE(m.radar(again)->colorIndex, 1);
    QCOMPARE(m.radar(again)->name, QStringLiteral("R2"));
}

void TstScenarioModel::radar_updateKeepsColor()
{
    ScenarioModel m;
    const ObjectId id = m.addRadar({37, 127});
    QSignalSpy changed(&m, &ScenarioModel::objectChanged);
    Radar r = *m.radar(id);
    r.colorIndex = 3;
    r.maxRangeKm = 1e9;   // 보정됨
    r.name = QStringLiteral("서해 레이다");
    QVERIFY(m.updateRadar(r));
    QCOMPARE(changed.count(), 1);
    QCOMPARE(m.radar(id)->colorIndex, 0);
    QCOMPARE(m.radar(id)->maxRangeKm, RadarLimits::MaxRangeKm);
    QCOMPARE(m.radar(id)->name, QStringLiteral("서해 레이다"));

    QVERIFY(m.moveRadar(id, {36.5, 126.5}));
    QCOMPARE(m.radar(id)->pos.lat, 36.5);
}

void TstScenarioModel::jammerAndFormation_single()
{
    ScenarioModel m;
    const ObjectId j = m.createJammer();
    QVERIFY(j);
    QCOMPARE(m.createJammer(), ObjectId(0));
    const ObjectId f = m.createFormation();
    QVERIFY(f);
    QCOMPARE(m.createFormation(), ObjectId(0));
    QCOMPARE(m.objectType(j), std::optional<ObjectType>(ObjectType::Jammer));
    QCOMPARE(m.objectType(f), std::optional<ObjectType>(ObjectType::Formation));
    QVERIFY(m.route(j));
    QVERIFY(m.route(f));
}

void TstScenarioModel::formation_constraints()
{
    ScenarioModel m;
    m.createFormation();
    FriendlyFormation f = *m.formation();
    f.formationSize = 5;
    QVERIFY(!m.updateFormation(f));
    f.formationSize = 0;
    QVERIFY(!m.updateFormation(f));
    f.formationSize = 2;
    f.jamming.targets.resize(9);
    QVERIFY(!m.updateFormation(f));
    f.jamming.resize(8);
    QVERIFY(m.updateFormation(f));
    QCOMPARE(m.formation()->formationSize, 2);
    QCOMPARE(m.formation()->jamming.targets.size(), 8);
}

void TstScenarioModel::route_editing()
{
    ScenarioModel m;
    const ObjectId j = m.createJammer();
    QSignalSpy changed(&m, &ScenarioModel::objectChanged);
    QVERIFY(m.appendWaypoint(j, {37, 125}));
    QVERIFY(m.appendWaypoint(j, {37, 126}));
    QVERIFY(m.appendWaypoint(j, {38, 126}));
    QCOMPARE(m.route(j)->segmentCount(), 2);
    QCOMPARE(m.route(j)->segments[0], JammerAircraft::defaultSegment());
    QVERIFY(m.moveWaypoint(j, 1, {37.5, 126}));
    QCOMPARE(m.route(j)->waypoints[1].lat, 37.5);
    QVERIFY(m.setSegment(j, 1, {4, 180}));
    QCOMPARE(m.route(j)->segments[1].speedMps, 180.0);
    QVERIFY(m.removeWaypoint(j, 2));
    QVERIFY(m.clearRoute(j));
    QVERIFY(m.route(j)->isEmpty());
    QCOMPARE(changed.count(), 7);

    const ObjectId r = m.addRadar({37, 127});
    QVERIFY(!m.appendWaypoint(r, {37, 127}));   // 레이다는 동선 없음
    QVERIFY(!m.moveWaypoint(j, 0, {37, 127}));  // 범위 밖
}

void TstScenarioModel::falseTargets_namingAndPosition()
{
    ScenarioModel m;
    const ObjectId a = m.addFalseTarget(FalseTargetType::Fixed);
    const ObjectId b = m.addFalseTarget(FalseTargetType::Moving);
    const ObjectId c = m.addFalseTarget(FalseTargetType::Noise);
    const ObjectId d = m.addFalseTarget(FalseTargetType::Random);
    QCOMPARE(m.falseTarget(a)->name, QStringLiteral("FT1"));
    QCOMPARE(m.falseTarget(d)->name, QStringLiteral("FT4"));

    QVERIFY(!m.falseTarget(a)->validate());
    QVERIFY(m.setPointPosition(a, {36, 125}));
    QVERIFY(m.falseTarget(a)->validate());
    QVERIFY(!m.setPointPosition(b, {36, 125}));   // 이동표적은 위치 대신 동선
    QVERIFY(m.appendWaypoint(b, {36, 125}));
    QVERIFY(!m.appendWaypoint(a, {36, 125}));     // 고정표적은 동선 없음
    QVERIFY(m.falseTarget(d)->validate());

    StandaloneFalseTarget t = *m.falseTarget(c);
    t.ftType = FalseTargetType::Fixed;   // 타입 변경 불가
    t.detectAngleDeg = 25;
    QVERIFY(m.updateFalseTarget(t));
    QCOMPARE(m.falseTarget(c)->ftType, FalseTargetType::Noise);
    QCOMPARE(m.falseTarget(c)->detectAngleDeg, 25.0);
}

void TstScenarioModel::remove_and_clear()
{
    ScenarioModel m;
    const ObjectId r = m.addRadar({37, 127});
    const ObjectId j = m.createJammer();
    const ObjectId f = m.addFalseTarget(FalseTargetType::Random);
    QCOMPARE(m.objectIds(), (QVector<ObjectId>{r, j, f}));
    QSignalSpy removed(&m, &ScenarioModel::objectRemoved);
    QVERIFY(m.removeObject(j));
    QVERIFY(!m.removeObject(j));
    QCOMPARE(removed.count(), 1);
    QVERIFY(!m.find(j));

    QSignalSpy reset(&m, &ScenarioModel::scenarioReset);
    m.clear();
    QCOMPARE(reset.count(), 1);
    QVERIFY(m.isEmpty());
}

void TstScenarioModel::lock_rejectsEdits()
{
    ScenarioModel m;
    const ObjectId r = m.addRadar({37, 127});
    QSignalSpy lockSpy(&m, &ScenarioModel::lockedChanged);
    m.setLocked(true);
    QCOMPARE(lockSpy.count(), 1);
    QCOMPARE(m.addRadar({37, 128}), ObjectId(0));
    QVERIFY(!m.moveRadar(r, {36, 127}));
    QVERIFY(!m.removeObject(r));
    QCOMPARE(m.createJammer(), ObjectId(0));
    m.setLocked(false);
    QVERIFY(m.moveRadar(r, {36, 127}));
}

void TstScenarioModel::replace_reissuesIds()
{
    ScenarioModel m;
    const ObjectId old = m.addRadar({37, 127});
    Radar a, b;
    a.id = 999;
    a.name = QStringLiteral("A");
    b.id = 999;
    QVERIFY(m.replaceRadars({a, b}));
    QCOMPARE(m.radars().size(), 2);
    QVERIFY(!m.radar(old));
    QVERIFY(m.radars()[0].id != m.radars()[1].id);
    QVERIFY(m.radars()[0].id != 999);
    QCOMPARE(m.radars()[1].name, QStringLiteral("R2"));   // 빈 이름 → 기본
    QCOMPARE(m.radars()[1].colorIndex, 1);

    QVector<Radar> many(6);
    QVERIFY(m.replaceRadars(many));
    QCOMPARE(m.radars().size(), 4);

    ScenarioData d;
    d.jammer = JammerAircraft{};
    d.datalinkIntervalSec = 2.5;
    QVERIFY(m.replaceAll(d));
    QVERIFY(m.radars().isEmpty());
    QVERIFY(m.jammer());
    QVERIFY(m.jammer()->id != 0);
    QCOMPARE(m.datalinkIntervalSec(), 2.5);
}

void TstScenarioModel::validate_messages()
{
    ScenarioModel m;
    QStringList errors;
    QVERIFY(!m.validate(&errors));
    QCOMPARE(errors.size(), 2);   // 레이다 없음, 전자전기 없음
    m.addRadar({37, 127});
    const ObjectId j = m.createJammer();
    m.appendWaypoint(j, {37, 125});
    m.appendWaypoint(j, {37.5, 125});
    QVERIFY2(m.validate(&errors), qPrintable(errors.join('\n')));
    m.addFalseTarget(FalseTargetType::Fixed);   // 위치 미지정
    QVERIFY(!m.validate(&errors));
    QVERIFY(errors.first().startsWith(QStringLiteral("FT1")));
}

void TstScenarioModel::datalink_clamped()
{
    ScenarioModel m;
    QSignalSpy spy(&m, &ScenarioModel::datalinkIntervalChanged);
    m.setDatalinkIntervalSec(0.0);
    QCOMPARE(m.datalinkIntervalSec(), ScenarioModel::MinDatalinkIntervalSec);
    m.setDatalinkIntervalSec(0.0);
    QCOMPARE(spy.count(), 1);
}

QTEST_GUILESS_MAIN(TstScenarioModel)
#include "tst_ScenarioModel.moc"
