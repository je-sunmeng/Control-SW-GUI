#include "FalseTargetEditor.h"

#include "EditorUtil.h"
#include "RouteSegmentTable.h"
#include "model/ScenarioModel.h"

#include <QFormLayout>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

FalseTargetEditor::FalseTargetEditor(ScenarioModel& model, QWidget* parent)
    : ObjectEditorBase(model, parent)
    , m_type(new QLabel(this))
    , m_pages(new QStackedWidget(this))
    , m_pointPage(new QWidget(this))
    , m_posState(new QLabel(this))
    , m_lat(EditorUtil::makeSpin(-90, 90, 5, QStringLiteral("°"), 0.01, this))
    , m_lon(EditorUtil::makeSpin(-180, 180, 5, QStringLiteral("°"), 0.01, this))
    , m_angleRow(new QWidget(this))
    , m_angle(EditorUtil::makeSpin(FalseTargetLimits::MinDetectAngleDeg, FalseTargetLimits::MaxDetectAngleDeg, 1,
                                   QStringLiteral("°"), 1, this))
    , m_route(new RouteSegmentTable(this))
    , m_randomPage(new QWidget(this))
{
    // --- 위치형(고정/잡음) 페이지 ---
    auto* pick = new QPushButton(QStringLiteral("지도에서 지정"), m_pointPage);
    connect(pick, &QPushButton::clicked, this, [this] { emit placePointRequested(m_id); });
    m_posState->setObjectName(QStringLiteral("placeholderLabel"));
    auto* angleLayout = new QHBoxLayout(m_angleRow);
    angleLayout->setContentsMargins(0, 0, 0, 0);
    angleLayout->addWidget(m_angle);
    m_angle->setToolTip(QStringLiteral("레이다→잡음표적 방위 ±(탐지각도/2) 섹터에서 해당 레이다 탐지 불가"));

    auto* pf = new QFormLayout(m_pointPage);
    pf->setContentsMargins(0, 0, 0, 0);
    pf->addRow(QStringLiteral("위도"), m_lat);
    pf->addRow(QStringLiteral("경도"), m_lon);
    pf->addRow(QStringLiteral("탐지각도"), m_angleRow);
    pf->addRow(QString(), pick);
    pf->addRow(QString(), m_posState);

    // --- 랜덤 페이지 ---
    auto* rl = new QVBoxLayout(m_randomPage);
    auto* info = new QLabel(QStringLiteral("랜덤 허위표적은 위치 입력이 없습니다.\n"
                                           "임무 중 각 레이다 탐지범위 안에 임의 위치로 생성되어\n"
                                           "옅은 반원으로 나타났다 사라집니다 (탐지 판정 대상 아님)."),
                            m_randomPage);
    info->setWordWrap(true);
    info->setObjectName(QStringLiteral("placeholderLabel"));
    rl->addWidget(info);
    rl->addStretch();

    m_pages->addWidget(m_pointPage);
    m_pages->addWidget(m_route);
    m_pages->addWidget(m_randomPage);

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("이름"), makeNameEdit());
    form->addRow(QStringLiteral("타입"), m_type);

    auto* l = new QVBoxLayout(this);
    l->setContentsMargins(4, 4, 4, 4);
    l->addLayout(form);
    l->addWidget(m_pages);
    l->addStretch(1);

    connectRouteTable(m_route);
    connect(m_lat, &QDoubleSpinBox::valueChanged, this, &FalseTargetEditor::commitPosition);
    connect(m_lon, &QDoubleSpinBox::valueChanged, this, &FalseTargetEditor::commitPosition);
    connect(m_angle, &QDoubleSpinBox::valueChanged, this, [this](double v) {
        if (const StandaloneFalseTarget* t = m_model.falseTarget(m_id)) {
            StandaloneFalseTarget c = *t;
            c.detectAngleDeg = v;
            m_model.updateFalseTarget(c);
        }
    });
}

void FalseTargetEditor::commitPosition()
{
    m_model.setPointPosition(m_id, {m_lat->value(), m_lon->value()});
}

void FalseTargetEditor::load()
{
    const StandaloneFalseTarget* t = m_model.falseTarget(m_id);
    if (!t)
        return;
    loadName();
    m_type->setText(toDisplayString(t->ftType));
    switch (t->ftType) {
    case FalseTargetType::Fixed:
    case FalseTargetType::Noise:
        m_pages->setCurrentWidget(m_pointPage);
        m_angleRow->setVisible(t->ftType == FalseTargetType::Noise);
        if (auto* f = qobject_cast<QFormLayout*>(m_pointPage->layout()))
            f->setRowVisible(m_angleRow, t->ftType == FalseTargetType::Noise);
        EditorUtil::setSilently(m_lat, t->posSet ? t->pos.lat : 0.0);
        EditorUtil::setSilently(m_lon, t->posSet ? t->pos.lon : 0.0);
        EditorUtil::setSilently(m_angle, t->detectAngleDeg);
        m_lat->setEnabled(t->posSet);
        m_lon->setEnabled(t->posSet);
        m_posState->setText(t->posSet ? QString() : QStringLiteral("위치 미지정 — [지도에서 지정]을 누르세요"));
        break;
    case FalseTargetType::Moving:
        m_pages->setCurrentWidget(m_route);
        m_route->setRoute(t->route);
        break;
    case FalseTargetType::Random:
        m_pages->setCurrentWidget(m_randomPage);
        break;
    }
}
