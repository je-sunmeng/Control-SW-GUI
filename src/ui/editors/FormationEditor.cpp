#include "FormationEditor.h"

#include "JammingConfigEditor.h"
#include "RouteSegmentTable.h"
#include "model/ScenarioModel.h"

#include <QFormLayout>
#include <QLineEdit>
#include <QGroupBox>
#include <QSpinBox>
#include <QVBoxLayout>

FormationEditor::FormationEditor(ScenarioModel& model, QWidget* parent)
    : ObjectEditorBase(model, parent)
    , m_size(new QSpinBox(this))
    , m_route(new RouteSegmentTable(this))
    , m_jamming(new JammingConfigEditor(this))
{
    m_size->setRange(Limits::MinFormationSize, Limits::MaxFormationSize);
    m_size->setSuffix(QStringLiteral(" 기"));
    m_size->setKeyboardTracking(false);
    m_size->setAlignment(Qt::AlignRight);

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("이름"), makeNameEdit());
    form->addRow(QStringLiteral("편대규모"), m_size);

    auto* routeBox = new QGroupBox(QStringLiteral("동선 (구간별 고도/속력)"), this);
    auto* rl = new QVBoxLayout(routeBox);
    rl->addWidget(m_route);

    auto* jamBox = new QGroupBox(QStringLiteral("교란설정"), this);
    auto* jl = new QVBoxLayout(jamBox);
    jl->addWidget(m_jamming);

    auto* l = new QVBoxLayout(this);
    l->setContentsMargins(4, 4, 4, 4);
    l->addLayout(form);
    l->addWidget(routeBox, 3);
    l->addWidget(jamBox, 2);

    connectRouteTable(m_route);
    connect(m_size, &QSpinBox::valueChanged, this, [this](int n) {
        if (!m_model.formation())
            return;
        FriendlyFormation f = *m_model.formation();
        f.formationSize = n;
        m_model.updateFormation(f);
    });
    connect(m_jamming, &JammingConfigEditor::configEdited, this, [this](const JammingConfig& cfg) {
        if (!m_model.formation())
            return;
        FriendlyFormation f = *m_model.formation();
        f.jamming = cfg;
        m_model.updateFormation(f);
    });
}

void FormationEditor::load()
{
    const auto& f = m_model.formation();
    if (!f || f->id != m_id)
        return;
    loadName();
    if (m_size->value() != f->formationSize) {
        const QSignalBlocker b(m_size);
        m_size->setValue(f->formationSize);
    }
    m_route->setRoute(f->route);
    m_jamming->setConfig(f->jamming);
}
