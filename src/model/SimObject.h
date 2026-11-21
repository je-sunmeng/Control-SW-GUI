#pragma once

#include "common/Types.h"

#include <QString>

// 모든 시나리오 오브젝트의 추상 베이스 (QObject 비상속 값 객체)
class SimObject {
public:
    virtual ~SimObject() = default;
    virtual ObjectType type() const = 0;

    ObjectId id = 0;
    QString name;

protected:
    SimObject() = default;
    SimObject(const SimObject&) = default;
    SimObject& operator=(const SimObject&) = default;
};

QString toDisplayString(ObjectType type);
QString toDisplayString(FalseTargetType type);
