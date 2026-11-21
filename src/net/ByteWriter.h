#pragma once

#include <QByteArray>
#include <QString>

// BigEndian 직렬화. 실수는 IEEE-754 비트 패턴을 BigEndian 으로 기록 (f32/f64 명시)
// 문자열: u16 길이 + UTF-8
class ByteWriter {
public:
    void u8(quint8 v);
    void u16(quint16 v);
    void u32(quint32 v);
    void i32(qint32 v);
    void f32(double v);
    void f64(double v);
    void str(const QString& s);
    void bytes(const QByteArray& b);

    const QByteArray& data() const { return m_buf; }
    int size() const { return int(m_buf.size()); }

private:
    QByteArray m_buf;
};
