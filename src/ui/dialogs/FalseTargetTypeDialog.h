#pragma once

#include "common/Types.h"

#include <QDialog>

#include <optional>

class QButtonGroup;

// 단독 허위표적 추가 시 타입 선택
class FalseTargetTypeDialog : public QDialog {
    Q_OBJECT
public:
    explicit FalseTargetTypeDialog(QWidget* parent = nullptr);

    FalseTargetType selectedType() const;

    // 모달로 묻고 선택 결과 반환 (취소 시 nullopt)
    static std::optional<FalseTargetType> ask(QWidget* parent);

private:
    QButtonGroup* m_group;
};
