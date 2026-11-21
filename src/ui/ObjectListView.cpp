#include "ObjectListView.h"

#include "ObjectTreeModel.h"
#include "model/ScenarioModel.h"

#include <QContextMenuEvent>
#include <QHeaderView>
#include <QMenu>

ObjectListView::ObjectListView(ScenarioModel& model, QWidget* parent)
    : QTreeView(parent)
    , m_model(model)
    , m_tree(new ObjectTreeModel(model, this))
{
    setModel(m_tree);
    setHeaderHidden(true);
    setRootIsDecorated(true);
    setUniformRowHeights(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setExpandsOnDoubleClick(true);
    expandAll();

    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &ObjectListView::onSelectionChanged);
    connect(m_tree, &ObjectTreeModel::rebuilt, this, &ObjectListView::restoreSelection);
}

void ObjectListView::onSelectionChanged()
{
    if (m_restoring)
        return;
    const QModelIndexList sel = selectionModel()->selectedIndexes();
    const ObjectId id = sel.isEmpty() ? 0 : ObjectTreeModel::objectIdOf(sel.first());
    if (id == m_current)
        return;
    m_current = id;
    emit objectSelected(id);
}

void ObjectListView::restoreSelection()
{
    // 재구성 후 펼침/선택 복원 (선택 대상이 삭제되었으면 해제 통지)
    m_restoring = true;
    expandAll();
    const QModelIndex idx = m_tree->indexOf(m_current);
    if (idx.isValid())
        selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect);
    m_restoring = false;
    if (!idx.isValid() && m_current != 0) {
        m_current = 0;
        emit objectSelected(0);
    }
}

void ObjectListView::setCurrentObject(ObjectId id)
{
    if (id == m_current)
        return;
    const QModelIndex idx = m_tree->indexOf(id);
    m_current = idx.isValid() ? id : 0;
    m_restoring = true;
    if (idx.isValid()) {
        selectionModel()->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect);
        scrollTo(idx);
    } else {
        selectionModel()->clearSelection();
    }
    m_restoring = false;
    emit objectSelected(m_current);
}

void ObjectListView::keyPressEvent(QKeyEvent* event)
{
    if ((event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) && m_current && !m_model.isLocked()) {
        emit deleteRequested(m_current);
        return;
    }
    QTreeView::keyPressEvent(event);
}

void ObjectListView::contextMenuEvent(QContextMenuEvent* event)
{
    const ObjectId id = ObjectTreeModel::objectIdOf(indexAt(event->pos()));
    if (!id)
        return;
    setCurrentObject(id);
    QMenu menu(this);
    QAction* redraw = nullptr;
    if (m_model.route(id))
        redraw = menu.addAction(QStringLiteral("동선 이어 그리기"));
    QAction* del = menu.addAction(QStringLiteral("삭제\tDel"));
    for (QAction* a : menu.actions())
        a->setEnabled(!m_model.isLocked());
    QAction* chosen = menu.exec(event->globalPos());
    if (chosen && chosen == del)
        emit deleteRequested(id);
    else if (chosen && chosen == redraw)
        emit editRouteRequested(id);
}
