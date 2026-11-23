#include "model/ScenarioModel.h"
#include "ui/MainLobeLampWidget.h"
#include "ui/ObjectPanel.h"
#include "ui/editors/RouteSegmentTable.h"

#include <QDir>
#include <QSettings>
#include <QTableWidget>
#include <QtTest>

// 설정창 UI 동작: 동선 테이블 높이 유동, 기본 파일명
class TstUiEditors : public QObject {
    Q_OBJECT
private slots:
    void initTestCase()
    {
        QCoreApplication::setOrganizationName(QStringLiteral("CtrlSW-test"));
        QCoreApplication::setApplicationName(QStringLiteral("tst_UiEditors"));
    }
    void routeTable_heightFollowsRowCount();
    void defaultFileName_format();
    void lamps_followListOrder_andLatchInLinked();
};

void TstUiEditors::lamps_followListOrder_andLatchInLinked()
{
    ScenarioModel m;
    MainLobeLampWidget w(m);
    const ObjectId r1 = m.addRadar({37, 126});
    const ObjectId r2 = m.addRadar({38, 126});
    const ObjectId r3 = m.addRadar({39, 126});
    QCOMPARE(w.radarAt(0), r1);
    QCOMPARE(w.radarAt(1), r2);
    QCOMPARE(w.radarAt(2), r3);
    m.removeObject(r2);                 // 2번 삭제 → 3번이 두 번째 자리로
    QCOMPARE(w.radarAt(1), r3);
    QCOMPARE(w.radarAt(2), ObjectId(0));

    // 연동모드: 수신 시점부터 계속 켜짐, 미수신 레이다는 비활성(이름 어둡게)
    w.setLinked(true);
    QVERIFY(!w.isActive(r1) && !w.isLampOn(0));
    w.onMainLobe(r3);
    QVERIFY(w.isActive(r3));
    QVERIFY(w.isLampOn(1));
    QVERIFY(!w.isActive(r1));
    QVERIFY(!w.isLampOn(0));
    QTest::qWait(1200);                 // 펄스 감쇠 시간이 지나도 유지
    QVERIFY(w.isLampOn(1));
    w.resetMission();
    QVERIFY(!w.isLampOn(1) && !w.isActive(r3));

    // 단독모드: 깜빡임(래치 아님)이지만 이름은 활성 표시
    w.setLinked(false);
    w.onMainLobe(r1);
    QVERIFY(w.isActive(r1));
    QVERIFY(!w.isLampOn(0));
}

void TstUiEditors::routeTable_heightFollowsRowCount()
{
    RouteSegmentTable w;
    auto* table = w.findChild<QTableWidget*>();
    QVERIFY(table);
    Route r;
    r.addWaypoint({37, 125});
    r.addWaypoint({37, 126});
    w.setRoute(r);   // 1구간
    const int h1 = table->height();
    r.addWaypoint({38, 126});
    r.addWaypoint({38, 127});
    w.setRoute(r);   // 3구간
    const int h3 = table->height();
    QVERIFY2(h3 > h1, qPrintable(QStringLiteral("%1 -> %2").arg(h1).arg(h3)));
    for (int i = 0; i < 10; ++i)
        r.addWaypoint({38.0 + i * 0.1, 127});
    w.setRoute(r);   // 12구간 → 최대 6행 높이에서 멈춤
    const int hMax = table->height();
    QVERIFY(hMax > h3);
    r.addWaypoint({40, 128});
    w.setRoute(r);
    QCOMPARE(table->height(), hMax);
}

void TstUiEditors::defaultFileName_format()
{
    // 종류별 폴더: <기본 폴더>/<종류>
    QSettings().setValue(QStringLiteral("paths/defaultFolder"), QDir::tempPath() + QStringLiteral("/ctrlsw_test_folders"));
    const QString radarDir = ObjectPanel::kindFolder(QStringLiteral("레이다"));
    const QString scenDir = ObjectPanel::kindFolder(QStringLiteral("시나리오"));
    QVERIFY(radarDir.endsWith(QStringLiteral("/ctrlsw_test_folders/레이다")));
    QVERIFY(QDir(radarDir).exists() && QDir(scenDir).exists());
    QVERIFY(radarDir != scenDir);
    QDir(QDir::tempPath() + QStringLiteral("/ctrlsw_test_folders")).removeRecursively();
    QSettings().remove(QStringLiteral("paths"));

    const QDateTime t(QDate(2026, 11, 23), QTime(14, 5));
    QCOMPARE(ObjectPanel::defaultFileName(QStringLiteral("레이다"), QStringLiteral("radar.json"), t),
             QStringLiteral("레이다_20261123_1405.radar.json"));
    QCOMPARE(ObjectPanel::defaultFileName(QStringLiteral("시나리오"), QStringLiteral("scenario.json"), t),
             QStringLiteral("시나리오_20261123_1405.scenario.json"));
}

QTEST_MAIN(TstUiEditors)
#include "tst_UiEditors.moc"
