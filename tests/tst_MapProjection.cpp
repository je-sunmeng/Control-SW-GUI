#include "common/GeoUtils.h"
#include "map/MapProjection.h"

#include <QtTest>

class TstMapProjection : public QObject {
    Q_OBJECT
private slots:
    void origin_isSceneZero();
    void roundTrip();
    void axes_direction();
    void scale_approxKm();
    void sceneRect_normalized();
};

void TstMapProjection::origin_isSceneZero()
{
    const MapProjection proj;
    const QPointF s = proj.toScene({37.5, 127.5});
    QVERIFY(qAbs(s.x()) < 1e-9 && qAbs(s.y()) < 1e-9);
}

void TstMapProjection::roundTrip()
{
    const MapProjection proj;
    for (double lat : {29.0, 33.3, 37.5, 42.0, 46.0})
        for (double lon : {116.0, 124.7, 127.5, 131.9, 137.0}) {
            const GeoPoint g = proj.toGeo(proj.toScene({lat, lon}));
            QVERIFY(qAbs(g.lat - lat) < 1e-9);
            QVERIFY(qAbs(g.lon - lon) < 1e-9);
        }
}

void TstMapProjection::axes_direction()
{
    const MapProjection proj;
    QVERIFY(proj.toScene({38.5, 127.5}).y() < 0);   // 북쪽 = 화면 위 (y 감소)
    QVERIFY(proj.toScene({37.5, 128.5}).x() > 0);   // 동쪽 = 화면 오른쪽
}

void TstMapProjection::scale_approxKm()
{
    // 기준위도 부근에서 scene 거리 ≈ 대권거리(km), 오차 1% 이내
    const MapProjection proj;
    const GeoPoint a{37.5, 127.5};
    for (const GeoPoint b : {GeoPoint{38.5, 127.5}, GeoPoint{37.5, 129.0}, GeoPoint{36.8, 126.6}}) {
        const QPointF d = proj.toScene(b) - proj.toScene(a);
        const double sceneKm = std::hypot(d.x(), d.y());
        const double km = GeoUtils::distanceKm(a, b);
        QVERIFY2(qAbs(sceneKm - km) / km < 0.01, qPrintable(QStringLiteral("%1 vs %2").arg(sceneKm).arg(km)));
    }
}

void TstMapProjection::sceneRect_normalized()
{
    const MapProjection proj;
    const QRectF r = proj.sceneRect(33.0, 124.0, 43.0, 131.0);
    QVERIFY(r.width() > 0 && r.height() > 0);
    QVERIFY(r.contains(proj.toScene({37.5, 127.5})));
    QVERIFY(qAbs(r.height() - 10 * MapProjection::KmPerDeg) < 1e-6);
}

QTEST_GUILESS_MAIN(TstMapProjection)
#include "tst_MapProjection.moc"
