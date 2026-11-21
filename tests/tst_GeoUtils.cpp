#include "common/GeoUtils.h"

#include <QtTest>

class TstGeoUtils : public QObject {
    Q_OBJECT
private slots:
    void distance_zero();
    void distance_seoulBusan();
    void bearing_cardinal();
    void destination_roundTrip();
    void interpolate_midpoint();
    void normalize_and_diff();
};

void TstGeoUtils::distance_zero()
{
    const GeoPoint p{37.5, 127.0};
    QCOMPARE(GeoUtils::distanceKm(p, p), 0.0);
}

void TstGeoUtils::distance_seoulBusan()
{
    // 서울시청 ↔ 부산시청 대권거리 ≈ 325 km
    const GeoPoint seoul{37.5663, 126.9779};
    const GeoPoint busan{35.1796, 129.0756};
    const double d = GeoUtils::distanceKm(seoul, busan);
    QVERIFY2(qAbs(d - 325.0) < 5.0, qPrintable(QString::number(d)));
}

void TstGeoUtils::bearing_cardinal()
{
    const GeoPoint o{37.0, 127.0};
    QVERIFY(qAbs(GeoUtils::bearingDeg(o, {38.0, 127.0}) - 0.0) < 1e-6);
    QVERIFY(qAbs(GeoUtils::bearingDeg(o, {36.0, 127.0}) - 180.0) < 1e-6);
    const double east = GeoUtils::bearingDeg(o, {37.0, 128.0});
    QVERIFY(east > 89.0 && east < 91.0);   // 같은 위도 동쪽 ≈ 90° (대권이라 약간 북쪽)
    const double west = GeoUtils::bearingDeg(o, {37.0, 126.0});
    QVERIFY(west > 269.0 && west < 271.0);
}

void TstGeoUtils::destination_roundTrip()
{
    const GeoPoint start{37.5, 127.5};
    for (double brg : {0.0, 45.0, 90.0, 135.0, 210.0, 333.0}) {
        for (double dist : {1.0, 50.0, 300.0}) {
            const GeoPoint dst = GeoUtils::destination(start, brg, dist);
            QVERIFY(qAbs(GeoUtils::distanceKm(start, dst) - dist) < 1e-6);
            QVERIFY(qAbs(GeoUtils::angleDiffDeg(GeoUtils::bearingDeg(start, dst), brg)) < 1e-6);
        }
    }
}

void TstGeoUtils::interpolate_midpoint()
{
    const GeoPoint a{36.0, 126.0}, b{38.0, 129.0};
    const GeoPoint m = GeoUtils::interpolate(a, b, 0.5);
    QVERIFY(qAbs(GeoUtils::distanceKm(a, m) - GeoUtils::distanceKm(m, b)) < 1e-6);
    const GeoPoint s = GeoUtils::interpolate(a, b, 0.0);
    QVERIFY(GeoUtils::distanceKm(a, s) < 1e-9);
    const GeoPoint same = GeoUtils::interpolate(a, a, 0.7);
    QCOMPARE(same.lat, a.lat);
}

void TstGeoUtils::normalize_and_diff()
{
    QCOMPARE(GeoUtils::normalizeDeg(-90.0), 270.0);
    QCOMPARE(GeoUtils::normalizeDeg(720.0), 0.0);
    QCOMPARE(GeoUtils::normalizeDeg(359.5), 359.5);
    QCOMPARE(GeoUtils::angleDiffDeg(10.0, 350.0), 20.0);
    QCOMPARE(GeoUtils::angleDiffDeg(350.0, 10.0), -20.0);
    QCOMPARE(GeoUtils::angleDiffDeg(180.0, 0.0), 180.0);
}

QTEST_GUILESS_MAIN(TstGeoUtils)
#include "tst_GeoUtils.moc"
