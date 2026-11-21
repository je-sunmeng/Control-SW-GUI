#include "ScenarioModel.h"

#include "common/Constants.h"
#include "log/Logger.h"

#include <QSet>

namespace {
const QString kSource = QStringLiteral("Scenario");
}

ScenarioModel::ScenarioModel(QObject* parent)
    : QObject(parent)
    , m_datalinkIntervalSec(Defaults::DatalinkIntervalSec)
{
}

// ---------------------------------------------------------------------------- 조회

const SimObject* ScenarioModel::find(ObjectId id) const
{
    if (id == 0)
        return nullptr;
    if (const Radar* r = radar(id))
        return r;
    if (m_jammer && m_jammer->id == id)
        return &*m_jammer;
    if (m_formation && m_formation->id == id)
        return &*m_formation;
    return falseTarget(id);
}

const Radar* ScenarioModel::radar(ObjectId id) const
{
    for (const Radar& r : m_radars)
        if (r.id == id)
            return &r;
    return nullptr;
}

const StandaloneFalseTarget* ScenarioModel::falseTarget(ObjectId id) const
{
    for (const StandaloneFalseTarget& t : m_falseTargets)
        if (t.id == id)
            return &t;
    return nullptr;
}

const Route* ScenarioModel::route(ObjectId id) const
{
    return const_cast<ScenarioModel*>(this)->mutableRoute(id);
}

std::optional<ObjectType> ScenarioModel::objectType(ObjectId id) const
{
    if (const SimObject* o = find(id))
        return o->type();
    return std::nullopt;
}

QVector<ObjectId> ScenarioModel::objectIds() const
{
    QVector<ObjectId> ids;
    for (const Radar& r : m_radars)
        ids << r.id;
    if (m_jammer)
        ids << m_jammer->id;
    if (m_formation)
        ids << m_formation->id;
    for (const StandaloneFalseTarget& t : m_falseTargets)
        ids << t.id;
    return ids;
}

bool ScenarioModel::isEmpty() const
{
    return m_radars.isEmpty() && !m_jammer && !m_formation && m_falseTargets.isEmpty();
}

ScenarioData ScenarioModel::snapshot() const
{
    return {m_radars, m_jammer, m_formation, m_falseTargets, m_datalinkIntervalSec};
}

bool ScenarioModel::validate(QStringList* errors) const
{
    QStringList e;
    if (m_radars.isEmpty())
        e << QStringLiteral("레이다가 1개 이상 필요합니다");
    QString r;
    if (!m_jammer)
        e << QStringLiteral("전자전기가 없습니다");
    else if (!m_jammer->route.isValid(&r))
        e << QStringLiteral("전자전기 동선: %1").arg(r);
    if (m_formation && !m_formation->route.isValid(&r))
        e << QStringLiteral("아군편대 동선: %1").arg(r);
    for (const StandaloneFalseTarget& t : m_falseTargets)
        if (!t.validate(&r))
            e << QStringLiteral("%1: %2").arg(t.name, r);
    if (errors)
        *errors = e;
    return e.isEmpty();
}

// ---------------------------------------------------------------------------- 내부

void ScenarioModel::reject(const QString& message) const
{
    if (m_logger)
        m_logger->error(message, kSource);
}

bool ScenarioModel::checkEditable(const QString& action) const
{
    if (!m_locked)
        return true;
    reject(QStringLiteral("임무 중에는 편집할 수 없습니다 (%1)").arg(action));
    return false;
}

Radar* ScenarioModel::mutableRadar(ObjectId id)
{
    for (Radar& r : m_radars)
        if (r.id == id)
            return &r;
    return nullptr;
}

StandaloneFalseTarget* ScenarioModel::mutableFalseTarget(ObjectId id)
{
    for (StandaloneFalseTarget& t : m_falseTargets)
        if (t.id == id)
            return &t;
    return nullptr;
}

Route* ScenarioModel::mutableRoute(ObjectId id, RouteSegment* defaultSeg)
{
    if (m_jammer && m_jammer->id == id) {
        if (defaultSeg)
            *defaultSeg = JammerAircraft::defaultSegment();
        return &m_jammer->route;
    }
    if (m_formation && m_formation->id == id) {
        if (defaultSeg)
            *defaultSeg = FriendlyFormation::defaultSegment();
        return &m_formation->route;
    }
    if (StandaloneFalseTarget* t = mutableFalseTarget(id); t && t->usesRoute()) {
        if (defaultSeg)
            *defaultSeg = StandaloneFalseTarget::defaultSegment();
        return &t->route;
    }
    return nullptr;
}

