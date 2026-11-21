#include "JammingConfigEditor.h"

#include "EditorUtil.h"
#include "common/Constants.h"

#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>

JammingConfigEditor::JammingConfigEditor(QWidget* parent)
    : QWidget(parent)
    , m_count(new QSpinBox(this))
    , m_table(new QTableWidget(0, 2, this))
    , m_noiseBw(EditorUtil::makeSpin(JammingLimits::MinNoiseBandwidthMHz, JammingLimits::MaxNoiseBandwidthMHz, 2,
                                     QStringLiteral("MHz"), 1, this))
{
    m_count->setRange(0, Limits::MaxFormationFalseTargets);
    m_count->setKeyboardTracking(false);
    m_count->setAlignment(Qt::AlignRight);
    m_count->setSuffix(QStringLiteral(" 개"));

    m_table->setHorizontalHeaderLabels({QStringLiteral("이격각도"), QStringLiteral("초기이격거리")});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    m_table->verticalHeader()->setDefaultSectionSize(26);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setMinimumHeight(90);
    m_table->setToolTip(QStringLiteral("이격각도: 편대 이동방향 기준 시계방향 / 이격거리: 임무 중 고정"));

    auto* form = new QFormLayout;
    form->setContentsMargins(0, 0, 0, 0);
    form->addRow(QStringLiteral("허위타겟 개수"), m_count);
    form->addRow(QStringLiteral("잡음신호폭"), m_noiseBw);

    auto* l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(4);
    l->addLayout(form);
    l->addWidget(m_table, 1);

    connect(m_count, &QSpinBox::valueChanged, this, [this](int n) {
        m_config.resize(n);
        emit configEdited(m_config);
    });
    connect(m_noiseBw, &QDoubleSpinBox::valueChanged, this, [this](double v) {
        m_config.noiseBandwidthMHz = v;
        emit configEdited(m_config);
    });
}

void JammingConfigEditor::rebuild(int rows)
{
    m_table->setRowCount(rows);
    for (int i = 0; i < rows; ++i) {
        auto* ang = EditorUtil::makeSpin(0, 359.99, 2, QStringLiteral("°"), 5);
        ang->setWrapping(true);
        auto* dist = EditorUtil::makeSpin(JammingLimits::MinDistanceKm, JammingLimits::MaxDistanceKm, 2, QStringLiteral("km"), 1);
        ang->setFrame(false);
        dist->setFrame(false);
        m_table->setCellWidget(i, 0, ang);
        m_table->setCellWidget(i, 1, dist);
        m_table->setVerticalHeaderItem(i, new QTableWidgetItem(QStringLiteral("T%1").arg(i + 1)));
        auto edited = [this, i, ang, dist] {
            if (i >= m_config.targets.size())
                return;
            m_config.targets[i] = {ang->value(), dist->value()};
            emit configEdited(m_config);
        };
        connect(ang, &QDoubleSpinBox::valueChanged, this, edited);
        connect(dist, &QDoubleSpinBox::valueChanged, this, edited);
    }
}

void JammingConfigEditor::setConfig(const JammingConfig& config)
{
    m_config = config;
    const int n = int(config.targets.size());
    {
        const QSignalBlocker b(m_count);
        m_count->setValue(n);
    }
    EditorUtil::setSilently(m_noiseBw, config.noiseBandwidthMHz);
    if (m_table->rowCount() != n)
        rebuild(n);
    for (int i = 0; i < n; ++i) {
        EditorUtil::setSilently(static_cast<QDoubleSpinBox*>(m_table->cellWidget(i, 0)), config.targets[i].angleDeg);
        EditorUtil::setSilently(static_cast<QDoubleSpinBox*>(m_table->cellWidget(i, 1)), config.targets[i].distanceKm);
    }
}
