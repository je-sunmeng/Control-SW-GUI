#include "ByteReader.h"

#include <QtEndian>

#include <cstring>

bool ByteReader::need(int n)
{
    if (!m_ok || n < 0 || m_pos + n > m_data.size()) {
        m_ok = false;
        return false;
    }
    return true;
}

quint8 ByteReader::u8()
{
    if (!need(1))
        return 0;
    return quint8(m_data[m_pos++]);
}

quint16 ByteReader::u16()
{
    if (!need(2))
        return 0;
    const quint16 v = qFromBigEndian<quint16>(m_data.constData() + m_pos);
    m_pos += 2;
    return v;
}

quint32 ByteReader::u32()
{
    if (!need(4))
        return 0;
    const quint32 v = qFromBigEndian<quint32>(m_data.constData() + m_pos);
    m_pos += 4;
    return v;
}

qint32 ByteReader::i32()
{
    return qint32(u32());
}

double ByteReader::f32()
{
    const quint32 bits = u32();
    float f;
    std::memcpy(&f, &bits, sizeof f);
    return m_ok ? double(f) : 0.0;
}

double ByteReader::f64()
{
    if (!need(8))
        return 0.0;
    const quint64 bits = qFromBigEndian<quint64>(m_data.constData() + m_pos);
    m_pos += 8;
    double d;
    std::memcpy(&d, &bits, sizeof d);
    return d;
}

QString ByteReader::str()
{
    const int n = u16();
    return QString::fromUtf8(bytes(n));
}

QByteArray ByteReader::bytes(int n)
{
    if (!need(n))
        return {};
    const QByteArray b = m_data.mid(m_pos, n);
    m_pos += n;
    return b;
}
