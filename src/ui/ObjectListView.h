#pragma once

#include "common/Types.h"

#include <QTreeView>

class ObjectTreeModel;
class ScenarioModel;

// 오브젝트 목록 + 선택 + 삭제(우클릭 메뉴 / Delete 키)
class ObjectListView : public QTreeView {
    Q_OBJECT
public:
    ObjectListView(ScenarioModel& model, QWidget* parent = nullptr);

    ObjectId currentObject() const { return m_current; }

public slots:
    void setCurrentObject(ObjectId id);

signals:
    void objectSelected(ObjectId id);   // 0 = 선택 해제
    void deleteRequested(ObjectId id);
    void editRouteRequested(ObjectId id);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    void onSelectionChanged();
    void restoreSelection();

    ScenarioModel& m_model;
    ObjectTreeModel* m_tree;
    ObjectId m_current = 0;
    bool m_restoring = false;
};
