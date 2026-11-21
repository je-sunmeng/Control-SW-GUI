#pragma once

#include "common/Types.h"

#include <QMetaType>
#include <QString>
#include <QVector>

enum class DetectionOrigin : quint8 { Local, External };

// 한 틱의 결과 (전시/송신용 값 묶음)
struct PlatformState {
    ObjectId id = 0;
    ObjectType type = ObjectType::Jammer;   // Jammer / Formation / StandaloneFalseTarget(이동)
    GeoPoint pos;
    double altKm = 0.0;
    double headingDeg = 0.0;
    double speedMps = 0.0;
    bool finished = false;
    int formationSize = 1;
};

struct BeamState {
    ObjectId radarId = 0;
    double azimuthDeg = 0.0;
    bool synced = false;
};

struct Detection {
    ObjectId radarId = 0;
    QString targetKey;      // "FFT<i>" 편대 허위타겟 / "FT<id>" 단독표적
    GeoPoint pos;
    double simTime = 0.0;
    DetectionOrigin origin = DetectionOrigin::Local;
};

struct GhostState {
    quint32 key = 0;
    ObjectId radarId = 0;
    GeoPoint pos;
    double bornAt = 0.0;
    double lifetimeSec = 2.0;

    // 0~1: fade in(앞 25%) → 유지 → fade out(뒤 35%)
    double opacityAt(double t) const;
};

struct NoiseSector {
    ObjectId radarId = 0;
    ObjectId noiseId = 0;
    double centerDeg = 0.0;   // 레이다 → 잡음표적 방위
    double widthDeg = 0.0;    // 탐지각도 (±width/2)
};

struct SimSnapshot {
    double simTime = 0.0;
    SimState state = SimState::PreMission;
    QVector<PlatformState> platforms;
    QVector<BeamState> beams;
    QVector<GeoPoint> formationFalseTargets;
    QVector<Detection> detections;   // 이번 틱에 새로 발생한 탐지
    QVector<GhostState> ghosts;      // 현재 살아있는 랜덤 고스트
    QVector<NoiseSector> noiseSectors;
};
Q_DECLARE_METATYPE(SimSnapshot)
Q_DECLARE_METATYPE(PlatformState)
