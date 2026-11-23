#include "log/LogFilterProxyModel.h"
#include "log/LogTableModel.h"
#include "log/Logger.h"

#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QThread>
#include <QtTest>

class TstLogger : public QObject {
    Q_OBJECT
private slots:
    void formatOpcode_layout();
    void hexDump_basic();
    void logger_emitsEntries();
    void logger_writesDailyFile();
    void logger_threadSafe();
    void tableModel_ringBuffer();
    void tableModel_columns();
    void filterProxy_typeAndText();
    void qtWarning_isInfoNotError();
};

void TstLogger::qtWarning_isInfoNotError()
{
    Logger logger;
    logger.installQtMessageHandler();
    QSignalSpy spy(&logger, &Logger::entryAdded);
    qWarning("Unable to open monitor interface to \\\\.\\DISPLAY1: test");
    QCOMPARE(spy.count(), 1);
    const auto e = spy.first().first().value<LogEntry>();
    QCOMPARE(e.type, LogType::Info);
    QVERIFY(e.content.startsWith(QStringLiteral("Qt 경고:")));
    qCritical("critical test");
    QCOMPARE(spy.last().first().value<LogEntry>().type, LogType::Error);
    Logger::uninstallQtMessageHandler();
}

void TstLogger::formatOpcode_layout()
{
    QCOMPARE(formatOpcode(0x21920010u), QStringLiteral("0x2192 0010"));
    QCOMPARE(formatOpcode(0xB1210100u), QStringLiteral("0xB121 0100"));
    QCOMPARE(formatOpcode(0u), QStringLiteral("0x0000 0000"));
}

void TstLogger::hexDump_basic()
{
    const QByteArray data = QByteArray::fromHex("21920010000000044142430A");
    const QString d = hexDump(data);
    QVERIFY(d.startsWith(QStringLiteral("0000  21 92 00 10")));
    QVERIFY(d.contains(QStringLiteral("ABC.")));
    QVERIFY(hexDump(QByteArray(40, 'x'), 16).contains(QStringLiteral("24 bytes")));
}

void TstLogger::logger_emitsEntries()
{
    Logger logger;
    QSignalSpy spy(&logger, &Logger::entryAdded);
    logger.info(QStringLiteral("hello"), QStringLiteral("App"));
    logger.error(QStringLiteral("bad"));
    logger.tx(QStringLiteral("분석SW"), 0x21920001u, 12, QStringLiteral("RadarInfo"), QByteArray(20, '\0'));
    logger.rx(QStringLiteral("재밍기법발생기"), 0xB1210100u, 4, QStringLiteral("MainLobe"));
    QCOMPARE(spy.count(), 4);

    const auto e2 = spy.at(2).at(0).value<LogEntry>();
    QCOMPARE(e2.type, LogType::Tx);
    QCOMPARE(e2.opcode, 0x21920001u);
    QCOMPARE(e2.size, 12u);
    QVERIFY(e2.time.isValid());
    QCOMPARE(spy.at(1).at(0).value<LogEntry>().type, LogType::Error);
}

void TstLogger::logger_writesDailyFile()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    Logger logger;
    QVERIFY(logger.setFileLoggingEnabled(true, dir.path()));
    logger.info(QStringLiteral("파일 기록 테스트"), QStringLiteral("App"));
    logger.tx(QStringLiteral("분석SW"), 0x21920010u, 0, QStringLiteral("MissionStart"));

    const QString path = logger.currentLogFilePath();
    QVERIFY(path.endsWith(QDate::currentDate().toString(QStringLiteral("yyyyMMdd")) + QStringLiteral(".log")));
    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString text = QString::fromUtf8(f.readAll());
    QVERIFY(text.contains(QStringLiteral("[INFO ] <App> 파일 기록 테스트")));
    QVERIFY(text.contains(QStringLiteral("[TX   ] <분석SW> 0x2192 0010 size=0 MissionStart")));

    QVERIFY(logger.setFileLoggingEnabled(false));
    QVERIFY(logger.currentLogFilePath().isEmpty());
}

void TstLogger::logger_threadSafe()
{
    QTemporaryDir dir;
    Logger logger;
    QVERIFY(logger.setFileLoggingEnabled(true, dir.path()));
    LogTableModel model(100000);
    connect(&logger, &Logger::entryAdded, &model, &LogTableModel::append);   // 다른 스레드 → 큐잉

    constexpr int kThreads = 4, kPerThread = 250;
    QList<QThread*> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads << QThread::create([&logger, t] {
            for (int i = 0; i < kPerThread; ++i)
                logger.info(QStringLiteral("t%1-%2").arg(t).arg(i));
        });
        threads.last()->start();
    }
    for (QThread* th : threads) {
        QVERIFY(th->wait(10000));
        delete th;
    }
    QTRY_COMPARE(model.rowCount(), kThreads * kPerThread);

    QFile f(logger.currentLogFilePath());
    QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
    int lines = 0;
    while (!f.atEnd()) {
        f.readLine();
        ++lines;
    }
    QCOMPARE(lines, kThreads * kPerThread);
}

