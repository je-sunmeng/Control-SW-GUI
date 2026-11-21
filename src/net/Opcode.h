#pragma once

#include <QString>
#include <QtGlobal>

// Opcode(32) = Src(8) | Dst(8) | CommandCode(16). 값 자체는 보유하지 않음 (ProtocolTable 에서 결정)
namespace Opcode {

constexpr quint32 make(quint8 src, quint8 dst, quint16 cmd)
{
    return (quint32(src) << 24) | (quint32(dst) << 16) | quint32(cmd);
}
constexpr quint8 src(quint32 op) { return quint8(op >> 24); }
constexpr quint8 dst(quint32 op) { return quint8(op >> 16); }
constexpr quint16 cmd(quint32 op) { return quint16(op & 0xFFFFu); }

QString toString(quint32 op);   // "0x2192 0010"

} // namespace Opcode
