#include "MainLobeNotifyMsg.h"

void MainLobeNotifyMsg::writePayload(ByteWriter& w) const
{
    w.u32(radarId);
    w.f64(searchTime);
    w.u8(quint8(detections.size()));
    for (const GeoPoint& p : detections) {
        w.f64(p.lat);
        w.f64(p.lon);
    }
}

bool MainLobeNotifyMsg::readPayload(ByteReader& r)
{
    radarId = r.u32();
    searchTime = r.f64();
    detections.clear();
    const int n = r.u8();
    for (int i = 0; i < n && r.ok(); ++i) {
        const double lat = r.f64(), lon = r.f64();
        detections.append({lat, lon});
    }
    return r.ok();
}

QString MainLobeNotifyMsg::summary() const
{
    return QStringLiteral("MainLobeNotify radar=#%1 t=%2 detections=%3")
        .arg(radarId)
        .arg(searchTime, 0, 'f', 2)
        .arg(detections.size());
}
