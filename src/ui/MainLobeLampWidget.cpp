#include "MainLobeLampWidget.h"

#include "LampIndicator.h"
#include "common/ColorPalette.h"
#include "model/ScenarioModel.h"

#include <QGridLayout>
#include <QLabel>

MainLobeLampWidget::MainLobeLampWidget(ScenarioModel& model, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
{
    auto* g = new QGridLayout(this);
    g->setContentsMargins(0, 0, 0, 0);
    g->setHorizontalSpacing(4);
    for (int i = 0; i < 4; ++i) {
        m_lamps[i] = new LampIndicator(ColorPalette::radar(i), 18, this);
        m_labels[i] = new QLabel(QStringLiteral("R%1").arg(i + 1), this);
        m_labels[i]->setAlignment(Qt::AlignCenter);
        g->addWidget(m_lamps[i], 0, i, Qt::AlignHCenter);
        g->addWidget(m_labels[i], 1, i, Qt::AlignHCenter);
    }
    connect(&m_model, &ScenarioModel::objectAdded, this, &MainLobeLampWidget::refreshRadars);
    connect(&m_model, &ScenarioModel::objectChanged, this, &MainLobeLampWidget::refreshRadars);
    connect(&m_model, &ScenarioModel::objectRemoved, this, &MainLobeLampWidget::refreshRadars);
    connect(&m_model, &ScenarioModel::scenarioReset, this, &MainLobeLampWidget::refreshRadars);
    refreshRadars();
}

void MainLobeLampWidget::refreshRadars()
{
    m_slotOf.clear();
    bool used[4] = {};
    for (const Radar& r : m_model.radars()) {
        if (r.colorIndex < 0 || r.colorIndex > 3)
            continue;
        m_slotOf.insert(r.id, r.colorIndex);
        used[r.colorIndex] = true;
        m_labels[r.colorIndex]->setText(r.name);
        m_labels[r.colorIndex]->setToolTip(r.name);
    }
    for (int i = 0; i < 4; ++i) {
        m_lamps[i]->setEnabled(used[i]);
        m_labels[i]->setEnabled(used[i]);
        if (!used[i])
            m_labels[i]->setText(QStringLiteral("R%1").arg(i + 1));
    }
}

void MainLobeLampWidget::onMainLobe(ObjectId radarId)
{
    const int slot = m_slotOf.value(radarId, -1);
    if (slot >= 0)
        m_lamps[slot]->pulse();
}
