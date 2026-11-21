#include "ObjectTreeModel.h"

#include "common/ColorPalette.h"
#include "model/ScenarioModel.h"
#include "ui/editors/EditorUtil.h"

#include <QPainter>
#include <QPixmap>

namespace {

QIcon swatch(const QColor& c, bool round = false)
{
    QPixmap pm(12, 12);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(c.darker(200));
    p.setBrush(c);
    round ? p.drawEllipse(1, 1, 10, 10) : p.drawRoundedRect(1, 1, 10, 10, 2, 2);
    return QIcon(pm);
}

int groupIndex(ObjectType t)
{
    return int(t);   // Radar, Jammer, Formation, StandaloneFalseTarget = 0..3
}

QString routeSummary(const Route& r)
{
    if (r.waypointCount() < 2)
        return QStringLiteral("동선 미완성");
    return QStringLiteral("WP %1 · %2 km").arg(r.waypointCount()).arg(r.totalLengthKm(), 0, 'f', 0);
}

} // namespace

ObjectTreeModel::ObjectTreeModel(ScenarioModel& model, QObject* parent)
    : QStandardItemModel(parent)
    , m_model(model)
{
    connect(&m_model, &ScenarioModel::objectAdded, this, &ObjectTreeModel::rebuild);
    connect(&m_model, &ScenarioModel::objectRemoved, this, &ObjectTreeModel::rebuild);
    connect(&m_model, &ScenarioModel::scenarioReset, this, &ObjectTreeModel::rebuild);
    connect(&m_model, &ScenarioModel::objectChanged, this, &ObjectTreeModel::refresh);
    rebuild();
}

void ObjectTreeModel::rebuild()
{
    clear();
    setHorizontalHeaderLabels({QStringLiteral("오브젝트")});
    const ObjectType types[] = {ObjectType::Radar, ObjectType::Jammer, ObjectType::Formation,
                                ObjectType::StandaloneFalseTarget};
    for (ObjectType t : types) {
        auto* g = new QStandardItem;
        g->setEditable(false);
        g->setSelectable(false);
        g->setData(int(t), GroupTypeRole);
        QFont f = g->font();
        f.setBold(true);
        g->setFont(f);
        appendRow(g);
        m_groups[groupIndex(t)] = g;
    }
    for (ObjectId id : m_model.objectIds()) {
        const auto type = m_model.objectType(id);
        auto* item = new QStandardItem;
        item->setEditable(false);
        item->setData(id, ObjectIdRole);
        m_groups[groupIndex(*type)]->appendRow(item);
        refresh(id);
    }
    updateGroupTitles();
    emit rebuilt();
}

void ObjectTreeModel::updateGroupTitles()
{
    m_groups[0]->setText(QStringLiteral("레이다 (%1/%2)").arg(m_model.radars().size()).arg(Limits::MaxRadars));
    m_groups[1]->setText(QStringLiteral("전자전기 (%1/1)").arg(m_model.jammer() ? 1 : 0));
    m_groups[2]->setText(QStringLiteral("아군편대 (%1/%2)").arg(m_model.formation() ? 1 : 0).arg(Limits::MaxFormations));
    m_groups[3]->setText(QStringLiteral("단독허위표적 (%1)").arg(m_model.falseTargets().size()));
}

QModelIndex ObjectTreeModel::indexOf(ObjectId id) const
{
    if (id == 0)
        return {};
    for (QStandardItem* g : m_groups) {
        if (!g)
            continue;
        for (int r = 0; r < g->rowCount(); ++r)
            if (g->child(r)->data(ObjectIdRole).toUInt() == id)
                return g->child(r)->index();
    }
    return {};
}

ObjectId ObjectTreeModel::objectIdOf(const QModelIndex& index)
{
    return index.isValid() ? index.data(ObjectIdRole).toUInt() : 0;
}

void ObjectTreeModel::refresh(ObjectId id)
{
    const QModelIndex idx = indexOf(id);
    if (!idx.isValid())
        return;
    QStandardItem* item = itemFromIndex(idx);
    item->setText(itemText(id));
    item->setToolTip(itemToolTip(id));
    if (const Radar* r = m_model.radar(id))
        item->setIcon(swatch(ColorPalette::radar(r->colorIndex)));
    else if (m_model.jammer() && m_model.jammer()->id == id)
        item->setIcon(swatch(ColorPalette::jammer(), true));
    else if (m_model.formation() && m_model.formation()->id == id)
        item->setIcon(swatch(ColorPalette::formation(), true));
    else
        item->setIcon(swatch(ColorPalette::falseTarget(), true));
}

QString ObjectTreeModel::itemText(ObjectId id) const
{
    if (const Radar* r = m_model.radar(id))
        return QStringLiteral("%1   %2 km").arg(r->name).arg(r->maxRangeKm, 0, 'f', 0);
    if (m_model.jammer() && m_model.jammer()->id == id)
        return QStringLiteral("%1   %2").arg(m_model.jammer()->name, routeSummary(m_model.jammer()->route));
    if (const auto& f = m_model.formation(); f && f->id == id)
        return QStringLiteral("%1   %2기 · 허위타겟 %3 · %4")
            .arg(f->name).arg(f->formationSize).arg(f->jamming.targets.size()).arg(routeSummary(f->route));
    if (const StandaloneFalseTarget* t = m_model.falseTarget(id)) {
        QString state;
        QString why;
        if (!t->validate(&why))
            state = QStringLiteral(" ⚠");
        return QStringLiteral("%1   [%2]%3").arg(t->name, toDisplayString(t->ftType), state);
    }
    return {};
}

QString ObjectTreeModel::itemToolTip(ObjectId id) const
{
    if (const Radar* r = m_model.radar(id))
        return QStringLiteral("%1\n%2\n주파수 %3 MHz · 스캔 %4 s · 빔폭 %5°")
            .arg(r->name, EditorUtil::formatGeo(r->pos.lat, r->pos.lon))
            .arg(r->freqMHz).arg(r->scanPeriodSec).arg(r->beamWidthDeg);
    if (const StandaloneFalseTarget* t = m_model.falseTarget(id)) {
        QString why;
        return t->validate(&why) ? t->name : QStringLiteral("%1: %2").arg(t->name, why);
    }
    if (const Route* r = m_model.route(id)) {
        QString why;
        return r->isValid(&why) ? QStringLiteral("총 %1 km · 소요 %2")
                                      .arg(r->totalLengthKm(), 0, 'f', 1)
                                      .arg(EditorUtil::formatDuration(r->totalDurationSec()))
                                : why;
    }
    return {};
}
