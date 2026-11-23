#include "MissionController.h"

#include "common/Constants.h"
#include "log/Logger.h"
#include "model/ScenarioModel.h"
#include "net/NetworkManager.h"
#include "net/messages/FalseTargetInfoMsg.h"
#include "net/messages/FormationInfoMsg.h"
#include "net/messages/JammerRouteMsg.h"
#include "net/messages/MainLobeNotifyMsg.h"
#include "net/messages/MissionStartMsg.h"
#include "net/messages/MissionStopMsg.h"
#include "net/messages/PositionMsg.h"
#include "net/messages/RadarInfoMsg.h"
#include "sim/RouteKinematics.h"
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

MissionController::MissionController(ScenarioModel& model, SimulationEngine& engine, Logger& logger,
                                     NetworkManager* network, QObject* parent)
    : QObject(parent)
    , m_model(model)
    , m_engine(engine)
    , m_logger(logger)
    , m_network(network)
    , m_port(Defaults::ServerPort)
{
    if (m_network) {
        connect(m_network, &NetworkManager::deviceConnectionChanged, this, &MissionController::setDeviceConnected);
        connect(m_network, &NetworkManager::messageReceived, this, &MissionController::onMessage);
    }
    connect(&m_engine, &SimulationEngine::jammerPositionDue, this,
            [this](const PlatformState& s) { onPositionDue(s, false); });
    connect(&m_engine, &SimulationEngine::formationPositionDue, this,
            [this](const PlatformState& s) { onPositionDue(s, true); });
    // 아군편대가 동선 끝에 도달하면 임무 종료 (연동모드는 STOP 메시지 포함)
    connect(&m_engine, &SimulationEngine::formationRouteCompleted, this, [this](double t) {
        m_logger.info(QStringLiteral("아군편대 동선 종료 (T+%1 s) → 임무 종료").arg(t, 0, 'f', 1), kSource);
        stop();
    });
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
        a.sendMission = !standalone && bothConnected && m_model.validate();
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
    if (m_network)
        m_network->setEnabled(m == RunMode::Linked, m_port);   // 단독모드: 서버 close
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
    const ScenarioData d = m_model.snapshot();
    bool ok = true;

    RadarInfoMsg radars;
    radars.radars = d.radars;
    // 분석SW: 레이다 / 전자전기 동선 / 편대 동선
    ok &= sendTo(DeviceCode::AnalysisSW, radars);
    if (d.jammer) {
        JammerRouteMsg jr;
        jr.route = d.jammer->route;
        ok &= sendTo(DeviceCode::AnalysisSW, jr);
    }
    if (d.formation) {
        FormationInfoMsg f;
        f.formation = *d.formation;
        f.includeJamming = false;
        ok &= sendTo(DeviceCode::AnalysisSW, f);
    }
    // 신호발생: 레이다 / 편대(+교란설정) / 단독허위표적
    ok &= sendTo(DeviceCode::SignalGen, radars);
    if (d.formation) {
        FormationInfoMsg f;
        f.formation = *d.formation;
        f.includeJamming = true;
        ok &= sendTo(DeviceCode::SignalGen, f);
    }
    FalseTargetInfoMsg ft;
    ft.targets = d.falseTargets;
    ok &= sendTo(DeviceCode::SignalGen, ft);

    m_missionSent = ok;
    if (ok)
        m_logger.info(QStringLiteral("임무전송 완료"), kSource);
    else
        m_logger.error(QStringLiteral("임무전송 실패 — 일부 메시지를 보내지 못했습니다"), kSource);
    updateActions();
    return ok;
}

bool MissionController::sendTo(DeviceCode dst, const IMessage& msg)
{
    if (!m_network)
        return true;   // 네트워크 없는 구성(테스트): 송신 생략
    return m_network->send(dst, msg);
}

void MissionController::onPositionDue(const PlatformState& state, bool formation)
{
    if (mode() != RunMode::Linked || this->state() != SimState::Running)
        return;
    if (formation) {
        FormationPositionMsg m;
        m.simTime = m_engine.simTime();
        m.pos = state.pos;
        m.altKm = state.altKm;
        m.headingDeg = state.headingDeg;
        sendTo(DeviceCode::SignalGen, m);
    } else {
        JammerPositionMsg m;
        m.simTime = m_engine.simTime();
        m.pos = state.pos;
        m.altKm = state.altKm;
        m.headingDeg = state.headingDeg;
        sendTo(DeviceCode::SignalGen, m);
    }
}

void MissionController::onMessage(DeviceCode src, const QSharedPointer<IMessage>& msg)
{
    if (!msg || msg->id() != MessageId::MainLobeNotify)
        return;   // Heartbeat 등은 로그만
    const auto* ml = static_cast<const MainLobeNotifyMsg*>(msg.data());
    if (src != DeviceCode::SignalGen) {
        m_logger.error(QStringLiteral("MainLobeNotify 는 신호발생장치에서만 수신해야 합니다"), kSource);
        return;
    }
    if (mode() != RunMode::Linked || (state() != SimState::Running && state() != SimState::Paused))
        return;   // 임무 중이 아니면 무시 (로그는 NetworkManager 가 기록)
    if (!m_model.radar(ml->radarId)) {
        m_logger.error(QStringLiteral("MainLobeNotify: 알 수 없는 레이다 ID %1").arg(ml->radarId), kSource);
        return;
    }
    // 메시지 1개 → 빔 동기화 + 램프(mainLobeSynced) + 탐지 전시
    m_engine.onExternalMainLobe(ml->radarId);
    for (int i = 0; i < ml->detections.size(); ++i) {
        Detection d;
        d.radarId = ml->radarId;
        d.targetKey = QStringLiteral("EXT%1").arg(i + 1);
        d.pos = ml->detections[i];
        d.simTime = m_engine.simTime();
        m_engine.externalSource().push(d);
    }
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
    if (mode() == RunMode::Linked) {
        const ScenarioData d = m_model.snapshot();
        MissionStartMsg toAnalysis;
        MissionStartMsg toSigGen;
        toSigGen.withStartPositions = true;
        if (d.formation && !d.formation->route.isEmpty())
            toSigGen.formationStart = d.formation->route.waypoints.first();
        if (d.jammer && !d.jammer->route.isEmpty())
            toSigGen.jammerStart = d.jammer->route.waypoints.first();
        if (!sendTo(DeviceCode::AnalysisSW, toAnalysis) || !sendTo(DeviceCode::SignalGen, toSigGen)) {
            if (errors)
                *errors = {QStringLiteral("임무시작 메시지를 보내지 못했습니다 (장치 연결 확인)")};
            return false;
        }
    }
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
    if (mode() == RunMode::Linked) {
        const MissionStopMsg stopMsg;
        sendTo(DeviceCode::AnalysisSW, stopMsg);
        sendTo(DeviceCode::SignalGen, stopMsg);
    }
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
