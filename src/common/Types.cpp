#include "Types.h"

#include <QString>

QString toDisplayString(SimState state)
{
    switch (state) {
    case SimState::PreMission: return QStringLiteral("임무 전");
    case SimState::Running:    return QStringLiteral("임무 중");
    case SimState::Paused:     return QStringLiteral("일시정지");
    case SimState::Stopped:    return QStringLiteral("중지");
    }
    return {};
}

QString toDisplayString(DeviceCode device)
{
    switch (device) {
    case DeviceCode::ControlSW:  return QStringLiteral("제어SW");
    case DeviceCode::AnalysisSW: return QStringLiteral("분석SW");
    case DeviceCode::SignalGen:  return QStringLiteral("재밍기법발생기");
    }
    return QStringLiteral("0x%1").arg(quint8(device), 2, 16, QLatin1Char('0'));
}
