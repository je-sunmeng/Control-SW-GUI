#include "common/GeoUtils.h"
#include "sim/RouteKinematics.h"
#include "sim/SimClock.h"

#include <QtTest>

class TstRouteKinematics : public QObject {
    Q_OBJECT
private slots:
    void empty_and_single();
    void alongSegments();
    void finished_stays();
    void heading_follows_route();
    void clock_speedAndPause();
};

void TstRouteKinematics::empty_and_single()
{
    Route r;
    QVERIFY(!RouteKinematics::stateAt(r, 10).valid);
    r.addWaypoint({37, 127});
    const auto s = RouteKinematics::stateAt(r, 10);
    QVERIFY(s.valid);
    QVERIFY(s.finished);
    QCOMPARE(s.pos.lat, 37.0);
}

void TstRouteKinematics::alongSegments()
{
    Route r;
    r.addWaypoint({37.0, 127.0});
    r.addWaypoint({38.0, 127.0}, {5.0, 100.0});   // 첫 구간 생성 시 기본값 적용
    r.addWaypoint({38.0, 128.0});
    r.setSegment(1, {7.0, 200.0});
    const double d0 = r.segmentDurationSec(0), d1 = r.segmentDurationSec(1);

    auto s = RouteKinematics::stateAt(r, -5);   // 시작 전 = 시작점
    QCOMPARE(s.pos.lat, 37.0);

    s = RouteKinematics::stateAt(r, d0 / 2);
    QVERIFY(qAbs(GeoUtils::distanceKm(s.pos, {37.0, 127.0}) - r.segmentLengthKm(0) / 2) < 1e-6);
    QCOMPARE(s.altKm, 5.0);
    QCOMPARE(s.speedMps, 100.0);
    QVERIFY(!s.finished);

    s = RouteKinematics::stateAt(r, d0 + d1 / 4);
    QCOMPARE(s.altKm, 7.0);
    QCOMPARE(s.speedMps, 200.0);
    QVERIFY(qAbs(GeoUtils::distanceKm(s.pos, {38.0, 127.0}) - r.segmentLengthKm(1) / 4) < 1e-6);
}

void TstRouteKinematics::finished_stays()
{
    Route r;
    r.addWaypoint({37.0, 127.0}, {5.0, 250.0});
    r.addWaypoint({37.0, 128.0});
    const double total = r.totalDurationSec();
    auto s = RouteKinematics::stateAt(r, total + 1000);
    QVERIFY(s.finished);
    QCOMPARE(s.speedMps, 0.0);
    QVERIFY(GeoUtils::distanceKm(s.pos, {37.0, 128.0}) < 1e-6);
    QVERIFY(qAbs(GeoUtils::angleDiffDeg(s.headingDeg, 90.0)) < 1.0);   // 마지막 진행방향 유지
}

void TstRouteKinematics::heading_follows_route()
{
    Route r;
    r.addWaypoint({37.0, 127.0});
    r.addWaypoint({38.0, 127.0});   // 북
    r.addWaypoint({38.0, 126.0});   // 서
    QVERIFY(qAbs(GeoUtils::angleDiffDeg(RouteKinematics::stateAt(r, 1).headingDeg, 0.0)) < 1e-6);
    const double t = r.segmentDurationSec(0) + 1;
    QVERIFY(qAbs(GeoUtils::angleDiffDeg(RouteKinematics::stateAt(r, t).headingDeg, 270.0)) < 1.0);
}

void TstRouteKinematics::clock_speedAndPause()
{
    qint64 now = 0;
    SimClock c;
    c.setTimeSource([&now] { return now; });
    c.start();
    now = 1000;
    QCOMPARE(c.simTimeSec(), 1.0);
    c.setSpeedFactor(4);
    now = 2000;
    QCOMPARE(c.simTimeSec(), 5.0);   // 1 + 1×4
    c.pause();
    now = 9000;
    QCOMPARE(c.simTimeSec(), 5.0);
    c.resume();
    now = 9500;
    QCOMPARE(c.simTimeSec(), 7.0);
    c.setSpeedFactor(3);             // 무효
    QCOMPARE(c.speedFactor(), 4);
    c.stop();
    now = 20000;
    QCOMPARE(c.simTimeSec(), 7.0);
    c.reset();
    QCOMPARE(c.simTimeSec(), 0.0);
}

QTEST_GUILESS_MAIN(TstRouteKinematics)
#include "tst_RouteKinematics.moc"