int ScenarioModel::freeRadarColorIndex() const
{
    QSet<int> used;
    for (const Radar& r : m_radars)
        used.insert(r.colorIndex);
    for (int i = 0; i < Limits::MaxRadars; ++i)
        if (!used.contains(i))
            return i;
    return -1;
}

QString ScenarioModel::nextFalseTargetName() const
{
    int maxNo = 0;
    for (const StandaloneFalseTarget& t : m_falseTargets) {
        if (t.name.startsWith(QLatin1String("FT"))) {
            bool ok = false;
            const int n = t.name.mid(2).toInt(&ok);
            if (ok)
                maxNo = std::max(maxNo, n);
        }
    }
    return QStringLiteral("FT%1").arg(maxNo + 1);
}

// ---------------------------------------------------------------------------- 레이다

ObjectId ScenarioModel::addRadar(const GeoPoint& pos)
{
    if (!checkEditable(QStringLiteral("레이다 추가")))
        return 0;
    const int color = freeRadarColorIndex();
    if (m_radars.size() >= Limits::MaxRadars || color < 0) {
        reject(QStringLiteral("레이다는 최대 %1개까지 추가할 수 있습니다").arg(Limits::MaxRadars));
        return 0;
    }
    Radar r;
    r.id = m_ids.next();
    r.colorIndex = color;
    r.name = QStringLiteral("R%1").arg(color + 1);
    r.pos = pos;
    r.clampValues();
    m_radars.append(r);
    emit objectAdded(r.id);
    return r.id;
}

bool ScenarioModel::updateRadar(const Radar& radar)
{
    if (!checkEditable(QStringLiteral("레이다 수정")))
        return false;
    Radar* r = mutableRadar(radar.id);
    if (!r)
        return false;
    const int color = r->colorIndex;   // 색상은 고정 (추가 시 배정)
    *r = radar;
    r->colorIndex = color;
    r->clampValues();
    emit objectChanged(r->id);
    return true;
}

bool ScenarioModel::moveRadar(ObjectId id, const GeoPoint& pos)
{
    if (!checkEditable(QStringLiteral("레이다 이동")))
        return false;
    Radar* r = mutableRadar(id);
    if (!r)
        return false;
    r->pos = pos;
    r->clampValues();
    emit objectChanged(id);
    return true;
}

// ---------------------------------------------------------------------------- 전자전기 / 편대

ObjectId ScenarioModel::createJammer()
{
    if (!checkEditable(QStringLiteral("전자전기 추가")))
        return 0;
    if (m_jammer) {
        reject(QStringLiteral("전자전기는 1개만 추가할 수 있습니다"));
        return 0;
    }
    JammerAircraft j;
    j.id = m_ids.next();
    j.name = QStringLiteral("전자전기");
    m_jammer = j;
    emit objectAdded(j.id);
    return j.id;
}

bool ScenarioModel::updateJammer(const JammerAircraft& jammer)
{
    if (!checkEditable(QStringLiteral("전자전기 수정")))
        return false;
    if (!m_jammer || m_jammer->id != jammer.id)
        return false;
    m_jammer = jammer;
    for (int i = 0; i < m_jammer->route.segments.size(); ++i)
        m_jammer->route.segments[i] = Route::clamp(m_jammer->route.segments[i]);
    emit objectChanged(jammer.id);
    return true;
}

ObjectId ScenarioModel::createFormation()
{
    if (!checkEditable(QStringLiteral("아군편대 추가")))
        return 0;
    if (m_formation) {
        reject(QStringLiteral("아군편대는 최대 %1개까지 추가할 수 있습니다").arg(Limits::MaxFormations));
        return 0;
    }
    FriendlyFormation f;
    f.id = m_ids.next();
    f.name = QStringLiteral("아군편대");
    m_formation = f;
    emit objectAdded(f.id);
    return f.id;
}

bool ScenarioModel::updateFormation(const FriendlyFormation& formation)
{
    if (!checkEditable(QStringLiteral("아군편대 수정")))
        return false;
    if (!m_formation || m_formation->id != formation.id)
        return false;
    if (formation.formationSize < Limits::MinFormationSize || formation.formationSize > Limits::MaxFormationSize) {
        reject(QStringLiteral("편대규모는 %1~%2 입니다").arg(Limits::MinFormationSize).arg(Limits::MaxFormationSize));
        return false;
    }
    if (formation.jamming.targets.size() > Limits::MaxFormationFalseTargets) {
        reject(QStringLiteral("허위타겟은 최대 %1개입니다").arg(Limits::MaxFormationFalseTargets));
        return false;
    }
    m_formation = formation;
    m_formation->jamming.clampValues();
    for (int i = 0; i < m_formation->route.segments.size(); ++i)
        m_formation->route.segments[i] = Route::clamp(m_formation->route.segments[i]);
    emit objectChanged(formation.id);
    return true;
}

