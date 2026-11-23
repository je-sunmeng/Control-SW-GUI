#include "DataLinkWidget.h"

#include "editors/EditorUtil.h"
#include "model/ScenarioModel.h"

#include <QFormLayout>

DataLinkWidget::DataLinkWidget(ScenarioModel& model, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
    , m_jammer(EditorUtil::makeSpin(ScenarioModel::MinDatalinkIntervalSec, ScenarioModel::MaxDatalinkIntervalSec, 1,
                                    QStringLiteral("s"), 0.1, this))
    , m_formation(EditorUtil::makeSpin(ScenarioModel::MinDatalinkIntervalSec, ScenarioModel::MaxDatalinkIntervalSec, 1,
                                       QStringLiteral("s"), 0.1, this))
{
    m_jammer->setValue(m_model.jammerIntervalSec());
    m_formation->setValue(m_model.datalinkIntervalSec());
    m_jammer->setToolTip(QStringLiteral("임무 중 전자전기 현재위치를 재밍기법발생기로 보내는 주기"));
    m_formation->setToolTip(QStringLiteral("임무 중 아군편대 현재위치를 재밍기법발생기로 보내는 주기"));

    auto* f = new QFormLayout(this);
    f->setContentsMargins(0, 0, 0, 0);
    f->addRow(QStringLiteral("전자전기 위치 전송간격"), m_jammer);
    f->addRow(QStringLiteral("편대 위치 전송간격"), m_formation);

    connect(m_jammer, &QDoubleSpinBox::valueChanged, &m_model, &ScenarioModel::setJammerIntervalSec);
    connect(m_formation, &QDoubleSpinBox::valueChanged, &m_model, &ScenarioModel::setDatalinkIntervalSec);
    connect(&m_model, &ScenarioModel::jammerIntervalChanged, this, [this](double v) { EditorUtil::setSilently(m_jammer, v); });
    connect(&m_model, &ScenarioModel::datalinkIntervalChanged, this, [this](double v) { EditorUtil::setSilently(m_formation, v); });
    connect(&m_model, &ScenarioModel::lockedChanged, this, [this](bool locked) { setEnabled(!locked); });
    setEnabled(!m_model.isLocked());
}
