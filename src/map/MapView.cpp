#include "MapView.h"

#include "GraticuleLayer.h"
#include "MapLayerManager.h"
#include "MapProjection.h"
#include "MapScene.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QWheelEvent>

#include <cmath>
#include <algorithm>
#include <iterator>

namespace {
// 한반도 중심 보기 영역 (제주~백두산, 서해~동해)
constexpr double kKoreaMinLat = 33.0, kKoreaMaxLat = 43.2, kKoreaMinLon = 124.0, kKoreaMaxLon = 131.5;
constexpr int kPanThresholdPx = 4;

const QColor kSea(0x0b, 0x14, 0x20);
const QColor kLabel(0x8a, 0x98, 0xab);
const QColor kLabelBg(0x0b, 0x14, 0x20, 200);

QString formatDeg(double v, bool isLat, double step)
{
    const int decimals = step >= 1.0 ? 0 : (step >= 0.5 ? 1 : 2);
    const QChar hemi = isLat ? (v >= 0 ? QLatin1Char('N') : QLatin1Char('S'))
                             : (v >= 0 ? QLatin1Char('E') : QLatin1Char('W'));
    return QStringLiteral("%1°%2").arg(std::abs(v), 0, 'f', decimals).arg(hemi);
}
} // namespace

MapView::MapView(MapScene* scene, QWidget* parent)
    : QGraphicsView(scene, parent)
    , m_mapScene(scene)
{
    setObjectName(QStringLiteral("MapView"));
    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);   // 전경 라벨/축척 막대가 뷰 좌표 고정이므로
    setOptimizationFlag(QGraphicsView::DontSavePainterState, true);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    // 로그창/패널 크기 변경으로 뷰 크기가 바뀌어도 지도가 움직이지 않도록 좌상단 위치 유지
    setResizeAnchor(QGraphicsView::NoAnchor);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setDragMode(QGraphicsView::NoDrag);
    setMouseTracking(true);
    setFrameShape(QFrame::NoFrame);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(400, 300);
}

double MapView::pxPerKm() const
{
    return transform().m11();
}

double MapView::minPxPerKm() const
{
    // 지도 데이터 영역이 화면을 빈틈없이 채우는 배율까지만 축소 (지도 바깥 빈 공간이 보이지 않게)
    const QRectF d = m_mapScene->dataRect();
    const QSize v = viewport()->size();
    if (d.isEmpty() || v.isEmpty())
        return 0.01;
    return std::max(v.width() / d.width(), v.height() / d.height());
}

void MapView::zoomBy(double factor)
{
    zoomBy(factor, viewport()->rect().center());
}

void MapView::zoomBy(double factor, const QPoint& viewportAnchor)
{
    const double cur = pxPerKm();
    const double target = std::clamp(cur * factor, minPxPerKm(), MaxPxPerKm);
    if (std::abs(target - cur) < 1e-12)
        return;
    const QPointF anchorScene = mapToScene(viewportAnchor);
    const ViewportAnchor old = transformationAnchor();
    setTransformationAnchor(QGraphicsView::NoAnchor);
    scale(target / cur, target / cur);
    // 앵커 지점이 같은 화면 위치에 남도록 보정
    const QPointF delta = mapToScene(viewportAnchor) - anchorScene;
    translate(delta.x(), delta.y());
    setTransformationAnchor(old);
    emit scaleChanged(pxPerKm());
}

void MapView::clampZoom()
{
    const double cur = pxPerKm();
    const double target = std::clamp(cur, minPxPerKm(), MaxPxPerKm);
    if (std::abs(target - cur) > 1e-12) {
        scale(target / cur, target / cur);
        emit scaleChanged(pxPerKm());
    }
}

void MapView::centerOnKorea()
{
    const QRectF r = m_mapScene->projection().sceneRect(kKoreaMinLat, kKoreaMinLon, kKoreaMaxLat, kKoreaMaxLon);
    fitInView(r, Qt::KeepAspectRatio);
    clampZoom();
    centerOn(r.center());
    emit scaleChanged(pxPerKm());
}

