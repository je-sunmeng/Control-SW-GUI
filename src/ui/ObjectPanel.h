#pragma once

#include "common/Types.h"

#include <QWidget>

class QDoubleSpinBox;
class Logger;
class MapEditController;
class ObjectActionBar;
class ObjectEditorStack;
class ObjectListView;
class ScenarioModel;

// 좌측 오브젝트 패널
//  ActionBar(추가/저장/불러오기) / 데이터링크 전송간격 / 오브젝트 목록 / 설정 에디터
//  추가 흐름(지도 편집 모드 진입)과 파일 저장/불러오기를 조율한다.
class ObjectPanel : public QWidget {
    Q_OBJECT
public:
    ObjectPanel(ScenarioModel& model, MapEditController& edit, Logger& logger, QWidget* parent = nullptr);

    ObjectId currentObject() const;

public slots:
    void selectObject(ObjectId id);

signals:
    void objectSelected(ObjectId id);

private:
    void onAdd(ObjectType type);
    void onSave(ObjectType type);
    void onLoad(ObjectType type);
    void onNewScenario();
    void onSaveScenario();
    void onLoadScenario();
    void onDelete(ObjectId id);
    void onLockedChanged(bool locked);

    QString askSavePath(const QString& title, const QString& filter, const QString& suffix);
    QString askOpenPath(const QString& title, const QString& filter);
    bool confirmReplace(const QString& what);

    ScenarioModel& m_model;
    MapEditController& m_edit;
    Logger& m_logger;
    ObjectActionBar* m_actions;
    QDoubleSpinBox* m_datalink;
    ObjectListView* m_list;
    ObjectEditorStack* m_editors;
};
