#pragma once

#include "model/ScenarioModel.h"

#include <QJsonObject>

// 모델 ↔ QJsonObject. fromJson 은 누락 필드에 기본값을 쓰고, 형식 오류 시 false + error.
// ID 는 저장하지 않는다(불러오기 시 재발급).
namespace JsonSerializer {

inline constexpr int SchemaVersion = 1;

QJsonObject toJson(const GeoPoint& p);
QJsonObject toJson(const Route& route);
QJsonObject toJson(const Radar& radar);
QJsonObject toJson(const JammerAircraft& jammer);
QJsonObject toJson(const FriendlyFormation& formation);
QJsonObject toJson(const StandaloneFalseTarget& target);

bool fromJson(const QJsonObject& o, GeoPoint& out, QString* error = nullptr);
bool fromJson(const QJsonObject& o, Route& out, QString* error = nullptr);
bool fromJson(const QJsonObject& o, Radar& out, QString* error = nullptr);
bool fromJson(const QJsonObject& o, JammerAircraft& out, QString* error = nullptr);
bool fromJson(const QJsonObject& o, FriendlyFormation& out, QString* error = nullptr);
bool fromJson(const QJsonObject& o, StandaloneFalseTarget& out, QString* error = nullptr);

QString toString(ScanDirection d);            // "CW" / "CCW"
QString toString(FalseTargetType t);          // "Fixed" / "Moving" / "Random" / "Noise"
bool parse(const QString& s, ScanDirection& out);
bool parse(const QString& s, FalseTargetType& out);

} // namespace JsonSerializer
