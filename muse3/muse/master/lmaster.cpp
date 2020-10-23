//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: lmaster.cpp,v 1.2.2.8 2009/03/09 02:05:18 terminator356 Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#include "lmaster.h"
#include "song.h"
#include "globals.h"
#include "shortcuts.h"
#include "debug.h"
#include "gconfig.h"
#include "undo.h"

#include <QGridLayout>
#include <QHeaderView>
#include <QMenuBar>
#include <QMessageBox>
#include <QStyle>
#include <QToolButton>
#include <QApplication>

// Forwards from header:
#include <QTreeWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QToolBar>
#include <QMenu>
#include <QTimer>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QAction>
#include "tempo.h"
#include "sig.h"
#include "posedit.h"
#include "sigedit.h"
#include "xml.h"

#define LMASTER_BEAT_COL 0
#define LMASTER_TIME_COL 1
#define LMASTER_TYPE_COL 2
#define LMASTER_VAL_COL  3

#define LMASTER_MSGBOX_STRING "Mastertrack List Editor"

namespace MusEGui {

LMasterLViewItem::LMasterLViewItem(QTreeWidget* parent)
    : QTreeWidgetItem(QTreeWidgetItem::UserType)
{
    parent->insertTopLevelItem(0, this);
}

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void LMaster::closeEvent(QCloseEvent* e)
{
    _isDeleting = true;  // Set flag so certain signals like songChanged, which may cause crash during delete, can be ignored.
    e->accept();
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void LMaster::songChanged(MusECore::SongChangedStruct_t type)
{
    if(_isDeleting)  // Ignore while while deleting to prevent crash.
        return;

    if (type & (SC_SIG | SC_TEMPO | SC_KEY ))
        updateList();
}

//---------------------------------------------------------
//   LMaster
//---------------------------------------------------------

LMaster::LMaster(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("MasterTrackList");
    _isDeleting = false;

    pos_editor = nullptr;
    tempo_editor = nullptr;
    sig_editor = nullptr;
    key_editor = nullptr;
    editedItem = nullptr;
    editingNewItem = false;

    setMinimumHeight(100);
    //setFixedWidth(400);            // FIXME: Arbitrary. But without this, sig editor is too wide. Must fix sig editor width...
    setFocusPolicy(Qt::NoFocus);


    comboboxTimer=new QTimer(this);
    comboboxTimer->setInterval(150);
    comboboxTimer->setSingleShot(true);
    connect(comboboxTimer, SIGNAL(timeout()), this, SLOT(comboboxTimerSlot()));


    tempoAction = new QAction(tr("Tempo"), this);
    signAction  = new QAction(tr("Signature"), this);
    keyAction   = new QAction(tr("Key"), this);
    posAction   = new QAction(tr("Position"), this);
    valAction   = new QAction(tr("Value"), this);
    delAction   = new QAction(tr("Delete"), this);

    tempoAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    signAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    keyAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    posAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    valAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    delAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);

    addAction(tempoAction);
    addAction(signAction);
    addAction(keyAction);
    addAction(posAction);
    addAction(valAction);
    addAction(delAction);

    connect(tempoAction, &QAction::triggered, [this]() { cmd(CMD_INSERT_TEMPO); } );
    connect(signAction,  &QAction::triggered, [this]() { cmd(CMD_INSERT_SIG); } );
    connect(keyAction,   &QAction::triggered, [this]() { cmd(CMD_INSERT_KEY); } );
    connect(posAction,   &QAction::triggered, [this]() { cmd(CMD_EDIT_BEAT); } );
    connect(valAction,   &QAction::triggered, [this]() { cmd(CMD_EDIT_VALUE); } );
    connect(delAction,   &QAction::triggered, [this]() { cmd(CMD_DELETE); } );


    // Toolbars ---------------------------------------------------------

    // NOTICE: Please ensure that any tool bar object names here match the names assigned
    //          to identical or similar toolbars in class MusE or other TopWin classes.
    //         This allows MusE::setCurrentMenuSharingTopwin() to do some magic
    //          to retain the original toolbar layout. If it finds an existing
    //          toolbar with the same object name, it /replaces/ it using insertToolBar(),
    //          instead of /appending/ with addToolBar().

    QToolBar* edit = new QToolBar(tr("Edit tools"), this);
    edit->addAction(tempoAction);
    edit->addAction(signAction);
    edit->addAction(keyAction);
    edit->addAction(posAction);
    edit->addAction(valAction);
    edit->addAction(delAction);

    //---------------------------------------------------
    //    master
    //---------------------------------------------------

    view = new QTreeWidget;
    view->setAllColumnsShowFocus(true);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    QStringList columnnames;
    columnnames << tr("Position")
                << tr("Time")
                << tr("Type")
                << tr("Value");
    view->setHeaderLabels(columnnames);
    view->setColumnWidth(2,70);
    view->setIndentation(2);

    //---------------------------------------------------
    //    Rest
    //---------------------------------------------------

    QGridLayout* mainGrid = new QGridLayout(this);
    mainGrid->setRowStretch(0, 100);
    mainGrid->setColumnStretch(0, 100);

    mainGrid->addWidget(edit,  0, 0);
    mainGrid->addWidget(view,  1, 0);
    updateList();

    tempo_editor = new QLineEdit(view->viewport());
    tempo_editor->setFrame(false);
    tempo_editor->hide();
    connect(tempo_editor, SIGNAL(editingFinished()), SLOT(editingFinished()));

    sig_editor = new SigEdit(view->viewport());
    sig_editor->setFrame(false);
    sig_editor->hide();
    connect(sig_editor, SIGNAL(editingFinished()), SLOT(editingFinished()));

    pos_editor = new PosEdit(view->viewport());
    pos_editor->setFrame(false);
    pos_editor->hide();
    connect(pos_editor, SIGNAL(editingFinished()), SLOT(editingFinished()));

    key_editor = new QComboBox(view->viewport());
    key_editor->setFrame(false);
    key_editor->addItems(MusECore::KeyEvent::keyStrs);
    key_editor->hide();
    connect(key_editor, SIGNAL(activated(int)), SLOT(editingFinished()));

//    connect(view, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), SLOT(select(QTreeWidgetItem*, QTreeWidgetItem*)));
    connect(view, SIGNAL(itemPressed(QTreeWidgetItem*, int)), SLOT(itemPressed(QTreeWidgetItem*, int)));
    connect(view, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), SLOT(itemDoubleClicked(QTreeWidgetItem*)));
    connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), SLOT(songChanged(MusECore::SongChangedStruct_t)));
    connect(this, SIGNAL(seekTo(int)), MusEGlobal::song, SLOT(seekTo(int)));

    initShortcuts();

    tempoAction->setToolTip(tr("Insert tempo change") + " (" + tempoAction->shortcut().toString() + ")");
    signAction->setToolTip(tr("Insert time signature change")  + " (" + signAction->shortcut().toString() + ")");
    keyAction->setToolTip(tr("Insert key change") + " (" + keyAction->shortcut().toString() + ")");
    posAction->setToolTip(tr("Edit position") + " (" + posAction->shortcut().toString() + ")");
    valAction->setToolTip(tr("Edit value") + " (" + valAction->shortcut().toString() + ")");
    delAction->setToolTip(tr("Delete event") + " (" + delAction->shortcut().toString() + ")");

    qApp->installEventFilter(this);
}

