//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: listedit.cpp,v 1.11.2.11 2009/05/24 21:43:44 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
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

#include <QAction>
#include <QActionGroup>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QTreeWidgetItem>

#include "listedit.h"
#include "mtscale.h"
#include "globals.h"
#include "helper.h"
#include "icons.h"
#include "editevent.h"
#include "xml.h"
#include "pitchedit.h"
#include "song.h"
#include "audio.h"
#include "shortcuts.h"
#include "midi.h"
#include "event.h"
#include "midiport.h"
#include "midictrl.h"
#include "minstrument.h"
#include "app.h"
#include "gconfig.h"

namespace MusEGui {

//---------------------------------------------------------
//   EventListItem
//---------------------------------------------------------

class EventListItem : public QTreeWidgetItem {
   public:
      MusECore::Event event;
      MusECore::MidiPart* part;

      EventListItem(QTreeWidget* parent, MusECore::Event ev, MusECore::MidiPart* p)
         : QTreeWidgetItem(parent) {
            event = ev;
            part  = p;
            }
      virtual QString text(int col) const;


      virtual bool operator< ( const QTreeWidgetItem & other ) const
        {
          int col = other.treeWidget()->sortColumn();
          EventListItem* eli = (EventListItem*) &other;
          switch(col)
          {
            case 0:
                  return event.tick() < eli->event.tick();
                  break;
            case 1:
                  return part->tick() + event.tick() < (eli->part->tick() + eli->event.tick());
                  break;
            case 2:
                  return text(col).localeAwareCompare(other.text(col)) < 0;
                  break;   
            case 3:
                  return part->track()->outChannel() < eli->part->track()->outChannel();
                  break;
            case 4:
                  return event.dataA() < eli->event.dataA();
                  break;
            case 5:
                  return event.dataB() < eli->event.dataB();
                  break;
            case 6:
                  return event.dataC() < eli->event.dataC();
                  break;
            case 7:
                  return event.lenTick() < eli->event.lenTick();
                  break;
            case 8:
                  return text(col).localeAwareCompare(other.text(col)) < 0;
                  break;
            default:
                  break;
            }
          return 0;
          }
      };

/*---------------------------------------------------------
 *    midi_meta_name
 *---------------------------------------------------------*/

static QString midiMetaComment(const MusECore::Event& ev)
      {
      int meta  = ev.dataA();
      QString s = MusECore::midiMetaName(meta);

      switch (meta) {
            case 0:
            case 0x2f:
            case 0x51:
            case 0x54:
            case 0x58:
            case 0x59:
            case 0x74:
            case 0x7f:  return s;

            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
            case 9:
            case 0x0a:
            case 0x0b:
            case 0x0c:
            case 0x0d:
            case 0x0e:
            case 0x0f:
                  {
                  s += QString(": ");
                  const char* txt = (char*)(ev.data());
                  int len = ev.dataLen();
                  char buffer[len+1];
                  memcpy(buffer, txt, len);
                  buffer[len] = 0;

                  for (int i = 0; i < len; ++i) {
                        if (buffer[i] == '\n' || buffer[i] == '\r')
                        buffer[i] = ' ';
                        }
                  return s + QString(buffer);
                  }

            case 0x20:
            case 0x21:
            default:
                  {
                  s += QString(": ");
                  int i;
                  int len = ev.lenTick();
                  int n = len > 10 ? 10 : len;
                  for (i = 0; i < n; ++i) {
                        if (i >= ev.dataLen())
                              break;
                        s += QString(" 0x");
                        QString k;
                        k.setNum(ev.data()[i] & 0xff, 16);
                        s += k;
                        }
                  if (i == 10)
                        s += QString("...");
                  return s;
                  }
            }
      }
      
//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void ListEdit::closeEvent(QCloseEvent* e)
      {
      _isDeleting = true;  // Set flag so certain signals like songChanged, which may cause crash during delete, can be ignored.
      emit isDeleting(static_cast<TopWin*>(this));
      e->accept();
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void ListEdit::songChanged(MusECore::SongChangedStruct_t type)
      {
      if(_isDeleting)  // Ignore while while deleting to prevent crash.
        return;
       
      if (!type)
            return;
      if (type & (// SC_MIDI_TRACK_PROP  FIXME Needed, but might make it slow!
           SC_PART_REMOVED | SC_PART_MODIFIED 
         | SC_PART_INSERTED | SC_EVENT_REMOVED | SC_EVENT_MODIFIED
         | SC_SIG  // Required so that bar/beat/tick of listed items are shown correctly.
         | SC_EVENT_INSERTED | SC_SELECTION)) {
            if (type & (SC_PART_REMOVED | SC_PART_INSERTED | SC_PART_MODIFIED | SC_SIG))
                  genPartlist();
            // close window if editor has no parts anymore
            if (parts()->empty()) {
                  close();
                  return;
                  }
            liste->setSortingEnabled(false);
            if (type == SC_SELECTION) {
                  // BUGFIX: I found the keyboard modifier states affect how QTreeWidget::setCurrentItem() operates.
                  //         So for example (not) holding shift while lassoo-ing notes in piano roll affected 
                  //          whether multiple items were selected in this event list editor! 
                  //         Also blockSignals() definitely required - was messing up selections. p4.0.18 Tim.
                  bool ci_done = false;
                  liste->blockSignals(true);  
                  // Go backwards to avoid QTreeWidget::setCurrentItem() dependency on KB modifiers!
                  for (int row = liste->topLevelItemCount() -1; row >= 0 ; --row) 
                  {
                    QTreeWidgetItem* i = liste->topLevelItem(row);
                    bool sel = ((EventListItem*)i)->event.selected();
                    if (i->isSelected() ^ sel) 
                    {
                      // Do setCurrentItem() before setSelected().
                      if(sel && !ci_done)
                      {
                        liste->setCurrentItem(i);
                        ci_done = true;
                      }  
                      i->setSelected(sel);
                    }
                  }
                  liste->blockSignals(false); 
                  
                  }
            else {
                  curPart = 0;
                  curTrack = 0;
                  liste->blockSignals(true);
                  liste->clear();
                  for (MusECore::iPart p = parts()->begin(); p != parts()->end(); ++p) {
                        MusECore::MidiPart* part = (MusECore::MidiPart*) (p->second);
                        if (part->sn() == curPartId)
                              curPart  = part;
                        
                        for (MusECore::ciEvent i = part->events().begin(); i != part->events().end(); ++i) {
                              EventListItem* item = new EventListItem(liste, i->second, part);
                              for (int col = 0; col < liste->columnCount(); ++col)
                                    item->setText(col, item->text(col));
                              item->setSelected(i->second.selected());
                              if (item->event.tick() == (unsigned) selectedTick) { //prevent compiler warning: comparison of signed/unsigned)
                                    liste->setCurrentItem(item);
                                    item->setSelected(true);
                                    liste->scrollToItem(item, QAbstractItemView::EnsureVisible);
                                    }
                              }
                        }
                  liste->blockSignals(false);   
                  }
            
            if(!curPart)
            {
              if(!parts()->empty())
              {
                curPart  = (MusECore::MidiPart*)(parts()->begin()->second);
                if(curPart)
                  curTrack = curPart->track();
                else  
                  curPart = 0;
              }      
            }
          }  
      liste->setSortingEnabled(true);
      }

//---------------------------------------------------------
//   text
//---------------------------------------------------------

QString EventListItem::text(int col) const
      {
      QString s;
      switch(col) {
            case 0:
                  s.setNum(event.tick());
                  break;
            case 1:
                  {
                  int t = event.tick() + part->tick();
                  int bar, beat;
                  unsigned tick;
                  MusEGlobal::sigmap.tickValues(t, &bar, &beat, &tick);
                  s = QString("%1.%2.%3")
                      .arg(bar + 1,  4, 10, QLatin1Char('0'))
                      .arg(beat + 1, 2, 10, QLatin1Char('0'))
                      .arg(tick,     3, 10, QLatin1Char('0'));
                  }
                  break;
            case 2:
                  switch(event.type()) {
                        case MusECore::Note:
                              s = QString("Note");
                              break;
                        case MusECore::Controller:
                              {
                              const char* cs;
                              switch (MusECore::midiControllerType(event.dataA())) {
                                    case MusECore::MidiController::Controller7:  cs = "Ctrl7"; break;
                                    case MusECore::MidiController::Controller14: cs = "Ctrl14"; break;
                                    case MusECore::MidiController::RPN:          cs = "RPN"; break;
                                    case MusECore::MidiController::NRPN:         cs = "NRPN"; break;
                                    case MusECore::MidiController::Pitch:        cs = "Pitch"; break;
                                    case MusECore::MidiController::Program:      cs = "Program"; break;
                                    case MusECore::MidiController::PolyAftertouch: cs = "PolyAftertouch"; break;
                                    case MusECore::MidiController::Aftertouch:   cs = "Aftertouch"; break;
                                    case MusECore::MidiController::RPN14:        cs = "RPN14"; break;
                                    case MusECore::MidiController::NRPN14:       cs = "NRPN14"; break;
                                    default:           cs = "Ctrl?"; break;
                                    }
                              s = QString(cs);
                              }
                              break;
                        case MusECore::Sysex:
                              s = QString("SysEx");
                              break;
                        case MusECore::Meta:
                              s = QString("Meta");
                              break;
                        case MusECore::Wave:
                              break;
                        default:
                              printf("unknown event type %d\n", event.type());
                        }
                  break;
            case 3:
                  switch(event.type()) {
                    case MusECore::Sysex:
                    case MusECore::Meta:
                      break;
                      
                    default:
                       s.setNum(part->track()->outChannel() + 1);
                  }
                  break;
            case 4:
                  if (event.isNote())
                        s =  MusECore::pitch2string(event.dataA());
                  else if (event.type() == MusECore::Controller)
                        s.setNum(event.dataA() & 0xffff);  // mask off type bits
                  else
                  {
                    switch(event.type()) {
                      case MusECore::Sysex:
                      case MusECore::Meta:
                        break;
                        
                    default:
                      s.setNum(event.dataA());
                    }
                  }
                  break;
            case 5:
                  if(event.type() == MusECore::Controller &&
                    MusECore::midiControllerType(event.dataA()) == MusECore::MidiController::Program) 
                  {
                    int val = event.dataB();
                    int hb = ((val >> 16) & 0xff) + 1;
                    if (hb == 0x100)
                      hb = 0;
                    int lb = ((val >> 8) & 0xff) + 1;
                    if (lb == 0x100)
                      lb = 0;
                    int pr = (val & 0xff) + 1;
                    if (pr == 0x100)
                      pr = 0;
                    s = QString("%1-%2-%3").arg(hb).arg(lb).arg(pr);
                  }
                  else        
                  {
                    switch(event.type()) {
                      case MusECore::Sysex:
                      case MusECore::Meta:
                        break;
                        
                    default:
                      s.setNum(event.dataB());
                    }
                  }
                  break;
            case 6:
                  switch(event.type()) {
                    case MusECore::Sysex:
                    case MusECore::Meta:
                      break;
                      
                    default:
                      s.setNum(event.dataC());
                  }
                  break;
            case 7:
                  switch(event.type()) {
                        case MusECore::Sysex:
                        case MusECore::Meta:
                          s.setNum(event.dataLen());
                        break;
                        
                        case MusECore::Controller:
                        break;
                        
                        default:
                          s.setNum(event.lenTick());
                        break;
                  }
                  break;
            case 8:
                  switch(event.type()) {
                        case MusECore::Controller:
                              {
                              MusECore::MidiPort* mp = &MusEGlobal::midiPorts[part->track()->outPort()];
                              MusECore::MidiController* mc = mp->midiController(event.dataA());
                              s = mc->name();
                              }
                              break;
                        case MusECore::Sysex:
                        case MusECore::Meta:
                              {
                              if(event.type() == MusECore::Sysex)
                                s = MusECore::nameSysex(event.dataLen(), event.data(),
                                      MusEGlobal::midiPorts[part->track()->outPort()].instrument()) + QString(" ");
                              else if(event.type() == MusECore::Meta)  
                                s = midiMetaComment(event) + QString(" ");
                              int i;
                              for (i = 0; i < 10; ++i) {
                                    if (i >= event.dataLen())
                                          break;
                                    s += QString(" 0x");
                                    QString k;
                                    k.setNum(event.data()[i] & 0xff, 16);
                                    s += k;
                                    }
                              if (i == 10)
                                    s += QString("...");
                              
                              }
                              break;
                        default:
                              break;
                        }
                  break;

            }
      return s;
      }

//---------------------------------------------------------
//   ListEdit
//---------------------------------------------------------

ListEdit::ListEdit(MusECore::PartList* pl, QWidget* parent, const char* name)
   : MidiEditor(TopWin::LISTE, 0, pl, parent, name)
      {
      selectedTick=0;
      
      insertItems = new QActionGroup(this);
      insertItems->setExclusive(false);
      insertNote = new QAction(QIcon(*note1Icon), tr("Insert Note"), insertItems);
      insertSysEx = new QAction(QIcon(*sysexIcon), tr("Insert SysEx"), insertItems);
      insertCtrl = new QAction(QIcon(*ctrlIcon), tr("Insert Ctrl"), insertItems);
      insertMeta = new QAction(QIcon(*metaIcon), tr("Insert Meta"), insertItems);

      connect(insertNote,    SIGNAL(triggered()), SLOT(editInsertNote()));
      connect(insertSysEx,   SIGNAL(triggered()), SLOT(editInsertSysEx()));
      connect(insertCtrl,    SIGNAL(triggered()), SLOT(editInsertCtrl()));
      connect(insertMeta,    SIGNAL(triggered()), SLOT(editInsertMeta()));

      //---------Pulldown Menu----------------------------
      
      menuEdit = menuBar()->addMenu(tr("&Edit"));
      menuEdit->addActions(MusEGlobal::undoRedo->actions());

      menuEdit->addSeparator();
#if 0 // DELETETHIS or implement?
      QAction *cutAction = menuEdit->addAction(QIcon(*editcutIconSet), tr("Cut"));
      connect(cutAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
      editSignalMapper->setMapping(cutAction, EList::CMD_CUT);
      cutAction->setShortcut(Qt::CTRL+Qt::Key_X);
      QAction *copyAction = menuEdit->addAction(QIcon(*editcopyIconSet), tr("Copy"));
      connect(copyAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
      editSignalMapper->setMapping(cutAction, EList::CMD_COPY);
      copyAction->setShortcut(Qt::CTRL+Qt::Key_C);
      QAction *pasteAction = menuEdit->addAction(QIcon(*editpasteIconSet), tr("Paste"));
      connect(pasteAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
      editSignalMapper->setMapping(cutAction, EList::CMD_PASTE);
      pasteAction->setShortcut(Qt::CTRL+Qt::Key_V);
      menuEdit->insertSeparator();
#endif
      QAction *deleteAction = menuEdit->addAction(tr("Delete Events"));
      deleteAction->setShortcut(Qt::Key_Delete);
      menuEdit->addSeparator();
      QAction *incAction = menuEdit->addAction(tr("Increase Tick"));
      QAction *decAction = menuEdit->addAction(tr("Decrease Tick"));

      menuEdit->addActions(insertItems->actions());

      connect(deleteAction, &QAction::triggered, [this]() { cmd(CMD_DELETE); } );
      connect(incAction,    &QAction::triggered, [this]() { cmd(CMD_INC); } );
      connect(decAction,    &QAction::triggered, [this]() { cmd(CMD_DEC); } );

      QMenu* settingsMenu = menuBar()->addMenu(tr("&Display"));
      settingsMenu->addAction(subwinAction);
      settingsMenu->addAction(shareAction);
      settingsMenu->addAction(fullscreenAction);


      // Toolbars ---------------------------------------------------------

      // NOTICE: Please ensure that any tool bar object names here match the names assigned 
      //          to identical or similar toolbars in class MusE or other TopWin classes. 
      //         This allows MusE::setCurrentMenuSharingTopwin() to do some magic
      //          to retain the original toolbar layout. If it finds an existing
      //          toolbar with the same object name, it /replaces/ it using insertToolBar(),
      //          instead of /appending/ with addToolBar().

      addToolBarBreak();
      
      QToolBar* insertTools = addToolBar(tr("Insert tools"));
      insertTools->setObjectName("list insert tools");
      insertTools->addActions(insertItems->actions());
      
      //
      //---------------------------------------------------
      //    liste
      //---------------------------------------------------
      //

      liste = new QTreeWidget(mainw);
      QFontMetrics fm(liste->font());
// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
      int n = fm.horizontalAdvance('9');
#else
      int n = fm.width('9');
#endif
      int b = 24;
// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
      int c = fm.horizontalAdvance(QString("Val B"));
#else
      int c = fm.width(QString("Val B"));
#endif
      int sortIndW = n * 3;
      liste->setAllColumnsShowFocus(true);
      liste->sortByColumn(0, Qt::AscendingOrder);

      liste->setSelectionMode(QAbstractItemView::ExtendedSelection);

      QStringList columnnames;
      columnnames << tr("Tick")
		  << tr("Bar")
		  << tr("Type")
		  << tr("Ch")
		  << tr("Val A")
		  << tr("Val B")
		  << tr("Val C")
		  << tr("Len")
		  << tr("Comment");

      liste->setHeaderLabels(columnnames);

      liste->setColumnWidth(0, n * 6 + b);
// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
      liste->setColumnWidth(1, fm.horizontalAdvance(QString("9999.99.999")) + b);
      liste->setColumnWidth(2, fm.horizontalAdvance(QString("Program")) + b);
#else
      liste->setColumnWidth(1, fm.width(QString("9999.99.999")) + b);
      liste->setColumnWidth(2, fm.width(QString("Program")) + b);
#endif
      liste->setColumnWidth(3, n * 2 + b + sortIndW);
      liste->setColumnWidth(4, c + b + sortIndW);
      liste->setColumnWidth(5, c + b + sortIndW);
      liste->setColumnWidth(6, c + b + sortIndW);
      liste->setColumnWidth(7, n * 4 + b + sortIndW);
// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
      liste->setColumnWidth(8, fm.horizontalAdvance(QString("MainVolume")) + 70);
#else
      liste->setColumnWidth(8, fm.width(QString("MainVolume")) + 70);
#endif

      connect(liste, SIGNAL(itemSelectionChanged()), SLOT(selectionChanged()));
      connect(liste, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), SLOT(doubleClicked(QTreeWidgetItem*)));
      //---------------------------------------------------
      //    Rest
      //---------------------------------------------------

      mainGrid->setRowStretch(1, 100);
      mainGrid->setColumnStretch(0, 100);
      mainGrid->addWidget(liste, 1, 0, 2, 1);
      connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), SLOT(songChanged(MusECore::SongChangedStruct_t)));

      if(pl->empty())
      {
        curPart = 0;
        curPartId = -1;
      }
      else
      {
        curPart   = (MusECore::MidiPart*)pl->begin()->second;
        if(curPart)
          curPartId = curPart->sn();
        else
        {
          curPart = 0;
          curPartId = -1;
        }
      }

      songChanged(-1);
      
      initShortcuts();
      
      setWindowTitle(tr("MusE: List Editor"));
      
      finalizeInit();
      }

//---------------------------------------------------------
//   ~ListEdit
//---------------------------------------------------------

ListEdit::~ListEdit()
      {
      }

//---------------------------------------------------------
//   editInsertNote
//---------------------------------------------------------

void ListEdit::editInsertNote()
      {
      if(!curPart)
        return;
        
      MusECore::Event event = EditNoteDialog::getEvent(curPart->tick(), MusECore::Event(), this);
      if (!event.empty()) {
            //No events before beginning of part + take Part offset into consideration
            unsigned tick = event.tick();
            if (tick < curPart->tick())
                  tick = 0;
            else
                  tick-= curPart->tick();
            event.setTick(tick);
            // Indicate do undo, and do not handle port controller values. 
            MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::AddEvent, 
                              event, curPart, false, false));
            }
      }

