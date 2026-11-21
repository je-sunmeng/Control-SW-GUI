#include "DeviceStatusWidget.h"

#include "LampIndicator.h"

#include <QGridLayout>
#include <QLabel>

DeviceStatusWidget::DeviceStatusWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* g = new QGridLayout(this);
    g->setContentsMargins(0, 0, 0, 0);
    g->setHorizontalSpacing(8);
    auto makeRow = [&](int row, DeviceCode dev) {
        Row r{new LampIndicator(QColor(0x51, 0xcf, 0x66), 14, this), new QLabel(this)};
        g->addWidget(r.lamp, row, 0);
        g->addWidget(new QLabel(QStringLiteral("%1 (0x%2)").arg(toDisplayString(dev)).arg(quint8(dev), 2, 16, QLatin1Char('0')).toUpper()
                                    .replace(QLatin1String("0X"), QLatin1String("0x")),
                                this),
                     row, 1);
        g->addWidget(r.state, row, 2, Qt::AlignRight);
        return r;
    };
    m_analysis = makeRow(0, DeviceCode::AnalysisSW);
    m_signalGen = makeRow(1, DeviceCode::SignalGen);
    m_server = new QLabel(this);
    m_server->setObjectName(QStringLiteral("placeholderLabel"));
    g->addWidget(m_server, 2, 0, 1, 3);
    g->setColumnStretch(1, 1);
    setNetworkActive(false);
    setListening(false, 0);
}

void DeviceStatusWidget::setConnected(DeviceCode dev, bool connected)
{
    Row& r = dev == DeviceCode::AnalysisSW ? m_analysis : m_signalGen;
    r.lamp->setOn(connected);
    r.state->setText(!m_active ? QStringLiteral("미사용") : connected ? QStringLiteral("연결") : QStringLiteral("미연결"));
}

void DeviceStatusWidget::setNetworkActive(bool active)
{
    m_active = active;
    for (Row* r : {&m_analysis, &m_signalGen}) {
        if (!active)
            r->lamp->setOn(false);
        r->state->setText(active ? (r->lamp->isOn() ? QStringLiteral("연결") : QStringLiteral("미연결"))
                                 : QStringLiteral("미사용"));
        r->state->setObjectName(QStringLiteral("placeholderLabel"));
    }
}

void DeviceStatusWidget::setListening(bool listening, quint16 port)
{
    m_server->setText(listening ? QStringLiteral("TCP 서버 대기 중 · 포트 %1").arg(port)
                                : QStringLiteral("TCP 서버 정지 (단독모드)"));
}