// ---------------------------------------------------------------------------- 단독 허위표적

ObjectId ScenarioModel::addFalseTarget(FalseTargetType type)
{
    if (!checkEditable(QStringLiteral("단독허위표적 추가")))
        return 0;
    StandaloneFalseTarget t;
    t.id = m_ids.next();
    t.ftType = type;
    t.name = nextFalseTargetName();
    m_falseTargets.append(t);
    emit objectAdded(t.id);
    return t.id;
}

bool ScenarioModel::updateFalseTarget(const StandaloneFalseTarget& target)
{
    if (!checkEditable(QStringLiteral("단독허위표적 수정")))
        return false;
    StandaloneFalseTarget* t = mutableFalseTarget(target.id);
    if (!t)
        return false;
    const FalseTargetType type = t->ftType;   // 타입은 생성 시 고정
    *t = target;
    t->ftType = type;
    t->detectAngleDeg = qBound(FalseTargetLimits::MinDetectAngleDeg, t->detectAngleDeg, FalseTargetLimits::MaxDetectAngleDeg);
    emit objectChanged(t->id);
    return true;
}

bool ScenarioModel::setPointPosition(ObjectId id, const GeoPoint& pos)
{
    if (!checkEditable(QStringLiteral("표적 위치 지정")))
        return false;
    StandaloneFalseTarget* t = mutableFalseTarget(id);
    if (!t || !t->usesPosition())
        return false;
    t->pos = pos;
    t->posSet = true;
    emit objectChanged(id);
    return true;
}

// ---------------------------------------------------------------------------- 동선

bool ScenarioModel::appendWaypoint(ObjectId id, const GeoPoint& pos)
{
    if (!checkEditable(QStringLiteral("웨이포인트 추가")))
        return false;
    RouteSegment def;
    Route* r = mutableRoute(id, &def);
    if (!r)
        return false;
    r->addWaypoint(pos, def);
    emit objectChanged(id);
    return true;
}

bool ScenarioModel::moveWaypoint(ObjectId id, int index, const GeoPoint& pos)
{
    if (!checkEditable(QStringLiteral("웨이포인트 이동")))
        return false;
    Route* r = mutableRoute(id);
    if (!r || !r->moveWaypoint(index, pos))
        return false;
    emit objectChanged(id);
    return true;
}

bool ScenarioModel::removeWaypoint(ObjectId id, int index)
{
    if (!checkEditable(QStringLiteral("웨이포인트 삭제")))
        return false;
    Route* r = mutableRoute(id);
    if (!r || !r->removeWaypoint(index))
        return false;
    emit objectChanged(id);
    return true;
}

bool ScenarioModel::clearRoute(ObjectId id)
{
    if (!checkEditable(QStringLiteral("동선 초기화")))
        return false;
    Route* r = mutableRoute(id);
    if (!r)
        return false;
    r->clear();
    emit objectChanged(id);
    return true;
}

bool ScenarioModel::setSegment(ObjectId id, int index, const RouteSegment& seg)
{
    if (!checkEditable(QStringLiteral("구간 설정")))
        return false;
    Route* r = mutableRoute(id);
    if (!r || !r->setSegment(index, seg))
        return false;
    emit objectChanged(id);
    return true;
}

// ---------------------------------------------------------------------------- 이름 / 삭제 / 초기화

bool ScenarioModel::renameObject(ObjectId id, const QString& name)
{
    if (!checkEditable(QStringLiteral("이름 변경")))
        return false;
    const QString n = name.trimmed();
    if (n.isEmpty())
        return false;
    SimObject* o = const_cast<SimObject*>(find(id));
    if (!o)
        return false;
    if (o->name == n)
        return true;
    o->name = n;
    emit objectChanged(id);
    return true;
}

bool ScenarioModel::removeObject(ObjectId id)
{
    if (!checkEditable(QStringLiteral("삭제")))
        return false;
    for (int i = 0; i < m_radars.size(); ++i) {
        if (m_radars[i].id == id) {
            m_radars.removeAt(i);
            emit objectRemoved(id);
            return true;
        }
    }
    if (m_jammer && m_jammer->id == id) {
        m_jammer.reset();
        emit objectRemoved(id);
        return true;
    }
    if (m_formation && m_formation->id == id) {
        m_formation.reset();
        emit objectRemoved(id);
        return true;
    }
    for (int i = 0; i < m_falseTargets.size(); ++i) {
        if (m_falseTargets[i].id == id) {
            m_falseTargets.removeAt(i);
            emit objectRemoved(id);
            return true;
        }
    }
    return false;
}

