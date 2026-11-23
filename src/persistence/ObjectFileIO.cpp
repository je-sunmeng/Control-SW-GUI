#include "ObjectFileIO.h"

#include "JsonSerializer.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>

namespace ObjectFileIO {

namespace {

const QString kRadar = QStringLiteral("radar");
const QString kJammer = QStringLiteral("jammer");
const QString kFormation = QStringLiteral("formation");
const QString kFalseTarget = QStringLiteral("falsetarget");
const QString kScenario = QStringLiteral("scenario");

bool fail(QString* error, const QString& msg)
{
    if (error)
        *error = msg;
    return false;
}

QJsonObject root(const QString& kind)
{
    return {{QStringLiteral("schemaVersion"), JsonSerializer::SchemaVersion}, {QStringLiteral("kind"), kind}};
}

bool writeJson(const QString& path, const QJsonObject& obj, QString* error)
{
    QSaveFile f(path);   // 원자적 저장 (실패 시 기존 파일 보존)
    if (!f.open(QIODevice::WriteOnly))
        return fail(error, QStringLiteral("파일을 쓸 수 없습니다: %1").arg(f.errorString()));
    f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    if (!f.commit())
        return fail(error, QStringLiteral("파일 저장 실패: %1").arg(f.errorString()));
    return true;
}

std::optional<QJsonObject> readJson(const QString& path, const QString& kind, QString* error)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        fail(error, QStringLiteral("파일을 열 수 없습니다: %1").arg(f.errorString()));
        return std::nullopt;
    }
    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
        fail(error, QStringLiteral("JSON 형식 오류: %1 (offset %2)").arg(pe.errorString()).arg(pe.offset));
        return std::nullopt;
    }
    const QJsonObject o = doc.object();
    const int ver = o.value(QLatin1String("schemaVersion")).toInt(-1);
    if (ver < 1 || ver > JsonSerializer::SchemaVersion) {
        fail(error, QStringLiteral("지원하지 않는 schemaVersion: %1").arg(ver));
        return std::nullopt;
    }
    const QString k = o.value(QLatin1String("kind")).toString();
    if (k != kind) {
        fail(error, QStringLiteral("파일 종류가 다릅니다 (기대: %1, 파일: %2)").arg(kind, k.isEmpty() ? QStringLiteral("없음") : k));
        return std::nullopt;
    }
    return o;
}

template <typename T>
std::optional<QVector<T>> readItems(const QJsonObject& o, QString* error)
{
    QVector<T> out;
    const QJsonArray items = o.value(QLatin1String("items")).toArray();
    for (int i = 0; i < items.size(); ++i) {
        T item;
        QString e;
        if (!JsonSerializer::fromJson(items.at(i).toObject(), item, &e)) {
            fail(error, QStringLiteral("항목 %1: %2").arg(i + 1).arg(e));
            return std::nullopt;
        }
        out.append(item);
    }
    return out;
}

template <typename T>
QJsonArray writeItems(const QVector<T>& items)
{
    QJsonArray arr;
    for (const T& item : items)
        arr.append(JsonSerializer::toJson(item));
    return arr;
}

template <typename T>
std::optional<T> readSingle(const QJsonObject& o, QString* error)
{
    T item;
    QString e;
    if (!JsonSerializer::fromJson(o.value(QLatin1String("item")).toObject(), item, &e)) {
        fail(error, e);
        return std::nullopt;
    }
    return item;
}

} // namespace

QString defaultSuffix(ObjectType type)
{
    switch (type) {
    case ObjectType::Radar:                 return kRadar + QStringLiteral(".json");
    case ObjectType::Jammer:                return kJammer + QStringLiteral(".json");
    case ObjectType::Formation:             return kFormation + QStringLiteral(".json");
    case ObjectType::StandaloneFalseTarget: return kFalseTarget + QStringLiteral(".json");
    }
    return QStringLiteral("json");
}

QString fileFilter(ObjectType type)
{
    switch (type) {
    case ObjectType::Radar:                 return QStringLiteral("레이다 (*.radar.json);;모든 파일 (*)");
    case ObjectType::Jammer:                return QStringLiteral("전자전기 (*.jammer.json);;모든 파일 (*)");
    case ObjectType::Formation:             return QStringLiteral("아군편대 (*.formation.json);;모든 파일 (*)");
    case ObjectType::StandaloneFalseTarget: return QStringLiteral("단독허위표적 (*.falsetarget.json);;모든 파일 (*)");
    }
    return QStringLiteral("모든 파일 (*)");
}

