#include "MissionStartMsg.h"

void MissionStartMsg::writePayload(ByteWriter& w) const
{
    if (!withStartPositions)
        return;
    w.f64(formationStart.lat);
    w.f64(formationStart.lon);
    w.f64(jammerStart.lat);
    w.f64(jammerStart.lon);
}

bool MissionStartMsg::readPayload(ByteReader& r)
{
    withStartPositions = !r.atEnd();
    if (withStartPositions) {
        formationStart = {r.f64(), r.f64()};
        jammerStart = {r.f64(), r.f64()};
    }
    return r.ok();
}

QString MissionStartMsg::summary() const
{
    if (!withStartPositions)
        return QStringLiteral("MissionStart");
    return QStringLiteral("MissionStart formation=(%1, %2) jammer=(%3, %4)")
        .arg(formationStart.lat, 0, 'f', 4)
        .arg(formationStart.lon, 0, 'f', 4)
        .arg(jammerStart.lat, 0, 'f', 4)
        .arg(jammerStart.lon, 0, 'f', 4);
}
