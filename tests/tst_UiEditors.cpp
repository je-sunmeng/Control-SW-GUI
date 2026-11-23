#include "ui/ObjectPanel.h"
#include "ui/editors/RouteSegmentTable.h"

#include <QTableWidget>
#include <QtTest>

// 설정창 UI 동작: 동선 테이블 높이 유동, 기본 파일명
class TstUiEditors : public QObject {
    Q_OBJECT
private slots:
    void routeTable_heightFollowsRowCount();
    void defaultFileName_format();
};

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
    const QDateTime t(QDate(2026, 11, 23), QTime(14, 5));
    QCOMPARE(ObjectPanel::defaultFileName(QStringLiteral("레이다"), QStringLiteral("radar.json"), t),
             QStringLiteral("레이다_20261123_1405.radar.json"));
    QCOMPARE(ObjectPanel::defaultFileName(QStringLiteral("시나리오"), QStringLiteral("scenario.json"), t),
             QStringLiteral("시나리오_20261123_1405.scenario.json"));
}

QTEST_MAIN(TstUiEditors)
#include "tst_UiEditors.moc"
