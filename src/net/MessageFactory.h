#pragma once

#include "IMessage.h"

#include <QHash>

#include <functional>

class ProtocolTable;

// MessageId → 생성자 레지스트리.
// 수신 시 (Src, Dst, Cmd) → ProtocolTable → MessageId → 생성 → readPayload. 미등록/매핑없음은 RawMessage
class MessageFactory {
public:
    struct DecodeResult {
        MessagePtr message;     // 실패해도 RawMessage 로 채워짐
        bool known = false;     // 매핑·등록된 메시지
        bool parsed = false;    // readPayload 성공 (known 일 때만 의미)
    };

    template <typename T>
    void registerType(MessageId id)
    {
        m_creators.insert(id, [] { return MessagePtr(new T); });
    }

    bool isRegistered(MessageId id) const { return m_creators.contains(id); }
    MessagePtr create(MessageId id) const;
    DecodeResult decode(quint32 opcode, const QByteArray& payload, const ProtocolTable& table) const;

    // 모든 메시지 클래스 등록 (MessageRegistry.cpp)
    static void registerAll(MessageFactory& factory);

private:
    QHash<MessageId, std::function<MessagePtr()>> m_creators;
};
