#pragma once

#include "common/Types.h"

#include <QWidget>

class QLineEdit;
class ScenarioModel;
class RouteSegmentTable;

// 오브젝트 에디터 공통: 대상 ID, 이름 필드, 모델 변경 시 자동 재로딩, 동선 테이블 연결
class ObjectEditorBase : public QWidget {
    Q_OBJECT
public:
    explicit ObjectEditorBase(ScenarioModel& model, QWidget* parent = nullptr);

    void setObject(ObjectId id);
    ObjectId objectId() const { return m_id; }

signals:
    void redrawRouteRequested(ObjectId id);
    void placePointRequested(ObjectId id);

protected:
    virtual void load() = 0;   // 모델 → 위젯 (m_id 유효할 때만 호출)

    QLineEdit* makeNameEdit();
    void loadName();
    void connectRouteTable(RouteSegmentTable* table);

    ScenarioModel& m_model;
    ObjectId m_id = 0;
    QLineEdit* m_name = nullptr;
};
