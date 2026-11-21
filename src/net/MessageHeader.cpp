#include "MessageHeader.h"

#include "ByteReader.h"
#include "ByteWriter.h"

QByteArray MessageHeader::encode() const
{
    ByteWriter w;
    w.u32(opcode);
    w.u32(dataSize);
    return w.data();
}

std::optional<MessageHeader> MessageHeader::decode(const QByteArray& data)
{
    if (data.size() < Size)
        return std::nullopt;
    ByteReader r(data.left(Size));
    MessageHeader h;
    h.opcode = r.u32();
    h.dataSize = r.u32();
    return h;
}

QByteArray MessageHeader::frame(quint32 opcode, const QByteArray& payload)
{
    QByteArray out = MessageHeader{opcode, quint32(payload.size())}.encode();
    out.append(payload);
    return out;
}
