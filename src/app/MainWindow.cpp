#include "MainWindow.h"

#include "AppContext.h"
#include "common/Types.h"
#include "log/LogTableModel.h"
#include "log/Logger.h"
#include "mission/MissionController.h"
#include "model/ScenarioModel.h"
#include "sim/SimulationEngine.h"
#include "map/MapEditController.h"
#include "map/MapLayerManager.h"
#include "map/MapScene.h"
#include "map/MapView.h"
#include "ui/ControlPanel.h"
#include "ui/LogPanel.h"
#include "ui/MapToolBar.h"
#include "ui/ObjectPanel.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDockWidget>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>

#include <cmath>

namespace {
const QString kGeometryKey = QStringLiteral("mainWindow/geometry");
const QString kStateKey = QStringLiteral("mainWindow/state");
constexpr int kLayoutVersion = 2;   // 도크 구성이 바뀌면 증가 → 이전 저장 레이아웃 무시
} // namespace

MainWindow::MainWindow(AppContext& ctx, QWidget* parent)
    : QMainWindow(parent)
    , m_ctx(ctx)
{
    setObjectName(QStringLiteral("MainWindow"));
    setWindowTitle(QStringLiteral("전자전기-레이다 시연 제어 SW  v%1").arg(QApplication::applicationVersion()));
    setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowNestedDocks);
    setCorner(Qt::BottomLeftCorner, Qt::BottomDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);

    setupCentral();
    setupToolBar();
    setupDocks();
    setupMenus();
    setupStatusBar();

    resize(1600, 960);
    applyDefaultLayout();
    m_defaultState = saveState(kLayoutVersion);
    restoreLayout();

    m_ctx.logger().info(QStringLiteral("메인 화면 초기화 완료"), QStringLiteral("UI"));
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent* event)
{
    saveLayout();
    m_ctx.logger().info(QStringLiteral("프로그램 종료"), QStringLiteral("App"));
    event->accept();
}

void MainWindow::setupMenus()
{
    QMenu* file = menuBar()->addMenu(QStringLiteral("파일(&F)"));
    QAction* quit = file->addAction(QStringLiteral("종료(&X)"), this, &QWidget::close);
    quit->setShortcut(QKeySequence::Quit);

    QMenu* view = menuBar()->addMenu(QStringLiteral("보기(&V)"));
    view->addAction(m_objectDock->toggleViewAction());
    view->addAction(m_controlDock->toggleViewAction());
    view->addAction(m_logDock->toggleViewAction());
    view->addSeparator();
    view->addAction(QStringLiteral("레이아웃 초기화"), this, [this] {
        restoreState(m_defaultState, kLayoutVersion);
        for (QDockWidget* d : {m_objectDock, m_controlDock, m_logDock})
            d->show();
        applyDefaultLayout();
        m_ctx.logger().info(QStringLiteral("레이아웃 초기화"), QStringLiteral("UI"));
    });

    QMenu* help = menuBar()->addMenu(QStringLiteral("도움말(&H)"));
    help->addAction(QStringLiteral("정보"), this, [this] {
        QMessageBox::about(this, QStringLiteral("정보"),
                           QStringLiteral("<b>전자전기-레이다 시연 제어 SW</b><br>버전 %1<br>Qt %2")
                               .arg(QApplication::applicationVersion(), QString::fromLatin1(qVersion())));
    });
}

void MainWindow::setupToolBar()
{
    m_mapToolBar = new MapToolBar(this);
    addToolBar(Qt::TopToolBarArea, m_mapToolBar);
    m_mapToolBar->setLayerManager(&m_ctx.mapLayers());
    connect(m_mapToolBar, &MapToolBar::centerOnKoreaRequested, m_mapView, &MapView::centerOnKorea);
}

void MainWindow::setupCentral()
{
    m_mapScene = new MapScene(m_ctx.mapLayers(), m_ctx.projection(), this);
    m_mapScene->setScenario(&m_ctx.scenario());
    m_mapView = new MapView(m_mapScene, this);
    setCentralWidget(m_mapView);
    m_edit = new MapEditController(m_ctx.scenario(), *m_mapView, *m_mapScene, this);
    connect(&m_ctx.engine(), &SimulationEngine::snapshotReady, m_mapScene, &MapScene::applySnapshot);
}

