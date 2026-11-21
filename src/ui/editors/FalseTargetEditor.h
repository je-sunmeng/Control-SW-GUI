#pragma once

#include "ObjectEditorBase.h"

class QDoubleSpinBox;
class QLabel;
class QStackedWidget;
class QWidget;
class RouteSegmentTable;

// 단독 허위표적: 타입별 입력 폼
//  고정: 위치 / 이동: 구간 테이블 / 랜덤: 안내문 / 잡음: 위치 + 탐지각도
class FalseTargetEditor : public ObjectEditorBase {
    Q_OBJECT
public:
    explicit FalseTargetEditor(ScenarioModel& model, QWidget* parent = nullptr);

protected:
    void load() override;

private:
    void commitPosition();

    QLabel* m_type;
    QStackedWidget* m_pages;
    QWidget* m_pointPage;
    QLabel* m_posState;
    QDoubleSpinBox* m_lat;
    QDoubleSpinBox* m_lon;
    QWidget* m_angleRow;
    QDoubleSpinBox* m_angle;
    RouteSegmentTable* m_route;
    QWidget* m_randomPage;
};
