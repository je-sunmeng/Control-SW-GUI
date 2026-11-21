#include "PositionMsg.h"

void PositionMsgBase::writePayload(ByteWriter& w) const
{
    w.f64(simTime);
    w.f64(pos.lat);
    w.f64(pos.lon);
    w.f32(altKm);
    w.f32(headingDeg);
}

bool PositionMsgBase::readPayload(ByteReader& r)
{
    simTime = r.f64();
    pos.lat = r.f64();
    pos.lon = r.f64();
    altKm = r.f32();
    headingDeg = r.f32();
    return r.ok();
}

QString PositionMsgBase::summary() const
{
    return QStringLiteral("%1 t=%2 (%3, %4) alt=%5km hdg=%6°")
        .arg(MessageIds::toString(id()))
        .arg(simTime, 0, 'f', 1)
        .arg(pos.lat, 0, 'f', 4)
        .arg(pos.lon, 0, 'f', 4)
        .arg(altKm, 0, 'f', 1)
        .arg(headingDeg, 0, 'f', 0);
}
