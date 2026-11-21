#pragma once

#include <QWidget>

class MissionController;
class NetworkManager;
class ScenarioModel;
class SimulationEngine;
class ModeSelector;
class DeviceStatusWidget;
class MainLobeLampWidget;
class SimControlWidget;
class SpeedSelector;

// 우측 제어 패널: 모드 / 장치 연결 상태 / MainLobe 램프 / 시뮬레이션 제어 / 배속
class ControlPanel : public QWidget {
    Q_OBJECT
public:
    ControlPanel(MissionController& mission, SimulationEngine& engine, ScenarioModel& model,
                 NetworkManager* network = nullptr, QWidget* parent = nullptr);

    DeviceStatusWidget* deviceStatus() const { return m_devices; }

private:
    void onStart();
    void onRefresh();
    void syncFromController();

    MissionController& m_mission;
    SimulationEngine& m_engine;
    ModeSelector* m_mode;
    DeviceStatusWidget* m_devices;
    MainLobeLampWidget* m_lamps;
    SimControlWidget* m_control;
    SpeedSelector* m_speed;
};
