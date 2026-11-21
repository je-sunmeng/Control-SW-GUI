#include "ControlPanel.h"

#include <QGroupBox>
#include <QLabel>
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

ControlPanel::ControlPanel(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("ControlPanel"));
    setMinimumWidth(220);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(6);
    layout->addWidget(section(QStringLiteral("운용 모드"), QStringLiteral("○ 단독  ○ 연동 (M4/M5)"), this));
    layout->addWidget(section(QStringLiteral("장치 연결 상태"), QStringLiteral("분석SW / 신호발생장치 (M5)"), this));
    layout->addWidget(section(QStringLiteral("MainLobe"), QStringLiteral("R1 R2 R3 R4 (M4/M5)"), this));
    layout->addWidget(section(QStringLiteral("시뮬레이션 제어"),
                              QStringLiteral("상태 / 임무전송 / 시작 / 일시정지 / 중지 / 새로고침 (M4)"), this));
    layout->addWidget(section(QStringLiteral("배속"), QStringLiteral("1x 2x 4x 8x (M4)"), this));
    layout->addStretch(1);
}
