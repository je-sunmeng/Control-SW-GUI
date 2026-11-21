#pragma once

#include "ObjectEditorBase.h"

class RouteSegmentTable;

class JammerEditor : public ObjectEditorBase {
    Q_OBJECT
public:
    explicit JammerEditor(ScenarioModel& model, QWidget* parent = nullptr);

protected:
    void load() override;

private:
    RouteSegmentTable* m_route;
};
