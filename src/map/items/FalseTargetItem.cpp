#include "FalseTargetItem.h"

#include "WaypointHandleItem.h"
#include "common/ColorPalette.h"
#include "map/MapProjection.h"
#include "model/ScenarioModel.h"

FalseTargetItem::FalseTargetItem(ObjectId id, const MapProjection& proj, FalseTargetType type)
    : MapObjectItem(id, proj)
    , m_handle(new WaypointHandleItem(type == FalseTargetType::Noise ? WaypointHandleItem::Shape::Cross
                                                                      : WaypointHandleItem::Shape::Triangle,
                                      ColorPalette::falseTarget(), type == FalseTargetType::Noise ? 14 : 12, this))
{
}

void FalseTargetItem::sync(const ScenarioModel& model)
{
    const StandaloneFalseTarget* t = model.falseTarget(m_id);
    if (!t)
        return;
    m_handle->setVisible(t->posSet);
    m_handle->setLabel(t->ftType == FalseTargetType::Noise
                           ? QStringLiteral("%1 잡음 %2°").arg(t->name).arg(t->detectAngleDeg, 0, 'g', 4)
                           : t->name);
    if (t->posSet)
        m_handle->syncPos(m_proj.toScene(t->pos));
}

void FalseTargetItem::setHighlighted(bool on)
{
    m_handle->setHighlighted(on);
}

void FalseTargetItem::setEditable(bool on)
{
    m_handle->setEditable(on);
}
