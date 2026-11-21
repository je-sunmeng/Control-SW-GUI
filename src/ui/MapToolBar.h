#pragma once

#include <QToolBar>

class QMenu;

// 상단 지도 툴바: 레이어 설정(체크 메뉴), "한반도 중심" 버튼
// M1: 골격만 제공(레이어 메뉴는 M2에서 MapLayerManager와 연결)
class MapToolBar : public QToolBar {
    Q_OBJECT
public:
    explicit MapToolBar(QWidget* parent = nullptr);

    QMenu* layerMenu() const { return m_layerMenu; }

signals:
    void centerOnKoreaRequested();

private:
    QMenu* m_layerMenu;
};
