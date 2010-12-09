//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: lmaster.cpp,v 1.2.2.8 2009/03/09 02:05:18 terminator356 Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#include "awl/posedit.h"
#include "awl/sigedit.h"

#include "lmaster.h"
#include "xml.h"
#include "song.h"
#include "globals.h"
#include "audio.h"
///#include "posedit.h"
///#include "sigedit.h"
#include "shortcuts.h"
#include "debug.h"

#include <QCloseEvent>
#include <QMenu>
#include <QMessageBox>
#include <QStyle>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>

#define LMASTER_BEAT_COL 0
#define LMASTER_TIME_COL 1
#define LMASTER_TYPE_COL 2
#define LMASTER_VAL_COL  3

#define LMASTER_MSGBOX_STRING          "MusE: List Editor"
//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void LMaster::closeEvent(QCloseEvent* e)
      {
      emit deleted((unsigned long)this);
      e->accept();
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void LMaster::songChanged(int type)
      {
      if (type & (SC_SIG | SC_TEMPO))
            updateList();
      }

//---------------------------------------------------------
//   LMaster
//---------------------------------------------------------

LMaster::LMaster()
   : MidiEditor(0, 0, 0)
      {
      pos_editor = 0;
      editor = 0;
      sig_editor = 0;
      editedItem = 0;
      editingNewItem = false;
      setWindowTitle(tr("MusE: Mastertrack"));
      setMinimumHeight(100);
      setFixedWidth(400);

      //---------Pulldown Menu----------------------------
      menuEdit = new QMenu(tr("&Edit"));
      QSignalMapper *signalMapper = new QSignalMapper(this);
      menuBar()->addMenu(menuEdit);
      menuEdit->addActions(undoRedo->actions());
      menuEdit->addSeparator();
      tempoAction = menuEdit->addAction(tr("Insert Tempo"));
      signAction = menuEdit->addAction(tr("Insert Signature"));
      posAction = menuEdit->addAction(tr("Edit Positon"));
      valAction = menuEdit->addAction(tr("Edit Value"));
      delAction = menuEdit->addAction(tr("Delete Event"));
      delAction->setShortcut(Qt::Key_Delete);

      connect(tempoAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(signAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(posAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(valAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(delAction, SIGNAL(triggered()), signalMapper, SLOT(map()));

      signalMapper->setMapping(tempoAction, CMD_INSERT_TEMPO);
      signalMapper->setMapping(signAction, CMD_INSERT_SIG);
      signalMapper->setMapping(posAction, CMD_EDIT_BEAT);
      signalMapper->setMapping(valAction, CMD_EDIT_VALUE);
      signalMapper->setMapping(delAction, CMD_DELETE);

      connect(signalMapper, SIGNAL(mapped(int)), SLOT(cmd(int)));

      //---------ToolBar----------------------------------
      tools = addToolBar(tr("Master tools"));
      tools->addActions(undoRedo->actions());

      //QToolBar* edit = new QToolBar(this, "edit tools");
      QToolBar* edit = addToolBar(tr("Edit tools"));
      //QToolButton* tempoButton = new QToolButton(edit);
      QToolButton* tempoButton = new QToolButton();
      //QToolButton* timeSigButton = new QToolButton(edit);
      QToolButton* timeSigButton = new QToolButton();
      tempoButton->setText(tr("Tempo"));
      timeSigButton->setText(tr("Timesig"));
      tempoButton->setToolTip(tr("new tempo"));
      timeSigButton->setToolTip(tr("new signature"));
      edit->addWidget(tempoButton);
      edit->addWidget(timeSigButton);
      
      ///Q3Accel* qa = new Q3Accel(this);
      ///qa->connectItem(qa->insertItem(Qt::CTRL+Qt::Key_Z), song, SLOT(undo()));
      ///qa->connectItem(qa->insertItem(Qt::CTRL+Qt::Key_Y), song, SLOT(redo()));

      //---------------------------------------------------
      //    master
      //---------------------------------------------------

      view = new QTreeWidget;
      view->setAllColumnsShowFocus(true);
      view->setSelectionMode(QAbstractItemView::SingleSelection);
      QStringList columnnames;
      columnnames << tr("Meter")
                  << tr("Time")
                  << tr("Type")
                  << tr("Value");
      view->setHeaderLabels(columnnames);
      view->setColumnWidth(2,80);
      view->header()->setStretchLastSection(true);

      //---------------------------------------------------
      //    Rest
      //---------------------------------------------------

//      QSizeGrip* corner = new QSizeGrip(mainw);

      mainGrid->setRowStretch(0, 100);
      mainGrid->setColumnStretch(0, 100);

      mainGrid->addWidget(view,  0, 0);
//      mainGrid->addWidget(corner,  1, 1, AlignBottom | AlignRight);
      updateList();

      connect(view, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), SLOT(select(QTreeWidgetItem*, QTreeWidgetItem*)));
      connect(view, SIGNAL(itemPressed(QTreeWidgetItem*, int)), SLOT(itemPressed(QTreeWidgetItem*, int)));
      connect(view, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), SLOT(itemDoubleClicked(QTreeWidgetItem*)));
      connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      connect(tempoButton, SIGNAL(clicked()), SLOT(tempoButtonClicked()));
      connect(timeSigButton, SIGNAL(clicked()), SLOT(timeSigButtonClicked()));

      initShortcuts();
      }

//---------------------------------------------------------
//   ~LMaster
//---------------------------------------------------------

LMaster::~LMaster()
      {
      //undoRedo->removeFrom(tools);  // p4.0.6 Removed
      }

//---------------------------------------------------------
//   insertSig
//---------------------------------------------------------

void LMaster::insertSig(const AL::SigEvent* ev)
      {
      new LMasterSigEventItem(view, ev);
      }

//---------------------------------------------------------
//   insertTempo
//---------------------------------------------------------

void LMaster::insertTempo(const TEvent* ev)
      {
      new LMasterTempoItem(view, ev);
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
      const TempoList* t = &tempomap;
      const AL::SigList* s   = &AL::sigmap;

      criTEvent it   = t->rbegin();
      AL::criSigEvent is = s->rbegin();
      for (;;) {
            if (it == t->rend()) {
                  while(is != s->rend()) {
                        insertSig(is->second);
                        ++is;
                        }
                  break;
                  }
            if (is == s->rend()) {
                  while (it != t->rend()) {
                        insertTempo(it->second);
                        ++it;
                        }
                  break;
                  }
            if (is->second->tick > it->second->tick) {
                  insertSig(is->second);
                  ++is;
                  }
            else {
                  insertTempo(it->second);
                  ++it;
                  }
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
//   readStatus
//---------------------------------------------------------

void LMaster::readStatus(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            if (token == Xml::Error || token == Xml::End)
                  break;
            switch (token) {
                  case Xml::TagStart:
                        if (tag == "midieditor")
                              MidiEditor::readStatus(xml);
                        else
                              xml.unknown("LMaster");
                        break;
                  case Xml::TagEnd:
                        if (tag == "lmaster")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void LMaster::writeStatus(int level, Xml& xml) const
      {
      xml.tag(level++, "lmaster");
      MidiEditor::writeStatus(level, xml);
      xml.tag(level, "/lmaster");
      }

//---------------------------------------------------------
//   select
//---------------------------------------------------------

void LMaster::select(QTreeWidgetItem* /*item*/, QTreeWidgetItem* /*previous_item*/)
      {
//      printf("select %x\n", unsigned(item));
      }

//---------------------------------------------------------
//   cmd
//---------------------------------------------------------

void LMaster::cmd(int cmd)
      {
      switch(cmd) {
            case CMD_DELETE: {
                  LMasterLViewItem* l = (LMasterLViewItem*) view->currentItem();
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
                                    audio->msgDeleteTempo(t->tick(), t->tempo(), true);
                                    break;
                                    }
                              case LMASTER_SIGEVENT:
                                    {
                                    LMasterSigEventItem* s = (LMasterSigEventItem*) l;
                                    audio->msgRemoveSig(s->tick(), s->z(), s->n());
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
      //printf("itemPressed, column: %d\n", column);
      if (editedItem) {
            if (editorColumn != column || editedItem != i)
            returnPressed();
            }
      else
            editorColumn = column;
      }

//---------------------------------------------------------
//   itemDoubleClicked(QListViewItem* item)
//!  Sets lmaster in edit mode, and opens editor for selected value
//---------------------------------------------------------
void LMaster::itemDoubleClicked(QTreeWidgetItem* i)
      {
      //printf("itemDoubleClicked\n");

      if (!editedItem && editorColumn == LMASTER_VAL_COL) {
            editedItem = (LMasterLViewItem*) i;
            QRect itemRect = view->visualItemRect(editedItem);
            int x1 = view->columnWidth(LMASTER_BEAT_COL) + view->columnWidth(LMASTER_TIME_COL)
                  + view->columnWidth(LMASTER_TYPE_COL);
            itemRect.setX(x1);
            //Qt makes crazy things with itemRect if this is called directly..
            if (editingNewItem) {
                  QFontMetrics fm(font());
                  int fw = style()->pixelMetric(QStyle::PM_DefaultFrameWidth,0 , this); // ddskrjo 0
                  int h  = fm.height() + fw * 2;
                  itemRect.setWidth(view->columnWidth(LMASTER_VAL_COL));
                  itemRect.setY(1);
                  itemRect.setHeight(h);
                  }


            // Edit tempo value:
            if (editedItem->getType() == LMASTER_TEMPO) {
                  if (!editor)
                        editor = new QLineEdit(view->viewport());
                  editor->setText(editedItem->text(LMASTER_VAL_COL));
                  editor->setGeometry(itemRect);
                  editor->show();
                  editor->setFocus();
                  editor->selectAll();
                  connect(editor, SIGNAL(returnPressed()), SLOT(returnPressed()));
                  }
            else { // Edit signatur value:
                  if (!sig_editor)
                        sig_editor = new SigEdit(view->viewport());
                  sig_editor->setValue(editedItem->text(LMASTER_VAL_COL));
                  sig_editor->setGeometry(itemRect);
                  sig_editor->show();
                  sig_editor->setFocus();
                  connect(sig_editor, SIGNAL(returnPressed()), SLOT(returnPressed()));
                  }
            }
      // Edit tempo or signal position:
      else if (!editedItem && editorColumn == LMASTER_BEAT_COL) {
            editedItem = (LMasterLViewItem*) i;
            // Don't allow movement of initial values:
            if  (editedItem->tick() == 0) {
                  QMessageBox::information(this, tr(LMASTER_MSGBOX_STRING),
                        tr("Reposition of the initial tempo and signature events is not allowed") );
                  editedItem = 0;
                  }
            // Everything OK
            else {
                  if (!pos_editor)
                        ///pos_editor = new PosEdit(view->viewport());
                        pos_editor = new Awl::PosEdit(view->viewport());
                  pos_editor->setValue(editedItem->tick());
                  QRect itemRect = view->visualItemRect(editedItem);
                  itemRect.setX(0);
                  itemRect.setWidth(view->columnWidth(LMASTER_BEAT_COL));
                  pos_editor->setGeometry(itemRect);
                  pos_editor->show();
                  pos_editor->setFocus();
                  connect(pos_editor, SIGNAL(returnPressed()), SLOT(returnPressed()));
                  }
            }
      }

//---------------------------------------------------------
//   returnPressed()
//!  called when editor is closed
//---------------------------------------------------------

void LMaster::returnPressed()
      {
      if (!editedItem)
            return;

      setFocus();
      // Tempo event:
      if (editedItem->getType() == LMASTER_TEMPO && editorColumn == LMASTER_VAL_COL) {
            QString input = editor->text();
            editor->hide();
            repaint();
            LMasterTempoItem* e = (LMasterTempoItem*) editedItem;
            const TEvent* t = e->getEvent();
            unsigned tick = t->tick;
            bool conversionOK;
            double dbl_input = input.toDouble(&conversionOK);
            if (conversionOK && dbl_input < 250.0) {
                  int tempo = (int) ((1000000.0 * 60.0)/dbl_input);

                  if (!editingNewItem) {
                        song->startUndo();
                        audio->msgDeleteTempo(tick, e->tempo(), false);
                        audio->msgAddTempo(tick, tempo, false);
                        song->endUndo(SC_TEMPO);
                        }
                  //
                  // New item edited:
                  //
                  else {
                        audio->msgAddTempo(tick, tempo, true);
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
                        LMasterTempoItem* t = (LMasterTempoItem*) editedItem;
                        int tempo = t->tempo();
                        song->startUndo();
                        audio->msgDeleteTempo(oldtick, tempo, false);
                        audio->msgAddTempo(newtick, tempo, false);
                        song->endUndo(SC_TEMPO);
                        // Select the item:
                        QTreeWidgetItem* newSelected = (QTreeWidgetItem*) getItemAtPos(newtick, LMASTER_TEMPO);
                        if (newSelected) {
                              view->clearSelection();
                              view->setCurrentItem(newSelected);
                              }
                        }
                  else if (editedItem->getType() == LMASTER_SIGEVENT) {
                        LMasterSigEventItem* t = (LMasterSigEventItem*) editedItem;
                        int z = t->z();
                        int n = t->n();
                        if (!editingNewItem) {
                              song->startUndo();
                              audio->msgRemoveSig(oldtick, z, n, false); //Delete first, in order to get sane tick-value
                              newtick = pos_editor->pos().tick();
                              audio->msgAddSig(newtick, z, n, false);
                              song->endUndo(SC_SIG);
                              }
                        else
                              audio->msgAddSig(newtick, z, n, false);
                        //audio->msgAddSig(newtick, z, n, true);

                        // Select the item:
                        QTreeWidgetItem* newSelected = (QTreeWidgetItem*) getItemAtPos(newtick, LMASTER_SIGEVENT);
                        if (newSelected) {
                              view->clearSelection();
                              view->setCurrentItem(newSelected);
                              }
                        }

                  }
            pos_editor->hide();
            repaint();
            }
      //
      // SigEvent, value changed:
      //
      else if (editedItem->getType() == LMASTER_SIGEVENT && editorColumn == LMASTER_VAL_COL) 
      {
            ///Sig newSig = sig_editor->sig();
            AL::TimeSignature newSig = sig_editor->sig();
            
            sig_editor->hide();
            
            // Added p3.3.43 Prevents aborting with 0 z or n.
            if(newSig.isValid())
            {
            
              LMasterSigEventItem* e = (LMasterSigEventItem*) editedItem;
              int tick = e->tick();
              if (!editingNewItem) {
                    song->startUndo();
                    if (tick > 0)
                          audio->msgRemoveSig(tick, e->z(), e->n(), false);
                    audio->msgAddSig(tick, newSig.z, newSig.n, false);
                    song->endUndo(SC_SIG);
                    }
              else
                    audio->msgAddSig(tick, newSig.z, newSig.n, true);
            }        
      }

      view->setFocus();
      // No item edited now:
      editedItem = 0;
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
//   LMasterTempoItem
//!  Initializes a LMasterTempoItem with a TEvent
//---------------------------------------------------------
LMasterTempoItem::LMasterTempoItem(QTreeWidget* parent, const TEvent* ev)
      : LMasterLViewItem(parent)
      {
      tempoEvent = ev;
      unsigned t = ev->tick;
      //QString c1, c2, c3, c4;
      int bar, beat;
      unsigned tick;
      AL::sigmap.tickValues(t, &bar, &beat, &tick);
      c1.sprintf("%04d.%02d.%03d", bar+1, beat+1, tick);

      double time = double(ev->frame) / double(sampleRate);
      int min = int(time) / 60;
      int sec = int(time) % 60;
      int msec = int((time - (min*60 + sec)) * 1000.0);
      c2.sprintf("%03d:%02d:%03d", min, sec, msec);
      c3 = "Tempo";
      double dt = (1000000.0 * 60.0)/ev->tempo;
      c4.setNum(dt, 'f', 3);
      setText(0, c1);
      setText(1, c2);
      setText(2, c3);
      setText(3, c4);
      }

//---------------------------------------------------------
//   LMasterSigEventItem
//!  Initializes a ListView item with a SigEvent
//---------------------------------------------------------
LMasterSigEventItem::LMasterSigEventItem(QTreeWidget* parent, const AL::SigEvent* ev)
      : LMasterLViewItem(parent)
      {
      sigEvent = ev;
      unsigned t = ev->tick;
      int bar, beat;
      unsigned tick;
      AL::sigmap.tickValues(t, &bar, &beat, &tick);
      c1.sprintf("%04d.%02d.%03d", bar+1, beat+1, tick);

      double time = double(tempomap.tick2frame(t)) / double (sampleRate);
      int min = int(time) / 60;
      int sec = int(time) % 60;
      int msec = int((time - (min*60 + sec)) * 1000.0);
      c2.sprintf("%03d:%02d:%03d", min, sec, msec);
      c3 = "Timesig";
      c4.sprintf("%d/%d", ev->sig.z, ev->sig.n);
      setText(0, c1);
      setText(1, c2);
      setText(2, c3);
      setText(3, c4);
      }

//---------------------------------------------------------
//   tempoButtonClicked()
//!  inserts a new tempo-item in the list and starts the editor for it
//---------------------------------------------------------
void LMaster::tempoButtonClicked()
      {
      LMasterTempoItem* lastTempo = (LMasterTempoItem*) getLastOfType(LMASTER_TEMPO);
      QString beatString = ((LMasterLViewItem*)lastTempo)->text(LMASTER_BEAT_COL);
      int m, b, t;
      Pos p = Pos(beatString);
      p.mbt(&m, &b, &t);
      m++; //Next bar
      int newTick = AL::sigmap.bar2tick(m, b, t);
      TEvent* ev = new TEvent(lastTempo->tempo(), newTick);
      new LMasterTempoItem(view, ev);
      QTreeWidgetItem* newTempoItem = view->topLevelItem(0);
      //LMasterTempoItem* newTempoItem = new LMasterTempoItem(view, ev);

      editingNewItem = true; // State
      editorColumn = LMASTER_VAL_COL; // Set that we edit editorColumn
      view->clearSelection();
      view->setCurrentItem(newTempoItem);
      itemDoubleClicked(newTempoItem);
      }


//---------------------------------------------------------
//   tempoButtonClicked()
//!  inserts a new sig-item in the list and starts the editor for it
//---------------------------------------------------------
void LMaster::timeSigButtonClicked()
      {
      LMasterSigEventItem* lastSig = (LMasterSigEventItem*) getLastOfType(LMASTER_SIGEVENT);
      QString beatString = ((LMasterLViewItem*)lastSig)->text(LMASTER_BEAT_COL);
      int m, b, t;
      Pos p = Pos(beatString);
      p.mbt(&m, &b, &t);
      m++;
      int newTick = AL::sigmap.bar2tick(m, b, t);
      AL::SigEvent* ev = new AL::SigEvent(AL::TimeSignature(lastSig->z(), lastSig->n()), newTick);
      new LMasterSigEventItem(view, ev);
      QTreeWidgetItem* newSigItem = view->topLevelItem(0);
      //LMasterSigEventItem* newSigItem = new LMasterSigEventItem(view, ev);

      editingNewItem = true; // State
      editorColumn = LMASTER_VAL_COL; // Set that we edit editorColumn
      view->clearSelection();
      view->setCurrentItem(newSigItem);
      itemDoubleClicked(newSigItem);
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

      return 0;
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
      posAction->setShortcut(shortcuts[SHRT_LM_EDIT_BEAT].key);
      valAction->setShortcut(shortcuts[SHRT_LM_EDIT_VALUE].key);
      }
