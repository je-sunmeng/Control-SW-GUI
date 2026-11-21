#pragma once

#include "common/Types.h"

#include <QHash>

// MainLobe 기준 빔 방위 계산
//   onMainLobe(radar, t, az0): t 시점에 빔이 az0 (= 레이다→전자전기 방위) 를 향함
//   beamAzimuthAt(t) = az0 ± 360·(t−t0)/scanPeriod   (CW:+, CCW:−), mod 360
class RadarScanner {
public:
    struct Params {
        double scanPeriodSec = 10.0;
        ScanDirection dir = ScanDirection::Clockwise;
    };

    void reset();
    void setRadar(ObjectId radarId, const Params& p);
    void removeRadar(ObjectId radarId);

    void onMainLobe(ObjectId radarId, double t, double azimuthDeg);

    bool isSynced(ObjectId radarId) const;
    double lastSyncTime(ObjectId radarId) const;   // 미동기화 시 -1
    double beamAzimuthAt(ObjectId radarId, double t) const;   // 미동기화 시 0

private:
    struct State {
        Params params;
        bool synced = false;
        double t0 = 0.0;
        double az0 = 0.0;
    };
    QHash<ObjectId, State> m_states;
};
