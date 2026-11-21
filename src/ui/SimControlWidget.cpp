#include "SimControlWidget.h"

#include "mission/MissionController.h"

#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

#include <cmath>

SimControlWidget::SimControlWidget(QWidget* parent)
    : QWidget(parent)
    , m_state(new QLabel(this))
    , m_time(new QLabel(this))
    , m_send(new QPushButton(QStringLiteral("임무전송"), this))
    , m_start(new QPushButton(QStringLiteral("시작"), this))
    , m_pause(new QPushButton(QStringLiteral("일시정지"), this))
    , m_stop(new QPushButton(QStringLiteral("중지"), this))
    , m_refresh(new QPushButton(QStringLiteral("새로고침"), this))
{
    setObjectName(QStringLiteral("SimControlWidget"));
    m_state->setObjectName(QStringLiteral("simStateLabel"));
    m_time->setObjectName(QStringLiteral("simTimeLabel"));
    m_state->setAlignment(Qt::AlignCenter);
    m_time->setAlignment(Qt::AlignCenter);
    m_start->setObjectName(QStringLiteral("startButton"));
    m_stop->setObjectName(QStringLiteral("stopButton"));
    m_send->setToolTip(QStringLiteral("연동모드: 분석SW·신호발생장치로 임무 정보 송신"));
    m_pause->setToolTip(QStringLiteral("단독모드 전용"));
    m_refresh->setToolTip(QStringLiteral("임무 전 상태로 초기화 (오브젝트 설정 유지)"));

    auto* g = new QGridLayout(this);
    g->setContentsMargins(0, 0, 0, 0);
    g->setSpacing(6);
    g->addWidget(m_state, 0, 0, 1, 2);
    g->addWidget(m_time, 1, 0, 1, 2);
    g->addWidget(m_send, 2, 0, 1, 2);
    g->addWidget(m_start, 3, 0);
    g->addWidget(m_pause, 3, 1);
    g->addWidget(m_stop, 4, 0);
    g->addWidget(m_refresh, 4, 1);
    for (QPushButton* b : {m_send, m_start, m_pause, m_stop, m_refresh})
        b->setMinimumHeight(30);

    connect(m_send, &QPushButton::clicked, this, &SimControlWidget::sendMissionClicked);
    connect(m_start, &QPushButton::clicked, this, &SimControlWidget::startClicked);
    connect(m_pause, &QPushButton::clicked, this, &SimControlWidget::pauseClicked);
    connect(m_stop, &QPushButton::clicked, this, &SimControlWidget::stopClicked);
    connect(m_refresh, &QPushButton::clicked, this, &SimControlWidget::refreshClicked);

    setState(SimState::PreMission);
    setSimTime(0);
}

void SimControlWidget::applyActions(const MissionActions& a)
{
    m_send->setEnabled(a.sendMission);
    m_start->setEnabled(a.start);
    m_start->setText(a.startIsResume ? QStringLiteral("재개") : QStringLiteral("시작"));
    m_pause->setEnabled(a.pause);
    m_pause->setVisible(a.pauseVisible);
    m_stop->setEnabled(a.stop);
    m_refresh->setEnabled(a.refresh);
}

void SimControlWidget::setState(SimState s)
{
    static const char* colors[] = {"#8a98ab", "#51cf66", "#ffd43b", "#ff6b6b"};
    m_state->setText(QStringLiteral("상태: %1").arg(toDisplayString(s)));
    m_state->setStyleSheet(QStringLiteral("font-size: 15pt; font-weight: bold; color: %1;").arg(QLatin1String(colors[int(s)])));
}

void SimControlWidget::setSimTime(double sec)
{
    const int total = int(std::floor(std::max(0.0, sec)));
    m_time->setText(QStringLiteral("T+%1:%2:%3")
                        .arg(total / 3600, 2, 10, QLatin1Char('0'))
                        .arg((total % 3600) / 60, 2, 10, QLatin1Char('0'))
                        .arg(total % 60, 2, 10, QLatin1Char('0')));
}
