#pragma once

#include "LogEntry.h"

#include <QAbstractTableModel>
#include <QList>

// 최대 maxRows 행을 유지하는 링버퍼 형태의 로그 테이블 모델.
// 컬럼: 시각 | 유형 | 장치 | Opcode | 크기 | 내용
class LogTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column { ColTime = 0, ColType, ColDevice, ColOpcode, ColSize, ColContent, ColumnCount };
    enum Role { LogTypeRole = Qt::UserRole + 1, EntryTimeRole };

    static constexpr int DefaultMaxRows = 10000;

    explicit LogTableModel(int maxRows = DefaultMaxRows, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    int maxRows() const { return m_maxRows; }
    void setMaxRows(int maxRows);
    const LogEntry& entryAt(int row) const { return m_entries.at(row); }

public slots:
    void append(const LogEntry& entry);
    void clear();

private:
    void trimTo(int limit);

    QList<LogEntry> m_entries;
    int m_maxRows;
};
