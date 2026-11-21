#pragma once

#include <QMainWindow>

class AppContext;
class QDockWidget;
class QLabel;
class MapToolBar;
class ObjectPanel;
class ControlPanel;
class LogPanel;
class MapScene;
class MapView;
class MapEditController;

// 전체 레이아웃 조립
//  상단: MapToolBar / 중앙: 지도 / 좌: ObjectPanel / 우: ControlPanel / 하단: LogPanel
//  도크 크기·배치는 QSettings에 저장/복원
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(AppContext& ctx, QWidget* parent = nullptr);
    ~MainWindow() override;

    MapView* mapView() const { return m_mapView; }

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void setupMenus();
    void setupToolBar();
    void setupCentral();
    void setupDocks();
    void setupStatusBar();
    void applyDefaultLayout();
    void restoreLayout();
    void saveLayout() const;
    QDockWidget* makeDock(const QString& title, const QString& objectName, QWidget* content,
                          Qt::DockWidgetAreas allowed);

    AppContext& m_ctx;
    MapToolBar* m_mapToolBar = nullptr;
    MapScene* m_mapScene = nullptr;
    MapView* m_mapView = nullptr;
    MapEditController* m_edit = nullptr;
    ObjectPanel* m_objectPanel = nullptr;
    QDockWidget* m_objectDock = nullptr;
    QDockWidget* m_controlDock = nullptr;
    QDockWidget* m_logDock = nullptr;
    QLabel* m_cursorLabel = nullptr;
    QLabel* m_stateLabel = nullptr;
    QLabel* m_scaleLabel = nullptr;
    QByteArray m_defaultState;
};
