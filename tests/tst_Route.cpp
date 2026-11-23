#include "common/GeoUtils.h"
#include "model/JammingConfig.h"
#include "model/Route.h"
#include "sim/RouteKinematics.h"

#include <QtTest>

class TstRoute : public QObject {
    Q_OBJECT
private slots:
    void addWaypoint_createsSegments();
    void removeWaypoint_keepsInvariant();
    void lengthAndDuration();
    void isValid_rules();
    void setSegment_clamps();
    void jammingConfig_resize();
    void loop_segments();
    void loop_kinematics_wraps();
};

void TstRoute::loop_segments()
{
    Route r;
    r.closed = true;
    r.addWaypoint({37, 125}, {9, 220});
    QCOMPARE(r.segmentCount(), 0);
    r.addWaypoint({37, 126});
    QCOMPARE(r.segmentCount(), 2);     // 갔다가 되돌아오는 구간
    QCOMPARE(r.endIndex(1), 0);
    r.addWaypoint({38, 126});
    QCOMPARE(r.segmentCount(), 3);
    QVERIFY(r.isValid());
    const double expected = GeoUtils::distanceKm({37, 125}, {37, 126}) + GeoUtils::distanceKm({37, 126}, {38, 126})
                          + GeoUtils::distanceKm({38, 126}, {37, 125});
    QVERIFY(qAbs(r.totalLengthKm() - expected) < 1e-9);

    QVERIFY(r.removeWaypoint(1));
    QCOMPARE(r.segmentCount(), 2);
    QVERIFY(r.isValid());
    QVERIFY(r.removeWaypoint(1));
    QCOMPARE(r.segmentCount(), 0);     // 점 1개 → 구간 없음

    Route open;
    open.addWaypoint({37, 125});
    open.addWaypoint({37, 126});
    open.addWaypoint({38, 126});
    open.setClosed(true);
    QCOMPARE(open.segmentCount(), 3);
    open.setClosed(false);
    QCOMPARE(open.segmentCount(), 2);
}

void TstRoute::loop_kinematics_wraps()
{
    Route r;
    r.closed = true;
    r.addWaypoint({37.0, 127.0}, {5.0, 250.0});
    r.addWaypoint({37.5, 127.0});
    r.addWaypoint({37.5, 127.5});
    const double lap = r.totalDurationSec();
    QVERIFY(lap > 0);
    const auto a = RouteKinematics::stateAt(r, 100.0);
    const auto b = RouteKinematics::stateAt(r, 100.0 + 3 * lap);   // 3바퀴 뒤 같은 위치
    QVERIFY(GeoUtils::distanceKm(a.pos, b.pos) < 1e-6);
    QVERIFY(!b.finished);
    QVERIFY(b.speedMps > 0);
    // 되돌아오는 구간 중간: 마지막 점과 시작점 사이
    const double t = lap - r.segmentDurationSec(2) / 2;
    const auto c = RouteKinematics::stateAt(r, t);
    QVERIFY(qAbs(GeoUtils::distanceKm(c.pos, {37.5, 127.5}) - r.segmentLengthKm(2) / 2) < 1e-3);
}

void TstRoute::addWaypoint_createsSegments()
{
    Route r;
    r.addWaypoint({37, 127}, {7.0, 300.0});
    QCOMPARE(r.segmentCount(), 0);
    r.addWaypoint({37, 128}, {7.0, 300.0});
    QCOMPARE(r.segmentCount(), 1);
    QCOMPARE(r.segments[0], (RouteSegment{7.0, 300.0}));
    r.setSegment(0, {8.0, 310.0});
    r.addWaypoint({38, 128}, {1.0, 1.0});   // 직전 구간 값 복사 (기본값 무시)
    QCOMPARE(r.segments[1], (RouteSegment{8.0, 310.0}));
}

void TstRoute::removeWaypoint_keepsInvariant()
{
    Route r;
    for (int i = 0; i < 4; ++i)
        r.addWaypoint({37.0 + i, 127.0});
    r.setSegment(0, {1, 100});
    r.setSegment(1, {2, 200});
    r.setSegment(2, {3, 300});

    QVERIFY(r.removeWaypoint(1));   // 구간 1(2→3) 삭제, 0번 구간 유지
    QCOMPARE(r.waypointCount(), 3);
    QCOMPARE(r.segmentCount(), 2);
    QCOMPARE(r.segments[0].speedMps, 100.0);
    QCOMPARE(r.segments[1].speedMps, 300.0);

    QVERIFY(r.removeWaypoint(2));   // 마지막 점 → 직전 구간 삭제
    QCOMPARE(r.segmentCount(), 1);
    QVERIFY(r.removeWaypoint(0));
    QCOMPARE(r.waypointCount(), 1);
    QCOMPARE(r.segmentCount(), 0);
    QVERIFY(!r.removeWaypoint(5));
}

void TstRoute::lengthAndDuration()
{
    Route r;
    r.addWaypoint({37.0, 127.0}, {5.0, 250.0});
    r.addWaypoint({38.0, 127.0}, {5.0, 250.0});
    const double km = GeoUtils::distanceKm({37.0, 127.0}, {38.0, 127.0});
    QVERIFY(qAbs(r.totalLengthKm() - km) < 1e-9);
    QVERIFY(qAbs(r.totalDurationSec() - km * 1000.0 / 250.0) < 1e-6);
}

void TstRoute::isValid_rules()
{
    Route r;
    QString why;
    QVERIFY(!r.isValid(&why));
    QVERIFY(!why.isEmpty());
    r.addWaypoint({37, 127});
    QVERIFY(!r.isValid());
    r.addWaypoint({37, 128});
    QVERIFY(r.isValid());
    r.segments[0].speedMps = 0;   // 직접 조작으로 범위 위반
    QVERIFY(!r.isValid());
}

void TstRoute::setSegment_clamps()
{
    Route r;
    r.addWaypoint({37, 127});
    r.addWaypoint({37, 128});
    QVERIFY(r.setSegment(0, {-5.0, 1e9}));
    QCOMPARE(r.segments[0].altitudeKm, RouteLimits::MinAltitudeKm);
    QCOMPARE(r.segments[0].speedMps, RouteLimits::MaxSpeedMps);
    QVERIFY(!r.setSegment(1, {}));
}

void TstRoute::jammingConfig_resize()
{
    JammingConfig j;
    j.resize(3);
    QCOMPARE(j.targets.size(), 3);
    QCOMPARE(j.targets[0].angleDeg, 45.0);
    QCOMPARE(j.targets[1].angleDeg, 315.0);
    j.targets[0].angleDeg = 10;
    j.resize(20);   // 최대 8
    QCOMPARE(j.targets.size(), 8);
    QCOMPARE(j.targets[0].angleDeg, 10.0);   // 기존 값 유지
    j.resize(0);
    QVERIFY(j.targets.isEmpty());

    j.targets = {{-30.0, -1.0}};
    j.noiseBandwidthMHz = -5;
    j.clampValues();
    QCOMPARE(j.targets[0].angleDeg, 330.0);
    QCOMPARE(j.targets[0].distanceKm, 0.0);
    QCOMPARE(j.noiseBandwidthMHz, 0.0);
}

QTEST_GUILESS_MAIN(TstRoute)
#include "tst_Route.moc"
