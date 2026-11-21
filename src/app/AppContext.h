#pragma once

#include <QObject>

class Logger;
class LogTableModel;

// 애플리케이션 전역 객체 소유자. (M1: Logger, LogTableModel
//  → 이후 ScenarioModel, SimulationEngine, NetworkManager, MissionController 추가)
class AppContext : public QObject {
    Q_OBJECT
public:
    explicit AppContext(QObject* parent = nullptr);
    ~AppContext() override;

    Logger& logger() const { return *m_logger; }
    LogTableModel& logModel() const { return *m_logModel; }

private:
    Logger* m_logger;
    LogTableModel* m_logModel;
};