void TstLogger::tableModel_ringBuffer()
{
    LogTableModel model(5);
    for (int i = 0; i < 12; ++i)
        model.append({QDateTime::currentDateTime(), LogType::Info, {}, 0, 0, QString::number(i), {}});
    QCOMPARE(model.rowCount(), 5);
    QCOMPARE(model.entryAt(0).content, QStringLiteral("7"));
    QCOMPARE(model.entryAt(4).content, QStringLiteral("11"));

    model.setMaxRows(2);
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.entryAt(0).content, QStringLiteral("10"));

    model.clear();
    QCOMPARE(model.rowCount(), 0);
}

void TstLogger::tableModel_columns()
{
    LogTableModel model;
    QCOMPARE(model.columnCount(), int(LogTableModel::ColumnCount));
    QCOMPARE(model.headerData(LogTableModel::ColOpcode, Qt::Horizontal).toString(), QStringLiteral("Opcode"));

    model.append({QDateTime::currentDateTime(), LogType::Info, QStringLiteral("App"), 0x1234u, 9, QStringLiteral("i"), {}});
    model.append({QDateTime::currentDateTime(), LogType::Rx, QStringLiteral("재밍기법발생기"), 0xB1210100u, 9,
                  QStringLiteral("r"), QByteArray::fromHex("B121010000000000")});

    // Info 행은 Opcode/크기를 표시하지 않음
    QVERIFY(model.data(model.index(0, LogTableModel::ColOpcode)).toString().isEmpty());
    QVERIFY(!model.data(model.index(0, LogTableModel::ColSize)).isValid());
    QCOMPARE(model.data(model.index(1, LogTableModel::ColOpcode)).toString(), QStringLiteral("0xB121 0100"));
    QCOMPARE(model.data(model.index(1, LogTableModel::ColSize)).toUInt(), 9u);
    QCOMPARE(model.data(model.index(1, LogTableModel::ColType)).toString(), QStringLiteral("수신"));
    QVERIFY(model.data(model.index(1, LogTableModel::ColContent), Qt::ToolTipRole).toString().contains(QStringLiteral("B1 21 01 00")));
    QCOMPARE(model.data(model.index(1, 0), LogTableModel::LogTypeRole).toInt(), int(LogType::Rx));
}

void TstLogger::filterProxy_typeAndText()
{
    LogTableModel model;
    const auto now = QDateTime::currentDateTime();
    model.append({now, LogType::Info, QStringLiteral("App"), 0, 0, QStringLiteral("시작"), {}});
    model.append({now, LogType::Error, QStringLiteral("Net"), 0, 0, QStringLiteral("접속 실패"), {}});
    model.append({now, LogType::Tx, QStringLiteral("분석SW"), 0x21920001u, 4, QStringLiteral("RadarInfo"), {}});
    model.append({now, LogType::Rx, QStringLiteral("재밍기법발생기"), 0xB1210100u, 4, QStringLiteral("MainLobe"), {}});

    LogFilterProxyModel proxy;
    proxy.setSourceModel(&model);
    QCOMPARE(proxy.rowCount(), 4);

    proxy.setTypeVisible(LogType::Info, false);
    proxy.setTypeVisible(LogType::Error, false);
    QCOMPARE(proxy.rowCount(), 2);
    QVERIFY(!proxy.isTypeVisible(LogType::Info));

    proxy.setTextFilter(QStringLiteral("radar"));   // 대소문자 무시
    QCOMPARE(proxy.rowCount(), 1);
    proxy.setTextFilter(QStringLiteral("B121"));    // Opcode 검색
    QCOMPARE(proxy.rowCount(), 1);
    proxy.setTextFilter(QString());
    proxy.setTypeVisible(LogType::Error, true);
    QCOMPARE(proxy.rowCount(), 3);

    model.append({now, LogType::Error, QStringLiteral("Net"), 0, 0, QStringLiteral("재접속"), {}});
    QCOMPARE(proxy.rowCount(), 4);
}

QTEST_GUILESS_MAIN(TstLogger)
#include "tst_Logger.moc"
