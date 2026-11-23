#pragma once

#include "common/Types.h"

#include <QWidget>

// 오브젝트별 추가/저장/불러오기 버튼 + 시나리오 전체 초기화/저장/불러오기
class ObjectActionBar : public QWidget {
    Q_OBJECT
public:
    explicit ObjectActionBar(QWidget* parent = nullptr);

    void setEditingEnabled(bool on);   // 임무 중 추가/불러오기/새로 비활성 (저장은 허용)

signals:
    void addRequested(ObjectType type);
    void saveRequested(ObjectType type);
    void loadRequested(ObjectType type);
    void newScenarioRequested();
    void saveScenarioRequested();
    void loadScenarioRequested();

private:
    QList<QWidget*> m_editButtons;
};
