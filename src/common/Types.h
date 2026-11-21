#pragma once

#include <QtGlobal>
#include <QMetaType>
#include <QString>

struct GeoPoint {
    double lat = 0.0; // deg, WGS84
    double lon = 0.0; // deg, WGS84
};
Q_DECLARE_METATYPE(GeoPoint)

enum class ObjectType { Radar, Jammer, Formation, StandaloneFalseTarget };

enum class FalseTargetType : quint8 { Fixed = 0, Moving = 1, Random = 2, Noise = 3 };

enum class ScanDirection : quint8 { Clockwise = 0, CounterClockwise = 1 };

enum class RunMode { Standalone, Linked };

// 임무 전 / 임무 중 / 일시정지(단독모드 전용) / 중지
enum class SimState { PreMission, Running, Paused, Stopped };

enum class DeviceCode : quint8 {
    ControlSW  = 0x21,
    AnalysisSW = 0x92,
    SignalGen  = 0xB1,
};

using ObjectId = quint32;

QString toDisplayString(SimState state);
QString toDisplayString(DeviceCode device);
