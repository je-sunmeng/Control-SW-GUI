#pragma once

#include "MapObjectItem.h"

#include <QColor>
#include <QPainterPath>
#include <QVector>

#include <functional>

class WaypointHandleItem;
struct Route;

// 동선: 대권 보간 폴리라인 + 웨이포인트 핸들(첫 점 'S') + 구간 라벨(고도/속력)
// 전자전기 / 아군편대 / 이동 허위표적 공용
class RouteItem : public MapObjectItem {
public:
    using WaypointMoved = std::function<void(int index, const QPointF& scenePos, bool finished)>;
    using Pressed = std::function<void()>;

    RouteItem(ObjectId id, const MapProjection& proj, const QColor& color, Qt::PenStyle style = Qt::SolidLine);

    void setCallbacks(WaypointMoved onMove, Pressed onPress);

    void sync(const ScenarioModel& model) override;
    void setHighlighted(bool on) override;
    void setEditable(bool on) override;

    int handleCount() const { return int(m_handles.size()); }
    WaypointHandleItem* handle(int i) const { return m_handles.value(i); }

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    static constexpr double DensifyStepKm = 20.0;
    static constexpr double SegmentLabelMinLod = 0.8;   // px/km

private:
    void rebuildHandles(int count);

    QColor m_color;
    Qt::PenStyle m_penStyle;
    QString m_name;
    QPainterPath m_path;
    struct SegLabel {
        QPointF scenePos;
        QString text;
    };
    QVector<SegLabel> m_labels;
    QVector<WaypointHandleItem*> m_handles;
    bool m_highlighted = false;
    bool m_editable = true;
    WaypointMoved m_onMove;
    Pressed m_onPress;
};
