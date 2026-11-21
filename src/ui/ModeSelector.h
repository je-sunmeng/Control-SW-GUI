#pragma once

#include "common/Types.h"

#include <QWidget>

class QRadioButton;

// 단독 / 연동 모드 선택
class ModeSelector : public QWidget {
    Q_OBJECT
public:
    explicit ModeSelector(QWidget* parent = nullptr);

    void setMode(RunMode mode);   // 시그널 없이 표시만 갱신

signals:
    void modeRequested(RunMode mode);

private:
    QRadioButton* m_standalone;
    QRadioButton* m_linked;
};
