#include "SpeedSelector.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSignalBlocker>

SpeedSelector::SpeedSelector(QWidget* parent)
    : QWidget(parent)
    , m_group(new QButtonGroup(this))
{
    setObjectName(QStringLiteral("SpeedSelector"));
    auto* l = new QHBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(4);
    for (int f : {1, 2, 4, 8}) {
        auto* b = new QPushButton(QStringLiteral("%1x").arg(f), this);
        b->setCheckable(true);
        b->setMinimumWidth(36);
        m_group->addButton(b, f);
        l->addWidget(b);
    }
    m_group->setExclusive(true);
    m_group->button(1)->setChecked(true);
    connect(m_group, &QButtonGroup::idClicked, this, &SpeedSelector::speedRequested);
}

void SpeedSelector::setSpeed(int factor)
{
    if (QAbstractButton* b = m_group->button(factor)) {
        const QSignalBlocker block(m_group);
        b->setChecked(true);
    }
}

int SpeedSelector::speed() const
{
    return m_group->checkedId();
}