//---------------------------------------------------------
//   ~LMaster
//---------------------------------------------------------

LMaster::~LMaster()
{
}

//---------------------------------------------------------
//   focusCanvas
//---------------------------------------------------------

void LMaster::focusCanvas()
{
    if(MusEGlobal::config.smartFocus)
    {
        view->setFocus();
        view->activateWindow();
    }
}

//---------------------------------------------------------
//   insertSig
//---------------------------------------------------------

void LMaster::insertSig(const MusECore::SigEvent* ev)
{
    new LMasterSigEventItem(view, ev);
}

//---------------------------------------------------------
//   insertTempo
//---------------------------------------------------------

void LMaster::insertTempo(const MusECore::TEvent* ev)
{
    new LMasterTempoItem(view, ev);
}

void LMaster::insertKey(const MusECore::KeyEvent& ev)
{
    new LMasterKeyEventItem(view, ev);
}
//---------------------------------------------------------
//   updateList
//---------------------------------------------------------

void LMaster::updateList()
{
    LMasterLViewItem* selected = (LMasterLViewItem*) view->currentItem();
    LMASTER_LVTYPE type = LMASTER_TEMPO;
    unsigned tick = 0;

    if (selected) {
        type = selected->getType();
        tick = selected->tick();
    }

    view->clear();
    const MusECore::TempoList* t = &MusEGlobal::tempomap;
    const MusECore::SigList* s   = &MusEGlobal::sigmap;
    const MusECore::KeyList* k   = &MusEGlobal::keymap;

    MusECore::criTEvent it   = t->rbegin();
    MusECore::criSigEvent is = s->rbegin();
    MusECore::criKeyEvent ik = k->rbegin();

    // three lists that should be added to the view.
    // question if it would not be easier to merge the lists and use a sorting algorithm?
    // how often is this function called? A: only on songChanged (SC_TEMPO && SC_SIG)

    for (;;) {

        // crazy long, must be possible to solve more elegantly...

        if (ik != k->rend() && is == s->rend() && it == t->rend()) {// ik biggest
            insertKey(ik->second);
            ++ik;
        }
        else if (is != s->rend() && ik == k->rend() && it == t->rend()) {// is biggest
            insertSig(is->second);
            ++is;
        }
        else if (it != t->rend() && ik == k->rend() && is == s->rend()) {// it biggest
            insertTempo(it->second);
            ++it;
        }

        else if ( ((ik != k->rend()) && (is == s->rend()) && (ik->second.tick >= it->second->tick))
                  || ((it == t->rend()) && (ik->second.tick >= is->second->tick ) )) {// ik biggest
            insertKey(ik->second);
            ++ik;
        }
        else if ( ((is != s->rend()) && (ik == k->rend()) && (is->second->tick >= it->second->tick))
                  || ((it == t->rend()) && (is->second->tick >= ik->second.tick ))) {// is biggest
            insertSig(is->second);
            ++is;
        }

        else if (((it != t->rend()) && (ik == k->rend()) && (it->second->tick >= is->second->tick))
                 || ((is == s->rend()) && (it->second->tick >= ik->second.tick ))) {// it biggest
            insertTempo(it->second);
            ++it;
        }

        else if (ik != k->rend() && ik->second.tick >= is->second->tick && ik->second.tick >= it->second->tick) {// ik biggest
            insertKey(ik->second);
            ++ik;
        }
        else if (is != s->rend() &&  is->second->tick >= it->second->tick && is->second->tick >= ik->second.tick) { // is biggest
            insertSig(is->second);
            ++is;
        }
        else if (it != t->rend() && it->second->tick >= is->second->tick && it->second->tick >= ik->second.tick) { // it biggest
            insertTempo(it->second);
            ++it;
        }
        if (ik == k->rend() && is == s->rend() && it == t->rend() )
            break;
    }

    // Try to reselect the previous selection:
    if(selected)
    {
        LMasterLViewItem* tmp = getItemAtPos(tick, type);
        if (tmp) {
            view->clearSelection();
            view->setCurrentItem(tmp);
        }
    }
}

