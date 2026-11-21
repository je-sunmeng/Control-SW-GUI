#pragma once

#include "common/Types.h"

#include <QStandardItemModel>

class ScenarioModel;
class QStandardItem;

// 오브젝트 목록 모델: 그룹 노드(레이다/전자전기/아군편대/단독허위표적) + 항목
//  - 추가/삭제/초기화 시 재구성, 변경 시 해당 행 텍스트만 갱신
//  - 레이다는 색상 아이콘
class ObjectTreeModel : public QStandardItemModel {
    Q_OBJECT
public:
    enum Role { ObjectIdRole = Qt::UserRole + 1, GroupTypeRole };

    explicit ObjectTreeModel(ScenarioModel& model, QObject* parent = nullptr);

    QModelIndex indexOf(ObjectId id) const;
    static ObjectId objectIdOf(const QModelIndex& index);

    void rebuild();

signals:
    void rebuilt();

private:
    void refresh(ObjectId id);
    QString itemText(ObjectId id) const;
    QString itemToolTip(ObjectId id) const;
    void updateGroupTitles();

    ScenarioModel& m_model;
    QStandardItem* m_groups[4] = {};
};
