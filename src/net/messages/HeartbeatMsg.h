#pragma once

#include "net/IMessage.h"

// 양방향: 연결/식별 확인. 접속 직후 클라이언트가 1회 송신하면 장치 식별에 사용
//   deviceCode(u8) + status(u8, 0 = 정상)
class HeartbeatMsg : public IMessage {
public:
    MessageId id() const override { return MessageId::Heartbeat; }
    void writePayload(ByteWriter& w) const override
    {
        w.u8(deviceCode);
        w.u8(status);
    }
    bool readPayload(ByteReader& r) override
    {
        deviceCode = r.u8();
        status = r.u8();
        return r.ok();
    }
    QString summary() const override
    {
        return QStringLiteral("Heartbeat device=0x%1 status=%2").arg(deviceCode, 2, 16, QLatin1Char('0')).arg(status);
    }

    quint8 deviceCode = 0;
    quint8 status = 0;
};
