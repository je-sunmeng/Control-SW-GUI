#pragma once

#include "ObjectEditorBase.h"

class QSpinBox;
class RouteSegmentTable;
class JammingConfigEditor;

// 아군편대: 이름, 편대규모(1~4), 동선, 교란설정
class FormationEditor : public ObjectEditorBase {
    Q_OBJECT
public:
    explicit FormationEditor(ScenarioModel& model, QWidget* parent = nullptr);

protected:
    void load() override;

private:
    QSpinBox* m_size;
    RouteSegmentTable* m_route;
    JammingConfigEditor* m_jamming;
};
