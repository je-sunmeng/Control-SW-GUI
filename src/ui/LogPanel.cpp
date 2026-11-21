#include "LogPanel.h"

#include "log/LogFilterProxyModel.h"
#include "log/LogTableModel.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>

LogPanel::LogPanel(LogTableModel& model, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
    , m_proxy(new LogFilterProxyModel(this))
    , m_view(new QTableView(this))
    , m_autoScroll(new QCheckBox(QStringLiteral("자동 스크롤"), this))
    , m_search(new QLineEdit(this))
    , m_count(new QLabel(this))
{
    setObjectName(QStringLiteral("LogPanel"));
    m_proxy->setSourceModel(&m_model);

    // --- 상단 필터 바 ---
    auto* bar = new QHBoxLayout;
    bar->setContentsMargins(4, 2, 4, 2);
    bar->setSpacing(10);
    bar->addWidget(new QLabel(QStringLiteral("유형:"), this));
    for (LogType t : {LogType::Info, LogType::Error, LogType::Tx, LogType::Rx}) {
        auto* cb = new QCheckBox(toDisplayString(t), this);
        cb->setChecked(true);
        cb->setObjectName(QStringLiteral("logFilter_%1").arg(toFileTag(t)));
        connect(cb, &QCheckBox::toggled, this, [this, t](bool on) {
            m_proxy->setTypeVisible(t, on);
            updateCountLabel();
        });
        bar->addWidget(cb);
    }
    bar->addSpacing(12);
    m_search->setPlaceholderText(QStringLiteral("검색 (장치/Opcode/내용)"));
    m_search->setClearButtonEnabled(true);
    m_search->setMaximumWidth(280);
    connect(m_search, &QLineEdit::textChanged, this, [this](const QString& text) {
        m_proxy->setTextFilter(text);
        updateCountLabel();
    });
    bar->addWidget(m_search, 1);
    bar->addStretch();
    bar->addWidget(m_count);
    m_autoScroll->setChecked(true);
    bar->addWidget(m_autoScroll);
    auto* clearBtn = new QPushButton(QStringLiteral("지우기"), this);
    connect(clearBtn, &QPushButton::clicked, &m_model, &LogTableModel::clear);
    bar->addWidget(clearBtn);

    // --- 테이블 ---
    m_view->setModel(m_proxy);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->setAlternatingRowColors(true);
    m_view->setWordWrap(false);
    m_view->setShowGrid(false);
    m_view->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_view->verticalHeader()->setVisible(false);
    m_view->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    m_view->verticalHeader()->setDefaultSectionSize(fontMetrics().height() + 6);
    auto* hh = m_view->horizontalHeader();
    hh->setHighlightSections(false);
    hh->setStretchLastSection(true);
    hh->resizeSection(LogTableModel::ColTime, 100);
    hh->resizeSection(LogTableModel::ColType, 50);
    hh->resizeSection(LogTableModel::ColDevice, 100);
    hh->resizeSection(LogTableModel::ColOpcode, 110);
    hh->resizeSection(LogTableModel::ColSize, 60);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    layout->addLayout(bar);
    layout->addWidget(m_view, 1);

    connect(m_proxy, &QAbstractItemModel::rowsInserted, this, &LogPanel::onRowsInserted);
    connect(m_proxy, &QAbstractItemModel::rowsRemoved, this, &LogPanel::updateCountLabel);
    connect(m_proxy, &QAbstractItemModel::modelReset, this, &LogPanel::updateCountLabel);
    updateCountLabel();
    QTimer::singleShot(0, m_view, &QTableView::scrollToBottom);
}

void LogPanel::onRowsInserted()
{
    updateCountLabel();
    if (m_autoScroll->isChecked())
        m_view->scrollToBottom();
}

void LogPanel::updateCountLabel()
{
    m_count->setText(QStringLiteral("%1 / %2").arg(m_proxy->rowCount()).arg(m_model.rowCount()));
}
