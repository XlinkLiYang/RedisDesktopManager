#include "treeview.h"
#include <QMouseEvent>
#include <QHeaderView>
#include <QMenu>
#include <QDebug>
#include <easylogging++.h>

#include "items/treeitem.h"
#include "model.h"

using namespace ConnectionsTree;

TreeView::TreeView(QWidget * parent)
    : QTreeView(parent)
{
    header()->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    header()->setStretchLastSection(false);
    setUniformRowHeights(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setColumnWidth(0, 400);

    connect(this, &TreeView::clicked, this, &TreeView::processClick);
    connect(this, &TreeView::wheelClicked, this, &TreeView::processWheelClick);
    connect(this, &TreeView::customContextMenuRequested, this, &TreeView::processContextMenu);
}

void TreeView::mousePressEvent(QMouseEvent * event)
{
    if (event->button() == Qt::MiddleButton) {            
        emit wheelClicked(indexAt(event->pos()));
    }

    return QTreeView::mousePressEvent(event);
}

QKeySequence keySequenceFromKeyEvent(QKeyEvent *keyEvent)
{
    int keyInt = keyEvent->key();
    Qt::Key key = static_cast<Qt::Key>(keyInt);
    if(key == Qt::Key_unknown) return QKeySequence();

    // the user have clicked just and only the special keys Ctrl, Shift, Alt, Meta.
    if(key == Qt::Key_Control || key == Qt::Key_Shift
            || key == Qt::Key_Alt || key == Qt::Key_Meta)
    {
        return QKeySequence();
    }

    // check for a combination of user clicks
    Qt::KeyboardModifiers modifiers = keyEvent->modifiers();
    QString keyText = keyEvent->text();

    QList<Qt::Key> modifiersList;
    if(modifiers & Qt::ShiftModifier)
        keyInt += Qt::SHIFT;
    if(modifiers & Qt::ControlModifier)
        keyInt += Qt::CTRL;
    if(modifiers & Qt::AltModifier)
        keyInt += Qt::ALT;
    if(modifiers & Qt::MetaModifier)
        keyInt += Qt::META;

    return QKeySequence(keyInt);
}


void TreeView::keyPressEvent(QKeyEvent *event)
{
    if (selectedIndexes().size() != 1)
        return QTreeView::keyPressEvent(event);

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        emit clicked(selectedIndexes()[0]);
        return;
    }

    // generic shortcut processing
    TreeItem* item = preProcessEvent(selectedIndexes()[0]);
    QKeySequence key = keySequenceFromKeyEvent(event);

    if (item == nullptr)
        return;

    auto menu = item->getContextMenu(*static_cast<TreeItem::ParentView* const>(this));

    if (menu.isNull())
        return;

    QList<QAction*> actions = menu->actions();

    foreach (QAction* action, actions) {
        if (!action->shortcut().isEmpty()
                && action->shortcut().matches(key)) {
            action->trigger();
            return;
        }
    }
    return QTreeView::keyPressEvent(event);
}

void TreeView::processContextMenu(const QPoint& point)
{    
    if (point.isNull() || QCursor::pos().isNull())
        return;

    TreeItem* item = preProcessEvent(indexAt(point));

    if (item == nullptr)
        return;

    auto menu = item->getContextMenu(*static_cast<TreeItem::ParentView* const>(this));

    if (menu.isNull())
        return;

    menu->exec(mapToGlobal(point));
}

void TreeView::processClick(const QModelIndex& index)
{    
    TreeItem* item = preProcessEvent(index);

    if (item == nullptr)
        return;

    LOG(DEBUG) << "Click on tree item: " << index.row();

    if (item->onClick(*static_cast<TreeItem::ParentView*>(this))) {
        expand(index);
    }
}

void TreeView::processWheelClick(const QModelIndex& index)
{    
    TreeItem* item = preProcessEvent(index);

    if (item == nullptr)
        return;

    item->onWheelClick(*static_cast<TreeItem::ParentView*>(this));
}


void TreeView::setModel(Model *model)
{
    QTreeView::setModel(static_cast<QAbstractItemModel*>(model));
}

const Model *TreeView::model() const
{
    return qobject_cast<Model*>(QTreeView::model());
}

QWidget *TreeView::getParentWidget()
{
    return dynamic_cast<QWidget*>(this);
}

TreeItem* TreeView::preProcessEvent(const QModelIndex &index)
{
    if (!index.isValid())
        return nullptr;

    TreeItem* item = model()->getItemFromIndex(index);

    if (item == nullptr || item->isLocked())
        return nullptr;

    return item;
}
