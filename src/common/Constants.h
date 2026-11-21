#pragma once

#include <QtGlobal>

namespace Limits {
inline constexpr int MaxRadars = 4;
inline constexpr int MaxFormations = 1;
inline constexpr int MinFormationSize = 1;
inline constexpr int MaxFormationSize = 4;
inline constexpr int MaxFormationFalseTargets = 8;
} // namespace Limits

namespace Defaults {
inline constexpr double KoreaCenterLat = 37.5;
inline constexpr double KoreaCenterLon = 127.5;
inline constexpr double DatalinkIntervalSec = 1.0;
inline constexpr double JammerPositionIntervalSec = 1.0;
inline constexpr quint16 ServerPort = 50021; // 협의 후 변경
} // namespace Defaults

namespace Net {
inline constexpr int HeaderSize = 8;
inline constexpr quint32 MaxPayloadSize = 16u * 1024u * 1024u;
} // namespace Net
