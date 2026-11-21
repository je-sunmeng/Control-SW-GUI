#include "ByteWriter.h"

#include <QtEndian>

#include <cstring>

namespace {
template <typename T>
void put(QByteArray& buf, T v)
{
    const T be = qToBigEndian(v);
    buf.append(reinterpret_cast<const char*>(&be), sizeof(T));
}
} // namespace

void ByteWriter::u8(quint8 v)
{
    m_buf.append(char(v));
}

void ByteWriter::u16(quint16 v)
{
    put(m_buf, v);
}

void ByteWriter::u32(quint32 v)
{
    put(m_buf, v);
}

void ByteWriter::i32(qint32 v)
{
    put(m_buf, v);
}

void ByteWriter::f32(double v)
{
    const float f = float(v);
    quint32 bits;
    std::memcpy(&bits, &f, sizeof bits);
    put(m_buf, bits);
}

void ByteWriter::f64(double v)
{
    quint64 bits;
    std::memcpy(&bits, &v, sizeof bits);
    put(m_buf, bits);
}

void ByteWriter::str(const QString& s)
{
    QByteArray utf8 = s.toUtf8();
    if (utf8.size() > 0xFFFF)
        utf8.truncate(0xFFFF);
    u16(quint16(utf8.size()));
    m_buf.append(utf8);
}

void ByteWriter::bytes(const QByteArray& b)
{
    m_buf.append(b);
}
