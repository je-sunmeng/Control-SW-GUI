#include "ObjectEditorStack.h"

#include "FalseTargetEditor.h"
#include "FormationEditor.h"
#include "JammerEditor.h"
#include "RadarEditor.h"
#include "model/ScenarioModel.h"

#include <QLabel>
#include <QScrollArea>

ObjectEditorStack::ObjectEditorStack(ScenarioModel& model, QWidget* parent)
    : QStackedWidget(parent)
    , m_model(model)
    , m_empty(new QLabel(QStringLiteral("목록 또는 지도에서 오브젝트를 선택하세요"), this))
    , m_radar(new RadarEditor(model, this))
    , m_jammer(new JammerEditor(model, this))
    , m_formation(new FormationEditor(model, this))
    , m_falseTarget(new FalseTargetEditor(model, this))
{
    auto* empty = static_cast<QLabel*>(m_empty);
    empty->setAlignment(Qt::AlignCenter);
    empty->setWordWrap(true);
    empty->setObjectName(QStringLiteral("placeholderLabel"));
    addWidget(m_empty);
    addEditor(m_radar);
    addEditor(m_jammer);
    addEditor(m_formation);
    addEditor(m_falseTarget);

    connect(&m_model, &ScenarioModel::objectRemoved, this, [this](ObjectId id) {
        if (id == m_current)
            setObject(0);
    });
    connect(&m_model, &ScenarioModel::scenarioReset, this, [this] { setObject(0); });
    connect(&m_model, &ScenarioModel::lockedChanged, this, [this](bool locked) { setEnabled(!locked); });
    setEnabled(!m_model.isLocked());
}

void ObjectEditorStack::addEditor(ObjectEditorBase* editor)
{
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);   // 패널 폭에 맞춤 (값이 스크롤바 뒤로 숨지 않게)
    scroll->setWidget(editor);
    addWidget(scroll);
    m_pageOf.insert(editor, scroll);
    connect(editor, &ObjectEditorBase::redrawRouteRequested, this, &ObjectEditorStack::redrawRouteRequested);
    connect(editor, &ObjectEditorBase::placePointRequested, this, &ObjectEditorStack::placePointRequested);
}

void ObjectEditorStack::setObject(ObjectId id)
{
    m_current = id;
    const auto type = m_model.objectType(id);
    ObjectEditorBase* editor = nullptr;
    if (type) {
        switch (*type) {
        case ObjectType::Radar:                 editor = m_radar; break;
        case ObjectType::Jammer:                editor = m_jammer; break;
        case ObjectType::Formation:             editor = m_formation; break;
        case ObjectType::StandaloneFalseTarget: editor = m_falseTarget; break;
        }
    }
    if (!editor) {
        m_current = 0;
        setCurrentWidget(m_empty);
        return;
    }
    editor->setObject(id);
    setCurrentWidget(m_pageOf.value(editor));
}
