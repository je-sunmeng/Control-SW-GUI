#include "RadarInfoMsg.h"

void RadarInfoMsg::writePayload(ByteWriter& w) const
{
    w.u8(quint8(radars.size()));
    for (const Radar& r : radars) {
        w.u32(r.id);
        w.f64(r.pos.lat);
        w.f64(r.pos.lon);
        w.f32(r.freqMHz);
        w.f32(r.priMs);
        w.f32(r.pwUs);
        w.f32(r.maxRangeKm);
        w.f32(r.scanPeriodSec);
        w.u8(quint8(r.scanDir));
        w.f32(r.beamWidthDeg);
    }
}

bool RadarInfoMsg::readPayload(ByteReader& r)
{
    radars.clear();
    const int n = r.u8();
    for (int i = 0; i < n && r.ok(); ++i) {
        Radar x;
        x.id = r.u32();
        x.pos.lat = r.f64();
        x.pos.lon = r.f64();
        x.freqMHz = r.f32();
        x.priMs = r.f32();
        x.pwUs = r.f32();
        x.maxRangeKm = r.f32();
        x.scanPeriodSec = r.f32();
        x.scanDir = r.u8() ? ScanDirection::CounterClockwise : ScanDirection::Clockwise;
        x.beamWidthDeg = r.f32();
        radars.append(x);
    }
    return r.ok();
}

QString RadarInfoMsg::summary() const
{
    QStringList items;
    for (const Radar& r : radars)
        items << QStringLiteral("#%1 %2MHz %3km").arg(r.id).arg(r.freqMHz, 0, 'f', 0).arg(r.maxRangeKm, 0, 'f', 0);
    return QStringLiteral("RadarInfo radars=%1 [%2]").arg(radars.size()).arg(items.join(QStringLiteral(", ")));
}
