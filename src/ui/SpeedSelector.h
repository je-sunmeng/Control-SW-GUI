#pragma once

#include <QWidget>

class QButtonGroup;

// 1/2/4/8 배속 (연동모드에서는 1배 고정·비활성)
class SpeedSelector : public QWidget {
    Q_OBJECT
public:
    explicit SpeedSelector(QWidget* parent = nullptr);

    void setSpeed(int factor);   // 시그널 없이 표시만
    int speed() const;

signals:
    void speedRequested(int factor);

private:
    QButtonGroup* m_group;
};
