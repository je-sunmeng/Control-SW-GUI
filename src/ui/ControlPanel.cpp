#include "ControlPanel.h"

#include "DataLinkWidget.h"
#include "DeviceStatusWidget.h"
#include "MainLobeLampWidget.h"
#include "ModeSelector.h"
#include "SimControlWidget.h"
#include "SpeedSelector.h"
#include "mission/MissionController.h"
#include "net/NetworkManager.h"
#include "sim/SimulationEngine.h"

#include <QGroupBox>
#include <QMessageBox>
#include <QVBoxLayout>

namespace {
QGroupBox* section(const QString& title, QWidget* content, QWidget* parent)
{
    auto* box = new QGroupBox(title, parent);
    auto* l = new QVBoxLayout(box);
    l->setContentsMargins(8, 8, 8, 8);
    l->addWidget(content);
    return box;
}
} // namespace

ControlPanel::ControlPanel(MissionController& mission, SimulationEngine& engine, ScenarioModel& model,
                           NetworkManager* network, QWidget* parent)
    : QWidget(parent)
    , m_mission(mission)
    , m_engine(engine)
    , m_mode(new ModeSelector(this))
    , m_devices(new DeviceStatusWidget(this))
    , m_dataLink(new DataLinkWidget(model, this))
    , m_dataLinkBox(nullptr)
    , m_lamps(new MainLobeLampWidget(model, this))
    , m_control(new SimControlWidget(this))
    , m_speed(new SpeedSelector(this))
{
    setObjectName(QStringLiteral("ControlPanel"));
    setMinimumWidth(240);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(6);
    layout->addWidget(section(QStringLiteral("운용 모드"), m_mode, this));
    layout->addWidget(section(QStringLiteral("장치 연결 상태"), m_devices, this));
    m_dataLinkBox = section(QStringLiteral("데이터링크"), m_dataLink, this);
    layout->addWidget(m_dataLinkBox);
    layout->addWidget(section(QStringLiteral("MainLobe"), m_lamps, this));
    layout->addWidget(section(QStringLiteral("시뮬레이션 제어"), m_control, this));
    layout->addWidget(section(QStringLiteral("배속"), m_speed, this));
    layout->addStretch(1);

    // 사용자 입력 → MissionController
    connect(m_mode, &ModeSelector::modeRequested, this, [this](RunMode m) {
        if (!m_mission.setMode(m))
            m_mode->setMode(m_mission.mode());
    });
    connect(m_speed, &SpeedSelector::speedRequested, this, [this](int f) {
        if (!m_mission.setSpeed(f))
            m_speed->setSpeed(m_engine.speedFactor());
    });
    connect(m_control, &SimControlWidget::sendMissionClicked, &m_mission, &MissionController::sendMission);
    connect(m_control, &SimControlWidget::startClicked, this, &ControlPanel::onStart);
    connect(m_control, &SimControlWidget::pauseClicked, &m_mission, &MissionController::pause);
    connect(m_control, &SimControlWidget::stopClicked, &m_mission, &MissionController::stop);
    connect(m_control, &SimControlWidget::refreshClicked, this, &ControlPanel::onRefresh);

    // 상태 → 표시
    connect(&m_mission, &MissionController::actionsChanged, this, &ControlPanel::syncFromController);
    connect(&m_mission, &MissionController::modeChanged, this, &ControlPanel::syncFromController);
    connect(&m_engine, &SimulationEngine::stateChanged, m_control, &SimControlWidget::setState);
    connect(&m_engine, &SimulationEngine::speedFactorChanged, m_speed, &SpeedSelector::setSpeed);
    connect(&m_engine, &SimulationEngine::snapshotReady, this,
            [this](const SimSnapshot& s) { m_control->setSimTime(s.simTime); });
    connect(&m_engine, &SimulationEngine::mainLobeSynced, m_lamps, &MainLobeLampWidget::onMainLobe);
    connect(&m_mission, &MissionController::modeChanged, m_lamps,
            [this](RunMode m) { m_lamps->setLinked(m == RunMode::Linked); });
    connect(&m_engine, &SimulationEngine::stateChanged, m_lamps, [this](SimState s) {
        if (s == SimState::PreMission)
            m_lamps->resetMission();   // 새로고침 → 모두 소등
    });
    m_lamps->setLinked(m_mission.mode() == RunMode::Linked);
    // 데이터링크 구역: 연동모드에서만 (장치 연결 상태 아래)
    connect(&m_mission, &MissionController::modeChanged, m_dataLinkBox,
            [this](RunMode m) { m_dataLinkBox->setVisible(m == RunMode::Linked); });
    m_dataLinkBox->setVisible(m_mission.mode() == RunMode::Linked);
    connect(&m_mission, &MissionController::modeChanged, this,
            [this](RunMode m) { m_devices->setNetworkActive(m == RunMode::Linked); });

    if (network) {
        connect(network, &NetworkManager::deviceConnectionChanged, m_devices, &DeviceStatusWidget::setConnected);
        connect(network, &NetworkManager::listeningChanged, m_devices, &DeviceStatusWidget::setListening);
    }

    syncFromController();
}

void ControlPanel::syncFromController()
{
    const MissionActions a = m_mission.actions();
    m_control->applyActions(a);
    m_mode->setMode(m_mission.mode());
    m_mode->setEnabled(a.modeChange);
    m_speed->setEnabled(a.speed);
    m_speed->setSpeed(m_engine.speedFactor());
    m_control->setState(m_mission.state());
}

void ControlPanel::onStart()
{
    QStringList errors;
    if (!m_mission.startOrResume(&errors) && !errors.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("임무 시작"),
                             QStringLiteral("임무를 시작할 수 없습니다.\n\n• %1").arg(errors.join(QStringLiteral("\n• "))));
    }
}

void ControlPanel::onRefresh()
{
    if (m_mission.actions().refreshNeedsConfirm
        && QMessageBox::question(this, QStringLiteral("새로고침"),
                                 QStringLiteral("임무를 중지하고 초기화합니다. 계속할까요?"))
               != QMessageBox::Yes)
        return;
    m_mission.refresh();
}
