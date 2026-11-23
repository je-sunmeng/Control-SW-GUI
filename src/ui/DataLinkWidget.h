#pragma once

#include <QWidget>

class QDoubleSpinBox;
class ScenarioModel;

// 데이터링크 전송간격 (연동모드에서만 표시): 전자전기 / 아군편대 위치를 재밍기법발생기로 보내는 주기
class DataLinkWidget : public QWidget {
    Q_OBJECT
public:
    explicit DataLinkWidget(ScenarioModel& model, QWidget* parent = nullptr);

private:
    ScenarioModel& m_model;
    QDoubleSpinBox* m_jammer;
    QDoubleSpinBox* m_formation;
};
