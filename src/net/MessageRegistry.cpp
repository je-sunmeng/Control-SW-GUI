// 모든 메시지 클래스 등록 — 새 메시지 = 이 파일에 1줄 추가 (docs/PROTOCOL_GUIDE.md 4.3)
#include "MessageFactory.h"

#include "messages/FalseTargetInfoMsg.h"
#include "messages/FormationInfoMsg.h"
#include "messages/HeartbeatMsg.h"
#include "messages/JammerRouteMsg.h"
#include "messages/MainLobeNotifyMsg.h"
#include "messages/MissionStartMsg.h"
#include "messages/MissionStopMsg.h"
#include "messages/PositionMsg.h"
#include "messages/RadarInfoMsg.h"

void MessageFactory::registerAll(MessageFactory& f)
{
    f.registerType<RadarInfoMsg>(MessageId::RadarInfo);
    f.registerType<JammerRouteMsg>(MessageId::JammerRoute);
    f.registerType<FormationInfoMsg>(MessageId::FormationInfo);
    f.registerType<FalseTargetInfoMsg>(MessageId::FalseTargetInfo);
    f.registerType<MissionStartMsg>(MessageId::MissionStart);
    f.registerType<MissionStopMsg>(MessageId::MissionStop);
    f.registerType<JammerPositionMsg>(MessageId::JammerPosition);
    f.registerType<FormationPositionMsg>(MessageId::FormationPosition);
    f.registerType<MainLobeNotifyMsg>(MessageId::MainLobeNotify);
    f.registerType<HeartbeatMsg>(MessageId::Heartbeat);
}
