//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: listedit.cpp,v 1.11.2.11 2009/05/24 21:43:44 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include <q3toolbar.h>
#include <qtooltip.h>
#include <qtoolbutton.h>
#include <qlayout.h>
#include <q3hbox.h>
#include <qscrollbar.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <q3buttongroup.h>
#include <q3listbox.h>
#include <q3listview.h>
#include <q3header.h>
#include <q3popupmenu.h>
#include <qmenubar.h>
#include <qaction.h>
#include <q3accel.h>
#include <Qt3Support>
//Added by qt3to4:
#include <QKeyEvent>
#include <Q3ActionGroup>
#include <QCloseEvent>

#include "listedit.h"
#include "mtscale.h"
#include "globals.h"
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

//---------------------------------------------------------
//   EventListItem
//---------------------------------------------------------

class EventListItem : public Q3ListViewItem {
   public:
      Event event;
      MidiPart* part;

      EventListItem(Q3ListView* parent, Event ev, MidiPart* p)
         : Q3ListViewItem(parent) {
            event = ev;
            part  = p;
            }
      virtual QString text(int col) const;
      virtual int compare(Q3ListViewItem* i, int col, bool ascend) const 
      {
          EventListItem* eli = (EventListItem*)i;
          switch(col)
          {
            case 0:
                  return event.tick() - eli->event.tick();
                  break;
            case 1:
                  return part->tick() + event.tick() - (eli->part->tick() + eli->event.tick());
                  break;
            case 2:
                  return key(col, ascend).localeAwareCompare(i->key(col, ascend));
                  break;   
            case 3:
                  return part->track()->outChannel() - eli->part->track()->outChannel();
                  break;
            case 4:
                  return event.dataA() - eli->event.dataA();
                  break;
            case 5:
                  return event.dataB() - eli->event.dataB();
                  break;
            case 6:
                  return event.dataC() - eli->event.dataC();
                  break;
            case 7:
                  return event.lenTick() - eli->event.lenTick();
                  break;
            case 8:
                  return key(col, ascend).localeAwareCompare(i->key(col, ascend));
                  break;
            default:
                  return 0;
            }
          }
      };
/*---------------------------------------------------------
 *    midi_meta_name
 *---------------------------------------------------------*/

