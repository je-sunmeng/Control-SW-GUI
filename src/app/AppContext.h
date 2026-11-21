#pragma once

#include "map/MapProjection.h"
#include "net/ProtocolTable.h"

#include <QObject>

class Logger;
class LogTableModel;
class MapLayerManager;
class ScenarioModel;
class SimulationEngine;
class MissionController;
class NetworkManager;

// 애플리케이션 전역 객체 소유자.
//  M1: Logger, LogTableModel / M2: MapProjection, MapLayerManager / M3: ScenarioModel
//  M4: SimulationEngine, MissionController / M5: ProtocolTable, NetworkManager
class AppContext : public QObject {
    Q_OBJECT
public:
    explicit AppContext(QObject* parent = nullptr);
    ~AppContext() override;

    Logger& logger() const { return *m_logger; }
    LogTableModel& logModel() const { return *m_logModel; }
    const MapProjection& projection() const { return m_projection; }
    MapLayerManager& mapLayers() const { return *m_mapLayers; }
    ScenarioModel& scenario() const { return *m_scenario; }
    SimulationEngine& engine() const { return *m_engine; }
    MissionController& mission() const { return *m_mission; }
    NetworkManager& network() const { return *m_network; }
    const ProtocolTable& protocol() const { return m_protocol; }

private:
    Logger* m_logger;
    LogTableModel* m_logModel;
    MapProjection m_projection;
    MapLayerManager* m_mapLayers;
    ScenarioModel* m_scenario;
    SimulationEngine* m_engine;
    ProtocolTable m_protocol;
    NetworkManager* m_network;
    MissionController* m_mission;
};
