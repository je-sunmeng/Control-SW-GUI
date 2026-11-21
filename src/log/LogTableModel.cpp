#include "LogTableModel.h"

#include <QBrush>
#include <QColor>

LogTableModel::LogTableModel(int maxRows, QObject* parent)
    : QAbstractTableModel(parent)
    , m_maxRows(qMax(1, maxRows))
{
}

int LogTableModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : int(m_entries.size());
}

int LogTableModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant LogTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_entries.size())
        return {};
    const LogEntry& e = m_entries.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case ColTime:    return e.time.toString(QStringLiteral("HH:mm:ss.zzz"));
        case ColType:    return toDisplayString(e.type);
        case ColDevice:  return e.device;
        case ColOpcode:  return e.isFrame() ? formatOpcode(e.opcode) : QString();
        case ColSize:    return e.isFrame() ? QVariant(e.size) : QVariant();
        case ColContent: return e.content;
        }
        break;
    case Qt::ToolTipRole:
        if (index.column() == ColTime)
            return e.time.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
        if (e.isFrame() && !e.raw.isEmpty())
            return QStringLiteral("<pre>%1</pre>").arg(hexDump(e.raw).toHtmlEscaped());
        if (index.column() == ColContent)
            return e.content;
        break;
    case Qt::ForegroundRole:
        switch (e.type) {
        case LogType::Error: return QBrush(QColor(0xFF, 0x6B, 0x6B));
        case LogType::Tx:    return QBrush(QColor(0x6C, 0xB6, 0xFF));
        case LogType::Rx:    return QBrush(QColor(0x7E, 0xE0, 0x8A));
        case LogType::Info:  break;   // 스타일시트 기본 색상
        }
        break;
    case Qt::TextAlignmentRole:
        if (index.column() == ColSize)
            return int(Qt::AlignRight | Qt::AlignVCenter);
        if (index.column() == ColType || index.column() == ColOpcode)
            return int(Qt::AlignCenter);
        break;
    case LogTypeRole:
        return int(e.type);
    case EntryTimeRole:
        return e.time;
    }
    return {};
}

QVariant LogTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractTableModel::headerData(section, orientation, role);
    switch (section) {
    case ColTime:    return QStringLiteral("시각");
    case ColType:    return QStringLiteral("유형");
    case ColDevice:  return QStringLiteral("장치");
    case ColOpcode:  return QStringLiteral("Opcode");
    case ColSize:    return QStringLiteral("크기");
    case ColContent: return QStringLiteral("내용");
    }
    return {};
}

void LogTableModel::setMaxRows(int maxRows)
{
    m_maxRows = qMax(1, maxRows);
    trimTo(m_maxRows);
}

void LogTableModel::append(const LogEntry& entry)
{
    trimTo(m_maxRows - 1);
    const int row = int(m_entries.size());
    beginInsertRows({}, row, row);
    m_entries.append(entry);
    endInsertRows();
}

void LogTableModel::clear()
{
    if (m_entries.isEmpty())
        return;
    beginResetModel();
    m_entries.clear();
    endResetModel();
}

void LogTableModel::trimTo(int limit)
{
    const int overflow = int(m_entries.size()) - qMax(0, limit);
    if (overflow <= 0)
        return;
    beginRemoveRows({}, 0, overflow - 1);
    m_entries.remove(0, overflow);
    endRemoveRows();
}