static QString midiMetaComment(const Event& ev)
      {
      int meta  = ev.dataA();
      QString s = midiMetaName(meta);

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
      emit deleted((unsigned long)this);
      e->accept();
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void ListEdit::songChanged(int type)
      {
      if (type == 0)
            return;
      if (type & (SC_PART_REMOVED | SC_PART_MODIFIED
         | SC_PART_INSERTED | SC_EVENT_REMOVED | SC_EVENT_MODIFIED
         | SC_EVENT_INSERTED | SC_SELECTION)) {
            if (type & (SC_PART_REMOVED | SC_PART_INSERTED))
                  genPartlist();
            // close window if editor has no parts anymore
            if (parts()->empty()) {
                  close(false);
                  return;
                  }
            if (type == SC_SELECTION) {
                  bool update = false;
                  Q3ListViewItem* ci = 0;
                  for (Q3ListViewItem* i = liste->firstChild(); i; i = i->nextSibling()) {
                        if (i->isSelected() ^ ((EventListItem*)i)->event.selected()) {
                                    i->setSelected(((EventListItem*)i)->event.selected());
                              if (i->isSelected())
                                    ci = i;
                              update = true;
                              }
                        }
                  if (update) {
                        if (ci)
                              liste->setCurrentItem(ci);
                        liste->triggerUpdate();
                        }
                  }
            else {
                  curPart = 0;
                  curTrack = 0;
                  liste->clear();
                  for (iPart p = parts()->begin(); p != parts()->end(); ++p) {
                        MidiPart* part = (MidiPart*) (p->second);
                        if (part->sn() == curPartId)
                              curPart  = part;
                        EventList* el = part->events();
                        for (iEvent i = el->begin(); i != el->end(); ++i) {
                              EventListItem* item = new EventListItem(liste, i->second, part);
                              item->setSelected(i->second.selected());
                              if (item->event.tick() == (unsigned) selectedTick) { //prevent compiler warning: comparison of signed/unsigned)
                                    liste->setCurrentItem(item);
                                    item->setSelected(true);
                                    liste->ensureItemVisible(item);
                                    }
                              }
                        }
                  }
            
            // p3.3.34
            //if (curPart == 0)
            //      curPart  = (MidiPart*)(parts()->begin()->second);
            //curTrack = curPart->track();
            if(!curPart)
            {
              if(!parts()->empty())
              {
                curPart  = (MidiPart*)(parts()->begin()->second);
                if(curPart)
                  curTrack = curPart->track();
                else  
                  curPart = 0;
              }      
            }
          }  
      }

//---------------------------------------------------------
//   text
//---------------------------------------------------------

QString EventListItem::text(int col) const
      {
      QString s;
      QString commentLabel;
      switch(col) {
            case 0:
                  s.setNum(event.tick());
                  break;
            case 1:
                  {
                  int t = event.tick() + part->tick();
                  int bar, beat;
                  unsigned tick;
                  sigmap.tickValues(t, &bar, &beat, &tick);
                  s.sprintf("%04d.%02d.%03d", bar+1, beat+1, tick);
                  }
                  break;
            case 2:
                  switch(event.type()) {
                        case Note:
                              s = QString("Note");
                              break;
                        case Controller:
                              {
                              const char* cs;
                              switch (midiControllerType(event.dataA())) {
                                    case MidiController::Controller7:  cs = "Ctrl7"; break;
                                    case MidiController::Controller14: cs = "Ctrl14"; break;
                                    case MidiController::RPN:          cs = "RPN"; break;
                                    case MidiController::NRPN:         cs = "NRPN"; break;
                                    case MidiController::Pitch:        cs = "Pitch"; break;
                                    case MidiController::Program:      cs = "Program"; break;
                                    case MidiController::RPN14:        cs = "RPN14"; break;
                                    case MidiController::NRPN14:       cs = "NRPN14"; break;
                                    default:           cs = "Ctrl?"; break;
                                    }
                              s = QString(cs);
                              }
                              break;
                        case Sysex:
                              {
                              commentLabel = QString("len ");
                              QString k;
                              k.setNum(event.dataLen());
                              commentLabel += k;
                              commentLabel += QString(" ");

                              commentLabel += nameSysex(event.dataLen(), event.data());
                              int i;
                              for (i = 0; i < 10; ++i) {
                                    if (i >= event.dataLen())
                                          break;
                                    commentLabel += QString(" 0x");
                                    QString k;
                                    k.setNum(event.data()[i] & 0xff, 16);
                                    commentLabel += k;
                                    }
                              if (i == 10)
                                    commentLabel += QString("...");
                              }
                              s = QString("SysEx");
                              break;
                        case PAfter:
                              s = QString("PoAT");
                              break;
                        case CAfter:
                              s = QString("ChAT");
                              break;
                        case Meta:
                              commentLabel = midiMetaComment(event);
                              s = QString("Meta");
                              break;
                        case Wave:
                              break;
                        default:
                              printf("unknown event type %d\n", event.type());
                        }
                  break;
            case 3:
                  s.setNum(part->track()->outChannel() + 1);
                  break;
            case 4:
                  if (event.isNote() || event.type() == PAfter)
                        s = pitch2string(event.dataA());
                  else if (event.type() == Controller)
                        s.setNum(event.dataA() & 0xffff);  // mask off type bits
                  else
                        s.setNum(event.dataA());
                  break;
            case 5:
                  if(event.type() == Controller &&
                     midiControllerType(event.dataA()) == MidiController::Program) 
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
                    s.sprintf("%d-%d-%d", hb, lb, pr);
                  }
                  else        
                    s.setNum(event.dataB());
                  break;
            case 6:
                  s.setNum(event.dataC());
                  break;
            case 7:
                  s.setNum(event.lenTick());
                  break;
            case 8:
                  switch(event.type()) {
                        case Controller:
                              {
                              MidiPort* mp = &midiPorts[part->track()->outPort()];
                              MidiController* mc = mp->midiController(event.dataA());
                              s = mc->name();
                              }
                              break;
                        case Sysex:
                              {
                              s = QString("len ");
                              QString k;
                              k.setNum(event.dataLen());
                              s += k;
                              s += QString(" ");

                              commentLabel += nameSysex(event.dataLen(), event.data());
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
                        case Meta:
                              s = midiMetaComment(event);
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

ListEdit::ListEdit(PartList* pl)
   : MidiEditor(0, 0, pl)
      {
      insertItems = new Q3ActionGroup(this, "InsertItems", false);
      insertNote = new Q3Action(tr("Insert Note"), QIcon(*note1Icon), tr("insert Note"),
        0, insertItems);
      insertSysEx = new Q3Action(tr("Insert SysEx"), QIcon(*sysexIcon), tr("insert SysEx"),
        0, insertItems);
      insertCtrl = new Q3Action(tr("Insert Ctrl"), QIcon(*ctrlIcon), tr("insert Ctrl"),
        0, insertItems);
      insertMeta = new Q3Action(tr("Insert Meta"), QIcon(*metaIcon), tr("insert Meta"),
        0, insertItems);
      insertCAfter = new Q3Action(tr("Insert Channel Aftertouch"), QIcon(*cafterIcon), tr("insert Channel Aftertouch"),
        0, insertItems);
      insertPAfter = new Q3Action(tr("Insert Key Aftertouch"), QIcon(*pafterIcon), tr("insert Poly Aftertouch"),
        0, insertItems);

      connect(insertNote,    SIGNAL(activated()), SLOT(editInsertNote()));
      connect(insertSysEx,   SIGNAL(activated()), SLOT(editInsertSysEx()));
      connect(insertCtrl,    SIGNAL(activated()), SLOT(editInsertCtrl()));
      connect(insertMeta,    SIGNAL(activated()), SLOT(editInsertMeta()));
      connect(insertCAfter,  SIGNAL(activated()), SLOT(editInsertCAfter()));
      connect(insertPAfter,  SIGNAL(activated()), SLOT(editInsertPAfter()));

      //---------Pulldown Menu----------------------------
      menuEdit = new Q3PopupMenu(this);
      menuBar()->insertItem(tr("&Edit"), menuEdit);
      undoRedo->addTo(menuEdit);

      Q3Accel* qa = new Q3Accel(this);
      qa->connectItem(qa->insertItem(Qt::CTRL+Qt::Key_Z), song, SLOT(undo()));
      qa->connectItem(qa->insertItem(Qt::CTRL+Qt::Key_Y), song, SLOT(redo()));

      menuEdit->insertSeparator();
#if 0
      menuEdit->insertItem(tr("Cut"),   EList::CMD_CUT);
      menuEdit->setAccel(Qt::CTRL+Qt::Key_X, EList::CMD_CUT);
      menuEdit->insertItem(tr("Copy"),  EList::CMD_COPY);
      menuEdit->setAccel(Qt::CTRL+Qt::Key_C, EList::CMD_COPY);
      menuEdit->insertItem(tr("Paste"), EList::CMD_PASTE);
      menuEdit->setAccel(Qt::CTRL+Qt::Key_V, EList::CMD_PASTE);
      menuEdit->insertSeparator();
#endif
      menuEdit->insertItem(tr("Delete Events"), CMD_DELETE);
      menuEdit->setAccel(Qt::Key_Delete, CMD_DELETE);
      menuEdit->insertSeparator();

      insertItems->addTo(menuEdit);

      connect(menuEdit, SIGNAL(activated(int)), SLOT(cmd(int)));

      //---------ToolBar----------------------------------
      listTools = new Q3ToolBar(this, "list-tools");
      listTools->setLabel(tr("List Tools"));
      undoRedo->addTo(listTools);
      Q3ToolBar* insertTools = new Q3ToolBar(this, "insert");
      insertTools->setLabel(tr("Insert Tools"));
      insertItems->addTo(insertTools);

      //
      //---------------------------------------------------
      //    liste
      //---------------------------------------------------
      //

      liste = new Q3ListView(mainw);
      QFontMetrics fm(liste->font());
      int n = fm.width('9');
      int b = 24;
      int c = fm.width(QString("Val B"));
      int sortIndW = n * 3;
      liste->setAllColumnsShowFocus(true);
      liste->setSorting(0);
      liste->setSelectionMode(Q3ListView::Extended);
      liste->setShowSortIndicator(true);
      liste->addColumn(tr("Tick"),  n * 6 + b);
      liste->addColumn(tr("Bar"),   fm.width(QString("9999.99.999")) + b);
      liste->addColumn(tr("Type"),  fm.width(QString("Program")) + b);
      liste->addColumn(tr("Ch"),    n * 2 + b + sortIndW);
      liste->addColumn(tr("Val A"), c + b + sortIndW);
      liste->addColumn(tr("Val B"), c + b + sortIndW);
      liste->addColumn(tr("Val C"), c + b + sortIndW);
      liste->addColumn(tr("Len"),   n * 4 + b + sortIndW);
      liste->addColumn(tr("Comment"), fm.width(QString("MainVolume")) + 70);
      liste->setResizeMode(Q3ListView::LastColumn);
      connect(liste, SIGNAL(selectionChanged()), SLOT(selectionChanged()));
      connect(liste, SIGNAL(doubleClicked(Q3ListViewItem*)), SLOT(doubleClicked(Q3ListViewItem*)));
      //---------------------------------------------------
      //    Rest
      //---------------------------------------------------

      mainGrid->setRowStretch(1, 100);
      mainGrid->setColStretch(0, 100);
      mainGrid->addMultiCellWidget(liste, 1, 2, 0, 0);
      connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      songChanged(-1);

      // p3.3.34
      // Was crashing because of -1 stored, because there was an invalid
      //  part pointer stored. 
      //curPart   = (MidiPart*)(pl->begin()->second);
      if(pl->empty())
      {
        curPart = 0;
        curPartId = -1;
      }
      else
      {
        curPart   = (MidiPart*)pl->begin()->second;
        if(curPart)
          curPartId = curPart->sn();
        else
        {
          curPart = 0;
          curPartId = -1;
        }
      }
      
      initShortcuts();
      }

//---------------------------------------------------------
//   ~ListEdit
//---------------------------------------------------------

ListEdit::~ListEdit()
      {
      undoRedo->removeFrom(listTools);
      }

//---------------------------------------------------------
//   editInsertNote
//---------------------------------------------------------

void ListEdit::editInsertNote()
      {
      // p3.3.34
      if(!curPart)
        return;
        
      Event event = EditNoteDialog::getEvent(curPart->tick(), Event(), this);
      if (!event.empty()) {
            //No events before beginning of part + take Part offset into consideration
            unsigned tick = event.tick();
            if (tick < curPart->tick())
                  tick = 0;
            else
                  tick-= curPart->tick();
            event.setTick(tick);
            // Indicate do undo, and do not handle port controller values. 
            //audio->msgAddEvent(event, curPart);
            audio->msgAddEvent(event, curPart, true, false, false);
            }
      }

//---------------------------------------------------------
//   editInsertSysEx
//---------------------------------------------------------

void ListEdit::editInsertSysEx()
      {
      // p3.3.34
      if(!curPart)
        return;
      
      Event event = EditSysexDialog::getEvent(curPart->tick(), Event(), this);
      if (!event.empty()) {
            //No events before beginning of part + take Part offset into consideration
            unsigned tick = event.tick();
            if (tick < curPart->tick())
                  tick = 0;
            else
                  tick-= curPart->tick();
            event.setTick(tick);
            // Indicate do undo, and do not handle port controller values. 
            //audio->msgAddEvent(event, curPart);
            audio->msgAddEvent(event, curPart, true, false, false);
            }
      }

//---------------------------------------------------------
//   editInsertCtrl
//---------------------------------------------------------

void ListEdit::editInsertCtrl()
      {
      // p3.3.34
      if(!curPart)
        return;
      
      Event event = EditCtrlDialog::getEvent(curPart->tick(), Event(), curPart, this);
      if (!event.empty()) {
            //No events before beginning of part + take Part offset into consideration
            unsigned tick = event.tick();
            if (tick < curPart->tick())
                  tick = 0;
            else
                  tick-= curPart->tick();
            event.setTick(tick);
            // Indicate do undo, and do port controller values and clone parts. 
            //audio->msgAddEvent(event, curPart);
            audio->msgAddEvent(event, curPart, true, true, true);
            }
      }

//---------------------------------------------------------
//   editInsertMeta
//---------------------------------------------------------

void ListEdit::editInsertMeta()
      {
      // p3.3.34
      if(!curPart)
        return;
      
      Event event = EditMetaDialog::getEvent(curPart->tick(), Event(), this);
      if (!event.empty()) {
            //No events before beginning of part + take Part offset into consideration
            unsigned tick = event.tick();
            if (tick < curPart->tick())
                  tick = 0;
            else
                  tick-= curPart->tick();
            event.setTick(tick);
            // Indicate do undo, and do not handle port controller values. 
            //audio->msgAddEvent(event, curPart);
            audio->msgAddEvent(event, curPart, true, false, false);
            }
      }

//---------------------------------------------------------
//   editInsertCAfter
//---------------------------------------------------------

void ListEdit::editInsertCAfter()
      {
      // p3.3.34
      if(!curPart)
        return;
      
      Event event = EditCAfterDialog::getEvent(curPart->tick(), Event(), this);
      if (!event.empty()) {
            //No events before beginning of part + take Part offset into consideration
            unsigned tick = event.tick();
            if (tick < curPart->tick())
                  tick = 0;
            else
                  tick-= curPart->tick();
            event.setTick(tick);
            // Indicate do undo, and do not handle port controller values. 
            //audio->msgAddEvent(event, curPart);
            audio->msgAddEvent(event, curPart, true, false, false);
            }
      }

//---------------------------------------------------------
//   editInsertPAfter
//---------------------------------------------------------

void ListEdit::editInsertPAfter()
      {
      // p3.3.34
      if(!curPart)
        return;
      
      Event ev;
      Event event = EditPAfterDialog::getEvent(curPart->tick(), ev, this);
      if (!event.empty()) {
            //No events before beginning of part + take Part offset into consideration
            unsigned tick = event.tick();
            if (tick < curPart->tick())
                  tick = 0;
            else
                  tick-= curPart->tick();
            event.setTick(tick);
            // Indicate do undo, and do not handle port controller values. 
            //audio->msgAddEvent(event, curPart);
            audio->msgAddEvent(event, curPart, true, false, false);
            }
      }

//---------------------------------------------------------
//   editEvent
//---------------------------------------------------------

void ListEdit::editEvent(Event& event, MidiPart* part)
      {
      int tick = event.tick() + part->tick();
      Event nevent;
      switch(event.type()) {
            case Note:
                  nevent = EditNoteDialog::getEvent(tick, event, this);
                  break;
            case Controller:
                  nevent = EditCtrlDialog::getEvent(tick, event, part, this);
                  break;
            case Sysex:
                  nevent = EditSysexDialog::getEvent(tick, event, this);
                  break;
            case PAfter:
                  nevent = EditPAfterDialog::getEvent(tick, event, this);
                  break;
            case CAfter:
                  nevent = EditCAfterDialog::getEvent(tick, event, this);
                  break;
            case Meta:
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
              if(event.type() == Controller)
                // Indicate do undo, and do port controller values and clone parts. 
                //audio->msgChangeEvent(event, nevent, part);
                audio->msgChangeEvent(event, nevent, part, true, true, true);
              else  
                // Indicate do undo, and do not do port controller values and clone parts. 
                //audio->msgChangeEvent(event, nevent, part);
                audio->msgChangeEvent(event, nevent, part, true, false, false);
            }      
          }
      }

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void ListEdit::readStatus(Xml& xml)
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
                              xml.unknown("ListEdit");
                        break;
                  case Xml::TagEnd:
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

void ListEdit::writeStatus(int level, Xml& xml) const
      {
      writePartList(level, xml);
      xml.tag(level++, "listeditor");
      MidiEditor::writeStatus(level, xml);
      xml.tag(level, "/listeditor");
      }

//---------------------------------------------------------
//   selectionChanged
//---------------------------------------------------------

void ListEdit::selectionChanged()
      {
      bool update = false;
      for (Q3ListViewItem* i = liste->firstChild(); i; i = i->nextSibling()) {
            if (i->isSelected() ^ ((EventListItem*)i)->event.selected()) {
                  ((EventListItem*)i)->event.setSelected(i->isSelected());
                  update = true;
                  }
            }
      if (update)
            song->update(SC_SELECTION);
      }

//---------------------------------------------------------
//   doubleClicked
//---------------------------------------------------------

void ListEdit::doubleClicked(Q3ListViewItem* item)
      {
      EventListItem* ev = (EventListItem*) item;
      editEvent(ev->event, ev->part);
      }

//---------------------------------------------------------
//   cmd
//---------------------------------------------------------

void ListEdit::cmd(int cmd)
      {
      switch(cmd) {
            case CMD_DELETE:
                  bool found = false;
                  for (Q3ListViewItem* i = liste->firstChild(); i; i = i->nextSibling()) 
                  {
                    EventListItem *item = (EventListItem *) i;
                    if (i->isSelected() || item->event.selected()) 
                    {
                          found = true;
                          break;
                    }
                  }
                  if(!found)
                    break;
                  song->startUndo();
                  
                  EventListItem *deletedEvent=NULL;
                  for (Q3ListViewItem* i = liste->firstChild(); i; i = i->nextSibling()) {
                        EventListItem *item = (EventListItem *) i;

                        if (i->isSelected() || item->event.selected()) {
                              deletedEvent=item;
                              // Indicate no undo, and do port controller values and clone parts. 
                              //audio->msgDeleteEvent(item->event, item->part, false);
                              audio->msgDeleteEvent(item->event, item->part, false, true, true);
                              }
                        }
                  
                  unsigned int nextTick=0;
                  // find biggest tick
                  for (Q3ListViewItem* i = liste->firstChild(); i; i = i->nextSibling()) {
                        EventListItem *item = (EventListItem *) i;
                        if (item->event.tick() > nextTick && item != deletedEvent)
                            nextTick=item->event.tick();
                  }
                  // check if there's a tick that is "just" bigger than the deleted
                  for (Q3ListViewItem* i = liste->firstChild(); i; i = i->nextSibling()) {
                        EventListItem *item = (EventListItem *) i;
                        if (item->event.tick() >= deletedEvent->event.tick() && 
                            item->event.tick() < nextTick &&
                            item != deletedEvent ) {
                            nextTick=item->event.tick();
                        }
                  }
                  selectedTick=nextTick;
                  song->endUndo(SC_EVENT_MODIFIED);
                  //printf("selected tick = %d\n", selectedTick);
                  //emit selectionChanged();
                  break;
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
      insertNote->setAccel(shortcuts[SHRT_LE_INS_NOTES].key);
      insertSysEx->setAccel(shortcuts[SHRT_LE_INS_SYSEX].key);
      insertCtrl->setAccel(shortcuts[SHRT_LE_INS_CTRL].key);
      insertMeta->setAccel(shortcuts[SHRT_LE_INS_META].key);
      insertCAfter->setAccel(shortcuts[SHRT_LE_INS_CHAN_AFTERTOUCH].key);
      insertPAfter->setAccel(shortcuts[SHRT_LE_INS_POLY_AFTERTOUCH].key);
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
