#include "FalseTargetInfoMsg.h"

#include "RouteCodec.h"

void FalseTargetInfoMsg::writePayload(ByteWriter& w) const
{
    w.u8(quint8(targets.size()));
    for (const StandaloneFalseTarget& t : targets) {
        w.u32(t.id);
        w.u8(quint8(t.ftType));
        switch (t.ftType) {
        case FalseTargetType::Fixed:
            w.f64(t.pos.lat);
            w.f64(t.pos.lon);
            break;
        case FalseTargetType::Moving:
            RouteCodec::write(w, t.route);
            break;
        case FalseTargetType::Random:
            break;
        case FalseTargetType::Noise:
            w.f64(t.pos.lat);
            w.f64(t.pos.lon);
            w.f32(t.detectAngleDeg);
            break;
        }
    }
}

bool FalseTargetInfoMsg::readPayload(ByteReader& r)
{
    targets.clear();
    const int n = r.u8();
    for (int i = 0; i < n && r.ok(); ++i) {
        StandaloneFalseTarget t;
        t.id = r.u32();
        const quint8 type = r.u8();
        if (type > quint8(FalseTargetType::Noise))
            return false;
        t.ftType = FalseTargetType(type);
        switch (t.ftType) {
        case FalseTargetType::Fixed:
            t.pos = {r.f64(), r.f64()};
            t.posSet = true;
            break;
        case FalseTargetType::Moving:
            t.route = RouteCodec::read(r);
            break;
        case FalseTargetType::Random:
            break;
        case FalseTargetType::Noise:
            t.pos.lat = r.f64();
            t.pos.lon = r.f64();
            t.detectAngleDeg = r.f32();
            t.posSet = true;
            break;
        }
        targets.append(t);
    }
    return r.ok();
}

QString FalseTargetInfoMsg::summary() const
{
    int counts[4] = {};
    for (const StandaloneFalseTarget& t : targets)
        ++counts[int(t.ftType)];
    return QStringLiteral("FalseTargetInfo count=%1 (고정 %2, 이동 %3, 랜덤 %4, 잡음 %5)")
        .arg(targets.size())
        .arg(counts[0])
        .arg(counts[1])
        .arg(counts[2])
        .arg(counts[3]);
}
