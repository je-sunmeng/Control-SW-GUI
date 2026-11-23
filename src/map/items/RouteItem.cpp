#include "RouteItem.h"

#include "WaypointHandleItem.h"
#include "common/GeoUtils.h"
#include "map/MapProjection.h"
#include "model/ScenarioModel.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include <cmath>
#include <algorithm>
#include <iterator>

namespace {
constexpr qreal kLabelMarginKm = 80.0;
}

RouteItem::RouteItem(ObjectId id, const MapProjection& proj, const QColor& color, Qt::PenStyle style)
    : MapObjectItem(id, proj)
    , m_color(color)
    , m_penStyle(style)
{
}

void RouteItem::setCallbacks(WaypointMoved onMove, Pressed onPress)
{
    m_onMove = std::move(onMove);
    m_onPress = std::move(onPress);
    rebuildHandles(int(m_handles.size()));
}

void RouteItem::rebuildHandles(int count)
{
    qDeleteAll(m_handles);
    m_handles.clear();
    for (int i = 0; i < count; ++i) {
        auto* h = new WaypointHandleItem(i == 0 ? WaypointHandleItem::Shape::Circle : WaypointHandleItem::Shape::Square,
                                         m_color, i == 0 ? 11 : 8, this);
        h->setEditable(m_editable);
        h->setHighlighted(m_highlighted);
        h->setMoveCallback([this, i](const QPointF& p, bool finished) {
            if (m_onMove)
                m_onMove(i, p, finished);
        });
        h->setPressCallback([this] {
            if (m_onPress)
                m_onPress();
        });
        m_handles.append(h);
    }
}

void RouteItem::sync(const ScenarioModel& model)
{
    const Route* route = model.route(m_id);
    const SimObject* obj = model.find(m_id);
    if (!route || !obj)
        return;
    m_name = obj->name;

    if (m_handles.size() != route->waypoints.size()) {
        // 드래그 중인 핸들이 있으면 재생성하지 않음 (개수는 드래그로 바뀌지 않음)
        rebuildHandles(int(route->waypoints.size()));
    }
    for (int i = 0; i < route->waypoints.size(); ++i) {
        m_handles[i]->syncPos(m_proj.toScene(route->waypoints[i]));
        m_handles[i]->setLabel(i == 0 ? QStringLiteral("S %1").arg(m_name) : QString());
    }

    // 대권 보간 폴리라인 (RouteKinematics 와 동일한 경로)
    QPainterPath p;
    m_labels.clear();
    // 루프 동선은 마지막 점 → 시작점 구간 포함
    const int segs = route->expectedSegmentCount();
    for (int i = 0; i < segs; ++i) {
        const GeoPoint a = route->waypoints[i], b = route->waypoints[route->endIndex(i)];
        const double len = GeoUtils::distanceKm(a, b);
        const int n = std::max(1, int(std::ceil(len / DensifyStepKm)));
        if (i == 0)
            p.moveTo(m_proj.toScene(a));
        for (int k = 1; k <= n; ++k)
            p.lineTo(m_proj.toScene(GeoUtils::interpolate(a, b, double(k) / n)));
        if (i < route->segments.size()) {
            const RouteSegment& s = route->segments[i];
            m_labels.append({m_proj.toScene(GeoUtils::interpolate(a, b, 0.5)),
                             QStringLiteral("%1 km · %2 m/s").arg(s.altitudeKm, 0, 'f', 1).arg(s.speedMps, 0, 'f', 0)});
        }
    }
    prepareGeometryChange();
    m_path = p;
}

void RouteItem::setHighlighted(bool on)
{
    m_highlighted = on;
    for (WaypointHandleItem* h : std::as_const(m_handles))
        h->setHighlighted(on);
    update();
}

void RouteItem::setEditable(bool on)
{
    m_editable = on;
    for (WaypointHandleItem* h : std::as_const(m_handles))
        h->setEditable(on);
}

QRectF RouteItem::boundingRect() const
{
    if (m_path.isEmpty())
        return {};
    return m_path.boundingRect().adjusted(-kLabelMarginKm, -kLabelMarginKm, kLabelMarginKm, kLabelMarginKm);
}

void RouteItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    if (m_path.isEmpty())
        return;
    painter->setRenderHint(QPainter::Antialiasing, true);
    QColor c = m_color;
    c.setAlpha(m_highlighted ? 255 : 200);
    QPen pen(c, m_highlighted ? 3.0 : 2.0, m_penStyle, Qt::RoundCap, Qt::RoundJoin);
    pen.setCosmetic(true);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(m_path);

    // 구간 라벨: 확대 시 또는 선택 시 표시 (화면 좌표 고정 크기)
    const double lod = QStyleOptionGraphicsItem::levelOfDetailFromTransform(painter->worldTransform());
    if (m_labels.isEmpty() || (lod < SegmentLabelMinLod && !m_highlighted))
        return;
    const QTransform xf = painter->worldTransform();
    painter->save();
    painter->resetTransform();
    QFont f = painter->font();
    f.setPointSize(8);
    painter->setFont(f);
    const QFontMetricsF fm(f);
    for (const SegLabel& l : std::as_const(m_labels)) {
        const QPointF d = xf.map(l.scenePos);
        QRectF box(0, 0, fm.horizontalAdvance(l.text) + 6, fm.height() + 2);
        box.moveCenter(d + QPointF(0, -fm.height()));
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0x0b, 0x14, 0x20, 180));
        painter->drawRoundedRect(box, 2, 2);
        painter->setPen(m_color.lighter(130));
        painter->drawText(box, Qt::AlignCenter, l.text);
    }
    painter->restore();
}
