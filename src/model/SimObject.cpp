#include "SimObject.h"

QString toDisplayString(ObjectType type)
{
    switch (type) {
    case ObjectType::Radar:                 return QStringLiteral("레이다");
    case ObjectType::Jammer:                return QStringLiteral("전자전기");
    case ObjectType::Formation:             return QStringLiteral("아군편대");
    case ObjectType::StandaloneFalseTarget: return QStringLiteral("단독허위표적");
    }
    return {};
}

QString toDisplayString(FalseTargetType type)
{
    switch (type) {
    case FalseTargetType::Fixed:  return QStringLiteral("고정");
    case FalseTargetType::Moving: return QStringLiteral("이동");
    case FalseTargetType::Random: return QStringLiteral("랜덤");
    case FalseTargetType::Noise:  return QStringLiteral("잡음");
    }
    return {};
}
