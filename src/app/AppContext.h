#pragma once

#include "map/MapProjection.h"

#include <QObject>

class Logger;
class LogTableModel;
class MapLayerManager;
class ScenarioModel;

// 애플리케이션 전역 객체 소유자.
//  M1: Logger, LogTableModel / M2: MapProjection, MapLayerManager / M3: ScenarioModel
//  → 이후 SimulationEngine, NetworkManager, MissionController 추가
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

private:
    Logger* m_logger;
    LogTableModel* m_logModel;
    MapProjection m_projection;
    MapLayerManager* m_mapLayers;
    ScenarioModel* m_scenario;
};