//---------------------------------------------------------
//   cmd
//---------------------------------------------------------

void LMaster::cmd(int cmd)
{
    editedItem = nullptr;
    tempo_editor->hide();
    pos_editor->hide();
    key_editor->hide();
    sig_editor->hide();

    switch(cmd) {
    case CMD_DELETE: {
        LMasterLViewItem* l = dynamic_cast<LMasterLViewItem*>(view->currentItem());
        if (!l)
            return;

        // Delete item:
        if (l->tick() != 0) {

            if (l == view->topLevelItem(view->topLevelItemCount() - 1))
                view->setCurrentItem(view->itemAbove(l));
            else
                view->setCurrentItem(view->itemBelow(l));

            switch (l->getType()) {
            case LMASTER_TEMPO:
            {
                LMasterTempoItem* t = (LMasterTempoItem*) l;
                MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::DeleteTempo,
                                                                  t->tick(), t->tempo()));
                break;
            }
            case LMASTER_SIGEVENT:
            {
                LMasterSigEventItem* s = (LMasterSigEventItem*) l;
                MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::DeleteSig,
                                                                  s->tick(), s->z(), s->n()));
                break;
            }
            case LMASTER_KEYEVENT:
            {
                LMasterKeyEventItem* k = (LMasterKeyEventItem*) l;
                MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::DeleteKey,
                                                                  k->tick(), k->key(), (int)k->isMinor()));
                break;
            }
            default:
                M_ERROR("Default switch statement reached");
                break;
            }
        }
        break;
    }
    case CMD_INSERT_TEMPO:
        tempoButtonClicked();
        break;
    case CMD_INSERT_SIG:
        timeSigButtonClicked();
        break;
    case CMD_INSERT_KEY:
        insertKey();
        break;
    case CMD_EDIT_BEAT:
    case CMD_EDIT_VALUE:
        cmd == CMD_EDIT_VALUE ? editorColumn = LMASTER_VAL_COL : editorColumn = LMASTER_BEAT_COL;
        if (view->currentItem() && !editedItem) {
            itemDoubleClicked(view->currentItem());
        }
        break;
    }
}

/*!
    \fn LMaster::itemPressed(QListViewItem* i, const QPoint& p, int column)
 */
void LMaster::itemPressed(QTreeWidgetItem* i, int column)
{
    if (editedItem) {
        if (editorColumn != column || editedItem != i)
            editingFinished();
    }
    else {
        if (key_editor)
            key_editor->hide();
        setFocus();
        editorColumn = column;
    }
}

