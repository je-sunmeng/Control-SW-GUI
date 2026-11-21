#include "ModeSelector.h"

#include <QHBoxLayout>
#include <QRadioButton>
#include <QSignalBlocker>

ModeSelector::ModeSelector(QWidget* parent)
    : QWidget(parent)
    , m_standalone(new QRadioButton(QStringLiteral("단독"), this))
    , m_linked(new QRadioButton(QStringLiteral("연동"), this))
{
    m_standalone->setToolTip(QStringLiteral("네트워크 미사용: 내부 MainLobe 생성·자체 탐지 판정, 배속/일시정지 가능"));
    m_linked->setToolTip(QStringLiteral("분석SW·신호발생장치 연동: 수신 MainLobe 탐지결과 전시, 1배속 고정"));
    m_standalone->setChecked(true);
    auto* l = new QHBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->addWidget(m_standalone);
    l->addWidget(m_linked);
    l->addStretch();
    connect(m_standalone, &QRadioButton::toggled, this, [this](bool on) {
        if (on)
            emit modeRequested(RunMode::Standalone);
    });
    connect(m_linked, &QRadioButton::toggled, this, [this](bool on) {
        if (on)
            emit modeRequested(RunMode::Linked);
    });
}

void ModeSelector::setMode(RunMode mode)
{
    const QSignalBlocker a(m_standalone), b(m_linked);
    (mode == RunMode::Standalone ? m_standalone : m_linked)->setChecked(true);
}