//---------------------------------------------------------
//   editInsertSysEx
//---------------------------------------------------------

void ListEdit::editInsertSysEx()
      {
      if(!curPart)
        return;
      
      MusECore::MidiInstrument* minstr = NULL;
      if(curPart->track())
        minstr = MusEGlobal::midiPorts[curPart->track()->outPort()].instrument();
      MusECore::Event event = EditSysexDialog::getEvent(curPart->tick(), MusECore::Event(), this, minstr);
      if (!event.empty()) {
            //No events before beginning of part + take Part offset into consideration
            unsigned tick = event.tick();
            if (tick < curPart->tick())
                  tick = 0;
            else
                  tick-= curPart->tick();
            event.setTick(tick);
            // Indicate do undo, and do not handle port controller values. 
            MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::AddEvent, 
                              event, curPart, false, false));
            }
      }

//---------------------------------------------------------
//   editInsertCtrl
//---------------------------------------------------------

void ListEdit::editInsertCtrl()
      {
      if(!curPart)
        return;
      
      MusECore::Event event = EditCtrlDialog::getEvent(curPart->tick(), MusECore::Event(), curPart, this);
      if (!event.empty()) {
            //No events before beginning of part + take Part offset into consideration
            unsigned tick = event.tick();
            if (tick < curPart->tick())
                  tick = 0;
            else
                  tick-= curPart->tick();
            event.setTick(tick);
            // Indicate do undo, and do port controller values and clone parts. 
            MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::AddEvent, 
                              event, curPart, true, true));
            }
      }

