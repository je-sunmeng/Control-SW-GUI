#include "ObjectPanel.h"

#include "ObjectActionBar.h"
#include "ObjectListView.h"
#include "dialogs/FalseTargetTypeDialog.h"
#include "editors/EditorUtil.h"
#include "editors/ObjectEditorStack.h"
#include "log/Logger.h"
#include "map/MapEditController.h"
#include "model/ScenarioModel.h"
#include "persistence/ObjectFileIO.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QSettings>
#include <QSplitter>
#include <QStandardPaths>
#include <QVBoxLayout>

namespace {
const QString kSource = QStringLiteral("Scenario");
const QString kLastDirKey = QStringLiteral("paths/objectFiles");

QGroupBox* boxed(const QString& title, QWidget* content, QWidget* parent)
{
    auto* box = new QGroupBox(title, parent);
    auto* l = new QVBoxLayout(box);
    l->setContentsMargins(4, 6, 4, 4);
    l->addWidget(content);
    return box;
}
} // namespace

ObjectPanel::ObjectPanel(ScenarioModel& model, MapEditController& edit, Logger& logger, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
    , m_edit(edit)
    , m_logger(logger)
    , m_actions(new ObjectActionBar(this))
    , m_datalink(EditorUtil::makeSpin(ScenarioModel::MinDatalinkIntervalSec, ScenarioModel::MaxDatalinkIntervalSec, 1,
                                      QStringLiteral("s"), 0.1, this))
    , m_list(new ObjectListView(model, this))
    , m_editors(new ObjectEditorStack(model, this))
{
    setObjectName(QStringLiteral("ObjectPanel"));
    setMinimumWidth(300);

    m_datalink->setValue(m_model.datalinkIntervalSec());
    m_datalink->setToolTip(QStringLiteral("연동모드에서 아군편대 현재위치를 신호발생장치로 보내는 주기"));
    auto* dlWidget = new QWidget(this);
    auto* dlForm = new QFormLayout(dlWidget);
    dlForm->setContentsMargins(0, 0, 0, 0);
    dlForm->addRow(QStringLiteral("편대 위치 전송간격"), m_datalink);

    auto* split = new QSplitter(Qt::Vertical, this);
    split->setChildrenCollapsible(false);
    split->addWidget(boxed(QStringLiteral("오브젝트 목록"), m_list, split));
    split->addWidget(boxed(QStringLiteral("설정"), m_editors, split));
    split->setStretchFactor(0, 1);
    split->setStretchFactor(1, 3);
    split->setSizes({200, 600});
    m_list->setMinimumHeight(120);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(6);
    layout->addWidget(boxed(QStringLiteral("오브젝트"), m_actions, this));
    layout->addWidget(boxed(QStringLiteral("데이터링크"), dlWidget, this));
    layout->addWidget(split, 1);

    // 버튼
    connect(m_actions, &ObjectActionBar::addRequested, this, &ObjectPanel::onAdd);
    connect(m_actions, &ObjectActionBar::saveRequested, this, &ObjectPanel::onSave);
    connect(m_actions, &ObjectActionBar::loadRequested, this, &ObjectPanel::onLoad);
    connect(m_actions, &ObjectActionBar::newScenarioRequested, this, &ObjectPanel::onNewScenario);
    connect(m_actions, &ObjectActionBar::saveScenarioRequested, this, &ObjectPanel::onSaveScenario);
    connect(m_actions, &ObjectActionBar::loadScenarioRequested, this, &ObjectPanel::onLoadScenario);

    // 데이터링크 간격 ↔ 모델
    connect(m_datalink, &QDoubleSpinBox::valueChanged, &m_model, &ScenarioModel::setDatalinkIntervalSec);
    connect(&m_model, &ScenarioModel::datalinkIntervalChanged, this,
            [this](double v) { EditorUtil::setSilently(m_datalink, v); });

    // 목록 ↔ 에디터
    connect(m_list, &ObjectListView::objectSelected, m_editors, &ObjectEditorStack::setObject);
    connect(m_list, &ObjectListView::objectSelected, this, &ObjectPanel::objectSelected);
    connect(m_list, &ObjectListView::deleteRequested, this, &ObjectPanel::onDelete);
    connect(m_list, &ObjectListView::editRouteRequested, this, [this](ObjectId id) { m_edit.startDrawRoute(id); });

    // 에디터 → 지도 편집
    connect(m_editors, &ObjectEditorStack::redrawRouteRequested, this, [this](ObjectId id) { m_edit.startDrawRoute(id); });
    connect(m_editors, &ObjectEditorStack::placePointRequested, this, [this](ObjectId id) { m_edit.startPlacePoint(id); });

    // 지도 편집 완료 → 선택
    connect(&m_edit, &MapEditController::objectCreated, this, &ObjectPanel::selectObject);
    connect(&m_edit, &MapEditController::editFinished, this, &ObjectPanel::selectObject);

    connect(&m_model, &ScenarioModel::lockedChanged, this, &ObjectPanel::onLockedChanged);
    onLockedChanged(m_model.isLocked());
}

ObjectId ObjectPanel::currentObject() const
{
    return m_list->currentObject();
}

void ObjectPanel::selectObject(ObjectId id)
{
    m_list->setCurrentObject(id);
}

void ObjectPanel::onLockedChanged(bool locked)
{
    m_actions->setEditingEnabled(!locked);
    m_datalink->setEnabled(!locked);
}

// ---------------------------------------------------------------------------- 추가

void ObjectPanel::onAdd(ObjectType type)
{
    switch (type) {
    case ObjectType::Radar:
        if (m_model.radars().size() >= Limits::MaxRadars) {
            QMessageBox::information(this, QStringLiteral("레이다 추가"),
                                     QStringLiteral("레이다는 최대 %1개까지 추가할 수 있습니다.").arg(Limits::MaxRadars));
            return;
        }
        m_edit.startPlaceRadar();
        break;
    case ObjectType::Jammer: {
        if (m_model.jammer()) {
            selectObject(m_model.jammer()->id);
            QMessageBox::information(this, QStringLiteral("전자전기 추가"),
                                     QStringLiteral("전자전기는 1개만 추가할 수 있습니다.\n"
                                                    "동선을 바꾸려면 설정의 [동선 다시 그리기]를 사용하세요."));
            return;
        }
        const ObjectId id = m_model.createJammer();
        selectObject(id);
        m_edit.startDrawRoute(id, true);
        break;
    }
    case ObjectType::Formation: {
        if (m_model.formation()) {
            selectObject(m_model.formation()->id);
            QMessageBox::information(this, QStringLiteral("아군편대 추가"),
                                     QStringLiteral("아군편대는 최대 %1개까지 추가할 수 있습니다.").arg(Limits::MaxFormations));
            return;
        }
        const ObjectId id = m_model.createFormation();
        selectObject(id);
        m_edit.startDrawRoute(id, true);
        break;
    }
    case ObjectType::StandaloneFalseTarget: {
        const auto ft = FalseTargetTypeDialog::ask(this);
        if (!ft)
            return;
        const ObjectId id = m_model.addFalseTarget(*ft);
        if (!id)
            return;
        selectObject(id);
        if (*ft == FalseTargetType::Moving)
            m_edit.startDrawRoute(id, true);
        else if (*ft != FalseTargetType::Random)
            m_edit.startPlacePoint(id, true);
        break;
    }
    }
}

void ObjectPanel::onDelete(ObjectId id)
{
    const SimObject* o = m_model.find(id);
    if (!o)
        return;
    const QString name = o->name;
    if (m_model.removeObject(id))
        m_logger.info(QStringLiteral("%1 삭제").arg(name), kSource);
}

// ---------------------------------------------------------------------------- 파일

QString ObjectPanel::askSavePath(const QString& title, const QString& filter, const QString& suffix)
{
    QSettings s;
    const QString dir = s.value(kLastDirKey, QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
    QString path = QFileDialog::getSaveFileName(this, title, dir, filter);
    if (path.isEmpty())
        return {};
    if (!path.endsWith(QLatin1Char('.') + suffix, Qt::CaseInsensitive)) {
        if (path.endsWith(QLatin1String(".json"), Qt::CaseInsensitive))
            path.chop(5);
        path += QLatin1Char('.') + suffix;
    }
    s.setValue(kLastDirKey, QFileInfo(path).absolutePath());
    return path;
}

QString ObjectPanel::askOpenPath(const QString& title, const QString& filter)
{
    QSettings s;
    const QString dir = s.value(kLastDirKey, QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
    const QString path = QFileDialog::getOpenFileName(this, title, dir, filter);
    if (!path.isEmpty())
        s.setValue(kLastDirKey, QFileInfo(path).absolutePath());
    return path;
}

bool ObjectPanel::confirmReplace(const QString& what)
{
    return QMessageBox::question(this, QStringLiteral("불러오기"),
                                 QStringLiteral("기존 %1을(를) 불러온 항목으로 교체합니다. 계속할까요?").arg(what))
           == QMessageBox::Yes;
}

void ObjectPanel::onSave(ObjectType type)
{
    const QString label = toDisplayString(type);
    bool empty = false;
    switch (type) {
    case ObjectType::Radar:                 empty = m_model.radars().isEmpty(); break;
    case ObjectType::Jammer:                empty = !m_model.jammer(); break;
    case ObjectType::Formation:             empty = !m_model.formation(); break;
    case ObjectType::StandaloneFalseTarget: empty = m_model.falseTargets().isEmpty(); break;
    }
    if (empty) {
        QMessageBox::information(this, QStringLiteral("%1 저장").arg(label), QStringLiteral("저장할 %1이(가) 없습니다.").arg(label));
        return;
    }
    const QString path = askSavePath(QStringLiteral("%1 저장").arg(label), ObjectFileIO::fileFilter(type),
                                     ObjectFileIO::defaultSuffix(type));
    if (path.isEmpty())
        return;
    QString err;
    bool ok = false;
    switch (type) {
    case ObjectType::Radar:                 ok = ObjectFileIO::saveRadars(path, m_model.radars(), &err); break;
    case ObjectType::Jammer:                ok = ObjectFileIO::saveJammer(path, *m_model.jammer(), &err); break;
    case ObjectType::Formation:             ok = ObjectFileIO::saveFormation(path, *m_model.formation(), &err); break;
    case ObjectType::StandaloneFalseTarget: ok = ObjectFileIO::saveFalseTargets(path, m_model.falseTargets(), &err); break;
    }
    if (ok) {
        m_logger.info(QStringLiteral("%1 저장: %2").arg(label, QDir::toNativeSeparators(path)), kSource);
    } else {
        m_logger.error(QStringLiteral("%1 저장 실패: %2").arg(label, err), kSource);
        QMessageBox::warning(this, QStringLiteral("%1 저장").arg(label), err);
    }
}

void ObjectPanel::onLoad(ObjectType type)
{
    const QString label = toDisplayString(type);
    const QString path = askOpenPath(QStringLiteral("%1 불러오기").arg(label), ObjectFileIO::fileFilter(type));
    if (path.isEmpty())
        return;
    m_edit.cancel();
    QString err;
    bool ok = false;
    int count = 0;
    switch (type) {
    case ObjectType::Radar:
        if (auto v = ObjectFileIO::loadRadars(path, &err)) {
            if (!m_model.radars().isEmpty() && !confirmReplace(label))
                return;
            count = int(v->size());
            ok = m_model.replaceRadars(*v);
        }
        break;
    case ObjectType::Jammer:
        if (auto v = ObjectFileIO::loadJammer(path, &err)) {
            if (m_model.jammer() && !confirmReplace(label))
                return;
            count = 1;
            ok = m_model.replaceJammer(*v);
        }
        break;
    case ObjectType::Formation:
        if (auto v = ObjectFileIO::loadFormation(path, &err)) {
            if (m_model.formation() && !confirmReplace(label))
                return;
            count = 1;
            ok = m_model.replaceFormation(*v);
        }
        break;
    case ObjectType::StandaloneFalseTarget:
        if (auto v = ObjectFileIO::loadFalseTargets(path, &err)) {
            if (!m_model.falseTargets().isEmpty() && !confirmReplace(label))
                return;
            count = int(v->size());
            ok = m_model.replaceFalseTargets(*v);
        }
        break;
    }
    if (ok) {
        m_logger.info(QStringLiteral("%1 %2개 불러오기: %3").arg(label).arg(count).arg(QDir::toNativeSeparators(path)), kSource);
    } else {
        m_logger.error(QStringLiteral("%1 불러오기 실패: %2").arg(label, err), kSource);
        QMessageBox::warning(this, QStringLiteral("%1 불러오기").arg(label), err.isEmpty() ? QStringLiteral("불러오지 못했습니다.") : err);
    }
}

void ObjectPanel::onNewScenario()
{
    if (m_model.isEmpty())
        return;
    if (QMessageBox::question(this, QStringLiteral("새 시나리오"), QStringLiteral("모든 오브젝트를 삭제합니다. 계속할까요?"))
        != QMessageBox::Yes)
        return;
    m_edit.cancel();
    m_model.clear();
    m_logger.info(QStringLiteral("새 시나리오"), kSource);
}

void ObjectPanel::onSaveScenario()
{
    const QString path = askSavePath(QStringLiteral("시나리오 저장"), ObjectFileIO::scenarioFileFilter(),
                                     QStringLiteral("scenario.json"));
    if (path.isEmpty())
        return;
    QString err;
    if (ObjectFileIO::saveScenario(path, m_model.snapshot(), &err)) {
        m_logger.info(QStringLiteral("시나리오 저장: %1").arg(QDir::toNativeSeparators(path)), kSource);
    } else {
        m_logger.error(QStringLiteral("시나리오 저장 실패: %1").arg(err), kSource);
        QMessageBox::warning(this, QStringLiteral("시나리오 저장"), err);
    }
}

void ObjectPanel::onLoadScenario()
{
    const QString path = askOpenPath(QStringLiteral("시나리오 불러오기"), ObjectFileIO::scenarioFileFilter());
    if (path.isEmpty())
        return;
    QString err;
    const auto data = ObjectFileIO::loadScenario(path, &err);
    if (!data) {
        m_logger.error(QStringLiteral("시나리오 불러오기 실패: %1").arg(err), kSource);
        QMessageBox::warning(this, QStringLiteral("시나리오 불러오기"), err);
        return;
    }
    if (!m_model.isEmpty() && !confirmReplace(QStringLiteral("시나리오")))
        return;
    m_edit.cancel();
    m_model.replaceAll(*data);
    m_logger.info(QStringLiteral("시나리오 불러오기: %1").arg(QDir::toNativeSeparators(path)), kSource);
}
