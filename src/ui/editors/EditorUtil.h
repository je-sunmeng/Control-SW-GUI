#pragma once

#include <QDoubleSpinBox>
#include <QSignalBlocker>
#include <QString>

#include <cmath>

// 에디터 공용 헬퍼
namespace EditorUtil {

// 범위/소수점/단위 지정 스핀박스. keyboardTracking 끔 → Enter/포커스 이동/화살표 시에만 valueChanged
inline QDoubleSpinBox* makeSpin(double min, double max, int decimals, const QString& suffix, double step,
                                QWidget* parent = nullptr)
{
    auto* s = new QDoubleSpinBox(parent);
    s->setRange(min, max);
    s->setDecimals(decimals);
    s->setSingleStep(step);
    if (!suffix.isEmpty())
        s->setSuffix(QLatin1Char(' ') + suffix);
    s->setKeyboardTracking(false);
    s->setAlignment(Qt::AlignRight);
    s->setAccelerated(true);
    return s;
}

// 모델 → 위젯 반영 (시그널 차단, 같은 값이면 건드리지 않음)
inline void setSilently(QDoubleSpinBox* s, double v)
{
    if (std::abs(s->value() - v) < std::pow(10.0, -s->decimals()) / 2)
        return;
    const QSignalBlocker b(s);
    s->setValue(v);
}

inline QString formatDuration(double sec)
{
    const int total = int(std::lround(sec));
    const int h = total / 3600, m = (total % 3600) / 60, s = total % 60;
    if (h > 0)
        return QStringLiteral("%1시간 %2분 %3초").arg(h).arg(m).arg(s);
    if (m > 0)
        return QStringLiteral("%1분 %2초").arg(m).arg(s);
    return QStringLiteral("%1초").arg(s);
}

inline QString formatGeo(double lat, double lon)
{
    return QStringLiteral("%1°%2 %3°%4")
        .arg(std::abs(lat), 0, 'f', 4).arg(lat >= 0 ? QLatin1Char('N') : QLatin1Char('S'))
        .arg(std::abs(lon), 0, 'f', 4).arg(lon >= 0 ? QLatin1Char('E') : QLatin1Char('W'));
}

} // namespace EditorUtil
