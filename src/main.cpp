#include "app/AppContext.h"
#include "app/MainWindow.h"
#include "log/Logger.h"
#include "map/MapView.h"

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
    parser.process(app);

    applyStyle(app);

    AppContext ctx;
    MainWindow window(ctx);
    window.show();

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
        QTimer::singleShot(800, &window, [&window, &ctx, path] {
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
