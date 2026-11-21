#include "LogFilterProxyModel.h"

#include "LogTableModel.h"

// Qt 6.10+는 begin/endFilterChange, 그 이전(6.8 LTS 등)은 invalidateRowsFilter 사용
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
#define LOG_FILTER_BEGIN() beginFilterChange()
#define LOG_FILTER_END() endFilterChange(QSortFilterProxyModel::Direction::Rows)
#else
#define LOG_FILTER_BEGIN() ((void)0)
#define LOG_FILTER_END() invalidateRowsFilter()
#endif

LogFilterProxyModel::LogFilterProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
}

bool LogFilterProxyModel::isTypeVisible(LogType type) const
{
    const int i = int(type);
    return i >= 0 && i < LogTypeCount && m_visible[i];
}

void LogFilterProxyModel::setTypeVisible(LogType type, bool visible)
{
    const int i = int(type);
    if (i < 0 || i >= LogTypeCount || m_visible[i] == visible)
        return;
    LOG_FILTER_BEGIN();
    m_visible[i] = visible;
    LOG_FILTER_END();
}

void LogFilterProxyModel::setTextFilter(const QString& text)
{
    const QString t = text.trimmed();
    if (t == m_text)
        return;
    LOG_FILTER_BEGIN();
    m_text = t;
    LOG_FILTER_END();
}

bool LogFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    const QAbstractItemModel* src = sourceModel();
    const QModelIndex idx = src->index(sourceRow, 0, sourceParent);
    const int type = src->data(idx, LogTableModel::LogTypeRole).toInt();
    if (type < 0 || type >= LogTypeCount || !m_visible[type])
        return false;
    if (m_text.isEmpty())
        return true;

    for (int col : {int(LogTableModel::ColDevice), int(LogTableModel::ColOpcode), int(LogTableModel::ColContent)}) {
        const QString v = src->data(src->index(sourceRow, col, sourceParent)).toString();
        if (v.contains(m_text, Qt::CaseInsensitive))
            return true;
    }
    return false;
}
