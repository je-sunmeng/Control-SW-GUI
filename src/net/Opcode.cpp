#include "Opcode.h"

#include "log/LogEntry.h"

namespace Opcode {

QString toString(quint32 op)
{
    return formatOpcode(op);
}

} // namespace Opcode
