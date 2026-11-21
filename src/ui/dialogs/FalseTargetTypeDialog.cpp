#include "FalseTargetTypeDialog.h"

#include "model/SimObject.h"

#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>

FalseTargetTypeDialog::FalseTargetTypeDialog(QWidget* parent)
    : QDialog(parent)
    , m_group(new QButtonGroup(this))
{
    setWindowTitle(QStringLiteral("단독허위표적 추가"));
    auto* l = new QVBoxLayout(this);
    l->addWidget(new QLabel(QStringLiteral("허위표적 타입을 선택하세요"), this));

    struct Opt {
        FalseTargetType type;
        QString desc;
    };
    const Opt opts[] = {
        {FalseTargetType::Fixed, QStringLiteral("지정한 위치에 고정")},
        {FalseTargetType::Moving, QStringLiteral("지도에 그린 동선을 따라 이동")},
        {FalseTargetType::Random, QStringLiteral("레이다 탐지범위 내 임의 위치에 나타났다 사라짐")},
        {FalseTargetType::Noise, QStringLiteral("위치 + 탐지각도: 해당 방위 섹터에서 레이다 탐지 불가")},
    };
    for (const Opt& o : opts) {
        auto* rb = new QRadioButton(QStringLiteral("%1  —  %2").arg(toDisplayString(o.type), o.desc), this);
        m_group->addButton(rb, int(o.type));
        l->addWidget(rb);
    }
    m_group->button(int(FalseTargetType::Fixed))->setChecked(true);

    auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    l->addWidget(box);
}

FalseTargetType FalseTargetTypeDialog::selectedType() const
{
    return FalseTargetType(m_group->checkedId());
}

std::optional<FalseTargetType> FalseTargetTypeDialog::ask(QWidget* parent)
{
    FalseTargetTypeDialog dlg(parent);
    if (dlg.exec() != QDialog::Accepted)
        return std::nullopt;
    return dlg.selectedType();
}