void MapView::wheelEvent(QWheelEvent* event)
{
    const QPoint delta = event->angleDelta();
    if (delta.y() == 0) {
        event->ignore();
        return;
    }
    // 120 = 마우스 휠 1칸 → 1.25배. 트랙패드의 작은 delta 도 비례 처리
    const double factor = std::pow(1.25, delta.y() / 120.0);
    zoomBy(factor, event->position().toPoint());
    event->accept();
}

void MapView::mousePressEvent(QMouseEvent* event)
{
    const QPoint pos = event->position().toPoint();
    if (event->button() == Qt::RightButton) {
        // 우클릭: 이동 없음, 즉시 클릭으로 전달 (편집 중 마지막 웨이포인트 삭제/취소 등)
        event->accept();
        emit mapClicked(m_mapScene->projection().toGeo(mapToScene(pos)), Qt::RightButton, event->modifiers());
        return;
    }
    if (event->button() == Qt::LeftButton && !m_editMode) {
        // 편집 모드가 아니면 핸들(레이다/웨이포인트/표적) 위에서는 핸들 드래그
        QGraphicsView::mousePressEvent(event);
        if (scene()->mouseGrabberItem()) {
            m_itemDrag = true;
            return;
        }
    }
    if (event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton) {
        // 빈 곳: 끌면 지도 이동, 움직이지 않고 떼면 클릭
        m_panning = true;
        m_panMoved = false;
        m_panButton = event->button();
        m_panStart = m_panLast = pos;
        event->accept();
        return;
    }
    QGraphicsView::mousePressEvent(event);
}

void MapView::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_editMode) {
        emit mapDoubleClicked(m_mapScene->projection().toGeo(mapToScene(event->position().toPoint())));
        event->accept();
        return;
    }
    QGraphicsView::mouseDoubleClickEvent(event);
}

void MapView::setEditMode(bool on)
{
    m_editMode = on;
    viewport()->setCursor(on ? Qt::CrossCursor : Qt::ArrowCursor);
}

void MapView::mouseMoveEvent(QMouseEvent* event)
{
    const QPoint pos = event->position().toPoint();
    if (m_panning) {
        if (!m_panMoved && (pos - m_panStart).manhattanLength() >= kPanThresholdPx) {
            m_panMoved = true;
            viewport()->setCursor(Qt::ClosedHandCursor);
        }
        if (m_panMoved) {
            const QPoint d = pos - m_panLast;
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() - d.x());
            verticalScrollBar()->setValue(verticalScrollBar()->value() - d.y());
        }
        m_panLast = pos;
    } else {
        QGraphicsView::mouseMoveEvent(event);
    }
    emit cursorGeoChanged(m_mapScene->projection().toGeo(mapToScene(pos)));
}

void MapView::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_panning && event->button() == m_panButton) {
        m_panning = false;
        viewport()->setCursor(m_editMode ? Qt::CrossCursor : Qt::ArrowCursor);
        if (!m_panMoved && m_panButton == Qt::LeftButton)
            emit mapClicked(m_mapScene->projection().toGeo(mapToScene(event->position().toPoint())),
                            Qt::LeftButton, event->modifiers());
        event->accept();
        return;
    }
    if (m_itemDrag && event->button() == Qt::LeftButton)
        m_itemDrag = false;
    QGraphicsView::mouseReleaseEvent(event);
}

void MapView::leaveEvent(QEvent* event)
{
    emit cursorLeft();
    QGraphicsView::leaveEvent(event);
}

void MapView::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Home:
        centerOnKorea();
        return;
    case Qt::Key_Plus:
    case Qt::Key_Equal:
        zoomBy(1.25);
        return;
    case Qt::Key_Minus:
        zoomBy(1 / 1.25);
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Escape:
    case Qt::Key_Backspace:
        emit editKeyPressed(event->key());
        event->accept();
        return;
    default:
        QGraphicsView::keyPressEvent(event);
    }
}

void MapView::showEvent(QShowEvent* event)
{
    QGraphicsView::showEvent(event);
    if (!m_initialFit) {
        m_initialFit = true;
        centerOnKorea();
    }
}

