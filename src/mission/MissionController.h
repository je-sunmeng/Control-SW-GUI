#pragma once

#include "common/Types.h"

#include <QObject>
#include <QStringList>

class Logger;
class NetworkManager;
class ScenarioModel;
class SimulationEngine;
class IMessage;
struct PlatformState;
template <typename T> class QSharedPointer;

// 버튼 활성화 상태 (DESIGN 4.6 표)
struct MissionActions {
    bool sendMission = false;
    bool sendMissionVisible = false;   // 연동모드에서만 표시
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
//  단독모드: 시작/일시정지/재개/중지/새로고침/배속 (네트워크 미사용)
//  연동모드: 서버 대기, 임무전송(레이다/동선/편대/허위표적), START/STOP, 위치 주기송신,
//           MainLobe 수신 → 빔 동기화 + 탐지 전시 + 램프 (network 가 nullptr 이면 송신 생략)
class MissionController : public QObject {
    Q_OBJECT
public:
    MissionController(ScenarioModel& model, SimulationEngine& engine, Logger& logger,
                      NetworkManager* network = nullptr, QObject* parent = nullptr);

    void setServerPort(quint16 port) { m_port = port; }
    quint16 serverPort() const { return m_port; }

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
    bool sendTo(DeviceCode dst, const IMessage& msg);
    void onMessage(DeviceCode src, const QSharedPointer<IMessage>& msg);
    void onPositionDue(const PlatformState& state, bool formation);

    ScenarioModel& m_model;
    SimulationEngine& m_engine;
    Logger& m_logger;
    NetworkManager* m_network;
    quint16 m_port;
    bool m_missionSent = false;
    bool m_analysisConnected = false;
    bool m_signalGenConnected = false;
    MissionActions m_actions;
};
