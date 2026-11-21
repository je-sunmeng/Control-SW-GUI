#include "log/Logger.h"
#include "net/Opcode.h"
#include "net/ProtocolTable.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class TstProtocolTable : public QObject {
    Q_OBJECT
private slots:
    void defaultResource_complete();
    void lookups_bothDirections();
    void validation_errors();
    void overrideFile_takesPrecedence();
};

void TstProtocolTable::defaultResource_complete()
{
    ProtocolTable t;
    QStringList errors, warnings;
    QVERIFY(t.loadFile(ProtocolTable::defaultResourcePath(), &errors, &warnings));
    QVERIFY2(errors.isEmpty(), qPrintable(errors.join('\n')));
    QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join('\n')));   // 모든 메시지 매핑 존재
    QCOMPARE(t.version(), QStringLiteral("0.1-draft"));
    QCOMPARE(t.size(), 17);
}

void TstProtocolTable::lookups_bothDirections()
{
    ProtocolTable t;
    QVERIFY(t.loadFile(ProtocolTable::defaultResourcePath()));
    QCOMPARE(*t.opcodeFor(MessageId::MissionStart, DeviceCode::ControlSW, DeviceCode::SignalGen), 0x21B10010u);
    QCOMPARE(*t.opcodeFor(MessageId::RadarInfo, DeviceCode::ControlSW, DeviceCode::AnalysisSW), 0x21920001u);
    QVERIFY(!t.cmdFor(MessageId::JammerRoute, DeviceCode::ControlSW, DeviceCode::SignalGen));   // 분석SW 전용
    QCOMPARE(*t.idForOpcode(0xB1210100u), MessageId::MainLobeNotify);
    QVERIFY(!t.idForOpcode(0xB1210999u));
}

void TstProtocolTable::validation_errors()
{
    ProtocolTable t;
    QStringList errors, warnings;
    const QByteArray json = R"({
      "protocolVersion": "x",
      "devices": { "ControlSW": "0x21", "AnalysisSW": "0x92", "SignalGen": "0xB1" },
      "messages": [
        { "id": "RadarInfo", "src": "ControlSW", "dst": "AnalysisSW", "cmd": "0x0001" },
        { "id": "JammerRoute", "src": "ControlSW", "dst": "AnalysisSW", "cmd": "0x0001" },
        { "id": "Bogus", "src": "ControlSW", "dst": "AnalysisSW", "cmd": "0x0005" },
        { "id": "MissionStop", "src": "Nowhere", "dst": "AnalysisSW", "cmd": "0x0012" },
        { "id": "MissionStart", "src": "0x21", "dst": "0xB1", "cmd": "16" }
      ]})";
    QVERIFY(t.loadJson(json, &errors, &warnings));   // 유효 항목이 있으면 로드는 성공
    QCOMPARE(errors.size(), 3);                       // 중복, 알 수 없는 id, 장치 오류
    QVERIFY(errors.join('\n').contains(QStringLiteral("중복")));
    QVERIFY(warnings.size() >= 5);                    // 매핑 누락 경고
    QCOMPARE(t.size(), 2);
    QCOMPARE(*t.cmdFor(MessageId::MissionStart, DeviceCode::ControlSW, DeviceCode::SignalGen), quint16(16));

    ProtocolTable t2;
    QVERIFY(!t2.loadJson("{bad json", &errors));
    QVERIFY(!t2.loadJson(R"({"messages": []})", &errors));
}

void TstProtocolTable::overrideFile_takesPrecedence()
{
    QTemporaryDir dir;
    QFile f(dir.filePath(ProtocolTable::overrideFileName()));
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(R"({"protocolVersion":"site-1","messages":[
        {"id":"MainLobeNotify","src":"SignalGen","dst":"ControlSW","cmd":"0x0A00"}]})");
    f.close();

    Logger log;
    QSignalSpy spy(&log, &Logger::entryAdded);
    ProtocolTable t;
    QVERIFY(t.loadDefault(&log, dir.path()));
    QCOMPARE(t.version(), QStringLiteral("site-1"));
    QCOMPARE(*t.idForOpcode(0xB1210A00u), MessageId::MainLobeNotify);
    QVERIFY(spy.count() >= 2);   // 누락 경고 + 로드 결과

    ProtocolTable fallback;
    QVERIFY(fallback.loadDefault(nullptr, QDir::tempPath() + QStringLiteral("/__no_such_dir__")));
    QCOMPARE(fallback.version(), QStringLiteral("0.1-draft"));
}

QTEST_GUILESS_MAIN(TstProtocolTable)
#include "tst_ProtocolTable.moc"
