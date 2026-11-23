// 제어SW 연동 시험용 모의 클라이언트 (분석SW / 재밍기법발생기 에뮬레이터)
//
//   ctrlsw_mock [--host 127.0.0.1] [--port 50021] [--device both|analysis|siggen]
//               [--protocol protocol.json] [--no-detections]
//
// 제어SW 를 연동모드로 두고 실행하면 두 장치로 접속 → [임무전송] → [시작] 시
// 재밍기법발생기가 레이다별 스캔주기마다 MainLobeNotify(+탐지결과) 를 보냅니다.
#include "MockDevice.h"

#include "common/Constants.h"
#include "net/ProtocolTable.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QTextStream>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("ctrlsw_mock"));
    QCoreApplication::setApplicationVersion(QStringLiteral(CTRLSW_VERSION));

    QCommandLineParser p;
    p.setApplicationDescription(QStringLiteral("CtrlSW 모의 클라이언트 (분석SW 0x92 / 재밍기법발생기 0xB1)"));
    p.addHelpOption();
    p.addVersionOption();
    QCommandLineOption hostOpt(QStringLiteral("host"), QStringLiteral("제어SW 주소"), QStringLiteral("host"), QStringLiteral("127.0.0.1"));
    QCommandLineOption portOpt(QStringLiteral("port"), QStringLiteral("제어SW 포트"), QStringLiteral("port"),
                               QString::number(Defaults::ServerPort));
    QCommandLineOption devOpt(QStringLiteral("device"), QStringLiteral("both | analysis | siggen"), QStringLiteral("device"),
                              QStringLiteral("both"));
    QCommandLineOption protoOpt(QStringLiteral("protocol"), QStringLiteral("protocol.json 경로 (기본: 내장)"), QStringLiteral("file"));
    QCommandLineOption noDetOpt(QStringLiteral("no-detections"), QStringLiteral("MainLobeNotify 에 탐지결과를 넣지 않음"));
    p.addOptions({hostOpt, portOpt, devOpt, protoOpt, noDetOpt});
    p.process(app);

    QTextStream out(stdout);
    ProtocolTable table;
    QStringList errors, warnings;
    const QString protoPath = p.isSet(protoOpt) ? p.value(protoOpt) : ProtocolTable::defaultResourcePath();
    if (!table.loadFile(protoPath, &errors, &warnings)) {
        out << "protocol.json 로드 실패: " << errors.join(QStringLiteral("; ")) << Qt::endl;
        return 1;
    }
    out << "protocol " << table.version() << " (" << table.size() << " 매핑) — " << protoPath << Qt::endl;

    const QString dev = p.value(devOpt).toLower();
    QList<MockDevice*> devices;
    if (dev == QLatin1String("both") || dev == QLatin1String("analysis"))
        devices << new MockDevice(DeviceCode::AnalysisSW, table, &app);
    if (dev == QLatin1String("both") || dev == QLatin1String("siggen"))
        devices << new MockDevice(DeviceCode::SignalGen, table, &app);
    if (devices.isEmpty()) {
        out << "--device 는 both | analysis | siggen" << Qt::endl;
        return 1;
    }
    for (MockDevice* d : devices) {
        d->setDetectionsEnabled(!p.isSet(noDetOpt));
        QObject::connect(d, &MockDevice::logLine, &app, [&out](const QString& l) { out << l << Qt::endl; });
        d->connectTo(p.value(hostOpt), quint16(p.value(portOpt).toUInt()));
    }
    return app.exec();
}
