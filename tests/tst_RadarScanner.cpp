#include "common/GeoUtils.h"
#include "sim/FalseTargetGenerator.h"
#include "sim/RadarScanner.h"

#include <QtTest>

class TstRadarScanner : public QObject {
    Q_OBJECT
private slots:
    void unsyncedUntilMainLobe();
    void clockwise_rotation();
    void counterClockwise_rotation();
    void resync_resetsReference();
    void formationTargets_rigid();
    void ghosts_lifecycle();
};

void TstRadarScanner::unsyncedUntilMainLobe()
{
    RadarScanner s;
    s.setRadar(1, {10.0, ScanDirection::Clockwise});
    QVERIFY(!s.isSynced(1));
    QCOMPARE(s.lastSyncTime(1), -1.0);
    s.onMainLobe(99, 0, 10);   // 미등록 레이다 무시
    QVERIFY(!s.isSynced(99));
}

void TstRadarScanner::clockwise_rotation()
{
    RadarScanner s;
    s.setRadar(1, {10.0, ScanDirection::Clockwise});
    s.onMainLobe(1, 2.0, 350.0);
    QCOMPARE(s.beamAzimuthAt(1, 2.0), 350.0);
    QVERIFY(qAbs(s.beamAzimuthAt(1, 2.5) - 8.0) < 1e-9);    // 350 + 18
    QVERIFY(qAbs(s.beamAzimuthAt(1, 12.0) - 350.0) < 1e-9);  // 한 바퀴
}

void TstRadarScanner::counterClockwise_rotation()
{
    RadarScanner s;
    s.setRadar(2, {4.0, ScanDirection::CounterClockwise});
    s.onMainLobe(2, 0.0, 10.0);
    QVERIFY(qAbs(s.beamAzimuthAt(2, 1.0) - 280.0) < 1e-9);  // 10 − 90
}

void TstRadarScanner::resync_resetsReference()
{
    RadarScanner s;
    s.setRadar(1, {10.0, ScanDirection::Clockwise});
    s.onMainLobe(1, 0.0, 0.0);
    s.onMainLobe(1, 5.0, 100.0);
    QCOMPARE(s.lastSyncTime(1), 5.0);
    QVERIFY(qAbs(s.beamAzimuthAt(1, 6.0) - 136.0) < 1e-9);
}

void TstRadarScanner::formationTargets_rigid()
{
    JammingConfig cfg;
    cfg.targets = {{90.0, 10.0}, {0.0, 0.0}};
    const GeoPoint p{37.0, 127.0};
    const auto pts = FalseTargetGenerator::formationTargets(p, 0.0, cfg);   // 북쪽 진행
    QCOMPARE(pts.size(), 2);
    QVERIFY(qAbs(GeoUtils::distanceKm(p, pts[0]) - 10.0) < 1e-6);
    QVERIFY(qAbs(GeoUtils::angleDiffDeg(GeoUtils::bearingDeg(p, pts[0]), 90.0)) < 1e-6);   // 진행방향 기준 시계방향 90° = 동
    QVERIFY(GeoUtils::distanceKm(p, pts[1]) < 1e-9);

    const auto turned = FalseTargetGenerator::formationTargets(p, 180.0, cfg);   // 남쪽 진행 → 서쪽
    QVERIFY(qAbs(GeoUtils::angleDiffDeg(GeoUtils::bearingDeg(p, turned[0]), 270.0)) < 1e-6);
}

void TstRadarScanner::ghosts_lifecycle()
{
    Radar r;
    r.id = 7;
    r.pos = {38.0, 126.0};
    r.maxRangeKm = 200;
    FalseTargetGenerator g(12345);
    int maxAlive = 0;
    for (double t = 0; t <= 60.0; t += 0.1) {
        g.updateGhosts(t, {r}, true);
        maxAlive = std::max(maxAlive, int(g.ghosts().size()));
        for (const GhostState& gh : g.ghosts()) {
            QCOMPARE(gh.radarId, ObjectId(7));
            QVERIFY(GeoUtils::distanceKm(r.pos, gh.pos) <= r.maxRangeKm + 1e-6);
            QVERIFY(t < gh.bornAt + gh.lifetimeSec);
            QVERIFY(gh.lifetimeSec >= FalseTargetGenerator::MinLifetimeSec && gh.lifetimeSec <= FalseTargetGenerator::MaxLifetimeSec);
        }
    }
    QVERIFY(maxAlive >= 1);
    QVERIFY(maxAlive <= 8);

    // 큰 시간 점프에도 폭증하지 않음
    g.updateGhosts(10000.0, {r}, true);
    QVERIFY(g.ghosts().size() <= 8);
    g.updateGhosts(10001.0, {r}, false);
    QVERIFY(g.ghosts().isEmpty());

    GhostState gs;
    gs.bornAt = 0;
    gs.lifetimeSec = 4;
    QCOMPARE(gs.opacityAt(0.5), 0.5);
    QCOMPARE(gs.opacityAt(2.0), 1.0);
    QCOMPARE(gs.opacityAt(4.0), 0.0);
}

QTEST_GUILESS_MAIN(TstRadarScanner)
#include "tst_RadarScanner.moc"