//---------------------------------------------------------
//   editInsertMeta
//---------------------------------------------------------

void ListEdit::editInsertMeta()
      {
      if(!curPart)
        return;
      
      MusECore::Event event = EditMetaDialog::getEvent(curPart->tick(), MusECore::Event(), this);
      if (!event.empty()) {
            //No events before beginning of part + take Part offset into consideration
            unsigned tick = event.tick();
            if (tick < curPart->tick())
                  tick = 0;
            else
                  tick-= curPart->tick();
            event.setTick(tick);
            // Indicate do undo, and do not handle port controller values. 
            MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::AddEvent, 
                              event, curPart, false, false));
            }
      }

//---------------------------------------------------------
//   editEvent
//---------------------------------------------------------

void ListEdit::editEvent(MusECore::Event& event, MusECore::MidiPart* part)
      {
      int tick = event.tick() + part->tick();
      MusECore::Event nevent;
      switch(event.type()) {
            case MusECore::Note:
                  nevent = EditNoteDialog::getEvent(tick, event, this);
                  break;
            case MusECore::Controller:
                  nevent = EditCtrlDialog::getEvent(tick, event, part, this);
                  break;
            case MusECore::Sysex:
                  {
                    MusECore::MidiInstrument* minstr = NULL;
                    if(part->track())
                      minstr = MusEGlobal::midiPorts[part->track()->outPort()].instrument();
                    nevent = EditSysexDialog::getEvent(tick, event, this, minstr);
                  }
                  break;
            case MusECore::Meta:
                  nevent = EditMetaDialog::getEvent(tick, event, this);
                  break;
            default:
                  return;
            }
      if (!nevent.empty()) {
            // TODO: check for event != nevent
            int tick = nevent.tick() - part->tick();
            nevent.setTick(tick);
            if (tick < 0)
                  printf("event not in part %d - %d - %d, not changed\n", part->tick(),
                     nevent.tick(), part->tick() + part->lenTick());
            else
            {
              if(event.type() == MusECore::Controller)
                // Indicate do undo, and do port controller values and clone parts. 
                MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::ModifyEvent,
                                  nevent, event, part, true, true));
              else  
                // Indicate do undo, and do not do port controller values and clone parts. 
                MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::ModifyEvent,
                                  nevent, event, part, false, false));
            }      
          }
      }

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void ListEdit::readStatus(MusECore::Xml& xml)
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
                              xml.unknown("ListEdit");
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "listeditor")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void ListEdit::writeStatus(int level, MusECore::Xml& xml) const
      {
      writePartList(level, xml);
      xml.tag(level++, "listeditor");
      MidiEditor::writeStatus(level, xml);
      xml.tag(level, "/listeditor");
      }

