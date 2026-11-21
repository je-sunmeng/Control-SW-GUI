#include "JammerEditor.h"

#include "RouteSegmentTable.h"
#include "model/ScenarioModel.h"

#include <QFormLayout>
#include <QLineEdit>
#include <QGroupBox>
#include <QVBoxLayout>

JammerEditor::JammerEditor(ScenarioModel& model, QWidget* parent)
    : ObjectEditorBase(model, parent)
    , m_route(new RouteSegmentTable(this))
{
    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("이름"), makeNameEdit());

    auto* routeBox = new QGroupBox(QStringLiteral("동선 (구간별 고도/속력)"), this);
    auto* rl = new QVBoxLayout(routeBox);
    rl->addWidget(m_route);

    auto* l = new QVBoxLayout(this);
    l->setContentsMargins(4, 4, 4, 4);
    l->addLayout(form);
    l->addWidget(routeBox, 1);

    connectRouteTable(m_route);
}

void JammerEditor::load()
{
    if (!m_model.jammer() || m_model.jammer()->id != m_id)
        return;
    loadName();
    m_route->setRoute(m_model.jammer()->route);
}
