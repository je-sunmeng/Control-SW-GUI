#pragma once

#include "model/JammingConfig.h"

#include <QWidget>

class QDoubleSpinBox;
class QSpinBox;
class QTableWidget;

// 교란설정: 허위타겟 개수(0~8) ↔ 테이블 행 수 동기화(이격각도, 초기이격거리), 잡음신호폭
class JammingConfigEditor : public QWidget {
    Q_OBJECT
public:
    explicit JammingConfigEditor(QWidget* parent = nullptr);

    void setConfig(const JammingConfig& config);
    JammingConfig config() const { return m_config; }

signals:
    void configEdited(const JammingConfig& config);

private:
    void rebuild(int rows);

    QSpinBox* m_count;
    QTableWidget* m_table;
    QDoubleSpinBox* m_noiseBw;
    JammingConfig m_config;
};