//---------------------------------------------------------
//   itemDoubleClicked(QListViewItem* item)
//!  Sets lmaster in edit mode, and opens editor for selected value
//---------------------------------------------------------
void LMaster::itemDoubleClicked(QTreeWidgetItem* i)
{
    emit seekTo(((LMasterLViewItem*) i)->tick());

    QFontMetrics fm(font());
    // Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
    int fnt_w = fm.horizontalAdvance('0');
#else
    int fnt_w = fm.width('0');
#endif
    if (!editedItem && editorColumn == LMASTER_VAL_COL) {
        editedItem = (LMasterLViewItem*) i;
        QRect itemRect = view->visualItemRect(editedItem);
        int x1 = view->columnWidth(LMASTER_BEAT_COL) + view->columnWidth(LMASTER_TIME_COL)
                + view->columnWidth(LMASTER_TYPE_COL);
        itemRect.setX(x1);
        //Qt makes crazy things with itemRect if this is called directly..
        if (editingNewItem) {
            int fw = style()->pixelMetric(QStyle::PM_DefaultFrameWidth,0 , this); // ddskrjo 0
            int h  = fm.height() + fw * 2;
            itemRect.setWidth(view->columnWidth(LMASTER_VAL_COL));
            itemRect.setY(1);
            itemRect.setHeight(h);
        }


        // Edit tempo value:
        if (editedItem->getType() == LMASTER_TEMPO) {
            tempo_editor->setText(editedItem->text(LMASTER_VAL_COL));
            tempo_editor->setGeometry(itemRect);
            tempo_editor->show();
            tempo_editor->setFocus();
            tempo_editor->selectAll();
        }
        else if (editedItem->getType() == LMASTER_SIGEVENT) { // Edit signatur value:
            sig_editor->setValue(((LMasterSigEventItem*)editedItem)->getEvent()->sig);
            int w = fnt_w * 14;
            if(w > itemRect.width())
                w = itemRect.width();
            sig_editor->setGeometry(itemRect.x(), itemRect.y(), w, itemRect.height());
            sig_editor->show();
            sig_editor->setFocus();
        }
        else if (editedItem->getType() == LMASTER_KEYEVENT) {
            key_editor->setGeometry(itemRect);
            LMasterKeyEventItem* kei = static_cast<LMasterKeyEventItem*>(editedItem);
            key_editor->setCurrentIndex(MusECore::KeyEvent::keyToIndex(kei->key(), kei->isMinor()));
            key_editor->show();
            key_editor->setFocus();
            comboboxTimer->start();
        }
        else {
            printf("illegal Master list type\n");
        }
    }
    // Edit tempo or signal position:
    else if (!editedItem && editorColumn == LMASTER_BEAT_COL) {
        editedItem = (LMasterLViewItem*) i;
        // Don't allow movement of initial values:
        if  (editedItem->tick() == 0) {
            QMessageBox::information(this, tr(LMASTER_MSGBOX_STRING),
                                     tr("Reposition of the initial tempo and signature events is not allowed") );
            editedItem = nullptr;
        }
        // Everything OK
        else {
            pos_editor->setValue(editedItem->tick());
            QRect itemRect = view->visualItemRect(editedItem);
            itemRect.setX(view->indentation());
            int w = view->columnWidth(LMASTER_BEAT_COL) - view->indentation();
            if(w < (fnt_w * 13))
                w = fnt_w * 13;
            itemRect.setWidth(w);
            pos_editor->setGeometry(itemRect);
            pos_editor->show();
            pos_editor->setFocus();
        }
    }
}

//---------------------------------------------------------
//   returnPressed()
//!  called when editor is closed
//---------------------------------------------------------

