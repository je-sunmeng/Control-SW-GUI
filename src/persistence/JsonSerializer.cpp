#include "JsonSerializer.h"

#include <QJsonArray>
#include <algorithm>
#include <iterator>

namespace JsonSerializer {

namespace {

bool fail(QString* error, const QString& msg)
{
    if (error)
        *error = msg;
    return false;
}

// 숫자 필드: 없으면 기본값 유지, 있으면 숫자여야 함
bool readNumber(const QJsonObject& o, const char* key, double& out, QString* error)
{
    const QJsonValue v = o.value(QLatin1String(key));
    if (v.isUndefined() || v.isNull())
        return true;
    if (!v.isDouble())
        return fail(error, QStringLiteral("'%1' 은(는) 숫자여야 합니다").arg(QLatin1String(key)));
    out = v.toDouble();
    return true;
}

bool readInt(const QJsonObject& o, const char* key, int& out, QString* error)
{
    double d = out;
    if (!readNumber(o, key, d, error))
        return false;
    out = int(d);
    return true;
}

} // namespace

// ---------------------------------------------------------------------------- enum

QString toString(ScanDirection d)
{
    return d == ScanDirection::Clockwise ? QStringLiteral("CW") : QStringLiteral("CCW");
}

QString toString(FalseTargetType t)
{
    switch (t) {
    case FalseTargetType::Fixed:  return QStringLiteral("Fixed");
    case FalseTargetType::Moving: return QStringLiteral("Moving");
    case FalseTargetType::Random: return QStringLiteral("Random");
    case FalseTargetType::Noise:  return QStringLiteral("Noise");
    }
    return {};
}

bool parse(const QString& s, ScanDirection& out)
{
    if (s.compare(QLatin1String("CW"), Qt::CaseInsensitive) == 0)
        out = ScanDirection::Clockwise;
    else if (s.compare(QLatin1String("CCW"), Qt::CaseInsensitive) == 0)
        out = ScanDirection::CounterClockwise;
    else
        return false;
    return true;
}

bool parse(const QString& s, FalseTargetType& out)
{
    for (FalseTargetType t : {FalseTargetType::Fixed, FalseTargetType::Moving, FalseTargetType::Random,
                              FalseTargetType::Noise}) {
        if (s.compare(toString(t), Qt::CaseInsensitive) == 0) {
            out = t;
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------- toJson

QJsonObject toJson(const GeoPoint& p)
{
    return {{QStringLiteral("lat"), p.lat}, {QStringLiteral("lon"), p.lon}};
}

QJsonObject toJson(const Route& route)
{
    QJsonArray wps;
    for (const GeoPoint& p : route.waypoints)
        wps.append(toJson(p));
    QJsonArray segs;
    for (const RouteSegment& s : route.segments)
        segs.append(QJsonObject{{QStringLiteral("altitudeKm"), s.altitudeKm}, {QStringLiteral("speedMps"), s.speedMps}});
    return {{QStringLiteral("waypoints"), wps}, {QStringLiteral("segments"), segs}};
}

QJsonObject toJson(const Radar& r)
{
    return {
        {QStringLiteral("name"), r.name},
        {QStringLiteral("lat"), r.pos.lat},
        {QStringLiteral("lon"), r.pos.lon},
        {QStringLiteral("freqMHz"), r.freqMHz},
        {QStringLiteral("priMs"), r.priMs},
        {QStringLiteral("pwUs"), r.pwUs},
        {QStringLiteral("maxRangeKm"), r.maxRangeKm},
        {QStringLiteral("scanPeriodSec"), r.scanPeriodSec},
        {QStringLiteral("scanDir"), toString(r.scanDir)},
        {QStringLiteral("beamWidthDeg"), r.beamWidthDeg},
    };
}

QJsonObject toJson(const JammerAircraft& j)
{
    return {{QStringLiteral("name"), j.name}, {QStringLiteral("route"), toJson(j.route)}};
}

QJsonObject toJson(const FriendlyFormation& f)
{
    QJsonArray targets;
    for (const FalseTargetOffset& t : f.jamming.targets)
        targets.append(QJsonObject{{QStringLiteral("angleDeg"), t.angleDeg}, {QStringLiteral("distanceKm"), t.distanceKm}});
    return {
        {QStringLiteral("name"), f.name},
        {QStringLiteral("formationSize"), f.formationSize},
        {QStringLiteral("route"), toJson(f.route)},
        {QStringLiteral("jamming"), QJsonObject{{QStringLiteral("falseTargets"), targets},
                                                {QStringLiteral("noiseBandwidthMHz"), f.jamming.noiseBandwidthMHz}}},
    };
}

QJsonObject toJson(const StandaloneFalseTarget& t)
{
    QJsonObject o{{QStringLiteral("name"), t.name}, {QStringLiteral("type"), toString(t.ftType)}};
    if (t.usesPosition() && t.posSet) {
        o.insert(QStringLiteral("lat"), t.pos.lat);
        o.insert(QStringLiteral("lon"), t.pos.lon);
    }
    if (t.usesRoute())
        o.insert(QStringLiteral("route"), toJson(t.route));
    if (t.ftType == FalseTargetType::Noise)
        o.insert(QStringLiteral("detectAngleDeg"), t.detectAngleDeg);
    return o;
}

// ---------------------------------------------------------------------------- fromJson

bool fromJson(const QJsonObject& o, GeoPoint& out, QString* error)
{
    if (!o.value(QLatin1String("lat")).isDouble() || !o.value(QLatin1String("lon")).isDouble())
        return fail(error, QStringLiteral("위경도(lat/lon) 값이 없습니다"));
    const double lat = o.value(QLatin1String("lat")).toDouble();
    const double lon = o.value(QLatin1String("lon")).toDouble();
    if (lat < -90 || lat > 90 || lon < -180 || lon > 180)
        return fail(error, QStringLiteral("위경도 범위 오류 (%1, %2)").arg(lat).arg(lon));
    out = {lat, lon};
    return true;
}

bool fromJson(const QJsonObject& o, Route& out, QString* error)
{
    Route r;
    const QJsonArray wps = o.value(QLatin1String("waypoints")).toArray();
    for (int i = 0; i < wps.size(); ++i) {
        GeoPoint p;
        QString e;
        if (!fromJson(wps.at(i).toObject(), p, &e))
            return fail(error, QStringLiteral("웨이포인트 %1: %2").arg(i + 1).arg(e));
        r.waypoints.append(p);
    }
    const QJsonArray segs = o.value(QLatin1String("segments")).toArray();
    const int needed = std::max(0, int(r.waypoints.size()) - 1);
    for (int i = 0; i < needed; ++i) {
        RouteSegment s = i > 0 ? r.segments.last() : RouteSegment{};
        if (i < segs.size()) {
            const QJsonObject so = segs.at(i).toObject();
            if (!readNumber(so, "altitudeKm", s.altitudeKm, error) || !readNumber(so, "speedMps", s.speedMps, error))
                return false;
        }
        r.segments.append(Route::clamp(s));
    }
    out = r;
    return true;
}

bool fromJson(const QJsonObject& o, Radar& out, QString* error)
{
    Radar r;
    r.name = o.value(QLatin1String("name")).toString();
    if (!fromJson(o, r.pos, error))
        return false;
    if (!readNumber(o, "freqMHz", r.freqMHz, error) || !readNumber(o, "priMs", r.priMs, error)
        || !readNumber(o, "pwUs", r.pwUs, error) || !readNumber(o, "maxRangeKm", r.maxRangeKm, error)
        || !readNumber(o, "scanPeriodSec", r.scanPeriodSec, error)
        || !readNumber(o, "beamWidthDeg", r.beamWidthDeg, error))
        return false;
    const QJsonValue dir = o.value(QLatin1String("scanDir"));
    if (!dir.isUndefined() && !parse(dir.toString(), r.scanDir))
        return fail(error, QStringLiteral("scanDir 은 CW 또는 CCW 여야 합니다"));
    r.clampValues();
    out = r;
    return true;
}

bool fromJson(const QJsonObject& o, JammerAircraft& out, QString* error)
{
    JammerAircraft j;
    j.name = o.value(QLatin1String("name")).toString();
    if (!fromJson(o.value(QLatin1String("route")).toObject(), j.route, error))
        return false;
    out = j;
    return true;
}

bool fromJson(const QJsonObject& o, FriendlyFormation& out, QString* error)
{
    FriendlyFormation f;
    f.name = o.value(QLatin1String("name")).toString();
    if (!readInt(o, "formationSize", f.formationSize, error))
        return false;
    if (f.formationSize < Limits::MinFormationSize || f.formationSize > Limits::MaxFormationSize)
        return fail(error, QStringLiteral("편대규모는 %1~%2 입니다").arg(Limits::MinFormationSize).arg(Limits::MaxFormationSize));
    if (!fromJson(o.value(QLatin1String("route")).toObject(), f.route, error))
        return false;
    const QJsonObject jam = o.value(QLatin1String("jamming")).toObject();
    const QJsonArray targets = jam.value(QLatin1String("falseTargets")).toArray();
    if (targets.size() > Limits::MaxFormationFalseTargets)
        return fail(error, QStringLiteral("허위타겟은 최대 %1개입니다").arg(Limits::MaxFormationFalseTargets));
    for (const QJsonValue& v : targets) {
        FalseTargetOffset t;
        const QJsonObject to = v.toObject();
        if (!readNumber(to, "angleDeg", t.angleDeg, error) || !readNumber(to, "distanceKm", t.distanceKm, error))
            return false;
        f.jamming.targets.append(t);
    }
    if (!readNumber(jam, "noiseBandwidthMHz", f.jamming.noiseBandwidthMHz, error))
        return false;
    f.jamming.clampValues();
    out = f;
    return true;
}

bool fromJson(const QJsonObject& o, StandaloneFalseTarget& out, QString* error)
{
    StandaloneFalseTarget t;
    t.name = o.value(QLatin1String("name")).toString();
    if (!parse(o.value(QLatin1String("type")).toString(), t.ftType))
        return fail(error, QStringLiteral("type 은 Fixed/Moving/Random/Noise 중 하나여야 합니다"));
    if (t.usesPosition() && o.contains(QLatin1String("lat"))) {
        if (!fromJson(o, t.pos, error))
            return false;
        t.posSet = true;
    }
    if (t.usesRoute() && !fromJson(o.value(QLatin1String("route")).toObject(), t.route, error))
        return false;
    if (!readNumber(o, "detectAngleDeg", t.detectAngleDeg, error))
        return false;
    t.detectAngleDeg = qBound(FalseTargetLimits::MinDetectAngleDeg, t.detectAngleDeg, FalseTargetLimits::MaxDetectAngleDeg);
    out = t;
    return true;
}

} // namespace JsonSerializer
