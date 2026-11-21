#include "ObjectEditorBase.h"

#include "RouteSegmentTable.h"
#include "model/ScenarioModel.h"

#include <QLineEdit>
#include <QSignalBlocker>

ObjectEditorBase::ObjectEditorBase(ScenarioModel& model, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
{
    connect(&m_model, &ScenarioModel::objectChanged, this, [this](ObjectId id) {
        if (id == m_id)
            load();
    });
}

void ObjectEditorBase::setObject(ObjectId id)
{
    m_id = id;
    if (m_model.find(id))
        load();
}

QLineEdit* ObjectEditorBase::makeNameEdit()
{
    m_name = new QLineEdit(this);
    m_name->setMaxLength(32);
    connect(m_name, &QLineEdit::editingFinished, this, [this] {
        if (!m_model.renameObject(m_id, m_name->text()))
            loadName();   // 빈 이름 등 거부 → 원래 값 복원
    });
    return m_name;
}

void ObjectEditorBase::loadName()
{
    const SimObject* o = m_model.find(m_id);
    if (!m_name || !o || m_name->text() == o->name)
        return;
    const QSignalBlocker b(m_name);
    m_name->setText(o->name);
}

void ObjectEditorBase::connectRouteTable(RouteSegmentTable* table)
{
    connect(table, &RouteSegmentTable::segmentEdited, this,
            [this](int i, const RouteSegment& seg) { m_model.setSegment(m_id, i, seg); });
    connect(table, &RouteSegmentTable::applyToAllRequested, this, [this](const RouteSegment& seg) {
        if (const Route* r = m_model.route(m_id)) {
            const int n = r->segmentCount();
            for (int i = 0; i < n; ++i)
                m_model.setSegment(m_id, i, seg);
        }
    });
    connect(table, &RouteSegmentTable::removeLastRequested, this, [this] {
        if (const Route* r = m_model.route(m_id); r && r->waypointCount() > 0)
            m_model.removeWaypoint(m_id, r->waypointCount() - 1);
    });
    connect(table, &RouteSegmentTable::redrawRequested, this, [this] {
        if (m_model.clearRoute(m_id))
            emit redrawRouteRequested(m_id);
    });
}
