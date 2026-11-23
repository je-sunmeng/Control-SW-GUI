#include "RouteSegmentTable.h"

#include "EditorUtil.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {
enum Col { ColSeg = 0, ColAlt, ColSpeed, ColCount };   // 구간 | 고도 | 속력 (좌표는 툴팁)
}

RouteSegmentTable::RouteSegmentTable(QWidget* parent)
    : QWidget(parent)
    , m_table(new QTableWidget(0, ColCount, this))
    , m_summary(new QLabel(this))
    , m_removeLast(new QPushButton(QStringLiteral("끝점 삭제"), this))
    , m_applyAll(new QPushButton(QStringLiteral("전체 적용"), this))
{
    m_table->setHorizontalHeaderLabels({QStringLiteral("구간"), QStringLiteral("고도"), QStringLiteral("속력")});
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    m_table->verticalHeader()->setDefaultSectionSize(28);
    m_table->horizontalHeader()->setSectionResizeMode(ColSeg, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ColAlt, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(ColSpeed, QHeaderView::Stretch);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);   // 높이 = 행 수에 맞춤
    m_table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_summary->setObjectName(QStringLiteral("placeholderLabel"));
    m_summary->setWordWrap(true);

    auto* redraw = new QPushButton(QStringLiteral("다시 그리기"), this);
    redraw->setToolTip(QStringLiteral("기존 웨이포인트를 지우고 지도에서 새로 입력"));
    connect(redraw, &QPushButton::clicked, this, &RouteSegmentTable::redrawRequested);
    connect(m_removeLast, &QPushButton::clicked, this, &RouteSegmentTable::removeLastRequested);
    m_removeLast->setToolTip(QStringLiteral("마지막 웨이포인트 삭제"));
    m_applyAll->setToolTip(QStringLiteral("1구간의 고도/속력을 모든 구간에 적용"));
    connect(m_applyAll, &QPushButton::clicked, this, [this] {
        if (!m_route.segments.isEmpty())
            emit applyToAllRequested(m_route.segments.first());
    });

    auto* buttons = new QHBoxLayout;
    buttons->setContentsMargins(0, 0, 0, 0);
    buttons->addWidget(redraw);
    buttons->addWidget(m_removeLast);
    buttons->addWidget(m_applyAll);

    EditorUtil::fitTableHeight(m_table);
    auto* l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(4);
    l->addWidget(m_table);
    l->addWidget(m_summary);
    l->addLayout(buttons);
}

void RouteSegmentTable::rebuild(int rows)
{
    m_table->setRowCount(rows);
    for (int i = 0; i < rows; ++i) {
        auto* item = new QTableWidgetItem;
        item->setFlags(Qt::ItemIsEnabled);
        item->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, ColSeg, item);
        auto* alt = EditorUtil::makeSpin(RouteLimits::MinAltitudeKm, RouteLimits::MaxAltitudeKm, 2, QStringLiteral("km"), 0.5);
        auto* spd = EditorUtil::makeSpin(RouteLimits::MinSpeedMps, RouteLimits::MaxSpeedMps, 1, QStringLiteral("m/s"), 10);
        alt->setFrame(false);
        spd->setFrame(false);
        m_table->setCellWidget(i, ColAlt, alt);
        m_table->setCellWidget(i, ColSpeed, spd);
        connect(alt, &QDoubleSpinBox::valueChanged, this, [this, i, alt, spd] {
            emit segmentEdited(i, {alt->value(), spd->value()});
        });
        connect(spd, &QDoubleSpinBox::valueChanged, this, [this, i, alt, spd] {
            emit segmentEdited(i, {alt->value(), spd->value()});
        });
    }
}

void RouteSegmentTable::setRoute(const Route& route)
{
    m_route = route;
    const int rows = route.segmentCount();
    if (m_table->rowCount() != rows) {
        rebuild(rows);
        EditorUtil::fitTableHeight(m_table);
    }
    for (int i = 0; i < rows; ++i) {
        const int e = route.endIndex(i);
        const GeoPoint a = route.waypoints[i], b = route.waypoints[e];
        const bool closing = route.closed && e == 0;   // 루프의 되돌아오는 구간
        QTableWidgetItem* seg = m_table->item(i, ColSeg);
        seg->setText(closing ? QStringLiteral("WP%1 → WP1 ↺").arg(i + 1)
                             : QStringLiteral("WP%1 → WP%2").arg(i + 1).arg(e + 1));
        seg->setToolTip(QStringLiteral("%1 → %2%3\n구간 길이 %4 km")
                            .arg(EditorUtil::formatGeo(a.lat, a.lon), EditorUtil::formatGeo(b.lat, b.lon))
                            .arg(closing ? QStringLiteral(" (시작점, 루프)") : QString())
                            .arg(route.segmentLengthKm(i), 0, 'f', 1));
        EditorUtil::setSilently(static_cast<QDoubleSpinBox*>(m_table->cellWidget(i, ColAlt)), route.segments[i].altitudeKm);
        EditorUtil::setSilently(static_cast<QDoubleSpinBox*>(m_table->cellWidget(i, ColSpeed)), route.segments[i].speedMps);
    }

    if (route.waypointCount() == 0)
        m_summary->setText(QStringLiteral("동선 없음 — [다시 그리기]로 지도에서 입력하세요"));
    else if (route.waypointCount() == 1)
        m_summary->setText(QStringLiteral("웨이포인트 1개 — 2개 이상 필요"));
    else
        m_summary->setText(QStringLiteral("%1웨이포인트 %2개 · 총 %3 km · 소요 %4")
                               .arg(route.closed ? QStringLiteral("루프(반복) · 한 바퀴 ") : QString())
                               .arg(route.waypointCount())
                               .arg(route.totalLengthKm(), 0, 'f', 1)
                               .arg(EditorUtil::formatDuration(route.totalDurationSec())));
    m_removeLast->setEnabled(route.waypointCount() > 0);
    m_applyAll->setEnabled(route.segmentCount() > 1);
}
