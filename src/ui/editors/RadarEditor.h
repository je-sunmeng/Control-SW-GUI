#pragma once

#include "ObjectEditorBase.h"

class QComboBox;
class QDoubleSpinBox;
class QLabel;

class RadarEditor : public ObjectEditorBase {
    Q_OBJECT
public:
    explicit RadarEditor(ScenarioModel& model, QWidget* parent = nullptr);

protected:
    void load() override;

private:
    void commit();

    QLabel* m_color;
    QDoubleSpinBox *m_lat, *m_lon, *m_freq, *m_pri, *m_pw, *m_range, *m_period, *m_beam;
    QComboBox* m_dir;
};
