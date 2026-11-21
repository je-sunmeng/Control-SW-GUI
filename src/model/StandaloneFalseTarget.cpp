#include "StandaloneFalseTarget.h"

bool StandaloneFalseTarget::validate(QString* reason) const
{
    auto fail = [reason](const QString& r) {
        if (reason)
            *reason = r;
        return false;
    };
    switch (ftType) {
    case FalseTargetType::Fixed:
        if (!posSet)
            return fail(QStringLiteral("위치가 지정되지 않았습니다"));
        break;
    case FalseTargetType::Moving: {
        QString r;
        if (!route.isValid(&r))
            return fail(r);
        break;
    }
    case FalseTargetType::Random:
        break;
    case FalseTargetType::Noise:
        if (!posSet)
            return fail(QStringLiteral("위치가 지정되지 않았습니다"));
        if (detectAngleDeg < FalseTargetLimits::MinDetectAngleDeg || detectAngleDeg > FalseTargetLimits::MaxDetectAngleDeg)
            return fail(QStringLiteral("탐지각도가 범위를 벗어났습니다"));
        break;
    }
    return true;
}
