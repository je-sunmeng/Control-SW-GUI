#pragma once

#include "FriendlyFormation.h"
#include "JammerAircraft.h"
#include "Radar.h"
#include "StandaloneFalseTarget.h"

#include "common/IdGenerator.h"

#include <QObject>
#include <QStringList>
#include <QVector>

#include <optional>

class Logger;

// 저장/불러오기 및 일괄 교체용 시나리오 전체 값
struct ScenarioData {
    QVector<Radar> radars;
    std::optional<JammerAircraft> jammer;
    std::optional<FriendlyFormation> formation;
    QVector<StandaloneFalseTarget> falseTargets;
    double datalinkIntervalSec = 1.0;          // 아군편대 위치 전송간격
    double jammerIntervalSec = 1.0;            // 전자전기 위치 전송간격
};

// 전체 시나리오의 단일 진실 공급원.
//  - UI/지도는 이 API 로만 수정하고, 시그널로 동기화한다.
//  - 제약(레이다 ≤4, 전자전기 ≤1, 편대 ≤1, 편대규모 1~4, 허위타겟 0~8) 위반 / 잠금 상태 편집은
//    false(또는 ID 0) 반환 + Logger 오류 기록.
class ScenarioModel : public QObject {
    Q_OBJECT
public:
    static constexpr double MinDatalinkIntervalSec = 0.1;
    static constexpr double MaxDatalinkIntervalSec = 60.0;

    explicit ScenarioModel(QObject* parent = nullptr);

    void setLogger(Logger* logger) { m_logger = logger; }

    // ---- 조회 ----
    const QVector<Radar>& radars() const { return m_radars; }
    const std::optional<JammerAircraft>& jammer() const { return m_jammer; }
    const std::optional<FriendlyFormation>& formation() const { return m_formation; }
    const QVector<StandaloneFalseTarget>& falseTargets() const { return m_falseTargets; }
    double datalinkIntervalSec() const { return m_datalinkIntervalSec; }
    double jammerIntervalSec() const { return m_jammerIntervalSec; }

    const SimObject* find(ObjectId id) const;
    const Radar* radar(ObjectId id) const;
    const StandaloneFalseTarget* falseTarget(ObjectId id) const;
    const Route* route(ObjectId id) const;   // 전자전기/편대/이동표적
    std::optional<ObjectType> objectType(ObjectId id) const;
    QVector<ObjectId> objectIds() const;     // 레이다 → 전자전기 → 편대 → 단독표적 순
    bool isEmpty() const;

    ScenarioData snapshot() const;
    // 임무 시작 가능 여부 검증 (오류 메시지 목록)
    bool validate(QStringList* errors = nullptr) const;

    bool isLocked() const { return m_locked; }

    // ---- 편집 ----
    ObjectId addRadar(const GeoPoint& pos);
    bool updateRadar(const Radar& radar);
    bool moveRadar(ObjectId id, const GeoPoint& pos);

    ObjectId createJammer();
    bool updateJammer(const JammerAircraft& jammer);

    ObjectId createFormation();
    bool updateFormation(const FriendlyFormation& formation);

    ObjectId addFalseTarget(FalseTargetType type);
    bool updateFalseTarget(const StandaloneFalseTarget& target);
    bool setPointPosition(ObjectId id, const GeoPoint& pos);   // 고정/잡음 표적 위치

    bool appendWaypoint(ObjectId id, const GeoPoint& pos);
    bool moveWaypoint(ObjectId id, int index, const GeoPoint& pos);
    bool removeWaypoint(ObjectId id, int index);
    bool clearRoute(ObjectId id);
    bool setSegment(ObjectId id, int index, const RouteSegment& seg);

    bool renameObject(ObjectId id, const QString& name);   // 빈 이름 거부
    bool removeObject(ObjectId id);
    void clear();
    void setDatalinkIntervalSec(double sec);   // 아군편대 위치 전송간격
    void setJammerIntervalSec(double sec);     // 전자전기 위치 전송간격

    // ---- 불러오기 (ID 재발급, 기존 항목 교체) ----
    bool replaceRadars(QVector<Radar> radars);
    bool replaceJammer(std::optional<JammerAircraft> jammer);
    bool replaceFormation(std::optional<FriendlyFormation> formation);
    bool replaceFalseTargets(QVector<StandaloneFalseTarget> targets);
    bool replaceAll(const ScenarioData& data);

    // 임무 중(Running/Paused) 편집 잠금
    void setLocked(bool locked);

signals:
    void objectAdded(ObjectId id);
    void objectChanged(ObjectId id);
    void objectRemoved(ObjectId id);
    void scenarioReset();
    void datalinkIntervalChanged(double sec);
    void jammerIntervalChanged(double sec);
    void lockedChanged(bool locked);

private:
    bool checkEditable(const QString& action) const;
    void reject(const QString& message) const;
    Radar* mutableRadar(ObjectId id);
    StandaloneFalseTarget* mutableFalseTarget(ObjectId id);
    Route* mutableRoute(ObjectId id, RouteSegment* defaultSeg = nullptr);
    int freeRadarColorIndex() const;
    QString nextFalseTargetName() const;
    void removeAllRadars();
    void removeAllFalseTargets();

    Logger* m_logger = nullptr;
    IdGenerator m_ids;
    QVector<Radar> m_radars;
    std::optional<JammerAircraft> m_jammer;
    std::optional<FriendlyFormation> m_formation;
    QVector<StandaloneFalseTarget> m_falseTargets;
    double m_datalinkIntervalSec = 1.0;
    double m_jammerIntervalSec = 1.0;
    bool m_locked = false;
};
