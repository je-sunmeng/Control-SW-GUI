#include "LogEntry.h"

QString toDisplayString(LogType type)
{
    switch (type) {
    case LogType::Info:  return QStringLiteral("정보");
    case LogType::Error: return QStringLiteral("오류");
    case LogType::Tx:    return QStringLiteral("송신");
    case LogType::Rx:    return QStringLiteral("수신");
    }
    return {};
}

QString toFileTag(LogType type)
{
    switch (type) {
    case LogType::Info:  return QStringLiteral("INFO");
    case LogType::Error: return QStringLiteral("ERROR");
    case LogType::Tx:    return QStringLiteral("TX");
    case LogType::Rx:    return QStringLiteral("RX");
    }
    return {};
}

QString formatOpcode(quint32 opcode)
{
    return QStringLiteral("0x%1 %2")
        .arg(opcode >> 16, 4, 16, QLatin1Char('0'))
        .arg(opcode & 0xFFFFu, 4, 16, QLatin1Char('0'))
        .toUpper()
        .replace(QLatin1String("0X"), QLatin1String("0x"));
}

QString hexDump(const QByteArray& data, int maxBytes)
{
    const int n = qMin<qsizetype>(data.size(), qMax(0, maxBytes));
    QString out;
    for (int off = 0; off < n; off += 16) {
        QString hex, ascii;
        for (int i = 0; i < 16; ++i) {
            if (off + i < n) {
                const auto b = static_cast<quint8>(data[off + i]);
                hex += QStringLiteral("%1 ").arg(b, 2, 16, QLatin1Char('0')).toUpper();
                ascii += (b >= 0x20 && b < 0x7F) ? QChar(b) : QChar('.');
            } else {
                hex += QStringLiteral("   ");
            }
            if (i == 7)
                hex += QLatin1Char(' ');
        }
        out += QStringLiteral("%1  %2 %3\n").arg(off, 4, 16, QLatin1Char('0')).arg(hex, ascii);
    }
    if (data.size() > n)
        out += QStringLiteral("... (%1 bytes 생략)\n").arg(data.size() - n);
    if (out.endsWith(QLatin1Char('\n')))
        out.chop(1);
    return out;
}

QString formatLogLine(const LogEntry& e)
{
    QString line = e.time.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"))
                 + QStringLiteral(" [%1]").arg(toFileTag(e.type), -5);
    if (!e.device.isEmpty())
        line += QStringLiteral(" <%1>").arg(e.device);
    if (e.isFrame())
        line += QStringLiteral(" %1 size=%2").arg(formatOpcode(e.opcode)).arg(e.size);
    line += QLatin1Char(' ') + e.content;
    return line;
}
