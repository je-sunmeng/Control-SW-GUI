#include "FormationInfoMsg.h"

#include "RouteCodec.h"

void FormationInfoMsg::writePayload(ByteWriter& w) const
{
    RouteCodec::write(w, formation.route);
    w.u8(quint8(formation.formationSize));
    w.u8(includeJamming ? 1 : 0);
    if (!includeJamming)
        return;
    w.u8(quint8(formation.jamming.targets.size()));
    for (const FalseTargetOffset& t : formation.jamming.targets) {
        w.f32(t.angleDeg);
        w.f32(t.distanceKm);
    }
    w.f32(formation.jamming.noiseBandwidthMHz);
}

bool FormationInfoMsg::readPayload(ByteReader& r)
{
    formation.route = RouteCodec::read(r);
    formation.formationSize = r.u8();
    includeJamming = r.u8() != 0;
    formation.jamming.targets.clear();
    if (includeJamming) {
        const int n = r.u8();
        for (int i = 0; i < n && r.ok(); ++i) {
            FalseTargetOffset t;
            t.angleDeg = r.f32();
            t.distanceKm = r.f32();
            formation.jamming.targets.append(t);
        }
        formation.jamming.noiseBandwidthMHz = r.f32();
    }
    return r.ok();
}

QString FormationInfoMsg::summary() const
{
    QString s = QStringLiteral("FormationInfo wp=%1 size=%2").arg(formation.route.waypointCount()).arg(formation.formationSize);
    if (includeJamming)
        s += QStringLiteral(" falseTargets=%1 noiseBW=%2MHz")
                 .arg(formation.jamming.targets.size())
                 .arg(formation.jamming.noiseBandwidthMHz, 0, 'f', 1);
    return s;
}
