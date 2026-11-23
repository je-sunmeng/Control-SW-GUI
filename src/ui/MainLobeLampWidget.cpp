#include "MainLobeLampWidget.h"

#include "LampIndicator.h"
#include "common/ColorPalette.h"
#include "model/ScenarioModel.h"

#include <QGridLayout>
#include <QLabel>

namespace {
const QString kDimStyle = QStringLiteral("color: #4f5763;");
}

MainLobeLampWidget::MainLobeLampWidget(ScenarioModel& model, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
{
    auto* g = new QGridLayout(this);
    g->setContentsMargins(0, 0, 0, 0);
    g->setHorizontalSpacing(4);
    for (int i = 0; i < SlotCount; ++i) {
        m_lamps[i] = new LampIndicator(ColorPalette::radar(i), 18, this);
        m_labels[i] = new QLabel(this);
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

bool MainLobeLampWidget::isLampOn(int slot) const
{
    return slot >= 0 && slot < SlotCount && m_lamps[slot]->isOn();
}

void MainLobeLampWidget::refreshRadars()
{
    // 목록 순서 그대로 슬롯 배정 (레이다 2 삭제 → 레이다 3 이 두 번째 슬롯)
    m_order.clear();
    for (const Radar& r : m_model.radars()) {
        if (m_order.size() >= SlotCount)
            break;
        m_order.append(r.id);
    }
    for (auto it = m_active.begin(); it != m_active.end();)   // 삭제된 레이다 정리
        it = m_order.contains(*it) ? std::next(it) : m_active.erase(it);
    for (int i = 0; i < SlotCount; ++i)
        updateSlot(i);
}

void MainLobeLampWidget::updateSlot(int slot)
{
    LampIndicator* lamp = m_lamps[slot];
    QLabel* label = m_labels[slot];
    const ObjectId id = radarAt(slot);
    const Radar* r = id ? m_model.radar(id) : nullptr;
    if (!r) {
        lamp->setOnColor(QColor(0x3a, 0x41, 0x4b));
        lamp->setOn(false);
        lamp->setEnabled(false);
        label->setText(QStringLiteral("—"));
        label->setToolTip(QString());
        label->setStyleSheet(kDimStyle);
        return;
    }
    const QColor c = ColorPalette::radar(r->colorIndex);
    const bool active = m_active.contains(id);
    lamp->setEnabled(true);
    lamp->setOnColor(c);
    lamp->setOn(m_linked && active);   // 연동모드: 수신 후 계속 켜짐
    label->setText(r->name);
    label->setToolTip(active ? QStringLiteral("%1: MainLobe 수신").arg(r->name)
                             : QStringLiteral("%1: MainLobe 미수신").arg(r->name));
    // 활성 = 레이다 색으로 밝게, 미활성 = 어둡게
    label->setStyleSheet(active ? QStringLiteral("color: %1; font-weight: bold;").arg(c.lighter(115).name()) : kDimStyle);
}

void MainLobeLampWidget::onMainLobe(ObjectId radarId)
{
    const int slot = int(m_order.indexOf(radarId));
    if (slot < 0)
        return;
    m_active.insert(radarId);
    updateSlot(slot);
    if (!m_linked)
        m_lamps[slot]->pulse();   // 단독모드: 깜빡임
}

void MainLobeLampWidget::setLinked(bool linked)
{
    m_linked = linked;
    for (int i = 0; i < SlotCount; ++i)
        updateSlot(i);
}

void MainLobeLampWidget::resetMission()
{
    m_active.clear();
    for (int i = 0; i < SlotCount; ++i)
        updateSlot(i);
}
