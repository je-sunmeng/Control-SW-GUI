#pragma once

#include "common/Types.h"

#include <QHash>
#include <QStackedWidget>

class ScenarioModel;
class ObjectEditorBase;
class RadarEditor;
class JammerEditor;
class FormationEditor;
class FalseTargetEditor;

// 선택 항목 타입에 따른 에디터 전환. 잠금(임무 중) 시 전체 비활성
class ObjectEditorStack : public QStackedWidget {
    Q_OBJECT
public:
    explicit ObjectEditorStack(ScenarioModel& model, QWidget* parent = nullptr);

    ObjectId currentObject() const { return m_current; }

public slots:
    void setObject(ObjectId id);

signals:
    void redrawRouteRequested(ObjectId id);
    void placePointRequested(ObjectId id);

private:
    void addEditor(ObjectEditorBase* editor);
    void applyLock(bool locked);

    ScenarioModel& m_model;
    ObjectId m_current = 0;
    QWidget* m_empty;
    RadarEditor* m_radar;
    JammerEditor* m_jammer;
    FormationEditor* m_formation;
    FalseTargetEditor* m_falseTarget;
    QHash<ObjectEditorBase*, QWidget*> m_pageOf;   // 에디터 → 감싸는 QScrollArea
};
