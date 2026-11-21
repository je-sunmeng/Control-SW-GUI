#pragma once

#include "common/Types.h"

#include <QObject>
#include <QStringList>

class Logger;
class ScenarioModel;
class SimulationEngine;

// 버튼 활성화 상태 (DESIGN 4.6 표)
struct MissionActions {
    bool sendMission = false;
    bool start = false;
    bool startIsResume = false;   // 시작 버튼 텍스트 "재개"
    bool pause = false;
    bool pauseVisible = true;     // 연동모드에서는 숨김
    bool stop = false;
    bool refresh = true;
    bool refreshNeedsConfirm = false;
    bool speed = false;
    bool modeChange = false;

    bool operator==(const MissionActions& o) const;
    bool operator!=(const MissionActions& o) const { return !(*this == o); }
};

// 버튼 동작의 단일 진입점 + 활성화 규칙 관리 (UI 비의존)
//  M4: 단독모드 시작/일시정지/재개/중지/새로고침/배속
//  M5: 연동모드 임무전송/장치 연결/위치 주기송신/MainLobe 수신 연동 예정
class MissionController : public QObject {
    Q_OBJECT
public:
    MissionController(ScenarioModel& model, SimulationEngine& engine, Logger& logger, QObject* parent = nullptr);

    RunMode mode() const;
    SimState state() const;
    bool missionSent() const { return m_missionSent; }
    bool isDeviceConnected(DeviceCode dev) const;
    MissionActions actions() const;

public slots:
    bool setMode(RunMode mode);
    bool sendMission();
    // 임무 전이면 시작, 일시정지면 재개. 실패 시 errors 채움
    bool startOrResume(QStringList* errors = nullptr);
    bool pause();
    void stop();
    // 초기화: 엔진 reset, 동적 아이템 제거, 모델 잠금 해제, 임무 전 (오브젝트 설정 유지)
    // 임무 중/일시정지 상태의 확인 질문은 UI 책임 (refreshNeedsConfirm)
    void refresh();
    bool setSpeed(int factor);
    void setDeviceConnected(DeviceCode dev, bool connected);

signals:
    void actionsChanged(const MissionActions& actions);
    void modeChanged(RunMode mode);

private:
    void updateActions();

    ScenarioModel& m_model;
    SimulationEngine& m_engine;
    Logger& m_logger;
    bool m_missionSent = false;
    bool m_analysisConnected = false;
    bool m_signalGenConnected = false;
    MissionActions m_actions;
};
