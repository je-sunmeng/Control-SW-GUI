#include "FrameDecoder.h"

FrameDecoder::FrameDecoder(quint32 maxPayload)
    : m_maxPayload(maxPayload)
{
}

void FrameDecoder::reset()
{
    m_buf.clear();
}

QVector<FrameDecoder::Frame> FrameDecoder::feed(const QByteArray& chunk)
{
    QVector<Frame> out;
    m_buf.append(chunk);
    int pos = 0;
    while (m_buf.size() - pos >= MessageHeader::Size) {
        const MessageHeader h = *MessageHeader::decode(m_buf.mid(pos, MessageHeader::Size));
        if (h.dataSize > m_maxPayload) {
            ++m_errors;
            m_lastError = QStringLiteral("DataSize %1 이(가) 최대 %2 바이트를 초과 — 수신 버퍼 리셋").arg(h.dataSize).arg(m_maxPayload);
            m_buf.clear();
            return out;
        }
        const qsizetype total = MessageHeader::Size + qsizetype(h.dataSize);
        if (m_buf.size() - pos < total)
            break;   // payload 미완성 → 다음 수신 대기
        Frame f;
        f.header = h;
        f.raw = m_buf.mid(pos, total);
        f.payload = f.raw.mid(MessageHeader::Size);
        out.append(f);
        pos += int(total);
    }
    if (pos > 0)
        m_buf.remove(0, pos);
    return out;
}
