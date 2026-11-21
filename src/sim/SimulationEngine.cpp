#include "SimulationEngine.h"

#include "RouteKinematics.h"
#include "common/GeoUtils.h"
#include <algorithm>
#include <iterator>

namespace {
constexpr int kMaxSyncPerTick = 64;   // 비정상 시간 점프 시 무한 루프 방지

PlatformState platformFrom(ObjectId id, ObjectType type, const KinematicState& k, int size = 1)
{
    PlatformState p;
    p.id = id;
    p.type = type;
    p.pos = k.pos;
    p.altKm = k.altKm;
    p.headingDeg = k.headingDeg;
    p.speedMps = k.speedMps;
    p.finished = k.finished;
    p.formationSize = size;
    return p;
}
} // namespace

SimulationEngine::SimulationEngine(ScenarioModel& model, QObject* parent)
    : QObject(parent)
    , m_model(model)
{
    qRegisterMetaType<SimSnapshot>();
    qRegisterMetaType<PlatformState>();
    m_timer.setInterval(DefaultTickMs);
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, &SimulationEngine::onTick);
    connect(&m_clock, &SimClock::speedFactorChanged, this, &SimulationEngine::speedFactorChanged);
}

bool SimulationEngine::setMode(RunMode mode)
{
    if (m_state != SimState::PreMission && m_state != SimState::Stopped)
        return false;
    m_mode = mode;
    if (mode == RunMode::Linked)
        m_clock.setSpeedFactor(1);
    return true;
}

void SimulationEngine::setState(SimState s)
{
    if (s == m_state)
        return;
    m_state = s;
    emit stateChanged(s);
}

bool SimulationEngine::setSpeedFactor(int factor)
{
    if (!SimClock::isValidSpeed(factor))
        return false;
    if (m_mode == RunMode::Linked && factor != 1)
        return false;
    m_clock.setSpeedFactor(factor);
    return true;
}

bool SimulationEngine::start(QString* error)
{
    if (m_state != SimState::PreMission) {
        if (error)
            *error = QStringLiteral("임무 전 상태에서만 시작할 수 있습니다");
        return false;
    }
    QStringList errs;
    if (!m_model.validate(&errs)) {
        if (error)
            *error = errs.join(QLatin1Char('\n'));
        return false;
    }
    m_data = m_model.snapshot();

    m_scanner.reset();
    for (const Radar& r : std::as_const(m_data.radars))
        m_scanner.setRadar(r.id, {r.scanPeriodSec, r.scanDir});
    m_generator.reset(m_seed);
    m_local.reset();
    m_external.reset();
    m_nextSync.clear();
    m_nextJammerDue = 0.0;
    m_nextFormationDue = 0.0;
    if (m_mode == RunMode::Linked)
        m_clock.setSpeedFactor(1);

    m_clock.start();
    setState(SimState::Running);
    advanceTo(0.0);
    if (m_timerEnabled)
        m_timer.start();
    return true;
}

bool SimulationEngine::pause()
{
    if (m_mode != RunMode::Standalone || m_state != SimState::Running)
        return false;
    m_clock.pause();
    m_timer.stop();
    setState(SimState::Paused);
    return true;
}

bool SimulationEngine::resume()
{
    if (m_mode != RunMode::Standalone || m_state != SimState::Paused)
        return false;
    m_clock.resume();
    setState(SimState::Running);
    if (m_timerEnabled)
        m_timer.start();
    return true;
}

void SimulationEngine::stop()
{
    if (m_state != SimState::Running && m_state != SimState::Paused)
        return;
    m_clock.stop();
    m_timer.stop();
    m_last.state = SimState::Stopped;
    setState(SimState::Stopped);
}

void SimulationEngine::reset()
{
    m_timer.stop();
    m_clock.reset();
    m_scanner.reset();
    m_generator.reset(m_seed);
    m_local.reset();
    m_external.reset();
    m_nextSync.clear();
    m_last = SimSnapshot{};
    setState(SimState::PreMission);
    emit snapshotReady(m_last);   // 전시 초기화
}

void SimulationEngine::onTick()
{
    if (m_state == SimState::Running)
        advanceTo(m_clock.simTimeSec());
}

KinematicState SimulationEngine::jammerAt(double t) const
{
    return m_data.jammer ? RouteKinematics::stateAt(m_data.jammer->route, t) : KinematicState{};
}

void SimulationEngine::syncRadar(ObjectId radarId, double t, const GeoPoint& radarPos)
{
    const KinematicState j = jammerAt(t);
    const double az = j.valid ? GeoUtils::bearingDeg(radarPos, j.pos) : 0.0;
    m_scanner.onMainLobe(radarId, t, az);
    emit mainLobeSynced(radarId);
}

