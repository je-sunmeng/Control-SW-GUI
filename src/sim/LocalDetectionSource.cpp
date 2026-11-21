#include "LocalDetectionSource.h"

#include "common/GeoUtils.h"

void LocalDetectionSource::reset()
{
    m_prev.clear();
}

bool LocalDetectionSource::inNoiseSector(double targetBearing, const NoiseSector& s)
{
    return std::abs(GeoUtils::angleDiffDeg(targetBearing, s.centerDeg)) <= s.widthDeg / 2.0;
}

bool LocalDetectionSource::sweptOver(double prevAz, double curAz, double az, ScanDirection dir, double maxSweepDeg)
{
    const bool cw = dir == ScanDirection::Clockwise;
    const double sweep = GeoUtils::normalizeDeg(cw ? curAz - prevAz : prevAz - curAz);
    if (sweep <= 0.0 || sweep > maxSweepDeg)
        return false;
    const double off = GeoUtils::normalizeDeg(cw ? az - prevAz : prevAz - az);
    return off > 0.0 && off <= sweep;
}

QVector<Detection> LocalDetectionSource::collect(const SimContext& ctx, double simTime)
{
    QVector<Detection> out;
    if (!ctx.radars)
        return out;
    for (int i = 0; i < ctx.radars->size() && i < ctx.beams.size(); ++i) {
        const Radar& radar = ctx.radars->at(i);
        const BeamState& beam = ctx.beams[i];
        BeamMemory& mem = m_prev[radar.id];
        if (!beam.synced) {
            mem.valid = false;
            continue;
        }
        if (!mem.valid) {
            mem = {beam.azimuthDeg, true};
            continue;
        }
        const double prev = mem.az;
        mem.az = beam.azimuthDeg;
        for (const DetectableTarget& tg : ctx.targets) {
            if (GeoUtils::distanceKm(radar.pos, tg.pos) > radar.maxRangeKm)
                continue;
            const double brg = GeoUtils::bearingDeg(radar.pos, tg.pos);
            // 한 틱에 반 바퀴 이상 도는 경우(비정상 지연)는 판정 생략
            if (!sweptOver(prev, beam.azimuthDeg, brg, radar.scanDir, 180.0))
                continue;
            bool jammed = false;
            for (const NoiseSector& s : ctx.noiseSectors) {
                if (s.radarId == radar.id && inNoiseSector(brg, s)) {
                    jammed = true;
                    break;
                }
            }
            if (jammed)
                continue;
            out.append({radar.id, tg.key, tg.pos, simTime, DetectionOrigin::Local});
        }
    }
    return out;
}
