#include "map/MapLayerManager.h"
#include "map/MapProjection.h"
#include "map/VectorLayer.h"

#include <QSignalSpy>
#include <QtTest>

class TstVectorLayer : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void parse_allGeometryTypes();
    void parse_invalid();
    void coarsePath_hasFewerVertices();
    void pathForLod_switches();
    void defaultLayers_loadFromResources();
    void manager_visibility();
};

void TstVectorLayer::initTestCase()
{
    QCoreApplication::setOrganizationName(QStringLiteral("CtrlSW-test"));
    QCoreApplication::setApplicationName(QStringLiteral("tst_VectorLayer"));
}

void TstVectorLayer::parse_allGeometryTypes()
{
    const QByteArray json = R"({
      "type": "FeatureCollection",
      "features": [
        {"type":"Feature","properties":{"name":"설악산","elevation":1708},
         "geometry":{"type":"Point","coordinates":[128.4653,38.1194]}},
        {"type":"Feature","properties":{},
         "geometry":{"type":"LineString","coordinates":[[127,37],[128,37],[128,38]]}},
        {"type":"Feature","properties":{},
         "geometry":{"type":"MultiLineString","coordinates":[[[126,36],[126.5,36.5]],[[129,35],[129.5,35.5]]]}},
        {"type":"Feature","properties":{},
         "geometry":{"type":"Polygon","coordinates":[[[125,34],[126,34],[126,35],[125,35],[125,34]]]}},
        {"type":"Feature","properties":{},
         "geometry":{"type":"MultiPolygon","coordinates":[[[[130,40],[131,40],[131,41],[130,40]]]]}},
        {"type":"Feature","properties":{},
         "geometry":{"type":"GeometryCollection","geometries":[{"type":"Point","coordinates":[127.5,37.5]}]}}
      ]})";
    const MapProjection proj;
    VectorLayer layer(QStringLiteral("t"), QStringLiteral("테스트"));
    QString err;
    QVERIFY2(layer.loadGeoJson(json, proj, &err), qPrintable(err));
    QCOMPARE(layer.featureCount(), 6);
    QCOMPARE(layer.points().size(), 2);
    QCOMPARE(layer.points().at(0).label, QStringLiteral("설악산 1708"));
    QCOMPARE(layer.vertexCount(), 1 + 3 + 4 + 5 + 4 + 1);
    QVERIFY(layer.hasArea());
    QVERIFY(layer.boundingRect().contains(proj.toScene({38.0, 128.0})));
    QVERIFY(layer.boundingRect().contains(proj.toScene({38.1194, 128.4653})));
    QVERIFY(!layer.path().isEmpty());
}

void TstVectorLayer::parse_invalid()
{
    const MapProjection proj;
    VectorLayer layer(QStringLiteral("t"), QStringLiteral("t"));
    QString err;
    QVERIFY(!layer.loadGeoJson("{ not json", proj, &err));
    QVERIFY(!err.isEmpty());
    QVERIFY(!layer.loadGeoJson(R"({"type":"Feature","geometry":{"type":"Circle","coordinates":[1,2]}})", proj, &err));
    QVERIFY(!layer.loadFile(QStringLiteral(":/map/does_not_exist.geojson"), proj, &err));
}

void TstVectorLayer::coarsePath_hasFewerVertices()
{
    // 0.001° 간격 지그재그(±0.0005°) 선: 단순화 허용오차(1.2 km)보다 작은 굴곡 → 대폭 감소
    QByteArray json = R"({"type":"LineString","coordinates":[)";
    for (int i = 0; i <= 2000; ++i) {
        if (i)
            json += ',';
        json += '[' + QByteArray::number(126.0 + i * 0.001, 'f', 4) + ',' + QByteArray::number(37.0 + (i % 2) * 0.0005, 'f', 4) + ']';
    }
    json += "]}";
    const MapProjection proj;
    VectorLayer layer(QStringLiteral("z"), QStringLiteral("z"));
    QVERIFY(layer.loadGeoJson(json, proj));
    QCOMPARE(layer.vertexCount(), 2001);
    QVERIFY2(layer.coarseVertexCount() < 10, qPrintable(QString::number(layer.coarseVertexCount())));
}

