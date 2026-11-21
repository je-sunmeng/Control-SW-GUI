#pragma once

#include <QHash>
#include <QToolBar>

class QAction;
class QMenu;
class MapLayerManager;

// 상단 지도 툴바: 레이어 설정(체크 메뉴), "한반도 중심" 버튼
class MapToolBar : public QToolBar {
    Q_OBJECT
public:
    explicit MapToolBar(QWidget* parent = nullptr);

    // 레이어 체크 메뉴를 MapLayerManager 와 양방향 동기화
    void setLayerManager(MapLayerManager* layers);
    QMenu* layerMenu() const { return m_layerMenu; }

signals:
    void centerOnKoreaRequested();

private:
    void rebuildLayerMenu();

    QMenu* m_layerMenu;
    MapLayerManager* m_layers = nullptr;
    QHash<QString, QAction*> m_layerActions;
};
