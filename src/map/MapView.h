#pragma once

#include "common/Types.h"

#include <QGraphicsView>

class MapScene;

// 지도 뷰
//  - 휠: 커서 기준 확대/축소 (배율 범위 제한)
//  - 우클릭/중클릭 드래그: 이동 (움직이지 않은 우클릭은 mapClicked 로 전달 → 편집 완료 등에 사용)
//  - Home: 한반도 중심, +/-: 확대/축소
//  - 커서 위경도 / 배율 변경 시그널, 위경도 라벨·축척 막대 전시
class MapView : public QGraphicsView {
    Q_OBJECT
public:
    static constexpr double MaxPxPerKm = 40.0;

    explicit MapView(MapScene* scene, QWidget* parent = nullptr);

    double pxPerKm() const;

    // 편집 모드: 왼쪽 클릭이 아이템(핸들)으로 전달되지 않고 mapClicked 로만 전달, 십자 커서
    void setEditMode(bool on);
    bool isEditMode() const { return m_editMode; }
    void zoomBy(double factor);
    void zoomBy(double factor, const QPoint& viewportAnchor);

public slots:
    void centerOnKorea();

signals:
    void cursorGeoChanged(const GeoPoint& pos);
    void cursorLeft();
    void mapClicked(const GeoPoint& pos, Qt::MouseButton button, Qt::KeyboardModifiers modifiers);
    void mapDoubleClicked(const GeoPoint& pos);
    void editKeyPressed(int key);   // Enter/Return/Escape/Backspace (편집 컨트롤러용)
    void scaleChanged(double pxPerKm);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    void drawForeground(QPainter* painter, const QRectF& rect) override;

private:
    double minPxPerKm() const;
    void clampZoom();
    void drawGraticuleLabels(QPainter* painter);
    void drawScaleBar(QPainter* painter);

    MapScene* m_mapScene;
    bool m_initialFit = false;
    bool m_editMode = false;
    bool m_panning = false;
    bool m_panMoved = false;
    Qt::MouseButton m_panButton = Qt::NoButton;
    QPoint m_panStart;
    QPoint m_panLast;
};
