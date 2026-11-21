#include "common/GeoUtils.h"
#include "sim/LocalDetectionSource.h"

#include <QtTest>

class TstLocalDetectionSource : public QObject {
    Q_OBJECT
private slots:
    void sweptOver_cases();
    void noiseSector_cases();
    void detectsOncePerRevolution_data();
    void detectsOncePerRevolution();
    void outOfRange_notDetected();
    void noiseSector_blocksOnlyThatRadar();
    void unsynced_noDetection();
};

void TstLocalDetectionSource::sweptOver_cases()
{
    using L = LocalDetectionSource;
    QVERIFY(L::sweptOver(10, 20, 15, ScanDirection::Clockwise));
    QVERIFY(L::sweptOver(10, 20, 20, ScanDirection::Clockwise));    // cur 포함
    QVERIFY(!L::sweptOver(10, 20, 10, ScanDirection::Clockwise));   // prev 제외
    QVERIFY(L::sweptOver(355, 5, 0, ScanDirection::Clockwise));     // 북 통과
    QVERIFY(!L::sweptOver(355, 5, 180, ScanDirection::Clockwise));
    QVERIFY(L::sweptOver(5, 355, 0, ScanDirection::CounterClockwise));
    QVERIFY(!L::sweptOver(5, 355, 0, ScanDirection::Clockwise));    // CW 로는 350° 회전 → 최대값 제한 없으면 포함
    QVERIFY(!L::sweptOver(20, 20, 20, ScanDirection::Clockwise));   // 정지
}

void TstLocalDetectionSource::noiseSector_cases()
{
    NoiseSector s{1, 9, 350.0, 20.0};
    QVERIFY(LocalDetectionSource::inNoiseSector(355, s));
    QVERIFY(LocalDetectionSource::inNoiseSector(0, s));
    QVERIFY(!LocalDetectionSource::inNoiseSector(1, s));
    QVERIFY(LocalDetectionSource::inNoiseSector(340, s));
}

namespace {
Radar makeRadar(ObjectId id, GeoPoint pos, ScanDirection dir = ScanDirection::Clockwise)
{
    Radar r;
    r.id = id;
    r.pos = pos;
    r.maxRangeKm = 300;
    r.scanPeriodSec = 10;
    r.scanDir = dir;
    return r;
}

// 빔을 dt 간격으로 회전시키며 탐지 수 집계
int runRevolutions(LocalDetectionSource& src, const QVector<Radar>& radars, const QVector<DetectableTarget>& targets,
                   const QVector<NoiseSector>& noise, double dt, double seconds, ObjectId countRadar = 0)
{
    int count = 0;
    for (double t = 0; t <= seconds + 1e-9; t += dt) {
        SimContext ctx;
        ctx.radars = &radars;
        for (const Radar& r : radars) {
            const double sign = r.scanDir == ScanDirection::Clockwise ? 1 : -1;
            ctx.beams.append({r.id, GeoUtils::normalizeDeg(sign * 360.0 * t / r.scanPeriodSec), true});
        }
        ctx.targets = targets;
        ctx.noiseSectors = noise;
        for (const Detection& d : src.collect(ctx, t))
            if (countRadar == 0 || d.radarId == countRadar)
                ++count;
    }
    return count;
}
} // namespace

void TstLocalDetectionSource::detectsOncePerRevolution_data()
{
    QTest::addColumn<double>("dt");
    QTest::addColumn<int>("dir");
    QTest::newRow("1x tick") << 0.033 << int(ScanDirection::Clockwise);
    QTest::newRow("8x tick") << 0.264 << int(ScanDirection::Clockwise);   // 틱당 9.5° > 빔폭 2°
    QTest::newRow("ccw") << 0.1 << int(ScanDirection::CounterClockwise);
}

void TstLocalDetectionSource::detectsOncePerRevolution()
{
    QFETCH(double, dt);
    QFETCH(int, dir);
    const QVector<Radar> radars{makeRadar(1, {37.0, 127.0}, ScanDirection(dir))};
    const GeoPoint tgt = GeoUtils::destination({37.0, 127.0}, 123.0, 100.0);
    LocalDetectionSource src;
    const int n = runRevolutions(src, radars, {{QStringLiteral("T"), tgt}}, {}, dt, 50.0 - 1e-6);   // 5바퀴
    QCOMPARE(n, 5);
}

void TstLocalDetectionSource::outOfRange_notDetected()
{
    const QVector<Radar> radars{makeRadar(1, {37.0, 127.0})};
    const GeoPoint far = GeoUtils::destination({37.0, 127.0}, 45.0, 301.0);
    LocalDetectionSource src;
    QCOMPARE(runRevolutions(src, radars, {{QStringLiteral("T"), far}}, {}, 0.05, 20.0), 0);
}

void TstLocalDetectionSource::noiseSector_blocksOnlyThatRadar()
{
    const Radar r1 = makeRadar(1, {37.0, 127.0});
    const Radar r2 = makeRadar(2, {37.0, 128.0});
    const QVector<Radar> radars{r1, r2};
    const GeoPoint tgt{37.8, 127.5};
    const GeoPoint noisePos = GeoUtils::destination(r1.pos, GeoUtils::bearingDeg(r1.pos, tgt) + 2.0, 50.0);
    // 잡음표적 → 각 레이다 기준 섹터 (엔진과 동일한 생성 방식)
    const QVector<NoiseSector> sectors{{1, 9, GeoUtils::bearingDeg(r1.pos, noisePos), 10.0},
                                       {2, 9, GeoUtils::bearingDeg(r2.pos, noisePos), 10.0}};
    LocalDetectionSource src;
    const QVector<DetectableTarget> targets{{QStringLiteral("T"), tgt}};
    QCOMPARE(runRevolutions(src, radars, targets, sectors, 0.05, 30.0 - 1e-6, 1), 0);   // R1: 섹터 안
    LocalDetectionSource src2;
    QCOMPARE(runRevolutions(src2, radars, targets, sectors, 0.05, 30.0 - 1e-6, 2), 3);  // R2: 다른 방위
}

void TstLocalDetectionSource::unsynced_noDetection()
{
    const QVector<Radar> radars{makeRadar(1, {37.0, 127.0})};
    LocalDetectionSource src;
    for (double t = 0; t < 20; t += 0.1) {
        SimContext ctx;
        ctx.radars = &radars;
        ctx.beams = {{1, GeoUtils::normalizeDeg(36.0 * t), false}};
        ctx.targets = {{QStringLiteral("T"), {37.5, 127.0}}};
        QVERIFY(src.collect(ctx, t).isEmpty());
    }
}

QTEST_GUILESS_MAIN(TstLocalDetectionSource)
#include "tst_LocalDetectionSource.moc"