void LMaster::editingFinished()
{
    if (!editedItem)
        return;

//    setFocus();

    // Tempo event:
    if (editedItem->getType() == LMASTER_TEMPO && editorColumn == LMASTER_VAL_COL) {
        if (tempo_editor->isHidden())
            return;

        tempo_editor->hide();
        repaint();

        LMasterTempoItem* e = dynamic_cast<LMasterTempoItem*>(editedItem);
        if (!e)
            return;

        const MusECore::TEvent* t = e->getEvent();
        QString input = tempo_editor->text();
        unsigned tick = t->tick;
        bool conversionOK;
        double dbl_input = input.toDouble(&conversionOK);

        if (conversionOK && dbl_input < 250.0) {
            int tempo = (int) ((1000000.0 * 60.0)/dbl_input);

            if (!editingNewItem) {
                MusEGlobal::song->startUndo();
                // Operation is undoable but do not start/end undo.
                MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::DeleteTempo,
                                                                  tick, e->tempo()), MusECore::Song::OperationUndoable);
                // Operation is undoable but do not start/end undo.
                MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::AddTempo,
                                                                  tick, tempo), MusECore::Song::OperationUndoable);
                MusEGlobal::song->endUndo(SC_TEMPO);
            }
            //
            // New item edited:
            //
            else {
                MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::AddTempo, tick, tempo));
            }
        }
        else {
            QMessageBox::warning(this, tr("MusE: List Editor"),
                                 tr("Input error, conversion not OK or value out of range"),
                                 QMessageBox::Ok, Qt::NoButton
                                 );
        }
    }
    //
    // Beat column, change position of a particular tempo or signature event
    //
    else if (editorColumn == LMASTER_BEAT_COL) {
        if (pos_editor->isHidden())
            return;

        pos_editor->hide();
        repaint();

        int oldtick = editedItem->tick();
        int newtick = pos_editor->pos().tick();
        if (newtick == 0) { // Do not allow change of position to beginning of song
            QMessageBox::warning(this, tr(LMASTER_MSGBOX_STRING),
                                 tr("Reposition of tempo and signature events to start position is not allowed!"),
                                 QMessageBox::Ok, Qt::NoButton
                                 );
        }
        else if (oldtick != newtick) {  // Ignore if tick hasn't changed
            if (editedItem->getType() == LMASTER_TEMPO) {
                LMasterTempoItem* t = dynamic_cast<LMasterTempoItem*>(editedItem);
                if (!t)
                    return;
                int tempo = t->tempo();
                MusEGlobal::song->startUndo();
                // Operation is undoable but do not start/end undo.
                MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::DeleteTempo,
                                                                  oldtick, tempo), MusECore::Song::OperationUndoable);
                // Operation is undoable but do not start/end undo.
                MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::AddTempo,
                                                                  newtick, tempo), MusECore::Song::OperationUndoable);
                MusEGlobal::song->endUndo(SC_TEMPO);
                // Select the item:
                QTreeWidgetItem* newSelected = (QTreeWidgetItem*) getItemAtPos(newtick, LMASTER_TEMPO);
                if (newSelected) {
                    view->clearSelection();
                    view->setCurrentItem(newSelected);
                }
            }
            else if (editedItem->getType() == LMASTER_SIGEVENT) {
                LMasterSigEventItem* t = dynamic_cast<LMasterSigEventItem*>(editedItem);
                if (!t)
                    return;
                int z = t->z();
                int n = t->n();
                newtick = MusEGlobal::sigmap.raster1(newtick, 0);
                if (!editingNewItem) {
                    MusEGlobal::song->startUndo();
                    //Delete first, in order to get sane tick-value
                    // Operation is undoable but do not start/end undo.
                    MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::DeleteSig,
                                                                      oldtick, z, n), MusECore::Song::OperationUndoable);
                    // Add will replace if found.
                    // Operation is undoable but do not start/end undo.
                    MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::AddSig,
                                                                      newtick, z, n), MusECore::Song::OperationUndoable);
                    MusEGlobal::song->endUndo(SC_SIG);
                }
                else //{
                    // Add will replace if found.
                    // Operation is undoable but do not start/end undo.
                    MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::AddSig,
                                                                      newtick, z, n), MusECore::Song::OperationUndoable);

                // Select the item:
                QTreeWidgetItem* newSelected = dynamic_cast<QTreeWidgetItem*>(getItemAtPos(newtick, LMASTER_SIGEVENT));
                if (newSelected) {
                    view->clearSelection();
                    view->setCurrentItem(newSelected);
                }
//                }
            }
            else if (editedItem->getType() == LMASTER_KEYEVENT) {
                LMasterKeyEventItem* k = dynamic_cast<LMasterKeyEventItem*>(editedItem);
                if (!k)
                    return;
                MusEGlobal::song->startUndo();
                // Operation is undoable but do not start/end undo.
                MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::DeleteKey,
                                                                  oldtick, k->key(), (int)k->isMinor()),  MusECore::Song::OperationUndoable);
                // Operation is undoable but do not start/end undo.
                MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::AddKey,
                                                                  newtick, k->key(), (int)k->isMinor()), MusECore::Song::OperationUndoable);
                MusEGlobal::song->endUndo(SC_KEY);

                // Select the item:
                QTreeWidgetItem* newSelected = dynamic_cast<QTreeWidgetItem*>(getItemAtPos(newtick, LMASTER_KEYEVENT));
                if (newSelected) {
                    view->clearSelection();
                    view->setCurrentItem(newSelected);
                }
            }
            else {
                printf("unknown master list event type!\n");
            }

        }
    }
    //
    // SigEvent, value changed:
    //
    else if (editedItem->getType() == LMASTER_SIGEVENT && editorColumn == LMASTER_VAL_COL)
    {
        if (sig_editor->isHidden())
            return;

        sig_editor->hide();
        repaint();

        MusECore::TimeSignature newSig = sig_editor->sig();

        // Added p3.3.43 Prevents aborting with 0 z or n.
        if(newSig.isValid())
        {
            LMasterSigEventItem* e = dynamic_cast<LMasterSigEventItem*>(editedItem);
            if (!e)
                return;
            int tick = e->tick();
            if (!editingNewItem) {
                MusEGlobal::song->startUndo();
                // Operation is undoable but do not start/end undo.
                MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::DeleteSig,
                                                                  tick, e->z(), e->n()), MusECore::Song::OperationUndoable);
                // Add will replace if found.
                // Operation is undoable but do not start/end undo.
                MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::AddSig,
                                                                  tick, newSig.z, newSig.n), MusECore::Song::OperationUndoable);
                MusEGlobal::song->endUndo(SC_SIG);
            }
            else
                // Add will replace if found.
                MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::AddSig,
                                                                  tick, newSig.z, newSig.n));
        }
        else {
            printf("Signature is not valid!\n");
        }
    }

    else if (editedItem->getType() == LMASTER_KEYEVENT && editorColumn == LMASTER_VAL_COL) {
        if (key_editor->isHidden())
            return;

        key_editor->hide();
        repaint();

        LMasterKeyEventItem* e = dynamic_cast<LMasterKeyEventItem*>(editedItem);
        if (!e)
            return;

        const MusECore::KeyEvent& t = e->getEvent();

        QString input = key_editor->currentText();
        unsigned tick = t.tick;
        const MusECore::KeyEvent key = MusECore::KeyEvent::stringToKey(input);

        if (!editingNewItem) {
            MusEGlobal::song->startUndo();
            // Operation is undoable but do not start/end undo.
            MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::DeleteKey,
                                                              tick, e->key(), (int)e->isMinor()),  MusECore::Song::OperationUndoable);
            // Operation is undoable but do not start/end undo.
            MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::AddKey,
                                                              tick, key.key, (int)key.minor), MusECore::Song::OperationUndoable);
            MusEGlobal::song->endUndo(SC_KEY);
        }
        //
        // New item edited:
        //
        else {
            MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::AddKey, tick, key.key, (int)key.minor));
        }
    }

    updateList();
    view->setFocus();
    // No item edited now:
    editedItem = nullptr;
    editorColumn = -1;
    editingNewItem = false;
}


