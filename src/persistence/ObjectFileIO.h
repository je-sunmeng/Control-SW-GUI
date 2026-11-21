#pragma once

#include "model/ScenarioModel.h"

#include <QString>

#include <optional>

// 오브젝트 항목별 저장/불러오기.
//   *.radar.json / *.jammer.json / *.formation.json / *.falsetarget.json / *.scenario.json
// 파일 루트: { "schemaVersion": 1, "kind": "<kind>", ... }
// 불러오기 결과의 ID 는 의미 없음 (ScenarioModel::replace* 에서 재발급)
namespace ObjectFileIO {

QString fileFilter(ObjectType type);   // QFileDialog 필터
QString scenarioFileFilter();
QString defaultSuffix(ObjectType type);   // "radar.json" 등

bool saveRadars(const QString& path, const QVector<Radar>& radars, QString* error = nullptr);
std::optional<QVector<Radar>> loadRadars(const QString& path, QString* error = nullptr);

bool saveJammer(const QString& path, const JammerAircraft& jammer, QString* error = nullptr);
std::optional<JammerAircraft> loadJammer(const QString& path, QString* error = nullptr);

bool saveFormation(const QString& path, const FriendlyFormation& formation, QString* error = nullptr);
std::optional<FriendlyFormation> loadFormation(const QString& path, QString* error = nullptr);

bool saveFalseTargets(const QString& path, const QVector<StandaloneFalseTarget>& targets, QString* error = nullptr);
std::optional<QVector<StandaloneFalseTarget>> loadFalseTargets(const QString& path, QString* error = nullptr);

bool saveScenario(const QString& path, const ScenarioData& data, QString* error = nullptr);
std::optional<ScenarioData> loadScenario(const QString& path, QString* error = nullptr);

} // namespace ObjectFileIO
