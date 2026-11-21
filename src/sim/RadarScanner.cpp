#include "RadarScanner.h"

#include "common/GeoUtils.h"

void RadarScanner::reset()
{
    m_states.clear();
}

void RadarScanner::setRadar(ObjectId radarId, const Params& p)
{
    m_states[radarId].params = p;
}

void RadarScanner::removeRadar(ObjectId radarId)
{
    m_states.remove(radarId);
}

void RadarScanner::onMainLobe(ObjectId radarId, double t, double azimuthDeg)
{
    auto it = m_states.find(radarId);
    if (it == m_states.end())
        return;
    it->synced = true;
    it->t0 = t;
    it->az0 = GeoUtils::normalizeDeg(azimuthDeg);
}

bool RadarScanner::isSynced(ObjectId radarId) const
{
    return m_states.value(radarId).synced;
}

double RadarScanner::lastSyncTime(ObjectId radarId) const
{
    const auto it = m_states.constFind(radarId);
    return it != m_states.cend() && it->synced ? it->t0 : -1.0;
}

double RadarScanner::beamAzimuthAt(ObjectId radarId, double t) const
{
    const auto it = m_states.constFind(radarId);
    if (it == m_states.cend() || !it->synced || it->params.scanPeriodSec <= 0)
        return 0.0;
    const double sweep = 360.0 * (t - it->t0) / it->params.scanPeriodSec;
    const double sign = it->params.dir == ScanDirection::Clockwise ? 1.0 : -1.0;
    return GeoUtils::normalizeDeg(it->az0 + sign * sweep);
}