/*!
    \fn LMasterLViewItem::text(int column)
    \brief Returns the initialized text to the View
 */
QString LMasterLViewItem::text(int column) const
{
    QString ret = "?";
    switch (column) {
    case LMASTER_BEAT_COL:
        ret = c1;
        break;
    case LMASTER_TIME_COL:
        ret = c2;
        break;
    case LMASTER_TYPE_COL:
        ret = c3;
        break;
    case LMASTER_VAL_COL:
        ret = c4;
        break;
    default:
        fprintf(stderr,"LMasterLViewItem::text(int): Default switch statement reached... Unknown column.\n");
        break;
    }
    return ret;
}

//---------------------------------------------------------
//   LMasterKeyEventItem
//!  Initializes a LMasterKeyEventItem with a KeyEvent
//---------------------------------------------------------
LMasterKeyEventItem::LMasterKeyEventItem(QTreeWidget* parent, const MusECore::KeyEvent& ev)
    : LMasterLViewItem(parent)
{
    keyEvent = ev;
    unsigned t = ev.tick;
    int bar, beat;
    unsigned tick;
    MusEGlobal::sigmap.tickValues(t, &bar, &beat, &tick);
    c1 = QString("%1.%2.%3")
            .arg(bar + 1,      4, 10, QLatin1Char('0'))
            .arg(beat + 1,     2, 10, QLatin1Char('0'))
            .arg(tick,         3, 10, QLatin1Char('0'));

    double time = double(MusEGlobal::tempomap.tick2frame(t)) / double(MusEGlobal::sampleRate);
    int min = int(time) / 60;
    int sec = int(time) % 60;
    int msec = int((time - (min*60 + sec)) * 1000.0);
    c2 = QString("%1:%2:%3")
            .arg(min,  3, 10, QLatin1Char('0'))
            .arg(sec,  2, 10, QLatin1Char('0'))
            .arg(msec, 3, 10, QLatin1Char('0'));
    c3 = "Key";
    c4 = ev.keyString();
    setText(0, c1);
    setText(1, c2);
    setText(2, c3);
    setText(3, c4);

}


LMASTER_LVTYPE LMasterKeyEventItem::getType() const { return LMASTER_KEYEVENT; }
const MusECore::KeyEvent& LMasterKeyEventItem::getEvent() const { return keyEvent; }
unsigned LMasterKeyEventItem::tick() const { return keyEvent.tick; }
MusECore::key_enum LMasterKeyEventItem::key() const { return keyEvent.key; }
bool LMasterKeyEventItem::isMinor() const { return keyEvent.minor; }