void ScenarioModel::clear()
{
    if (!checkEditable(QStringLiteral("전체 삭제")))
        return;
    m_radars.clear();
    m_jammer.reset();
    m_formation.reset();
    m_falseTargets.clear();
    emit scenarioReset();
}

void ScenarioModel::setDatalinkIntervalSec(double sec)
{
    if (!checkEditable(QStringLiteral("데이터링크 전송간격")))
        return;
    sec = qBound(MinDatalinkIntervalSec, sec, MaxDatalinkIntervalSec);
    if (qFuzzyCompare(sec, m_datalinkIntervalSec))
        return;
    m_datalinkIntervalSec = sec;
    emit datalinkIntervalChanged(sec);
}

// ---------------------------------------------------------------------------- 불러오기

void ScenarioModel::removeAllRadars()
{
    while (!m_radars.isEmpty()) {
        const ObjectId id = m_radars.takeLast().id;
        emit objectRemoved(id);
    }
}

void ScenarioModel::removeAllFalseTargets()
{
    while (!m_falseTargets.isEmpty()) {
        const ObjectId id = m_falseTargets.takeLast().id;
        emit objectRemoved(id);
    }
}

bool ScenarioModel::replaceRadars(QVector<Radar> radars)
{
    if (!checkEditable(QStringLiteral("레이다 불러오기")))
        return false;
    if (radars.size() > Limits::MaxRadars) {
        reject(QStringLiteral("레이다 %1개 중 앞의 %2개만 불러옵니다").arg(radars.size()).arg(Limits::MaxRadars));
        radars.resize(Limits::MaxRadars);
    }
    removeAllRadars();
    for (int i = 0; i < radars.size(); ++i) {
        Radar r = radars[i];
        r.id = m_ids.next();
        r.colorIndex = i;
        if (r.name.isEmpty())
            r.name = QStringLiteral("R%1").arg(i + 1);
        r.clampValues();
        m_radars.append(r);
        emit objectAdded(r.id);
    }
    return true;
}

bool ScenarioModel::replaceJammer(std::optional<JammerAircraft> jammer)
{
    if (!checkEditable(QStringLiteral("전자전기 불러오기")))
        return false;
    if (m_jammer) {
        const ObjectId old = m_jammer->id;
        m_jammer.reset();
        emit objectRemoved(old);
    }
    if (jammer) {
        jammer->id = m_ids.next();
        if (jammer->name.isEmpty())
            jammer->name = QStringLiteral("전자전기");
        m_jammer = std::move(jammer);
        emit objectAdded(m_jammer->id);
    }
    return true;
}

bool ScenarioModel::replaceFormation(std::optional<FriendlyFormation> formation)
{
    if (!checkEditable(QStringLiteral("아군편대 불러오기")))
        return false;
    if (m_formation) {
        const ObjectId old = m_formation->id;
        m_formation.reset();
        emit objectRemoved(old);
    }
    if (formation) {
        formation->id = m_ids.next();
        if (formation->name.isEmpty())
            formation->name = QStringLiteral("아군편대");
        formation->formationSize = qBound(Limits::MinFormationSize, formation->formationSize, Limits::MaxFormationSize);
        formation->jamming.clampValues();
        m_formation = std::move(formation);
        emit objectAdded(m_formation->id);
    }
    return true;
}

bool ScenarioModel::replaceFalseTargets(QVector<StandaloneFalseTarget> targets)
{
    if (!checkEditable(QStringLiteral("단독허위표적 불러오기")))
        return false;
    removeAllFalseTargets();
    for (StandaloneFalseTarget t : targets) {
        t.id = m_ids.next();
        if (t.name.isEmpty())
            t.name = nextFalseTargetName();
        m_falseTargets.append(t);
        emit objectAdded(t.id);
    }
    return true;
}

bool ScenarioModel::replaceAll(const ScenarioData& data)
{
    if (!checkEditable(QStringLiteral("시나리오 불러오기")))
        return false;
    replaceRadars(data.radars);
    replaceJammer(data.jammer);
    replaceFormation(data.formation);
    replaceFalseTargets(data.falseTargets);
    setDatalinkIntervalSec(data.datalinkIntervalSec);
    return true;
}

void ScenarioModel::setLocked(bool locked)
{
    if (m_locked == locked)
        return;
    m_locked = locked;
    emit lockedChanged(locked);
}