QDockWidget* MainWindow::makeDock(const QString& title, const QString& objectName, QWidget* content,
                                  Qt::DockWidgetAreas allowed)
{
    auto* dock = new QDockWidget(title, this);
    dock->setObjectName(objectName);
    dock->setAllowedAreas(allowed);
    dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable
                      | QDockWidget::DockWidgetClosable);
    dock->setWidget(content);
    return dock;
}

void MainWindow::setupDocks()
{
    m_objectPanel = new ObjectPanel(m_ctx.scenario(), *m_edit, m_ctx.logger());
    m_objectDock = makeDock(QStringLiteral("오브젝트"), QStringLiteral("ObjectDock"), m_objectPanel,
                            Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_controlDock = makeDock(QStringLiteral("제어"), QStringLiteral("ControlDock"),
                             new ControlPanel(m_ctx.mission(), m_ctx.engine(), m_ctx.scenario(), &m_ctx.network()),
                             Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_logDock = makeDock(QStringLiteral("로그"), QStringLiteral("LogDock"), new LogPanel(m_ctx.logModel()),
                         Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);

    addDockWidget(Qt::LeftDockWidgetArea, m_objectDock);
    addDockWidget(Qt::RightDockWidgetArea, m_controlDock);
    addDockWidget(Qt::BottomDockWidgetArea, m_logDock);

    // 목록 선택 ↔ 지도 하이라이트
    connect(m_objectPanel, &ObjectPanel::objectSelected, m_mapScene, &MapScene::setHighlightedObject);
    connect(m_mapScene, &MapScene::objectClicked, m_objectPanel, &ObjectPanel::selectObject);
}

void MainWindow::setupStatusBar()
{
    m_stateLabel = new QLabel(QStringLiteral("상태: %1").arg(toDisplayString(SimState::PreMission)), this);
    m_cursorLabel = new QLabel(QStringLiteral("위도 -  경도 -"), this);
    m_cursorLabel->setMinimumWidth(240);
    m_scaleLabel = new QLabel(this);
    m_scaleLabel->setMinimumWidth(110);
    statusBar()->addWidget(m_stateLabel);
    connect(&m_ctx.engine(), &SimulationEngine::stateChanged, this, [this](SimState st) {
        m_stateLabel->setText(QStringLiteral("상태: %1").arg(toDisplayString(st)));
    });
    statusBar()->addPermanentWidget(m_scaleLabel);
    statusBar()->addPermanentWidget(m_cursorLabel);

    connect(m_mapView, &MapView::cursorGeoChanged, this, [this](const GeoPoint& p) {
        m_cursorLabel->setText(QStringLiteral("위도 %1°%2  경도 %3°%4")
                                   .arg(std::abs(p.lat), 0, 'f', 4).arg(p.lat >= 0 ? 'N' : 'S')
                                   .arg(std::abs(p.lon), 0, 'f', 4).arg(p.lon >= 0 ? 'E' : 'W'));
    });
    connect(m_edit, &MapEditController::hintChanged, this, [this](const QString& hint) {
        hint.isEmpty() ? statusBar()->clearMessage() : statusBar()->showMessage(hint);
    });
    connect(m_mapView, &MapView::cursorLeft, this, [this] { m_cursorLabel->setText(QStringLiteral("위도 -  경도 -")); });
    connect(m_mapView, &MapView::scaleChanged, this, [this](double ppk) {
        m_scaleLabel->setText(QStringLiteral("배율 %1 px/km").arg(ppk, 0, 'f', ppk < 1 ? 2 : 1));
    });
}

void MainWindow::applyDefaultLayout()
{
    resizeDocks({m_objectDock, m_controlDock}, {380, 260}, Qt::Horizontal);
    resizeDocks({m_logDock}, {220}, Qt::Vertical);
}

void MainWindow::restoreLayout()
{
    QSettings s;
    const QByteArray geom = s.value(kGeometryKey).toByteArray();
    if (!geom.isEmpty())
        restoreGeometry(geom);
    const QByteArray state = s.value(kStateKey).toByteArray();
    if (!state.isEmpty())
        restoreState(state, kLayoutVersion);
}

void MainWindow::saveLayout() const
{
    QSettings s;
    s.setValue(kGeometryKey, saveGeometry());
    s.setValue(kStateKey, saveState(kLayoutVersion));
}
