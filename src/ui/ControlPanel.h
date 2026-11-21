#pragma once

#include <QWidget>

// 우측 제어 패널 컨테이너
//  ModeSelector / DeviceStatus / MainLobe Lamp / SimControl / SpeedSelector
// M1: 섹션 골격만 배치. 각 위젯은 M4(단독), M5(연동)에서 구현 후 교체.
class ControlPanel : public QWidget {
    Q_OBJECT
public:
    explicit ControlPanel(QWidget* parent = nullptr);
};