QString scenarioFileFilter()
{
    return QStringLiteral("시나리오 (*.scenario.json);;모든 파일 (*)");
}

bool saveRadars(const QString& path, const QVector<Radar>& radars, QString* error)
{
    QJsonObject o = root(kRadar);
    o.insert(QStringLiteral("items"), writeItems(radars));
    return writeJson(path, o, error);
}

std::optional<QVector<Radar>> loadRadars(const QString& path, QString* error)
{
    const auto o = readJson(path, kRadar, error);
    return o ? readItems<Radar>(*o, error) : std::nullopt;
}

bool saveJammer(const QString& path, const JammerAircraft& jammer, QString* error)
{
    QJsonObject o = root(kJammer);
    o.insert(QStringLiteral("item"), JsonSerializer::toJson(jammer));
    return writeJson(path, o, error);
}

std::optional<JammerAircraft> loadJammer(const QString& path, QString* error)
{
    const auto o = readJson(path, kJammer, error);
    return o ? readSingle<JammerAircraft>(*o, error) : std::nullopt;
}

bool saveFormation(const QString& path, const FriendlyFormation& formation, QString* error)
{
    QJsonObject o = root(kFormation);
    o.insert(QStringLiteral("item"), JsonSerializer::toJson(formation));
    return writeJson(path, o, error);
}

std::optional<FriendlyFormation> loadFormation(const QString& path, QString* error)
{
    const auto o = readJson(path, kFormation, error);
    return o ? readSingle<FriendlyFormation>(*o, error) : std::nullopt;
}

bool saveFalseTargets(const QString& path, const QVector<StandaloneFalseTarget>& targets, QString* error)
{
    QJsonObject o = root(kFalseTarget);
    o.insert(QStringLiteral("items"), writeItems(targets));
    return writeJson(path, o, error);
}

std::optional<QVector<StandaloneFalseTarget>> loadFalseTargets(const QString& path, QString* error)
{
    const auto o = readJson(path, kFalseTarget, error);
    return o ? readItems<StandaloneFalseTarget>(*o, error) : std::nullopt;
}

bool saveScenario(const QString& path, const ScenarioData& data, QString* error)
{
    QJsonObject o = root(kScenario);
    o.insert(QStringLiteral("datalinkIntervalSec"), data.datalinkIntervalSec);
    o.insert(QStringLiteral("jammerIntervalSec"), data.jammerIntervalSec);
    o.insert(QStringLiteral("radars"), writeItems(data.radars));
    if (data.jammer)
        o.insert(QStringLiteral("jammer"), JsonSerializer::toJson(*data.jammer));
    if (data.formation)
        o.insert(QStringLiteral("formation"), JsonSerializer::toJson(*data.formation));
    o.insert(QStringLiteral("falseTargets"), writeItems(data.falseTargets));
    return writeJson(path, o, error);
}

std::optional<ScenarioData> loadScenario(const QString& path, QString* error)
{
    const auto o = readJson(path, kScenario, error);
    if (!o)
        return std::nullopt;
    ScenarioData d;
    d.datalinkIntervalSec = o->value(QLatin1String("datalinkIntervalSec")).toDouble(d.datalinkIntervalSec);
    d.jammerIntervalSec = o->value(QLatin1String("jammerIntervalSec")).toDouble(d.jammerIntervalSec);

    auto section = [&](const char* key, auto& out) -> bool {
        using Item = typename std::decay_t<decltype(out)>::value_type;
        QJsonObject wrap{{QStringLiteral("items"), o->value(QLatin1String(key)).toArray()}};
        QString e;
        auto items = readItems<Item>(wrap, &e);
        if (!items)
            return fail(error, QStringLiteral("%1: %2").arg(QLatin1String(key), e));
        out = *items;
        return true;
    };
    if (!section("radars", d.radars) || !section("falseTargets", d.falseTargets))
        return std::nullopt;

    QString e;
    if (o->contains(QLatin1String("jammer"))) {
        JammerAircraft j;
        if (!JsonSerializer::fromJson(o->value(QLatin1String("jammer")).toObject(), j, &e)) {
            fail(error, QStringLiteral("jammer: %1").arg(e));
            return std::nullopt;
        }
        d.jammer = j;
    }
    if (o->contains(QLatin1String("formation"))) {
        FriendlyFormation f;
        if (!JsonSerializer::fromJson(o->value(QLatin1String("formation")).toObject(), f, &e)) {
            fail(error, QStringLiteral("formation: %1").arg(e));
            return std::nullopt;
        }
        d.formation = f;
    }
    return d;
}

} // namespace ObjectFileIO