//---------------------------------------------------------
//   LMasterTempoItem
//!  Initializes a LMasterTempoItem with a TEvent
//---------------------------------------------------------
LMasterTempoItem::LMasterTempoItem(QTreeWidget* parent, const MusECore::TEvent* ev)
    : LMasterLViewItem(parent)
{
    tempoEvent = ev;
    unsigned t = ev->tick;
    int bar, beat;
    unsigned tick;
    MusEGlobal::sigmap.tickValues(t, &bar, &beat, &tick);
    c1 = QString("%1.%2.%3")
            .arg(bar + 1,      4, 10, QLatin1Char('0'))
            .arg(beat + 1,     2, 10, QLatin1Char('0'))
            .arg(tick,         3, 10, QLatin1Char('0'));

    double time = double(MusEGlobal::tempomap.tick2frame(t)) / double(MusEGlobal::sampleRate);
    int min = int(time) / 60;
    int sec = int(time) % 60;
    int msec = int((time - (min*60 + sec)) * 1000.0);
    c2 = QString("%1:%2:%3")
            .arg(min,  3, 10, QLatin1Char('0'))
            .arg(sec,  2, 10, QLatin1Char('0'))
            .arg(msec, 3, 10, QLatin1Char('0'));
    c3 = "Tempo";
    double dt = (1000000.0 * 60.0)/ev->tempo;
    c4.setNum(dt, 'f', 3);
    setText(0, c1);
    setText(1, c2);
    setText(2, c3);
    setText(3, c4);
}

LMASTER_LVTYPE LMasterTempoItem::getType() const { return LMASTER_TEMPO; }
const MusECore::TEvent* LMasterTempoItem::getEvent() const { return tempoEvent; }
unsigned LMasterTempoItem::tick() const { return tempoEvent->tick; }
int LMasterTempoItem::tempo() const { return tempoEvent->tempo; }

//---------------------------------------------------------
//   LMasterSigEventItem
//!  Initializes a ListView item with a SigEvent
//---------------------------------------------------------
LMasterSigEventItem::LMasterSigEventItem(QTreeWidget* parent, const MusECore::SigEvent* ev)
    : LMasterLViewItem(parent)
{
    sigEvent = ev;
    unsigned t = ev->tick;
    int bar, beat;
    unsigned tick;
    MusEGlobal::sigmap.tickValues(t, &bar, &beat, &tick);
    c1 = QString("%1.%2.%3")
            .arg(bar + 1,      4, 10, QLatin1Char('0'))
            .arg(beat + 1,     2, 10, QLatin1Char('0'))
            .arg(tick,         3, 10, QLatin1Char('0'));

    double time = double(MusEGlobal::tempomap.tick2frame(t)) / double (MusEGlobal::sampleRate);
    int min = int(time) / 60;
    int sec = int(time) % 60;
    int msec = int((time - (min*60 + sec)) * 1000.0);
    c2 = QString("%1:%2:%3")
            .arg(min,  3, 10, QLatin1Char('0'))
            .arg(sec,  2, 10, QLatin1Char('0'))
            .arg(msec, 3, 10, QLatin1Char('0'));
    c3 = "Timesig";
    c4 = QString("%1  /  %2").arg(ev->sig.z).arg(ev->sig.n);
    setText(0, c1);
    setText(1, c2);
    setText(2, c3);
    setText(3, c4);
}

LMASTER_LVTYPE LMasterSigEventItem::getType() const { return LMASTER_SIGEVENT; }
const MusECore::SigEvent* LMasterSigEventItem::getEvent() const { return sigEvent; }
unsigned LMasterSigEventItem::tick() const { return sigEvent->tick; }
int LMasterSigEventItem::z() const { return sigEvent->sig.z; }
int LMasterSigEventItem::n() const { return sigEvent->sig.n; }

//---------------------------------------------------------
//   tempoButtonClicked()
//!  inserts a new tempo-item in the list and starts the editor for it
//---------------------------------------------------------
void LMaster::tempoButtonClicked()
{
    LMasterTempoItem* lastTempo = (LMasterTempoItem*) getLastOfType(LMASTER_TEMPO);
    //      QString beatString = ((LMasterLViewItem*)lastTempo)->text(LMASTER_BEAT_COL); DELETETHIS?
    //      int m, b, t;
    //      Pos p = Pos(beatString);
    //      p.mbt(&m, &b, &t);
    //      m++; //Next bar
    //      int newTick = MusEGlobal::sigmap.bar2tick(m, b, t);
    int newTick = MusEGlobal::song->cpos();
    MusECore::TEvent* ev = new MusECore::TEvent(lastTempo->tempo(), newTick);
    new LMasterTempoItem(view, ev);
    QTreeWidgetItem* newTempoItem = view->topLevelItem(0);

    editingNewItem = true; // State
    editorColumn = LMASTER_VAL_COL; // Set that we edit editorColumn
    view->clearSelection();
    view->setCurrentItem(newTempoItem);
    itemDoubleClicked(newTempoItem);
}


