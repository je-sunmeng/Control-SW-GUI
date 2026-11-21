#pragma once

#include <QString>

#include <optional>

// 내부 논리 메시지 ID (Command Code 와 분리). 코드 전반은 이 ID 만 사용한다.
// 새 메시지 추가 시: 여기 1줄 + MessageId.cpp 이름 테이블 1줄 (docs/PROTOCOL_GUIDE.md 4.3)
enum class MessageId {
    RadarInfo,
    JammerRoute,
    FormationInfo,
    FalseTargetInfo,
    MissionStart,
    MissionStop,
    JammerPosition,
    FormationPosition,
    MainLobeNotify,
    Heartbeat,
    Raw,   // 미등록 Opcode 수신 (로그용)
};

namespace MessageIds {
QString toString(MessageId id);                         // protocol.json 의 "id" 문자열
std::optional<MessageId> fromString(const QString& s);
// protocol.json 매핑이 있어야 하는 메시지 목록 (Raw 제외)
const QList<MessageId>& all();
} // namespace MessageIds