void MapView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    if (m_initialFit)
        clampZoom();
}

void MapView::drawBackground(QPainter* painter, const QRectF& rect)
{
    painter->fillRect(rect, kSea);
}

void MapView::drawForeground(QPainter* painter, const QRectF&)
{
    painter->save();
    painter->resetTransform();
    painter->setRenderHint(QPainter::Antialiasing, true);
    if (m_mapScene->layerManager().isVisible(MapLayerManager::GraticuleId))
        drawGraticuleLabels(painter);
    drawScaleBar(painter);
    painter->restore();
}

void MapView::drawGraticuleLabels(QPainter* painter)
{
    const MapProjection& proj = m_mapScene->projection();
    const double step = GraticuleLayer::intervalForLod(pxPerKm());
    const QRect vr = viewport()->rect();
    const GeoPoint tl = proj.toGeo(mapToScene(vr.topLeft()));
    const GeoPoint br = proj.toGeo(mapToScene(vr.bottomRight()));

    QFont f = painter->font();
    f.setPointSize(8);
    painter->setFont(f);
    const QFontMetrics fm(f);

    auto label = [&](const QString& text, QPointF at) {
        QRectF box = fm.boundingRect(text).adjusted(-3, -1, 3, 1);
        box.moveCenter(at);
        box.moveLeft(std::clamp(box.left(), 0.0, vr.width() - box.width()));
        box.moveTop(std::clamp(box.top(), 0.0, vr.height() - box.height()));
        painter->fillRect(box, kLabelBg);
        painter->setPen(kLabel);
        painter->drawText(box, Qt::AlignCenter, text);
    };

    const auto inData = [](double lat, double lon) {
        return lat >= MapScene::MinLat - 1e-9 && lat <= MapScene::MaxLat + 1e-9 && lon >= MapScene::MinLon - 1e-9
               && lon <= MapScene::MaxLon + 1e-9;
    };
    for (double lon = std::ceil(tl.lon / step) * step; lon <= br.lon + 1e-9; lon += step) {
        if (!inData(std::clamp(tl.lat, MapScene::MinLat, MapScene::MaxLat), lon))
            continue;
        const QPointF p = mapFromScene(proj.toScene({tl.lat, lon}));
        label(formatDeg(lon, false, step), {p.x(), 10.0});
    }
    for (double lat = std::ceil(br.lat / step) * step; lat <= tl.lat + 1e-9; lat += step) {
        if (!inData(lat, std::clamp(tl.lon, MapScene::MinLon, MapScene::MaxLon)))
            continue;
        const QPointF p = mapFromScene(proj.toScene({lat, tl.lon}));
        label(formatDeg(lat, true, step), {28.0, p.y()});
    }
}

void MapView::drawScaleBar(QPainter* painter)
{
    // 화면 폭의 약 1/6 에 맞는 1-2-5 단위 거리
    const double ppk = pxPerKm();
    if (ppk <= 0)
        return;
    const double targetKm = (viewport()->width() / 6.0) / ppk;
    const double mag = std::pow(10.0, std::floor(std::log10(targetKm)));
    double km = mag;
    for (double m : {1.0, 2.0, 5.0, 10.0})
        if (m * mag <= targetKm)
            km = m * mag;
    const double len = km * ppk;

    const QPointF o(16, viewport()->height() - 18);
    painter->setPen(QPen(kLabel, 2));
    painter->drawLine(o, o + QPointF(len, 0));
    painter->drawLine(o + QPointF(0, -5), o + QPointF(0, 0));
    painter->drawLine(o + QPointF(len, -5), o + QPointF(len, 0));
    QFont f = painter->font();
    f.setPointSize(8);
    painter->setFont(f);
    const QString text = km >= 1 ? QStringLiteral("%1 km").arg(km) : QStringLiteral("%1 m").arg(km * 1000);
    painter->drawText(QRectF(o.x(), o.y() - 18, len, 14), Qt::AlignCenter, text);
}