//---------------------------------------------------------
//   timeSigButtonClicked()
//!  inserts a new sig-item in the list and starts the editor for it
//---------------------------------------------------------
void LMaster::timeSigButtonClicked()
{
    LMasterSigEventItem* lastSig = (LMasterSigEventItem*) getLastOfType(LMASTER_SIGEVENT);
    //      QString beatString = ((LMasterLViewItem*)lastSig)->text(LMASTER_BEAT_COL); DELETETHIS
    //      int m, b, t;
    //      Pos p = Pos(beatString);
    //      p.mbt(&m, &b, &t);
    //      m++;
    //      int newTick = MusEGlobal::sigmap.bar2tick(m, b, t);
    int newTick = MusEGlobal::song->cpos();
    newTick = MusEGlobal::sigmap.raster1(newTick, 0);
    MusECore::SigEvent* ev = new MusECore::SigEvent(MusECore::TimeSignature(lastSig->z(), lastSig->n()), newTick);
    new LMasterSigEventItem(view, ev);
    QTreeWidgetItem* newSigItem = view->topLevelItem(0);

    editingNewItem = true; // State
    editorColumn = LMASTER_VAL_COL; // Set that we edit editorColumn
    view->clearSelection();
    view->setCurrentItem(newSigItem);
    itemDoubleClicked(newSigItem);
}

//---------------------------------------------------------
//   insertKey()
//!  inserts a new key in the list and starts the editor for it
//---------------------------------------------------------
void LMaster::insertKey()
{
    LMasterKeyEventItem* lastKey = (LMasterKeyEventItem*) getLastOfType(LMASTER_KEYEVENT);

    //QString beatString = ((LMasterLViewItem*)lastKey)->text(LMASTER_BEAT_COL); DELETETHIS
    //int m, b, t;
    //Pos p = Pos(beatString);
    //p.mbt(&m, &b, &t);
    //m++; //Next bar

    int newTick = MusEGlobal::song->cpos();
    new LMasterKeyEventItem(view, MusECore::KeyEvent(
                                lastKey ? lastKey->key() : MusECore::KEY_C, newTick, lastKey ? lastKey->isMinor() : false));
    QTreeWidgetItem* newKeyItem = view->topLevelItem(0);

    editingNewItem = true; // State
    editorColumn = LMASTER_VAL_COL; // Set that we edit editorColumn
    view->clearSelection();
    view->setCurrentItem(newKeyItem);
    itemDoubleClicked(newKeyItem);
}


/*!
    \fn LMaster::getLastOfType(LMASTER_LVTYPE t)
 */
LMasterLViewItem* LMaster::getLastOfType(LMASTER_LVTYPE t)
{
    LMasterLViewItem* tmp = (LMasterLViewItem*) view->topLevelItem(view->topLevelItemCount() - 1);
    while (tmp->getType() != t) {
        tmp = (LMasterLViewItem*) view->itemAbove(tmp);
    }
    return tmp;
}


/*!
    \fn LMaster::getItemAtPos(unsigned tick, LMASTER_LVTYPE t)
 */
LMasterLViewItem* LMaster::getItemAtPos(unsigned tick, LMASTER_LVTYPE t)
{
    LMasterLViewItem* tmp = (LMasterLViewItem*) view->topLevelItem(0);
    while (tmp) {
        if (tmp->getType() == t && tmp->tick() == tick)
            return tmp;
        tmp = (LMasterLViewItem*) view->itemBelow(tmp);
    }

    return nullptr;
}


/*!
    \fn LMaster::configChanged()
 */
void LMaster::configChanged()
{
    initShortcuts();
}


/*!
    \fn LMaster::initShortcuts()
 */
void LMaster::initShortcuts()
{
    tempoAction->setShortcut(shortcuts[SHRT_LM_INS_TEMPO].key);
    signAction->setShortcut(shortcuts[SHRT_LM_INS_SIG].key);
    keyAction->setShortcut(shortcuts[SHRT_LM_INS_KEY].key);
    posAction->setShortcut(shortcuts[SHRT_LM_EDIT_BEAT].key);
    valAction->setShortcut(shortcuts[SHRT_LM_EDIT_VALUE].key);
    delAction->setShortcut(shortcuts[SHRT_DELETE].key);
}

void LMaster::comboboxTimerSlot()
{
    key_editor->showPopup();
}

QSize LMaster::sizeHint() const {
    return QSize(380, 400);
}

bool LMaster::eventFilter(QObject* obj, QEvent *e)
{
    if (obj == view && e->type() == QEvent::KeyPress) {
        QKeyEvent* ke = static_cast<QKeyEvent*>(e);
        if (ke->key() == Qt::Key_Return) {
            editingFinished();
            return true;
        }
    }

    if (!view->hasFocus())
        return false;

    if (e->type() == QEvent::Shortcut) {
        QShortcutEvent* sev = static_cast<QShortcutEvent*>(e);
        if (sev->isAmbiguous()) {
            for (const auto& action : actions()) {
                if (action->shortcut() == sev->key()) {
                    action->trigger();
                    return true;
                }
            }
        }
    }

    return false;
}


} // namespace MusEGui
