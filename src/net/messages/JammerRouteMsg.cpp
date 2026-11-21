#include "JammerRouteMsg.h"

#include "RouteCodec.h"

void JammerRouteMsg::writePayload(ByteWriter& w) const
{
    RouteCodec::write(w, route);
}

bool JammerRouteMsg::readPayload(ByteReader& r)
{
    route = RouteCodec::read(r);
    return r.ok();
}

QString JammerRouteMsg::summary() const
{
    return QStringLiteral("JammerRoute wp=%1 %2km").arg(route.waypointCount()).arg(route.totalLengthKm(), 0, 'f', 1);
}
