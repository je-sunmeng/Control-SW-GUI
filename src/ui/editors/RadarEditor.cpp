#include "RadarEditor.h"

#include "EditorUtil.h"
#include "common/ColorPalette.h"
#include "model/ScenarioModel.h"

#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>

using namespace RadarLimits;
using EditorUtil::makeSpin;

RadarEditor::RadarEditor(ScenarioModel& model, QWidget* parent)
    : ObjectEditorBase(model, parent)
    , m_color(new QLabel(this))
    , m_lat(makeSpin(-90, 90, 5, QStringLiteral("°"), 0.01, this))
    , m_lon(makeSpin(-180, 180, 5, QStringLiteral("°"), 0.01, this))
    , m_freq(makeSpin(MinFreqMHz, MaxFreqMHz, 1, QStringLiteral("MHz"), 10, this))
    , m_pri(makeSpin(MinPriMs, MaxPriMs, 3, QStringLiteral("ms"), 0.1, this))
    , m_pw(makeSpin(MinPwUs, MaxPwUs, 2, QStringLiteral("µs"), 0.1, this))
    , m_range(makeSpin(MinRangeKm, MaxRangeKm, 1, QStringLiteral("km"), 10, this))
    , m_period(makeSpin(MinScanPeriodSec, MaxScanPeriodSec, 2, QStringLiteral("s"), 1, this))
    , m_beam(makeSpin(MinBeamWidthDeg, MaxBeamWidthDeg, 2, QStringLiteral("°"), 0.5, this))
    , m_dir(new QComboBox(this))
{
    m_dir->addItem(QStringLiteral("시계방향 (CW)"), int(ScanDirection::Clockwise));
    m_dir->addItem(QStringLiteral("반시계방향 (CCW)"), int(ScanDirection::CounterClockwise));
    m_color->setFixedSize(16, 16);

    auto* nameRow = new QHBoxLayout;
    nameRow->addWidget(m_color);
    nameRow->addWidget(makeNameEdit(), 1);

    auto* form = new QFormLayout(this);
    form->setContentsMargins(4, 4, 4, 4);
    form->setLabelAlignment(Qt::AlignRight);
    form->addRow(QStringLiteral("이름"), nameRow);
    form->addRow(QStringLiteral("위도"), m_lat);
    form->addRow(QStringLiteral("경도"), m_lon);
    form->addRow(QStringLiteral("주파수"), m_freq);
    form->addRow(QStringLiteral("PRI"), m_pri);
    form->addRow(QStringLiteral("PW"), m_pw);
    form->addRow(QStringLiteral("최대탐지거리"), m_range);
    form->addRow(QStringLiteral("스캔주기"), m_period);
    form->addRow(QStringLiteral("스캔방향"), m_dir);
    form->addRow(QStringLiteral("빔폭"), m_beam);

    for (QDoubleSpinBox* s : {m_lat, m_lon, m_freq, m_pri, m_pw, m_range, m_period, m_beam})
        connect(s, &QDoubleSpinBox::valueChanged, this, &RadarEditor::commit);
    connect(m_dir, &QComboBox::currentIndexChanged, this, &RadarEditor::commit);
}

void RadarEditor::load()
{
    const Radar* r = m_model.radar(m_id);
    if (!r)
        return;
    loadName();
    m_color->setStyleSheet(QStringLiteral("background:%1; border-radius:3px;").arg(ColorPalette::radar(r->colorIndex).name()));
    EditorUtil::setSilently(m_lat, r->pos.lat);
    EditorUtil::setSilently(m_lon, r->pos.lon);
    EditorUtil::setSilently(m_freq, r->freqMHz);
    EditorUtil::setSilently(m_pri, r->priMs);
    EditorUtil::setSilently(m_pw, r->pwUs);
    EditorUtil::setSilently(m_range, r->maxRangeKm);
    EditorUtil::setSilently(m_period, r->scanPeriodSec);
    EditorUtil::setSilently(m_beam, r->beamWidthDeg);
    const QSignalBlocker b(m_dir);
    m_dir->setCurrentIndex(m_dir->findData(int(r->scanDir)));
}

void RadarEditor::commit()
{
    const Radar* cur = m_model.radar(m_id);
    if (!cur)
        return;
    Radar r = *cur;
    r.pos = {m_lat->value(), m_lon->value()};
    r.freqMHz = m_freq->value();
    r.priMs = m_pri->value();
    r.pwUs = m_pw->value();
    r.maxRangeKm = m_range->value();
    r.scanPeriodSec = m_period->value();
    r.scanDir = ScanDirection(m_dir->currentData().toInt());
    r.beamWidthDeg = m_beam->value();
    m_model.updateRadar(r);
}