//---------------------------------------------------------
//   readConfiguration
//---------------------------------------------------------

void ListEdit::readConfiguration(MusECore::Xml& xml)
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
                              TopWin::readConfiguration(LISTE, xml);
                        else
                              xml.unknown("ListEdit");
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "listedit")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   writeConfiguration
//---------------------------------------------------------

void ListEdit::writeConfiguration(int level, MusECore::Xml& xml)
      {
      xml.tag(level++, "listedit");
      TopWin::writeConfiguration(LISTE, level, xml);
      xml.tag(level, "/listedit");
      }

//---------------------------------------------------------
//   selectionChanged
//---------------------------------------------------------

void ListEdit::selectionChanged()
      {
      bool update = false;
      EventListItem* eli;
      for (int row = 0; row < liste->topLevelItemCount(); ++row) {
            QTreeWidgetItem* i = liste->topLevelItem(row);
            eli = (EventListItem*)i;
            if (i->isSelected() ^ eli->event.selected()) {
                  MusEGlobal::song->selectEvent(eli->event, eli->part, i->isSelected());
                  update = true;
                  }
            }
      if (update)
            MusEGlobal::song->update(SC_SELECTION);
      }

//---------------------------------------------------------
//   doubleClicked
//---------------------------------------------------------

void ListEdit::doubleClicked(QTreeWidgetItem* item)
      {
      EventListItem* ev = (EventListItem*) item;
      selectedTick = ev->event.tick();
      editEvent(ev->event, ev->part);
      }