void TstVectorLayer::pathForLod_switches()
{
    const MapProjection proj;
    VectorLayer layer(QStringLiteral("l"), QStringLiteral("l"));
    QVERIFY(layer.loadGeoJson(R"({"type":"LineString","coordinates":[[127,37],[127.001,37.0004],[128,37]]})", proj));
    QCOMPARE(&layer.pathForLod(0.1), &layer.coarsePath());
    QCOMPARE(&layer.pathForLod(5.0), &layer.path());
}

void TstVectorLayer::defaultLayers_loadFromResources()
{
    const MapProjection proj;
    MapLayerManager mgr;
    const int loaded = mgr.loadDefaultLayers(proj);
    QCOMPARE(loaded, 10);
    for (const char* id : {"countries", "kr_land", "coastline_world", "kr_coastline", "borders", "kr_admin1",
                           "rivers", "kr_mountains", "kr_mdl", "kr_dmz"}) {
        VectorLayer* l = mgr.layer(QString::fromLatin1(id));
        QVERIFY2(l, id);
        QVERIFY2(l->vertexCount() > 0, id);
    }
    QVERIFY(mgr.contains(MapLayerManager::GraticuleId));
    QVERIFY(!mgr.layer(MapLayerManager::GraticuleId));

    // 군사분계선은 대략 N37.7~38.7, E126.1~128.4 범위
    const QRectF mdl = mgr.layer(QStringLiteral("kr_mdl"))->boundingRect();
    QVERIFY(mdl.intersects(proj.sceneRect(37.7, 126.0, 38.7, 128.5)));
    QVERIFY(proj.sceneRect(37.0, 125.5, 39.0, 129.0).contains(mdl));

    // 한반도 산: 직접 입력 목록 포함, 경계가 모든 포인트를 포함 (뷰 컬링 회귀 방지)
    const VectorLayer* peaks = mgr.layer(QStringLiteral("kr_mountains"));
    bool hasJiri = false;
    for (const auto& p : peaks->points()) {
        hasJiri |= p.label.startsWith(QStringLiteral("지리산"));
        QVERIFY2(peaks->boundingRect().adjusted(-1e-6, -1e-6, 1e-6, 1e-6).contains(p.scenePos), qPrintable(p.label));
    }
    QVERIFY(hasJiri);
}

void TstVectorLayer::manager_visibility()
{
    MapLayerManager mgr;
    mgr.addVirtualLayer(QStringLiteral("a"), QStringLiteral("A"), true);
    mgr.addVirtualLayer(QStringLiteral("a"), QStringLiteral("dup"), true);   // 중복 무시
    QCOMPARE(mgr.layerIds(), QStringList{QStringLiteral("a")});

    QSignalSpy spy(&mgr, &MapLayerManager::layerVisibilityChanged);
    mgr.setVisible(QStringLiteral("a"), false);
    mgr.setVisible(QStringLiteral("a"), false);   // 변화 없으면 시그널 없음
    mgr.setVisible(QStringLiteral("zz"), true);   // 없는 레이어 무시
    QCOMPARE(spy.count(), 1);
    QVERIFY(!mgr.isVisible(QStringLiteral("a")));

    mgr.saveVisibility();
    MapLayerManager other;
    other.addVirtualLayer(QStringLiteral("a"), QStringLiteral("A"), true);
    other.restoreVisibility();
    QVERIFY(!other.isVisible(QStringLiteral("a")));
    QSettings().remove(QStringLiteral("map"));
}

QTEST_GUILESS_MAIN(TstVectorLayer)
#include "tst_VectorLayer.moc"