void SimulationEngine::syncStandaloneMainLobes(double t)
{
    for (const Radar& r : std::as_const(m_data.radars)) {
        auto it = m_nextSync.find(r.id);
        if (it == m_nextSync.end()) {
            syncRadar(r.id, 0.0, r.pos);   // 시작 시점
            it = m_nextSync.insert(r.id, r.scanPeriodSec);
        }
        int guard = 0;
        while (*it <= t && guard++ < kMaxSyncPerTick) {
            syncRadar(r.id, *it, r.pos);
            *it += r.scanPeriodSec;
        }
        if (*it <= t)   // 점프가 너무 큰 경우 현재 시각 기준으로 재정렬
            *it = t + r.scanPeriodSec;
    }
}

void SimulationEngine::onExternalMainLobe(ObjectId radarId)
{
    if (m_state != SimState::Running && m_state != SimState::Paused)
        return;
    for (const Radar& r : std::as_const(m_data.radars)) {
        if (r.id == radarId) {
            syncRadar(radarId, m_clock.simTimeSec(), r.pos);
            return;
        }
    }
}

SimSnapshot SimulationEngine::advanceTo(double t)
{
    SimSnapshot s;
    s.simTime = t;
    s.state = m_state;

    // --- 이동체 ---
    PlatformState jammerState, formationState;
    bool hasFormation = false;
    if (m_data.jammer) {
        jammerState = platformFrom(m_data.jammer->id, ObjectType::Jammer, jammerAt(t));
        s.platforms.append(jammerState);
    }
    if (m_data.formation && m_data.formation->route.waypointCount() > 0) {
        const KinematicState k = RouteKinematics::stateAt(m_data.formation->route, t);
        formationState = platformFrom(m_data.formation->id, ObjectType::Formation, k, m_data.formation->formationSize);
        s.platforms.append(formationState);
        hasFormation = true;
        s.formationFalseTargets = FalseTargetGenerator::formationTargets(k.pos, k.headingDeg, m_data.formation->jamming);
    }

    // --- 판정 대상 / 잡음섹터 ---
    SimContext ctx;
    ctx.radars = &m_data.radars;
    for (int i = 0; i < s.formationFalseTargets.size(); ++i)
        ctx.targets.append({QStringLiteral("FFT%1").arg(i + 1), s.formationFalseTargets[i]});
    bool hasRandom = false;
    for (const StandaloneFalseTarget& ft : std::as_const(m_data.falseTargets)) {
        switch (ft.ftType) {
        case FalseTargetType::Fixed:
            if (ft.posSet)
                ctx.targets.append({QStringLiteral("FT%1").arg(ft.id), ft.pos});
            break;
        case FalseTargetType::Moving: {
            const KinematicState k = RouteKinematics::stateAt(ft.route, t);
            if (k.valid) {
                s.platforms.append(platformFrom(ft.id, ObjectType::StandaloneFalseTarget, k));
                ctx.targets.append({QStringLiteral("FT%1").arg(ft.id), k.pos});
            }
            break;
        }
        case FalseTargetType::Random:
            hasRandom = true;
            break;
        case FalseTargetType::Noise:
            if (ft.posSet)
                for (const Radar& r : std::as_const(m_data.radars))
                    s.noiseSectors.append({r.id, ft.id, GeoUtils::bearingDeg(r.pos, ft.pos), ft.detectAngleDeg});
            break;
        }
    }
    ctx.noiseSectors = s.noiseSectors;

    // --- 빔 ---
    if (m_mode == RunMode::Standalone && (m_state == SimState::Running || m_state == SimState::Paused))
        syncStandaloneMainLobes(t);
    for (const Radar& r : std::as_const(m_data.radars))
        s.beams.append({r.id, m_scanner.beamAzimuthAt(r.id, t), m_scanner.isSynced(r.id)});
    ctx.beams = s.beams;

    // --- 탐지 / 고스트 ---
    IDetectionSource& source = m_mode == RunMode::Standalone ? static_cast<IDetectionSource&>(m_local)
                                                             : static_cast<IDetectionSource&>(m_external);
    s.detections = source.collect(ctx, t);
    m_generator.updateGhosts(t, m_data.radars, hasRandom);
    s.ghosts = m_generator.ghosts();

    // --- 주기 이벤트 (틱당 최대 1회, 밀린 주기는 건너뜀) ---
    if (m_data.jammer && t >= m_nextJammerDue) {
        emit jammerPositionDue(jammerState);
        while (m_nextJammerDue <= t)
            m_nextJammerDue += Defaults::JammerPositionIntervalSec;
    }
    if (hasFormation && t >= m_nextFormationDue) {
        emit formationPositionDue(formationState);
        const double interval = std::max(ScenarioModel::MinDatalinkIntervalSec, m_data.datalinkIntervalSec);
        while (m_nextFormationDue <= t)
            m_nextFormationDue += interval;
    }

    m_last = s;
    emit snapshotReady(s);
    return s;
}
