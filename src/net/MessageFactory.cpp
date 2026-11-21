#include "MessageFactory.h"

#include "ProtocolTable.h"
#include "messages/RawMessage.h"

MessagePtr MessageFactory::create(MessageId id) const
{
    const auto it = m_creators.constFind(id);
    return it == m_creators.cend() ? MessagePtr() : (*it)();
}

MessageFactory::DecodeResult MessageFactory::decode(quint32 opcode, const QByteArray& payload,
                                                    const ProtocolTable& table) const
{
    DecodeResult res;
    if (const auto id = table.idForOpcode(opcode)) {
        if (MessagePtr msg = create(*id)) {
            res.known = true;
            ByteReader r(payload);
            res.parsed = msg->readPayload(r) && r.finished();
            if (res.parsed) {
                res.message = msg;
                return res;
            }
        }
    }
    auto raw = QSharedPointer<RawMessage>::create();
    raw->opcode = opcode;
    raw->data = payload;
    res.message = raw;
    return res;
}
