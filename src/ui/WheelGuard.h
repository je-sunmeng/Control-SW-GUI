#pragma once

#include <QObject>

// 앱 전역 이벤트 필터: 스핀박스/콤보박스 위에서 마우스 휠로 값이 바뀌지 않게 한다.
// 휠 이벤트는 가장 가까운 바깥 스크롤 영역(설정 패널, 테이블 등)으로 전달 → 값 대신 화면이 스크롤된다.
// 사용: app.installEventFilter(new WheelGuard(&app));
class WheelGuard : public QObject {
    Q_OBJECT
public:
    explicit WheelGuard(QObject* parent = nullptr);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
};