//---------------------------------------------------------
//   cmd
//---------------------------------------------------------

void ListEdit::cmd(int cmd)
{
    bool found = false;
    for (int row = 0; row < liste->topLevelItemCount(); ++row)
    {
      QTreeWidgetItem* i = liste->topLevelItem(row);
      EventListItem *item = (EventListItem *) i;
      if (i->isSelected() || item->event.selected())
      {
            found = true;
            break;
      }
    }

  switch(cmd) {
        case CMD_DELETE:
              {
              if(!found)
                break;

              MusECore::Undo operations;

              EventListItem *deletedEvent=NULL;
              for (int row = 0; row < liste->topLevelItemCount(); ++row) {
                    QTreeWidgetItem* i = liste->topLevelItem(row);
                    EventListItem *item = (EventListItem *) i;
                    if (i->isSelected() || item->event.selected()) {
                          deletedEvent=item;
                          // Port controller values and clone parts.
                          operations.push_back(MusECore::UndoOp(MusECore::UndoOp::DeleteEvent,item->event, item->part, true, true));
                    }
              }

              unsigned int nextTick=0;
              // find biggest tick
              for (int row = 0; row < liste->topLevelItemCount(); ++row) {
                    QTreeWidgetItem* i = liste->topLevelItem(row);
                    EventListItem *item = (EventListItem *) i;
                    if (item->event.tick() > nextTick && item != deletedEvent)
                        nextTick=item->event.tick();
              }
              // check if there's a tick that is "just" bigger than the deleted
              for (int row = 0; row < liste->topLevelItemCount(); ++row) {
                    QTreeWidgetItem* i = liste->topLevelItem(row);
                    EventListItem *item = (EventListItem *) i;
                    if (item->event.tick() >= deletedEvent->event.tick() &&
                        item->event.tick() < nextTick &&
                        item != deletedEvent ) {
                        nextTick=item->event.tick();
                    }
              }
              selectedTick=nextTick;

              MusEGlobal::song->applyOperationGroup(operations);
              }
              break;
        case CMD_INC:
        case CMD_DEC:
              {
              if(!found)
                break;

              MusECore::Undo operations;

              for (int row = 0; row < liste->topLevelItemCount(); ++row) {
                    QTreeWidgetItem* i = liste->topLevelItem(row);
                    EventListItem *item = (EventListItem *) i;

                    if (i->isSelected() || item->event.selected()) {
                          MusECore::Event new_event=item->event.clone();
                          new_event.setTick( new_event.tick() + ( cmd==CMD_INC?1:-1) );
                          operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyEvent,new_event, item->event, item->part, false, false));
                          selectedTick=new_event.tick();
                          break;
                          }
                    }

              MusEGlobal::song->applyOperationGroup(operations);

              }
  }
}

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void ListEdit::configChanged()
      {
      initShortcuts();
      }

//---------------------------------------------------------
//   initShortcuts
//---------------------------------------------------------

void ListEdit::initShortcuts()
      {
      insertNote->setShortcut(shortcuts[SHRT_LE_INS_NOTES].key);
      insertSysEx->setShortcut(shortcuts[SHRT_LE_INS_SYSEX].key);
      insertCtrl->setShortcut(shortcuts[SHRT_LE_INS_CTRL].key);
      insertMeta->setShortcut(shortcuts[SHRT_LE_INS_META].key);
      }

//---------------------------------------------------------
//   viewKeyPressEvent
//---------------------------------------------------------

void ListEdit::keyPressEvent(QKeyEvent* event)
      {
      int key = event->key();
      if (key == Qt::Key_Escape) {
            close();
            return;
            }
      }

//---------------------------------------------------------
//   focusCanvas
//---------------------------------------------------------

void ListEdit::focusCanvas()
{
  if(MusEGlobal::config.smartFocus)
  {
    liste->setFocus();
    liste->activateWindow();
  }
}

} // namespace MusEGui
