#pragma once

#include "common/Types.h"

#include <QWidget>

class QLabel;
class QPushButton;
struct MissionActions;

// 상태 표기 + 임무전송/시작(재개)/일시정지(단독 전용)/중지/새로고침
class SimControlWidget : public QWidget {
    Q_OBJECT
public:
    explicit SimControlWidget(QWidget* parent = nullptr);

    void applyActions(const MissionActions& actions);
    void setState(SimState state);
    void setSimTime(double sec);

signals:
    void sendMissionClicked();
    void startClicked();
    void pauseClicked();
    void stopClicked();
    void refreshClicked();

private:
    QLabel* m_state;
    QLabel* m_time;
    QPushButton* m_send;
    QPushButton* m_start;
    QPushButton* m_pause;
    QPushButton* m_stop;
    QPushButton* m_refresh;
};
