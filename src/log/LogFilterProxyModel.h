#pragma once

#include "LogEntry.h"

#include <QSortFilterProxyModel>

// 로그 유형별 표시 여부 + 텍스트(장치/Opcode/내용) 검색 필터
class LogFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit LogFilterProxyModel(QObject* parent = nullptr);

    bool isTypeVisible(LogType type) const;
    QString textFilter() const { return m_text; }

public slots:
    void setTypeVisible(LogType type, bool visible);
    void setTextFilter(const QString& text);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    bool m_visible[LogTypeCount] = {true, true, true, true};
    QString m_text;
};
