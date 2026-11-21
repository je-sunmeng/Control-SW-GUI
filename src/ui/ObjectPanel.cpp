#include "ObjectPanel.h"

#include <QGroupBox>
#include <QLabel>
#include <QSplitter>
#include <QVBoxLayout>

namespace {
QGroupBox* section(const QString& title, const QString& note, QWidget* parent)
{
    auto* box = new QGroupBox(title, parent);
    auto* l = new QVBoxLayout(box);
    auto* label = new QLabel(note, box);
    label->setObjectName(QStringLiteral("placeholderLabel"));
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);
    l->addWidget(label);
    return box;
}
} // namespace

ObjectPanel::ObjectPanel(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("ObjectPanel"));
    setMinimumWidth(260);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(6);
    layout->addWidget(section(QStringLiteral("오브젝트"),
                              QStringLiteral("탐지기 / 전자전기 / 아군편대 / 단독허위표적\n추가 · 저장 · 불러오기 (M3)"), this));
    layout->addWidget(section(QStringLiteral("데이터링크 전송간격"), QStringLiteral("(M3)"), this));

    auto* split = new QSplitter(Qt::Vertical, this);
    split->setChildrenCollapsible(false);
    split->addWidget(section(QStringLiteral("오브젝트 목록"), QStringLiteral("ObjectListView (M3)"), split));
    split->addWidget(section(QStringLiteral("설정"), QStringLiteral("ObjectEditorStack (M3)"), split));
    split->setStretchFactor(0, 1);
    split->setStretchFactor(1, 2);
    layout->addWidget(split, 1);
}
