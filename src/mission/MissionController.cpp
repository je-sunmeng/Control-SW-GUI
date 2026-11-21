#include "MissionController.h"

#include "log/Logger.h"
#include "model/ScenarioModel.h"
#include "sim/SimulationEngine.h"

namespace {
const QString kSource = QStringLiteral("Mission");

QString modeName(RunMode m)
{
    return m == RunMode::Standalone ? QStringLiteral("단독") : QStringLiteral("연동");
}
} // namespace

bool MissionActions::operator==(const MissionActions& o) const
{
    return sendMission == o.sendMission && start == o.start && startIsResume == o.startIsResume && pause == o.pause
           && pauseVisible == o.pauseVisible && stop == o.stop && refresh == o.refresh
           && refreshNeedsConfirm == o.refreshNeedsConfirm && speed == o.speed && modeChange == o.modeChange;
}

MissionController::MissionController(ScenarioModel& model, SimulationEngine& engine, Logger& logger, QObject* parent)
    : QObject(parent)
    , m_model(model)
    , m_engine(engine)
    , m_logger(logger)
{
    connect(&m_engine, &SimulationEngine::stateChanged, this, &MissionController::updateActions);
    connect(&m_model, &ScenarioModel::objectAdded, this, &MissionController::updateActions);
    connect(&m_model, &ScenarioModel::objectChanged, this, &MissionController::updateActions);
    connect(&m_model, &ScenarioModel::objectRemoved, this, &MissionController::updateActions);
    connect(&m_model, &ScenarioModel::scenarioReset, this, &MissionController::updateActions);
    m_actions = actions();
}

RunMode MissionController::mode() const
{
    return m_engine.mode();
}

SimState MissionController::state() const
{
    return m_engine.state();
}

bool MissionController::isDeviceConnected(DeviceCode dev) const
{
    switch (dev) {
    case DeviceCode::AnalysisSW: return m_analysisConnected;
    case DeviceCode::SignalGen:  return m_signalGenConnected;
    case DeviceCode::ControlSW:  return true;
    }
    return false;
}

MissionActions MissionController::actions() const
{
    MissionActions a;
    const bool standalone = mode() == RunMode::Standalone;
    const bool bothConnected = m_analysisConnected && m_signalGenConnected;
    a.pauseVisible = standalone;
    switch (state()) {
    case SimState::PreMission:
        a.sendMission = !standalone && bothConnected;
        a.start = standalone ? m_model.validate() : (m_missionSent && bothConnected);
        a.refresh = true;
        a.speed = standalone;
        a.modeChange = true;
        break;
    case SimState::Running:
        a.pause = standalone;
        a.stop = true;
        a.refresh = true;
        a.refreshNeedsConfirm = true;
        a.speed = standalone;
        break;
    case SimState::Paused:   // 단독모드에만 존재
        a.start = true;
        a.startIsResume = true;
        a.stop = true;
        a.refresh = true;
        a.refreshNeedsConfirm = true;
        a.speed = true;
        break;
    case SimState::Stopped:
        a.refresh = true;
        break;
    }
    return a;
}

void MissionController::updateActions()
{
    const MissionActions a = actions();
    if (a == m_actions)
        return;
    m_actions = a;
    emit actionsChanged(a);
}

bool MissionController::setMode(RunMode m)
{
    if (m == mode())
        return true;
    if (state() != SimState::PreMission || !m_engine.setMode(m)) {
        m_logger.error(QStringLiteral("임무 전 상태에서만 모드를 변경할 수 있습니다"), kSource);
        return false;
    }
    m_missionSent = false;
    m_logger.info(QStringLiteral("운용 모드: %1").arg(modeName(m)), kSource);
    emit modeChanged(m);
    updateActions();
    return true;
}

bool MissionController::sendMission()
{
    if (!actions().sendMission) {
        m_logger.error(QStringLiteral("임무전송 불가 (연동모드 + 두 장치 연결 필요)"), kSource);
        return false;
    }
    // TODO(M5): 분석SW / 신호발생장치로 임무 메시지 송신
    m_missionSent = true;
    m_logger.info(QStringLiteral("임무전송 완료"), kSource);
    updateActions();
    return true;
}

bool MissionController::startOrResume(QStringList* errors)
{
    if (state() == SimState::Paused) {
        if (!m_engine.resume())
            return false;
        m_logger.info(QStringLiteral("임무 재개 (T+%1 s)").arg(m_engine.simTime(), 0, 'f', 1), kSource);
        return true;
    }
    QStringList errs;
    if (!m_model.validate(&errs)) {
        if (errors)
            *errors = errs;
        m_logger.error(QStringLiteral("임무 시작 불가: %1").arg(errs.join(QStringLiteral(" / "))), kSource);
        return false;
    }
    if (mode() == RunMode::Linked && !m_missionSent) {
        if (errors)
            *errors = {QStringLiteral("임무전송 후 시작할 수 있습니다")};
        return false;
    }
    // TODO(M5): 연동모드 START 메시지 송신 (신호발생장치에는 편대/전자전기 시작위치 포함)
    QString err;
    if (!m_engine.start(&err)) {
        if (errors)
            *errors = {err};
        m_logger.error(QStringLiteral("임무 시작 실패: %1").arg(err), kSource);
        return false;
    }
    m_model.setLocked(true);
    m_logger.info(QStringLiteral("임무 시작 (%1모드, %2배속)").arg(modeName(mode())).arg(m_engine.speedFactor()), kSource);
    return true;
}

bool MissionController::pause()
{
    if (mode() != RunMode::Standalone) {
        m_logger.error(QStringLiteral("연동모드에서는 일시정지할 수 없습니다"), kSource);
        return false;
    }
    if (!m_engine.pause())
        return false;
    m_logger.info(QStringLiteral("일시정지 (T+%1 s)").arg(m_engine.simTime(), 0, 'f', 1), kSource);
    return true;
}

void MissionController::stop()
{
    if (state() != SimState::Running && state() != SimState::Paused)
        return;
    // TODO(M5): 양 장치 STOP 메시지 송신
    m_engine.stop();
    m_logger.info(QStringLiteral("임무 중지 (T+%1 s)").arg(m_engine.simTime(), 0, 'f', 1), kSource);
}

void MissionController::refresh()
{
    if (state() == SimState::Running || state() == SimState::Paused)
        stop();
    m_engine.reset();
    m_model.setLocked(false);
    m_missionSent = false;
    m_logger.info(QStringLiteral("새로고침: 임무 전 상태로 초기화"), kSource);
    updateActions();
}

bool MissionController::setSpeed(int factor)
{
    if (!actions().speed || !m_engine.setSpeedFactor(factor))
        return false;
    m_logger.info(QStringLiteral("배속 %1x").arg(factor), kSource);
    return true;
}

void MissionController::setDeviceConnected(DeviceCode dev, bool connected)
{
    if (dev == DeviceCode::AnalysisSW)
        m_analysisConnected = connected;
    else if (dev == DeviceCode::SignalGen)
        m_signalGenConnected = connected;
    if (!connected)
        m_missionSent = false;   // 재연결 시 임무 재전송 필요
    updateActions();
}
