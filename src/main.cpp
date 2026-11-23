#include "app/AppContext.h"
#include "app/MainWindow.h"
#include "log/Logger.h"
#include "map/MapView.h"
#include "mission/MissionController.h"
#include "model/ScenarioModel.h"
#include "persistence/ObjectFileIO.h"
#include "ui/WheelGuard.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QStyleFactory>
#include <QTimer>

namespace {
void applyStyle(QApplication& app)
{
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QFile qss(QStringLiteral(":/styles/dark.qss"));
    if (qss.open(QIODevice::ReadOnly | QIODevice::Text))
        app.setStyleSheet(QString::fromUtf8(qss.readAll()));
}
} // namespace

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("CtrlSW"));
    QApplication::setApplicationName(QStringLiteral("CtrlSW"));
    QApplication::setApplicationVersion(QStringLiteral(CTRLSW_VERSION));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("전자전기-레이다 시연 제어 SW"));
    parser.addHelpOption();
    parser.addVersionOption();
    // 개발/원격 점검용: 창을 띄운 뒤 화면을 PNG로 저장하고 종료
    QCommandLineOption screenshotOpt(QStringLiteral("screenshot"),
                                     QStringLiteral("메인 창을 <file>(PNG)로 캡처한 뒤 종료 (개발용)"),
                                     QStringLiteral("file"));
    parser.addOption(screenshotOpt);
    QCommandLineOption zoomOpt(QStringLiteral("map-zoom"),
                               QStringLiteral("시작 시 지도 배율을 <px/km>로 설정 (개발용, 한반도 중심 기준)"),
                               QStringLiteral("pxPerKm"));
    parser.addOption(zoomOpt);
    QCommandLineOption scenarioOpt(QStringLiteral("scenario"), QStringLiteral("시작 시 시나리오 파일(*.scenario.json) 불러오기"),
                                   QStringLiteral("file"));
    parser.addOption(scenarioOpt);
    QCommandLineOption startOpt(QStringLiteral("start"),
                                QStringLiteral("임무 자동 시작 (단독: 즉시 / 연동: 두 장치 연결 시 임무전송 후 시작) — 개발/시연용"));
    parser.addOption(startOpt);
    QCommandLineOption speedOpt(QStringLiteral("speed"), QStringLiteral("배속 1/2/4/8"), QStringLiteral("factor"));
    parser.addOption(speedOpt);
    QCommandLineOption shotDelayOpt(QStringLiteral("screenshot-delay"), QStringLiteral("스크린샷 지연(ms, 기본 800)"),
                                    QStringLiteral("ms"));
    parser.addOption(shotDelayOpt);
    QCommandLineOption portOpt(QStringLiteral("port"), QStringLiteral("연동모드 TCP 서버 포트 (기본 50021)"), QStringLiteral("port"));
    parser.addOption(portOpt);
    QCommandLineOption linkedOpt(QStringLiteral("linked"), QStringLiteral("시작 시 연동모드로 전환 (서버 대기)"));
    parser.addOption(linkedOpt);
    parser.process(app);

    applyStyle(app);
    app.installEventFilter(new WheelGuard(&app));   // 휠로 설정값 변경 금지 (스크롤로 전달)

    AppContext ctx;
    MainWindow window(ctx);
    window.show();

    if (parser.isSet(scenarioOpt)) {
        QString err;
        if (const auto data = ObjectFileIO::loadScenario(parser.value(scenarioOpt), &err)) {
            ctx.scenario().replaceAll(*data);
            ctx.logger().info(QStringLiteral("시나리오 불러오기: %1").arg(parser.value(scenarioOpt)), QStringLiteral("Scenario"));
        } else {
            ctx.logger().error(QStringLiteral("시나리오 불러오기 실패: %1").arg(err), QStringLiteral("Scenario"));
        }
    }

    if (parser.isSet(portOpt))
        ctx.mission().setServerPort(quint16(parser.value(portOpt).toUInt()));
    if (parser.isSet(linkedOpt))
        ctx.mission().setMode(RunMode::Linked);
    if (parser.isSet(speedOpt))
        ctx.mission().setSpeed(parser.value(speedOpt).toInt());
    if (parser.isSet(startOpt)) {
        if (ctx.mission().mode() == RunMode::Standalone) {
            QStringList errors;
            if (!ctx.mission().startOrResume(&errors))
                ctx.logger().error(QStringLiteral("자동 시작 실패: %1").arg(errors.join(QStringLiteral(" / "))), QStringLiteral("App"));
        } else {
            // 연동모드: 두 장치가 연결되면 임무전송 → 시작 (시연/시험용)
            auto* conn = new QMetaObject::Connection;
            *conn = QObject::connect(&ctx.mission(), &MissionController::actionsChanged, &ctx.mission(),
                                     [&ctx, conn](const MissionActions& a) {
                                         if (a.sendMission && !ctx.mission().missionSent()) {
                                             ctx.mission().sendMission();
                                         } else if (a.start && ctx.mission().missionSent()) {
                                             QObject::disconnect(*conn);
                                             delete conn;
                                             ctx.mission().startOrResume();
                                         }
                                     });
            ctx.logger().info(QStringLiteral("자동 시작 대기: 두 장치 연결 시 임무전송 후 시작"), QStringLiteral("App"));
        }
    }

    if (parser.isSet(zoomOpt)) {
        const double target = parser.value(zoomOpt).toDouble();
        QTimer::singleShot(100, &window, [&window, target] {
            MapView* v = window.mapView();
            if (target > 0 && v->pxPerKm() > 0)
                v->zoomBy(target / v->pxPerKm());
        });
    }

    if (parser.isSet(screenshotOpt)) {
        const QString path = parser.value(screenshotOpt);
        const int delay = parser.isSet(shotDelayOpt) ? parser.value(shotDelayOpt).toInt() : 800;
        QTimer::singleShot(delay, &window, [&window, &ctx, path] {
            const bool ok = window.grab().save(path, "PNG");
            if (ok)
                ctx.logger().info(QStringLiteral("스크린샷 저장: %1").arg(path), QStringLiteral("App"));
            else
                ctx.logger().error(QStringLiteral("스크린샷 저장 실패: %1").arg(path), QStringLiteral("App"));
            QApplication::exit(ok ? 0 : 1);
        });
    }

    return app.exec();
}
