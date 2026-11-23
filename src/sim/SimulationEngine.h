#pragma once

#include "ExternalDetectionSource.h"
#include "FalseTargetGenerator.h"
#include "LocalDetectionSource.h"
#include "RadarScanner.h"
#include "RouteKinematics.h"
#include "SimClock.h"
#include "SimSnapshot.h"

#include "model/ScenarioModel.h"

#include <QHash>
#include <QObject>
#include <QTimer>

// 시뮬레이션 엔진
//  - QTimer 틱(기본 33ms, 전시용). 연산은 SimClock 시간 기반이므로 틱 지연과 무관
//  - 상태머신: PreMission → Running ⇄ Paused(단독 전용) → Stopped → (reset) PreMission
//  - 단독모드: 시작 시점 및 매 스캔주기마다 내부 MainLobe 생성(빔이 전자전기 방향을 향함) + 자체 탐지
//  - 연동모드: 외부 MainLobe(onExternalMainLobe) 로만 빔 동기화, 탐지는 ExternalDetectionSource
//  - 주기 이벤트(시뮬레이션 시간 기준): 1s 전자전기 위치, 데이터링크 간격 편대 위치
class SimulationEngine : public QObject {
    Q_OBJECT
public:
    static constexpr int DefaultTickMs = 33;

    explicit SimulationEngine(ScenarioModel& model, QObject* parent = nullptr);

    RunMode mode() const { return m_mode; }
    bool setMode(RunMode mode);   // 임무 전/중지 상태에서만
    SimState state() const { return m_state; }
    int speedFactor() const { return m_clock.speedFactor(); }
    double simTime() const { return m_clock.simTimeSec(); }
    const SimSnapshot& lastSnapshot() const { return m_last; }

    SimClock& clock() { return m_clock; }
    ExternalDetectionSource& externalSource() { return m_external; }

    bool start(QString* error = nullptr);
    bool pause();    // 단독모드 전용
    bool resume();   // 단독모드 전용
    void stop();
    void reset();
    bool setSpeedFactor(int factor);   // 연동모드는 1 고정

    // 연동모드: 신호발생장치 MainLobe 수신 → 빔 동기화 (az = 레이다→현재 전자전기 방위)
    void onExternalMainLobe(ObjectId radarId);

    // 테스트/수동 구동: 시뮬레이션 시간 t 의 상태 계산 및 시그널 발생
    SimSnapshot advanceTo(double simTime);

    void setTickIntervalMs(int ms) { m_timer.setInterval(ms); }
    void setTimerEnabled(bool on) { m_timerEnabled = on; }   // 테스트에서 수동 구동 시 false
    void setRandomSeed(quint32 seed) { m_seed = seed; }

signals:
    void snapshotReady(const SimSnapshot& snapshot);
    void stateChanged(SimState state);
    void speedFactorChanged(int factor);
    void jammerPositionDue(const PlatformState& state);
    void formationPositionDue(const PlatformState& state);
    void mainLobeSynced(ObjectId radarId);
    // 아군편대가 동선 마지막 점에 도달 (임무 1회당 1번) → MissionController 가 임무 종료
    void formationRouteCompleted(double simTime);

private:
    void onTick();
    void setState(SimState s);
    KinematicState jammerAt(double t) const;
    void syncStandaloneMainLobes(double t);
    void syncRadar(ObjectId radarId, double t, const GeoPoint& radarPos);

    ScenarioModel& m_model;
    ScenarioData m_data;   // 임무 시작 시점 시나리오 사본
    RunMode m_mode = RunMode::Standalone;
    SimState m_state = SimState::PreMission;
    SimClock m_clock;
    QTimer m_timer;
    bool m_timerEnabled = true;
    quint32 m_seed = 0;

    RadarScanner m_scanner;
    FalseTargetGenerator m_generator;
    LocalDetectionSource m_local;
    ExternalDetectionSource m_external;
    QHash<ObjectId, double> m_nextSync;   // 단독모드 다음 MainLobe 시각
    double m_nextJammerDue = 0.0;
    double m_nextFormationDue = 0.0;
    bool m_formationDone = false;
    SimSnapshot m_last;
};
