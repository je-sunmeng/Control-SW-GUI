#pragma once

#include "common/Types.h"
#include "net/IMessage.h"

// 임무시작
//  분석SW 행: payload 없음
//  신호발생 행(withStartPositions): formationLat(f64), formationLon(f64), jammerLat(f64), jammerLon(f64)
//   (아군편대가 없으면 편대 위치 = 0,0 + hasFormation 플래그 없음 → ICD 협의 대상)
// 설계서 2.1 의 MissionStartMsg / MissionStartSigGenMsg 를 목적지 플래그로 통합
class MissionStartMsg : public IMessage {
public:
    MessageId id() const override { return MessageId::MissionStart; }
    void writePayload(ByteWriter& w) const override;
    bool readPayload(ByteReader& r) override;
    QString summary() const override;

    bool withStartPositions = false;
    GeoPoint formationStart;
    GeoPoint jammerStart;
};
