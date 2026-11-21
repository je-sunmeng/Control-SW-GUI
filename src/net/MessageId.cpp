#include "MessageId.h"

#include <QList>

namespace MessageIds {

namespace {
struct Name {
    MessageId id;
    const char* name;
};
const Name kNames[] = {
    {MessageId::RadarInfo, "RadarInfo"},
    {MessageId::JammerRoute, "JammerRoute"},
    {MessageId::FormationInfo, "FormationInfo"},
    {MessageId::FalseTargetInfo, "FalseTargetInfo"},
    {MessageId::MissionStart, "MissionStart"},
    {MessageId::MissionStop, "MissionStop"},
    {MessageId::JammerPosition, "JammerPosition"},
    {MessageId::FormationPosition, "FormationPosition"},
    {MessageId::MainLobeNotify, "MainLobeNotify"},
    {MessageId::Heartbeat, "Heartbeat"},
};
} // namespace

QString toString(MessageId id)
{
    if (id == MessageId::Raw)
        return QStringLiteral("Raw");
    for (const Name& n : kNames)
        if (n.id == id)
            return QString::fromLatin1(n.name);
    return QStringLiteral("Unknown");
}

std::optional<MessageId> fromString(const QString& s)
{
    for (const Name& n : kNames)
        if (s == QLatin1String(n.name))
            return n.id;
    return std::nullopt;
}

const QList<MessageId>& all()
{
    static const QList<MessageId> ids = [] {
        QList<MessageId> l;
        for (const Name& n : kNames)
            l << n.id;
        return l;
    }();
    return ids;
}

} // namespace MessageIds
