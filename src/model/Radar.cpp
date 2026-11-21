#include "Radar.h"

#include "common/Constants.h"

#include <QtGlobal>

void Radar::clampValues()
{
    using namespace RadarLimits;
    pos.lat = qBound(-90.0, pos.lat, 90.0);
    pos.lon = qBound(-180.0, pos.lon, 180.0);
    freqMHz = qBound(MinFreqMHz, freqMHz, MaxFreqMHz);
    priMs = qBound(MinPriMs, priMs, MaxPriMs);
    pwUs = qBound(MinPwUs, pwUs, MaxPwUs);
    maxRangeKm = qBound(MinRangeKm, maxRangeKm, MaxRangeKm);
    scanPeriodSec = qBound(MinScanPeriodSec, scanPeriodSec, MaxScanPeriodSec);
    beamWidthDeg = qBound(MinBeamWidthDeg, beamWidthDeg, MaxBeamWidthDeg);
    colorIndex = qBound(0, colorIndex, Limits::MaxRadars - 1);
}
