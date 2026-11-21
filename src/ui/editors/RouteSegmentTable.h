#pragma once

#include "model/Route.h"

#include <QWidget>

class QLabel;
class QPushButton;
class QTableWidget;

// 동선 구간별 고도/속력 테이블 (전자전기/편대/이동표적 공용)
//  행 = 구간, 열 = 시작점 | 끝점 (읽기전용 위경도) | 고도[km] | 속력[m/s]
class RouteSegmentTable : public QWidget {
    Q_OBJECT
public:
    explicit RouteSegmentTable(QWidget* parent = nullptr);

    void setRoute(const Route& route);

signals:
    void segmentEdited(int index, const RouteSegment& segment);
    void applyToAllRequested(const RouteSegment& segment);
    void redrawRequested();
    void removeLastRequested();

private:
    void rebuild(int rows);

    QTableWidget* m_table;
    QLabel* m_summary;
    QPushButton* m_removeLast;
    QPushButton* m_applyAll;
    Route m_route;
};
