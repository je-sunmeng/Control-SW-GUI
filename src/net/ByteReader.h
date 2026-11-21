#pragma once

#include <QByteArray>
#include <QString>

// BigEndian 역직렬화. 언더런 시 ok()=false 가 되고 이후 읽기는 0 을 반환
class ByteReader {
public:
    explicit ByteReader(const QByteArray& data)
        : m_data(data)
    {
    }

    quint8 u8();
    quint16 u16();
    quint32 u32();
    qint32 i32();
    double f32();
    double f64();
    QString str();
    QByteArray bytes(int n);

    bool ok() const { return m_ok; }
    bool atEnd() const { return m_pos >= m_data.size(); }
    int remaining() const { return int(m_data.size()) - m_pos; }
    // 정상 완료: 언더런 없음 + 남은 바이트 없음
    bool finished() const { return m_ok && atEnd(); }

private:
    bool need(int n);

    QByteArray m_data;
    int m_pos = 0;
    bool m_ok = true;
};
