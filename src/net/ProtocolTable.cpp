#include "ProtocolTable.h"

#include "Opcode.h"
#include "log/Logger.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <algorithm>
#include <iterator>

namespace {

std::optional<quint32> parseHex(const QString& s, quint32 max)
{
    bool ok = false;
    const QString t = s.trimmed();
    const quint32 v = t.startsWith(QLatin1String("0x"), Qt::CaseInsensitive) ? t.mid(2).toUInt(&ok, 16) : t.toUInt(&ok, 10);
    if (!ok || v > max)
        return std::nullopt;
    return v;
}

} // namespace

bool ProtocolTable::loadDefault(Logger* logger, const QString& appDir)
{
    const QString overridePath = QDir(appDir).filePath(overrideFileName());
    const QString path = QFile::exists(overridePath) ? overridePath : defaultResourcePath();
    QStringList errors, warnings;
    const bool ok = loadFile(path, &errors, &warnings);
    if (logger) {
        const QString src = QStringLiteral("Protocol");
        for (const QString& e : std::as_const(errors))
            logger->error(e, src);
        for (const QString& w : std::as_const(warnings))
            logger->error(QStringLiteral("경고: %1").arg(w), src);
        if (ok)
            logger->info(QStringLiteral("protocol.json 로드: 버전 %1, 매핑 %2개 (%3)")
                             .arg(m_version)
                             .arg(m_entries.size())
                             .arg(QDir::toNativeSeparators(path)),
                         src);
    }
    return ok;
}

bool ProtocolTable::loadFile(const QString& path, QStringList* errors, QStringList* warnings)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (errors)
            *errors << QStringLiteral("protocol.json 을 열 수 없습니다: %1").arg(path);
        return false;
    }
    const bool ok = loadJson(f.readAll(), errors, warnings);
    if (ok)
        m_source = path;
    return ok;
}

bool ProtocolTable::loadJson(const QByteArray& json, QStringList* errors, QStringList* warnings)
{
    QStringList errs, warns;
    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errors)
            *errors << QStringLiteral("protocol.json 형식 오류: %1").arg(pe.errorString());
        return false;
    }
    const QJsonObject root = doc.object();

    // 장치 이름 → 코드 (코드 체계는 Types.h DeviceCode 와 일치해야 함)
    QHash<QString, quint8> devices{{QStringLiteral("ControlSW"), quint8(DeviceCode::ControlSW)},
                                   {QStringLiteral("AnalysisSW"), quint8(DeviceCode::AnalysisSW)},
                                   {QStringLiteral("SignalGen"), quint8(DeviceCode::SignalGen)}};
    const QJsonObject devObj = root.value(QLatin1String("devices")).toObject();
    for (auto it = devObj.begin(); it != devObj.end(); ++it) {
        const auto code = parseHex(it.value().toString(), 0xFF);
        if (!code) {
            errs << QStringLiteral("devices.%1 값 오류").arg(it.key());
            continue;
        }
        if (devices.contains(it.key()) && devices.value(it.key()) != *code)
            warns << QStringLiteral("devices.%1 = 0x%2 가 내장 코드와 다릅니다").arg(it.key()).arg(*code, 2, 16, QLatin1Char('0'));
        devices.insert(it.key(), quint8(*code));
    }
    auto device = [&](const QJsonValue& v) -> std::optional<quint8> {
        const QString s = v.toString();
        if (devices.contains(s))
            return devices.value(s);
        if (const auto c = parseHex(s, 0xFF))
            return quint8(*c);
        return std::nullopt;
    };

    QVector<ProtocolEntry> entries;
    QHash<quint64, MessageId> byOpcode;
    const QJsonArray msgs = root.value(QLatin1String("messages")).toArray();
    for (int i = 0; i < msgs.size(); ++i) {
        const QJsonObject m = msgs.at(i).toObject();
        const QString idStr = m.value(QLatin1String("id")).toString();
        const auto id = MessageIds::fromString(idStr);
        const auto src = device(m.value(QLatin1String("src")));
        const auto dst = device(m.value(QLatin1String("dst")));
        const auto cmd = parseHex(m.value(QLatin1String("cmd")).toString(), 0xFFFF);
        if (!id) {
            errs << QStringLiteral("messages[%1]: 알 수 없는 id '%2'").arg(i).arg(idStr);
            continue;
        }
        if (!src || !dst || !cmd) {
            errs << QStringLiteral("messages[%1] (%2): src/dst/cmd 값 오류").arg(i).arg(idStr);
            continue;
        }
        const quint64 k = key(*src, *dst, quint16(*cmd));
        if (byOpcode.contains(k)) {
            errs << QStringLiteral("messages[%1] (%2): (src,dst,cmd) = %3 중복 (%4 와 충돌)")
                        .arg(i)
                        .arg(idStr, Opcode::toString(Opcode::make(*src, *dst, quint16(*cmd))),
                             MessageIds::toString(byOpcode.value(k)));
            continue;
        }
        byOpcode.insert(k, *id);
        entries.append({*id, *src, *dst, quint16(*cmd)});
    }
    if (entries.isEmpty())
        errs << QStringLiteral("유효한 메시지 매핑이 없습니다");

    // 코드에 있는 메시지의 매핑 누락 → 해당 메시지만 송수신 불가
    for (MessageId id : MessageIds::all()) {
        const bool found = std::any_of(entries.cbegin(), entries.cend(), [id](const ProtocolEntry& e) { return e.id == id; });
        if (!found)
            warns << QStringLiteral("%1 매핑 없음 (송수신 불가)").arg(MessageIds::toString(id));
    }

    if (errors)
        *errors << errs;
    if (warnings)
        *warnings << warns;
    if (entries.isEmpty())
        return false;
    m_version = root.value(QLatin1String("protocolVersion")).toString(QStringLiteral("?"));
    m_entries = entries;
    m_byOpcode = byOpcode;
    return true;
}

std::optional<quint16> ProtocolTable::cmdFor(MessageId id, DeviceCode src, DeviceCode dst) const
{
    for (const ProtocolEntry& e : m_entries)
        if (e.id == id && e.src == quint8(src) && e.dst == quint8(dst))
            return e.cmd;
    return std::nullopt;
}

std::optional<quint32> ProtocolTable::opcodeFor(MessageId id, DeviceCode src, DeviceCode dst) const
{
    if (const auto c = cmdFor(id, src, dst))
        return Opcode::make(quint8(src), quint8(dst), *c);
    return std::nullopt;
}

std::optional<MessageId> ProtocolTable::idFor(quint8 src, quint8 dst, quint16 cmd) const
{
    const auto it = m_byOpcode.constFind(key(src, dst, cmd));
    if (it == m_byOpcode.cend())
        return std::nullopt;
    return *it;
}

std::optional<MessageId> ProtocolTable::idForOpcode(quint32 op) const
{
    return idFor(Opcode::src(op), Opcode::dst(op), Opcode::cmd(op));
}
