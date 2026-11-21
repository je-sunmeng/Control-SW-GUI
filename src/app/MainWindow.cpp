#include "MainWindow.h"

#include "AppContext.h"
#include "common/Types.h"
#include "log/LogTableModel.h"
#include "log/Logger.h"
#include "ui/ControlPanel.h"
#include "ui/LogPanel.h"
#include "ui/MapPlaceholder.h"
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

namespace {
const QString kGeometryKey = QStringLiteral("mainWindow/geometry");
const QString kStateKey = QStringLiteral("mainWindow/state");
constexpr int kLayoutVersion = 1;   // 도크 구성이 바뀌면 증가 → 이전 저장 레이아웃 무시
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

    setupToolBar();
    setupCentral();
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
    connect(m_mapToolBar, &MapToolBar::centerOnKoreaRequested, this, [this] {
        m_ctx.logger().info(QStringLiteral("한반도 중심 이동 요청 (지도는 M2에서 구현)"), QStringLiteral("Map"));
    });
}

void MainWindow::setupCentral()
{
    setCentralWidget(new MapPlaceholder(this));
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
    m_objectDock = makeDock(QStringLiteral("오브젝트"), QStringLiteral("ObjectDock"), new ObjectPanel,
                            Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_controlDock = makeDock(QStringLiteral("제어"), QStringLiteral("ControlDock"), new ControlPanel,
                             Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_logDock = makeDock(QStringLiteral("로그"), QStringLiteral("LogDock"), new LogPanel(m_ctx.logModel()),
                         Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);

    addDockWidget(Qt::LeftDockWidgetArea, m_objectDock);
    addDockWidget(Qt::RightDockWidgetArea, m_controlDock);
    addDockWidget(Qt::BottomDockWidgetArea, m_logDock);
}

void MainWindow::setupStatusBar()
{
    m_stateLabel = new QLabel(QStringLiteral("상태: %1").arg(toDisplayString(SimState::PreMission)), this);
    m_cursorLabel = new QLabel(QStringLiteral("위도 -  경도 -"), this);
    m_cursorLabel->setMinimumWidth(220);
    statusBar()->addWidget(m_stateLabel);
    statusBar()->addPermanentWidget(m_cursorLabel);
}

void MainWindow::applyDefaultLayout()
{
    resizeDocks({m_objectDock, m_controlDock}, {320, 260}, Qt::Horizontal);
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
