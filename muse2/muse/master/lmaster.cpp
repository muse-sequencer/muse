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

#include "awl/posedit.h"
#include "awl/sigedit.h"

#include "lmaster.h"
#include "xml.h"
#include "song.h"
#include "globals.h"
#include "app.h"
#include "audio.h"
#include "shortcuts.h"
#include "debug.h"
#include "gconfig.h"

#include <QCloseEvent>
#include <QGridLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSignalMapper>
#include <QStyle>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QComboBox>
#include <QTimer>

#define LMASTER_BEAT_COL 0
#define LMASTER_TIME_COL 1
#define LMASTER_TYPE_COL 2
#define LMASTER_VAL_COL  3

#define LMASTER_MSGBOX_STRING          "MusE: List Editor"

namespace MusECore {

//don't remove or insert new elements in keyStrs.
//only renaming (keeping the semantic sense) is allowed! (flo)
QStringList keyStrs = QStringList()
                      << "C (sharps)" << "G" << "D" << "A"<< "E" << "B" << "F#"
                      << "C (flats)"<< "F"<< "Bb" << "Eb"<< "Ab"<< "Db"<< "Gb";

//don't change this function (except when renaming stuff)
key_enum stringToKey(QString input) //flo
{
	int index = keyStrs.indexOf(input);
	key_enum map[]={KEY_C, KEY_G, KEY_D, KEY_A, KEY_E, KEY_B, KEY_FIS, KEY_C_B, KEY_F, KEY_BES, KEY_ES, KEY_AS, KEY_DES, KEY_GES};
	return map[index];
}

//don't change the below two functions (except when renaming stuff)
int keyToIndex(key_enum key)
{
  int index=0;
	switch(key)
	{
		case KEY_C:   index= 0; break;
		case KEY_G:   index= 1; break;
		case KEY_D:   index= 2; break;
		case KEY_A:   index= 3; break;
		case KEY_E:   index= 4; break;
		case KEY_B:   index= 5; break;
		case KEY_FIS: index= 6; break;
		case KEY_C_B: index= 7; break;
		case KEY_F:   index= 8; break;
		case KEY_BES: index= 9; break;
		case KEY_ES:  index=10; break;
		case KEY_AS:  index=11; break;
		case KEY_DES: index=12; break;
		case KEY_GES: index=13; break;

		case KEY_SHARP_BEGIN:
		case KEY_SHARP_END:
		case KEY_B_BEGIN:
		case KEY_B_END:
			printf("ILLEGAL FUNCTION CALL: keyToIndex called with key_sharp_begin etc.\n");
      return 0;
			break;
		
		default:
			printf("ILLEGAL FUNCTION CALL: keyToIndex called with illegal key value (not in enum)\n");
      return 0;
	}
	return index;
}

QString keyToString(key_enum key)
{
	return keyStrs[keyToIndex(key)];
}

} // namespace MusECore

