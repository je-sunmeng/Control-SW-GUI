#pragma once

#include "common/Types.h"

#include <QDateTime>
#include <QWidget>

class QDoubleSpinBox;
class Logger;
class MapEditController;
class ObjectActionBar;
class ObjectEditorStack;
class ObjectListView;
class ScenarioModel;

// 좌측 오브젝트 패널
//  ActionBar(추가/저장/불러오기) / 오브젝트 목록 / 설정 에디터 (데이터링크 전송간격은 제어 패널, 연동모드)
//  추가 흐름(지도 편집 모드 진입)과 파일 저장/불러오기를 조율한다.
class ObjectPanel : public QWidget {
    Q_OBJECT
public:
    ObjectPanel(ScenarioModel& model, MapEditController& edit, Logger& logger, QWidget* parent = nullptr);

    ObjectId currentObject() const;

    // 오브젝트/시나리오 파일 기본 저장 폴더 (설정값 없으면 <문서>/CtrlSW, 없으면 생성)
    static QString defaultFolder();
    // 종류별 폴더: <기본 폴더>/<종류> (레이다/전자전기/아군편대/단독허위표적/시나리오), 없으면 생성
    static QString kindFolder(const QString& kind);
    // 기본 파일명: <종류>_yyyyMMdd_HHmm.<suffix>
    static QString defaultFileName(const QString& kind, const QString& suffix, const QDateTime& when);

public slots:
    void selectObject(ObjectId id);
    void chooseDefaultFolder();   // 파일 메뉴: 기본 저장 폴더 설정
    void openDefaultFolder();     // 파일 메뉴: 저장 폴더 열기

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

    QString askSavePath(const QString& title, const QString& filter, const QString& suffix, const QString& kind);
    static QString startFolder(const QString& kind);   // 종류별 마지막 사용 폴더(존재 시) 또는 종류 폴더
    QString askOpenPath(const QString& title, const QString& filter, const QString& kind);
    bool confirmReplace(const QString& what);

    ScenarioModel& m_model;
    MapEditController& m_edit;
    Logger& m_logger;
    ObjectActionBar* m_actions;
    ObjectListView* m_list;
    ObjectEditorStack* m_editors;
};
