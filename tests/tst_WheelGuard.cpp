#include "ui/WheelGuard.h"

#include <QApplication>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QtTest>

// 스핀박스/콤보박스 위 휠: 값은 그대로, 바깥 스크롤 영역이 스크롤
class TstWheelGuard : public QObject {
    Q_OBJECT
private slots:
    void wheelScrollsInsteadOfChangingValue();
};

static void wheelOn(QWidget* w, int angle)
{
    const QPointF local = w->rect().center();
    QWheelEvent ev(local, w->mapToGlobal(local), QPoint(), QPoint(0, angle), Qt::NoButton, Qt::NoModifier,
                   Qt::NoScrollPhase, false);
    QApplication::sendEvent(w, &ev);
}

void TstWheelGuard::wheelScrollsInsteadOfChangingValue()
{
    qApp->installEventFilter(new WheelGuard(qApp));
    QScrollArea area;
    auto* content = new QWidget;
    auto* l = new QVBoxLayout(content);
    auto* spin = new QDoubleSpinBox;
    spin->setRange(0, 100);
    spin->setValue(50);
    auto* combo = new QComboBox;
    combo->addItems({"a", "b", "c"});
    l->addWidget(spin);
    l->addWidget(combo);
    l->addSpacing(3000);   // 스크롤 가능하도록 길게
    area.setWidget(content);
    area.resize(300, 200);
    area.show();
    QVERIFY(QTest::qWaitForWindowExposed(&area));

    QCOMPARE(area.verticalScrollBar()->value(), 0);
    wheelOn(spin, -120);                    // 아래로 한 칸
    QCOMPARE(spin->value(), 50.0);          // 값 변경 없음
    QVERIFY(area.verticalScrollBar()->value() > 0);   // 대신 스크롤
    const int v = area.verticalScrollBar()->value();
    wheelOn(combo, -120);
    QCOMPARE(combo->currentIndex(), 0);
    QVERIFY(area.verticalScrollBar()->value() > v);
}

QTEST_MAIN(TstWheelGuard)
#include "tst_WheelGuard.moc"
