#include "WheelGuard.h"

#include <QAbstractScrollArea>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QComboBox>
#include <QWheelEvent>

WheelGuard::WheelGuard(QObject* parent)
    : QObject(parent)
{
}

bool WheelGuard::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() != QEvent::Wheel)
        return false;
    auto* w = qobject_cast<QWidget*>(watched);
    if (!w)
        return false;

    // 스핀박스/콤보박스 자신 또는 그 내부 위젯(스핀박스의 QLineEdit 등)인지
    QWidget* control = nullptr;
    for (QWidget* p = w; p; p = p->parentWidget()) {
        if (qobject_cast<QAbstractSpinBox*>(p) || qobject_cast<QComboBox*>(p)) {
            control = p;
            break;
        }
        if (qobject_cast<QAbstractScrollArea*>(p))
            break;   // 스크롤 영역에 먼저 닿으면 대상 아님
    }
    if (!control)
        return false;

    // 바깥 스크롤 영역의 viewport 로 같은 휠 이벤트 전달
    for (QWidget* p = control->parentWidget(); p; p = p->parentWidget()) {
        if (auto* area = qobject_cast<QAbstractScrollArea*>(p)) {
            auto* we = static_cast<QWheelEvent*>(event);
            QWidget* vp = area->viewport();
            QWheelEvent forwarded(vp->mapFromGlobal(we->globalPosition()), we->globalPosition(), we->pixelDelta(),
                                  we->angleDelta(), we->buttons(), we->modifiers(), we->phase(), we->inverted());
            QApplication::sendEvent(vp, &forwarded);
            return true;
        }
    }
    return true;   // 스크롤 영역이 없으면 휠 무시 (값 변경 금지)
}
