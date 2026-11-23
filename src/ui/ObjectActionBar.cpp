#include "ObjectActionBar.h"

#include "model/SimObject.h"

#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QToolButton>

namespace {
QToolButton* button(const QString& text, const QString& tip, QWidget* parent)
{
    auto* b = new QToolButton(parent);
    b->setText(text);
    b->setToolTip(tip);
    b->setToolButtonStyle(Qt::ToolButtonTextOnly);
    b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    return b;
}
} // namespace

ObjectActionBar::ObjectActionBar(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("ObjectActionBar"));
    auto* g = new QGridLayout(this);
    g->setContentsMargins(0, 0, 0, 0);
    g->setHorizontalSpacing(4);
    g->setVerticalSpacing(2);

    struct Row {
        ObjectType type;
        QString label;
    };
    const Row rows[] = {
        {ObjectType::Radar, QStringLiteral("레이다")},
        {ObjectType::Jammer, QStringLiteral("전자전기")},
        {ObjectType::Formation, QStringLiteral("아군편대")},
        {ObjectType::StandaloneFalseTarget, QStringLiteral("단독허위표적")},
    };
    int r = 0;
    for (const Row& row : rows) {
        g->addWidget(new QLabel(row.label, this), r, 0);
        auto* add = button(QStringLiteral("추가"), QStringLiteral("%1 추가").arg(row.label), this);
        auto* save = button(QStringLiteral("저장"), QStringLiteral("%1 저장").arg(row.label), this);
        auto* load = button(QStringLiteral("불러오기"), QStringLiteral("%1 불러오기 (기존 항목 교체)").arg(row.label), this);
        const ObjectType t = row.type;
        connect(add, &QToolButton::clicked, this, [this, t] { emit addRequested(t); });
        connect(save, &QToolButton::clicked, this, [this, t] { emit saveRequested(t); });
        connect(load, &QToolButton::clicked, this, [this, t] { emit loadRequested(t); });
        g->addWidget(add, r, 1);
        g->addWidget(save, r, 2);
        g->addWidget(load, r, 3);
        m_editButtons << add << load;
        ++r;
    }

    auto* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet(QStringLiteral("color:#353c46;"));
    g->addWidget(line, r++, 0, 1, 4);

    g->addWidget(new QLabel(QStringLiteral("시나리오"), this), r, 0);
    auto* nw = button(QStringLiteral("초기화"), QStringLiteral("모든 오브젝트를 삭제하고 시나리오 초기화"), this);
    auto* save = button(QStringLiteral("저장"), QStringLiteral("시나리오 전체 저장 (*.scenario.json)"), this);
    auto* load = button(QStringLiteral("불러오기"), QStringLiteral("시나리오 전체 불러오기 (현재 시나리오 교체)"), this);
    connect(nw, &QToolButton::clicked, this, &ObjectActionBar::newScenarioRequested);
    connect(save, &QToolButton::clicked, this, &ObjectActionBar::saveScenarioRequested);
    connect(load, &QToolButton::clicked, this, &ObjectActionBar::loadScenarioRequested);
    g->addWidget(nw, r, 1);
    g->addWidget(save, r, 2);
    g->addWidget(load, r, 3);
    m_editButtons << nw << load;
    g->setColumnStretch(1, 1);
    g->setColumnStretch(2, 1);
    g->setColumnStretch(3, 1);
}

void ObjectActionBar::setEditingEnabled(bool on)
{
    for (QWidget* w : std::as_const(m_editButtons))
        w->setEnabled(on);
}