namespace MusEGui {

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void LMaster::closeEvent(QCloseEvent* e)
      {
      _isDeleting = true;  // Set flag so certain signals like songChanged, which may cause crash during delete, can be ignored.
      
      emit isDeleting(static_cast<TopWin*>(this));
      e->accept();
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void LMaster::songChanged(int type)
      {
      if(_isDeleting)  // Ignore while while deleting to prevent crash.
        return;
      
      if (type & (SC_SIG | SC_TEMPO | SC_KEY ))
            updateList();
      }

//---------------------------------------------------------
//   LMaster
//---------------------------------------------------------

LMaster::LMaster()
   : MidiEditor(TopWin::LMASTER, 0, 0, 0)
      {
      pos_editor = 0;
      tempo_editor = 0;
      sig_editor = 0;
      key_editor = 0;
      editedItem = 0;
      editingNewItem = false;
      setWindowTitle(tr("MusE: Mastertrack"));
      setMinimumHeight(100);
      //setFixedWidth(400);            // FIXME: Arbitrary. But without this, sig editor is too wide. Must fix sig editor width...
      setFocusPolicy(Qt::NoFocus);


      comboboxTimer=new QTimer(this);
      comboboxTimer->setInterval(150);
      comboboxTimer->setSingleShot(true);
      connect(comboboxTimer, SIGNAL(timeout()), this, SLOT(comboboxTimerSlot()));


      //---------Pulldown Menu----------------------------
      menuEdit = menuBar()->addMenu(tr("&Edit"));
      QSignalMapper *signalMapper = new QSignalMapper(this);
      menuEdit->addActions(MusEGlobal::undoRedo->actions());
      menuEdit->addSeparator();
      tempoAction = menuEdit->addAction(tr("Insert Tempo"));
      signAction = menuEdit->addAction(tr("Insert Signature"));
      keyAction = menuEdit->addAction(tr("Insert Key"));
      posAction = menuEdit->addAction(tr("Edit Positon"));
      valAction = menuEdit->addAction(tr("Edit Value"));
      delAction = menuEdit->addAction(tr("Delete Event"));
      delAction->setShortcut(Qt::Key_Delete);

      QMenu* settingsMenu = menuBar()->addMenu(tr("Window &Config"));
      settingsMenu->addAction(subwinAction);
      settingsMenu->addAction(shareAction);
      settingsMenu->addAction(fullscreenAction);

      
      connect(tempoAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(signAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(keyAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(posAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(valAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(delAction, SIGNAL(triggered()), signalMapper, SLOT(map()));

      signalMapper->setMapping(tempoAction, CMD_INSERT_TEMPO);
      signalMapper->setMapping(signAction, CMD_INSERT_SIG);
      signalMapper->setMapping(keyAction, CMD_INSERT_KEY);
      signalMapper->setMapping(posAction, CMD_EDIT_BEAT);
      signalMapper->setMapping(valAction, CMD_EDIT_VALUE);
      signalMapper->setMapping(delAction, CMD_DELETE);

      connect(signalMapper, SIGNAL(mapped(int)), SLOT(cmd(int)));

      // Toolbars ---------------------------------------------------------
      QToolBar* edit = addToolBar(tr("Edit tools"));
      edit->setObjectName("Master List Edit Tools");
      QToolButton* tempoButton = new QToolButton();
      QToolButton* timeSigButton = new QToolButton();
      QToolButton* keyButton = new QToolButton();
      tempoButton->setFocusPolicy(Qt::NoFocus);
      timeSigButton->setFocusPolicy(Qt::NoFocus);
      keyButton->setFocusPolicy(Qt::NoFocus);
      tempoButton->setText(tr("Tempo"));
      timeSigButton->setText(tr("Timesig"));
      keyButton->setText(tr("Key"));
      tempoButton->setToolTip(tr("new tempo"));
      timeSigButton->setToolTip(tr("new signature"));
      keyButton->setToolTip(tr("new key"));
      edit->addWidget(tempoButton);
      edit->addWidget(timeSigButton);
      edit->addWidget(keyButton);
      
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

      mainGrid->setRowStretch(0, 100);
      mainGrid->setColumnStretch(0, 100);

      mainGrid->addWidget(view,  0, 0);
      updateList();

      tempo_editor = new QLineEdit(view->viewport());
      tempo_editor->setFrame(false);
      tempo_editor->hide();
      connect(tempo_editor, SIGNAL(returnPressed()), SLOT(returnPressed()));
      sig_editor = new SigEdit(view->viewport());
      sig_editor->setFrame(false);
      sig_editor->hide();
      connect(sig_editor, SIGNAL(returnPressed()), SLOT(returnPressed()));
      pos_editor = new Awl::PosEdit(view->viewport());
      pos_editor->setFrame(false);
      pos_editor->hide();
      connect(pos_editor, SIGNAL(returnPressed()), SLOT(returnPressed()));
      key_editor = new QComboBox(view->viewport());
      key_editor->setFrame(false);
      key_editor->addItems(MusECore::keyStrs);
      key_editor->hide();
      connect(key_editor, SIGNAL(activated(int)), SLOT(returnPressed()));

      connect(view, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), SLOT(select(QTreeWidgetItem*, QTreeWidgetItem*)));
      connect(view, SIGNAL(itemPressed(QTreeWidgetItem*, int)), SLOT(itemPressed(QTreeWidgetItem*, int)));
      connect(view, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), SLOT(itemDoubleClicked(QTreeWidgetItem*)));
      connect(MusEGlobal::song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      connect(this, SIGNAL(seekTo(int)), MusEGlobal::song, SLOT(seekTo(int)));
      connect(tempoButton, SIGNAL(clicked()), SLOT(tempoButtonClicked()));
      connect(timeSigButton, SIGNAL(clicked()), SLOT(timeSigButtonClicked()));
      connect(keyButton, SIGNAL(clicked()), SLOT(insertKey()));

      initShortcuts();
      finalizeInit();
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

void LMaster::insertSig(const AL::SigEvent* ev)
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
      const AL::SigList* s   = &AL::sigmap;
      const MusECore::KeyList* k   = &MusEGlobal::keymap;

      MusECore::criTEvent it   = t->rbegin();
      AL::criSigEvent is = s->rbegin();
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
//   readStatus
//---------------------------------------------------------

void LMaster::readStatus(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            if (token == MusECore::Xml::Error || token == MusECore::Xml::End)
                  break;
            switch (token) {
                  case MusECore::Xml::TagStart:
                        if (tag == "midieditor")
                              MidiEditor::readStatus(xml);
                        else
                              xml.unknown("LMaster");
                        break;
                  case MusECore::Xml::TagEnd:
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

void LMaster::writeStatus(int level, MusECore::Xml& xml) const
      {
      xml.tag(level++, "lmaster");
      MidiEditor::writeStatus(level, xml);
      xml.tag(level, "/lmaster");
      }

//---------------------------------------------------------
//   readConfiguration
//---------------------------------------------------------

void LMaster::readConfiguration(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "topwin")
                              TopWin::readConfiguration(LMASTER, xml);
                        else
                              xml.unknown("LMaster");
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "lmaster")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   writeConfiguration
//---------------------------------------------------------

void LMaster::writeConfiguration(int level, MusECore::Xml& xml)
      {
      xml.tag(level++, "lmaster");
      TopWin::writeConfiguration(LMASTER, level, xml);
      xml.tag(level, "/lmaster");
      }

//---------------------------------------------------------
//   select
//---------------------------------------------------------

//DELETETHIS (whole function)? or is this todo?
void LMaster::select(QTreeWidgetItem* /*item*/, QTreeWidgetItem* /*previous_item*/)
      {
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
                                    MusEGlobal::audio->msgDeleteTempo(t->tick(), t->tempo(), true);
                                    break;
                                    }
                              case LMASTER_SIGEVENT:
                                    {
                                    LMasterSigEventItem* s = (LMasterSigEventItem*) l;
                                    MusEGlobal::audio->msgRemoveSig(s->tick(), s->z(), s->n());
                                    break;
                                    }
                              case LMASTER_KEYEVENT:
                                    {
                                    LMasterKeyEventItem* k = (LMasterKeyEventItem*) l;
                                    MusEGlobal::audio->msgRemoveKey(k->tick(), k->key());
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
            returnPressed();
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
      int fnt_w = fm.width('0');
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
                  key_editor->setCurrentIndex(keyToIndex(dynamic_cast<LMasterKeyEventItem*>(editedItem)->key()));
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
                  editedItem = 0;
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

void LMaster::returnPressed()
      {
      if (!editedItem)
            return;

      setFocus();
      // Tempo event:
      if (editedItem->getType() == LMASTER_TEMPO && editorColumn == LMASTER_VAL_COL) {
            QString input = tempo_editor->text();
            tempo_editor->hide();
            repaint();
            LMasterTempoItem* e = (LMasterTempoItem*) editedItem;
            const MusECore::TEvent* t = e->getEvent();
            unsigned tick = t->tick;
            bool conversionOK;
            double dbl_input = input.toDouble(&conversionOK);
            if (conversionOK && dbl_input < 250.0) {
                  int tempo = (int) ((1000000.0 * 60.0)/dbl_input);

                  if (!editingNewItem) {
                        MusEGlobal::song->startUndo();
                        MusEGlobal::audio->msgDeleteTempo(tick, e->tempo(), false);
                        MusEGlobal::audio->msgAddTempo(tick, tempo, false);
                        MusEGlobal::song->endUndo(SC_TEMPO);
                        }
                  //
                  // New item edited:
                  //
                  else {
                        MusEGlobal::audio->msgAddTempo(tick, tempo, true);
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
                        MusEGlobal::song->startUndo();
                        MusEGlobal::audio->msgDeleteTempo(oldtick, tempo, false);
                        MusEGlobal::audio->msgAddTempo(newtick, tempo, false);
                        MusEGlobal::song->endUndo(SC_TEMPO);
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
                              MusEGlobal::song->startUndo();
                              MusEGlobal::audio->msgRemoveSig(oldtick, z, n, false); //Delete first, in order to get sane tick-value
                              newtick = pos_editor->pos().tick();
                              MusEGlobal::audio->msgAddSig(newtick, z, n, false);
                              MusEGlobal::song->endUndo(SC_SIG);
                              }
                        else
                              MusEGlobal::audio->msgAddSig(newtick, z, n, false);

                        // Select the item:
                        QTreeWidgetItem* newSelected = (QTreeWidgetItem*) getItemAtPos(newtick, LMASTER_SIGEVENT);
                        if (newSelected) {
                              view->clearSelection();
                              view->setCurrentItem(newSelected);
                              }
                        }
                  else if (editedItem->getType() == LMASTER_KEYEVENT) {
                        LMasterKeyEventItem* k = (LMasterKeyEventItem*) editedItem;
                        MusECore::key_enum key = k->key();
                        MusEGlobal::song->startUndo();
                        MusEGlobal::audio->msgRemoveKey(oldtick, key, false);
                        MusEGlobal::audio->msgAddKey(newtick, key, false);
                        MusEGlobal::song->endUndo(SC_KEY);

                        // Select the item:
                        QTreeWidgetItem* newSelected = (QTreeWidgetItem*) getItemAtPos(newtick, LMASTER_KEYEVENT);
                        if (newSelected) {
                              view->clearSelection();
                              view->setCurrentItem(newSelected);
                              }
                        }
                  else {
                    printf("unknown master list event type!\n");
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
            AL::TimeSignature newSig = sig_editor->sig();
            
            sig_editor->hide();
            
            // Added p3.3.43 Prevents aborting with 0 z or n.
            if(newSig.isValid())
            {
              LMasterSigEventItem* e = (LMasterSigEventItem*) editedItem;
              int tick = e->tick();
              if (!editingNewItem) {
                    MusEGlobal::song->startUndo();
                    MusEGlobal::audio->msgRemoveSig(tick, e->z(), e->n(), false);
                    MusEGlobal::audio->msgAddSig(tick, newSig.z, newSig.n, false);
                    MusEGlobal::song->endUndo(SC_SIG);
                    }
              else
                    MusEGlobal::audio->msgAddSig(tick, newSig.z, newSig.n, true);
            }
            else {
              printf("Signature is not valid!\n");
            }
      }

      else if (editedItem->getType() == LMASTER_KEYEVENT && editorColumn == LMASTER_VAL_COL) {
          QString input = key_editor->currentText();
          key_editor->hide();
          repaint();
          LMasterKeyEventItem* e = (LMasterKeyEventItem*) editedItem;
          const MusECore::KeyEvent& t = e->getEvent();
          unsigned tick = t.tick;
          MusECore::key_enum key = MusECore::stringToKey(input);

          if (!editingNewItem) {
                      MusEGlobal::song->startUndo();
                      MusEGlobal::audio->msgRemoveKey(tick, e->key(), false);
                      MusEGlobal::audio->msgAddKey(tick, key, false);
                      MusEGlobal::song->endUndo(SC_KEY);
                    }
              //
              // New item edited:
              //
              else {
                    MusEGlobal::audio->msgAddKey(tick, key, true);
                    }
          }
      updateList();
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
      AL::sigmap.tickValues(t, &bar, &beat, &tick);
      c1.sprintf("%04d.%02d.%03d", bar+1, beat+1, tick);

      double time = double(MusEGlobal::tempomap.tick2frame(t)) / double(MusEGlobal::sampleRate);
      int min = int(time) / 60;
      int sec = int(time) % 60;
      int msec = int((time - (min*60 + sec)) * 1000.0);
      c2.sprintf("%03d:%02d:%03d", min, sec, msec);
      c3 = "Key";
      c4 = keyToString(ev.key);
      setText(0, c1);
      setText(1, c2);
      setText(2, c3);
      setText(3, c4);

      }


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
      AL::sigmap.tickValues(t, &bar, &beat, &tick);
      c1.sprintf("%04d.%02d.%03d", bar+1, beat+1, tick);

      double time = double(MusEGlobal::tempomap.tick2frame(t)) / double(MusEGlobal::sampleRate);
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

      double time = double(MusEGlobal::tempomap.tick2frame(t)) / double (MusEGlobal::sampleRate);
      int min = int(time) / 60;
      int sec = int(time) % 60;
      int msec = int((time - (min*60 + sec)) * 1000.0);
      c2.sprintf("%03d:%02d:%03d", min, sec, msec);
      c3 = "Timesig";
      c4.sprintf("%d  /  %d", ev->sig.z, ev->sig.n);
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
//      QString beatString = ((LMasterLViewItem*)lastTempo)->text(LMASTER_BEAT_COL); DELETETHIS?
//      int m, b, t;
//      Pos p = Pos(beatString);
//      p.mbt(&m, &b, &t);
//      m++; //Next bar
//      int newTick = AL::sigmap.bar2tick(m, b, t);
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
//      int newTick = AL::sigmap.bar2tick(m, b, t);
      int newTick = MusEGlobal::song->cpos();
      AL::SigEvent* ev = new AL::SigEvent(AL::TimeSignature(lastSig->z(), lastSig->n()), newTick);
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
      new LMasterKeyEventItem(view, MusECore::KeyEvent(lastKey->key(), newTick));
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
      keyAction->setShortcut(shortcuts[SHRT_LM_INS_KEY].key);
      posAction->setShortcut(shortcuts[SHRT_LM_EDIT_BEAT].key);
      valAction->setShortcut(shortcuts[SHRT_LM_EDIT_VALUE].key);
      }

void LMaster::comboboxTimerSlot()
{
    key_editor->showPopup();
}

} // namespace MusEGui
