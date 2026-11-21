#pragma once

#include <QWidget>

class QCheckBox;
class QLabel;
class QLineEdit;
class QTableView;
class LogTableModel;
class LogFilterProxyModel;

// 하단 로그 패널: 유형 필터, 검색, 자동 스크롤, 지우기 + 로그 테이블
class LogPanel : public QWidget {
    Q_OBJECT
public:
    explicit LogPanel(LogTableModel& model, QWidget* parent = nullptr);

private:
    void onRowsInserted();
    void updateCountLabel();

    LogTableModel& m_model;
    LogFilterProxyModel* m_proxy;
    QTableView* m_view;
    QCheckBox* m_autoScroll;
    QLineEdit* m_search;
    QLabel* m_count;
};
