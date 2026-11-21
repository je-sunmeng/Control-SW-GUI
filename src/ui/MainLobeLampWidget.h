#pragma once

#include "common/Types.h"

#include <QHash>
#include <QWidget>

class QLabel;
class LampIndicator;
class ScenarioModel;

// 레이다별 MainLobe 램프 R1~R4 (레이다 색상). MainLobe 수신(단독: 내부 생성) 시 점등 후 감쇠
class MainLobeLampWidget : public QWidget {
    Q_OBJECT
public:
    explicit MainLobeLampWidget(ScenarioModel& model, QWidget* parent = nullptr);

public slots:
    void onMainLobe(ObjectId radarId);
    void refreshRadars();

private:
    ScenarioModel& m_model;
    LampIndicator* m_lamps[4];
    QLabel* m_labels[4];
    QHash<ObjectId, int> m_slotOf;   // radarId → colorIndex(=램프 위치)
};
