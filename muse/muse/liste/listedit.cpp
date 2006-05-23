//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: listedit.cpp,v 1.43 2005/11/05 23:15:23 wschweer Exp $
//  (C) Copyright 1999-2005 Werner Schweer (ws@seh.de)
//=========================================================

#if 0 //TD
#include "listedit.h"
#include "globals.h"
#include "icons.h"
#include "song.h"
#include "audio.h"
#include "shortcuts.h"
#include "midi.h"
#include "event.h"
#include "midictrl.h"
#include "esettings.h"

#include "al/xml.h"
#include "al/tempo.h"
#include "al/sig.h"
#include "awl/utils.h"
#include "awl/sigedit.h"
#include "awl/posedit.h"
#include "awl/pitchedit.h"

#include "instruments/minstrument.h"

enum { L_TIMESIG=1001, L_TEMPOSIG, L_NOTES, L_SYSEX, L_PAFTER,
       L_CAFTER, L_META, L_CTRL, L_WAVE, L_PART, L_TRACK
      };

//---------------------------------------------------------
//   ListCtrl
//---------------------------------------------------------

struct ListCtrl {
      int tick;
      Track* track;
      int id;
      CVal val;
      ListCtrl(int t, Track* tr, int d, CVal v) {
            tick  = t;
            track = tr;
            id    = d;
            val   = v;
            }
      ListCtrl() {
            tick  = 0;
            track = 0;
            id    = 0;
            val.i = 0;
            }
      };

//---------------------------------------------------------
//    midi_meta_name
//---------------------------------------------------------

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

            case 1 ... 15:
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
//   paintCell
//---------------------------------------------------------

void LItem::paintCell(QPainter* p, const QColorGroup& cg,
   int column, int width, int align)
      {
      if (!p)
            return;
      Q3ListView *lv = listView();
      if (!lv )
            return;

      if (isSelected() && (column == 0 || lv->allColumnsShowFocus())) {
            p->fillRect(0, 0, width, height(), cg.brush(QColorGroup::Highlight));
            p->setPen(cg.highlightedText());
            }
      else
            p->fillRect(0, 0, width, height(), QColor(200, 200, zebra ? 200 : 230));

      QString t = text(column);
      if (!t.isEmpty()) {
            if (!(align & Qt::AlignVCenter || align & Qt::AlignBottom))
                  align |= Qt::AlignTop;
            int marg = lv->itemMargin();
            p->drawText(marg, marg, width-marg-marg, height(), align, t);
            }
      }

//---------------------------------------------------------
//   text
//---------------------------------------------------------

QString LItem::text(int col) const
      {
      QString s;
      bool isMidi = lli()->track()->isMidiTrack();

      if (col == 0) {
            s.sprintf("%04d", _pos.tick());
            return s;
            }
      if (col == 1) {
            int bar, beat;
            unsigned tick;
            AL::sigmap.tickValues(_pos.tick(), &bar, &beat, &tick);
            s.sprintf("%04d.%02d.%03d", bar+1, beat+1, tick);
            return s;
            }
      switch(_rtti) {
            case L_TIMESIG:
                  if (col == 2)
                        s = QString("Time");
                  else if (col == 3) {
                        s.sprintf("%d/%d", _sig->z, _sig->n);
                        }
                  break;
            case L_TEMPOSIG:
                  if (col == 2)
                        s = QString("Tempo");
                  else if (col == 3) {
                        double t = (1000000.0 * 60.0)/_tempo->tempo;
                        s.setNum(t, 'f', 3);
                        }
                  break;
            case L_NOTES:
                  if (col == 2)
                        s = "Note";
                  else if (col == 3)
                        s = Awl::pitch2string(_event.dataA());
                  else if (col == 4)
                        s.setNum(_event.dataB(), *showHex ? 16 : 10);
                  else if (col == 5)
                        s.setNum(_event.lenTick());
                  break;

            case L_WAVE:
                  if (col == 2)
                        s = "Wave";
                  else if (col == 5)
                        s.setNum(_event.lenTick());
                  break;

            case L_SYSEX:
                  if (col == 2)
                        s = "SysEx";
                  else if (col == 6) {
                        s = QString("len ");
                        QString k;
                        k.setNum(_event.dataLen());
                        s += k;
                        s += QString(" ");

                        s += nameSysex(_event.dataLen(), _event.data());
                        int i;
                        for (i = 0; i < 10; ++i) {
                              if (i >= _event.dataLen())
                                    break;
                              s += QString(" 0x");
                              QString k;
                              k.setNum(_event.data()[i] & 0xff, 16);
                              s += k;
                              }
                        if (i == 10)
                              s += QString("...");
                        }
                  break;
            case L_PAFTER:
                  if (col == 2)
                        s = "PAfter";
                  else if (col == 3)
                        s.setNum(_event.dataA(), *showHex ? 16 : 10);
                  else if (col == 4)
                        s.setNum(_event.dataB(), *showHex ? 16 : 10);
                  break;
            case L_CAFTER:
                  if (col == 2)
                        s = "CAfter";
                  else if (col == 3)
                        s.setNum(_event.dataA(), *showHex ? 16 : 10);
                  break;
            case L_META:
                  if (col == 2)
                        s = "Meta";
                  else if (col == 3)
                        s.setNum(_event.dataA(), *showHex ? 16 : 10);
                  else if (col == 6)
                        s = midiMetaComment(_event);
                  break;

            case L_CTRL:
                  if (col == 2)
                        s = QString("Ctrl");
                  else if (col == 3) {
                        if (isMidi && lli()->ctrl() == CTRL_PROGRAM) {
                              s = "Prog";
                              }
                        else
                              s.setNum(lli()->ctrl());
                        }
                  else if (col == 4) {
                        if (isMidi) {
                              if (lli()->ctrl() == CTRL_PROGRAM) {
                                    int val = _val.i;
                                    int hb = (val >> 16) & 0xff;
                                    int lb = (val >>  8) & 0xff;
                                    int pr = (val) & 0xff;
                                    char buffer[16];
                                    if (*showHex) {
                                          if (hb == 0xff)
                                                s += "--:";
                                          else {
                                                sprintf(buffer, "%02x:", hb);
                                                s += buffer;
                                                }
                                          if (lb == 0xff)
                                                s += "--:";
                                          else {
                                                sprintf(buffer, "%02x:", lb);
                                                s += buffer;
                                                }
                                          if (pr == 0xff)
                                                s += "--";
                                          else {
                                                sprintf(buffer, "%02x", pr);
                                                s += buffer;
                                                }
                                          }
                                    else {
                                          if (hb == 0xff)
                                                s += "---:";
                                          else {
                                                sprintf(buffer, "%03d:", hb);
                                                s += buffer;
                                                }
                                          if (lb == 0xff)
                                                s += "---:";
                                          else {
                                                sprintf(buffer, "%03d:", lb);
                                                s += buffer;
                                                }
                                          if (pr == 0xff)
                                                s += "---";
                                          else {
                                                sprintf(buffer, "%03d", pr);
                                                s += buffer;
                                                }
                                          }
                                    }
                              else
                                    s.setNum(_val.i, *showHex ? 16 : 10);
                              }
                        else
                              s.setNum(_val.f, 'f', 2);
                        }
                  else if (col == 6)
                        s = lli()->track()->getController(lli()->ctrl())->name();
                  break;
            }
      return s;
      }

//---------------------------------------------------------
//   updateList
//---------------------------------------------------------

void ListEdit::updateList()
      {
      list->clear();
      menuEdit->setItemEnabled(CMD_DELETE, false);
      menuEdit->setItemEnabled(CMD_INSERT_SIG, false);
      menuEdit->setItemEnabled(CMD_INSERT_TEMPO, false);

      Q3ListViewItemIterator i(lists);
      for (; i.current(); ++i) {
            int id = i.current()->rtti();
            if (id < 1000)
                  continue;
            LLItem* item = (LLItem*)(i.current());
            if (!item->isOn())
                  continue;
            switch (id) {
                  case L_TIMESIG:
                        menuEdit->setItemEnabled(CMD_INSERT_SIG, true);
                        for (AL::iSigEvent i = AL::sigmap.begin(); i != AL::sigmap.end(); ++i) {
                              LItem* item = new LItem(list, id, &showHex);
                              item->setPos(Pos(i->second->tick));
                              item->setSigEvent(i->second);
                              }
                        break;
                  case L_TEMPOSIG:
                        menuEdit->setItemEnabled(CMD_INSERT_TEMPO, true);
                        for (AL::iTEvent i = AL::tempomap.begin(); i != AL::tempomap.end(); ++i) {
                              LItem* item = new LItem(list, id, &showHex);
                              item->setPos(Pos(i->second->tick));
                              item->setTempo(i->second);
                              }
                        break;
                  case L_NOTES:
                  case L_SYSEX:
                  case L_PAFTER:
                  case L_CAFTER:
                  case L_META:
                        {
                        menuEdit->setItemEnabled(CMD_INSERT_NOTE, true);
                        menuEdit->setItemEnabled(CMD_INSERT_SYSEX, true);
                        menuEdit->setItemEnabled(CMD_INSERT_PAFTER, true);
                        menuEdit->setItemEnabled(CMD_INSERT_CAFTER, true);
                        menuEdit->setItemEnabled(CMD_INSERT_META, true);
                        MidiTrack* track = (MidiTrack*)(item->track());
                        PartList* pl = track->parts();
                        for (iPart ip = pl->begin(); ip !=pl->end(); ++ip) {
                              Part* part = ip->second;
                              EventList* el = part->events();
                              for (iEvent i = el->begin(); i != el->end(); ++i) {
                                    EventType t = i->second.type();
                                    if ((id == L_NOTES && t == Note)
                                       || (id == L_SYSEX && t == Sysex)
                                       || (id == L_PAFTER && t == PAfter)
                                       || (id == L_CAFTER && t == CAfter)
                                       || (id == L_META && t == Meta)) {
                                          LItem* litem = new LItem(list, id, &showHex);
                                          litem->setEvent(i->second);
                                          litem->setPos(Pos(i->first + ip->first));
                                          litem->setPart(part);
                                          litem->setLLi(item);
                                          }
                                    }
                              }
                        }
                        break;

                  case L_WAVE:
                        {
                        WaveTrack* track = (WaveTrack*)(item->track());
                        PartList* pl = track->parts();
                        for (iPart ip = pl->begin(); ip !=pl->end(); ++ip) {
                              Part* part = ip->second;
                              EventList* el = part->events();
                              for (iEvent i = el->begin(); i != el->end(); ++i) {
                                    EventType t = i->second.type();
                                    if (t != Wave) {
                                          printf("ListEdit::updateList(): wrong part type in wave track\n");
                                          continue;
                                          }
                                    LItem* litem = new LItem(list, id, &showHex);
                                    litem->setEvent(i->second);
                                    litem->setPos(Pos(i->first + ip->first));
                                    litem->setLLi(item);
                                    }
                              }
                        }
                        break;

                  case L_CTRL:
                        {
                        menuEdit->setItemEnabled(CMD_INSERT_CTRL, true);
                        Track* track = item->track();
                        CtrlList* cl = track->controller();
                        for (iCtrl ic = cl->begin(); ic != cl->end(); ++ic) {
                              Ctrl* c = ic->second;
                              if (c->id() != item->ctrl())
                                    continue;
                              for (iCtrlVal iv = c->begin(); iv != c->end(); ++iv) {
                                    LItem* litem = new LItem(list, id, &showHex);
                                    litem->setVal(iv->second);
                                    litem->setPos(Pos(iv->first));
                                    litem->setLLi(item);
                                    }
                              }
                        }
                        break;
                  }
            }
      Q3ListViewItemIterator ii(list);
      bool f = false;
      for (; ii.current(); ++ii) {
            ((LItem*)(ii.current()))->setZebra(f);
            f = !f;
            }
      }

//---------------------------------------------------------
//   updateLists
//---------------------------------------------------------

void ListEdit::updateLists()
      {
      lists->clear();

      Q3ListViewItem* item = new Q3ListViewItem(lists, tr("Master"));
      LLItem* lit = new LLItem(item, "Tempo Signature", L_TEMPOSIG);
      connect(lit, SIGNAL(clicked(LLItem*)), SLOT(updateList()));
      lit = new LLItem(item, "Time Signature", L_TIMESIG);
      connect(lit, SIGNAL(clicked(LLItem*)), SLOT(updateList()));

      TrackList* tracks = song->tracks();
      for (irTrack it = tracks->rbegin(); it != tracks->rend(); ++it) {
            Track* track = *it;
            Q3ListViewItem* item = new Q3CheckListItem(lists, track->name(), Q3CheckListItem::CheckBoxController);
            genListsTrack(track, item);
            }
      updateList();
      }

//---------------------------------------------------------
//   ListEdit
//---------------------------------------------------------

ListEdit::ListEdit(QWidget*, PartList*)
      {
      showHex    = false;
      curEditor  = 0;
      editItem   = 0;
      editCol    = -1;
      curPart    = 0;

      setWindowTitle("MusE: List Editor");

      //---------Pulldown Menu----------------------------
      menuEdit = new Q3PopupMenu(this);
      menuBar()->insertItem(tr("&Edit"), menuEdit);
//TD      undoRedo->addTo(menuEdit);
      menuEdit->addSeparator();
      menuEdit->addAction(tr("Insert Tempo"), CMD_INSERT_TEMPO);
      menuEdit->addAction(tr("Insert Signature"), CMD_INSERT_SIG);
      menuEdit->addAction(tr("Insert Note"), CMD_INSERT_NOTE);
      menuEdit->addAction(tr("Insert SysEx"), CMD_INSERT_SYSEX);
      menuEdit->addAction(tr("Insert PolyAfterTouch"), CMD_INSERT_PAFTER);
      menuEdit->addAction(tr("Insert After Touch"), CMD_INSERT_CAFTER);
      menuEdit->addAction(tr("Insert Meta"), CMD_INSERT_META);
      menuEdit->addAction(tr("Insert Controller"), CMD_INSERT_CTRL);

      menuEdit->insertSeparator();
      menuEdit->insertItem(tr("Delete Event"), CMD_DELETE);
      menuEdit->setAccel(Qt::Key_Delete, CMD_DELETE);
      connect(menuEdit, SIGNAL(activated(int)), SLOT(cmd(int)));

      menuView = new Q3PopupMenu(this);
      menuView->setCheckable(true);
      menuBar()->insertItem(tr("&View"), menuView);
      menuView->insertItem(tr("Hex Midi Values"), CMD_SET_HEX);
      menuView->insertItem(tr("Decimal Midi Values"), CMD_SET_DEC);
      menuView->setItemChecked(CMD_SET_HEX, showHex);
      menuView->setItemChecked(CMD_SET_DEC, !showHex);
      connect(menuView, SIGNAL(activated(int)), SLOT(cmd(int)));

      listTools = new QToolBar(this, "list-tools");
      listTools->setLabel(tr("List Tools"));
//TD      undoRedo->addTo(listTools);
      QToolBar* tb = new QToolBar(this, "CurPart");
      tb->addWidget(new QLabel(tr("CurTrack:")));
      tb_t  = new QLabel("----");
      tb->addWidget(tb_t);
      tb_t->setFrameShape(Q3Frame::LineEditPanel);
      tb_t->setLineWidth(2);
      tb->addWidget(new QLabel(tr("CurPart:")));
      tb_p  = new QLabel(tr("----"));
      tb->addWidget(tb_p);
      tb_p->setFrameShape(Q3Frame::LineEditPanel);
      tb_p->setLineWidth(2);

      setCentralWidget(new QWidget(this, "qt_central_widget"));
      QGridLayout* grid = new QGridLayout(centralWidget(), 1, 1, 11, 6, "Form2Layout");

      splitter = new QSplitter(Qt::Horizontal, centralWidget());
      splitter->setOpaqueResize(true);
      splitter->setChildrenCollapsible(true);

      lists = new Q3ListView(splitter);
      lists->viewport()->setPaletteBackgroundColor(QColor(200, 200, 230));
      lists->setRootIsDecorated(true);
      lists->addColumn(tr("Filter"));
      lists->setResizeMode(Q3ListView::LastColumn);
      lists->setColumnWidthMode(0, Q3ListView::Maximum);
      lists->setSorting(-1);
      splitter->setResizeMode(lists, QSplitter::Auto);

      list = new Q3ListView(splitter);
      list->viewport()->setPaletteBackgroundColor(QColor(200, 200, 230));

      list->setAllColumnsShowFocus(true);
      list->setSelectionMode(Q3ListView::Single);
      list->addColumn(tr("Tick"));
      list->addColumn(tr("Bar"));
      list->addColumn(tr("Type"));
      list->addColumn(tr("Val 1"));
      list->addColumn(tr("Val 2"));
      list->addColumn(tr("Len"));
      list->addColumn(tr("Comment"));
      list->setResizeMode(Q3ListView::LastColumn);

      list->setSorting(0);
      list->setColumnAlignment(0, Qt::AlignRight);

      grid->addWidget(splitter, 0, 0);

      sigEditor   = new Awl::SigEdit(list->viewport());
      textEditor  = new QLineEdit(list->viewport());
      pitchEditor = new Awl::PitchEdit(list->viewport());
      posEditor   = new Awl::PosEdit;
      sigEditor->hide();
      textEditor->hide();
      pitchEditor->hide();
      posEditor->hide();

      updateLists();
      connect(list, SIGNAL(doubleClicked(Q3ListViewItem*,const QPoint&,int)), SLOT(editColumn(Q3ListViewItem*,const QPoint&,int)));
      connect(list->header(), SIGNAL(sizeChange(int,int,int)), SLOT(colResized()));
      connect(list, SIGNAL(selectionChanged()), SLOT(listSelectionChanged()));

      connect(sigEditor, SIGNAL(returnPressed()), SLOT(returnPressed()));
      connect(textEditor, SIGNAL(returnPressed()), SLOT(returnPressed()));
      connect(pitchEditor, SIGNAL(returnPressed()), SLOT(returnPressed()));
      connect(posEditor, SIGNAL(returnPressed()), SLOT(returnPressed()));

      connect(sigEditor, SIGNAL(escapePressed()), SLOT(escapePressed()));
      connect(pitchEditor, SIGNAL(escapePressed()), SLOT(escapePressed()));
      connect(posEditor, SIGNAL(escapePressed()), SLOT(escapePressed()));
      connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      connect(song, SIGNAL(trackAdded(Track*,int)), SLOT(trackAdded(Track*,int)));
      connect(song, SIGNAL(trackRemoved(Track*)), SLOT(trackRemoved(Track*)));
      }

//---------------------------------------------------------
//   ~ListEdit
//---------------------------------------------------------

ListEdit::~ListEdit()
      {
      }

//---------------------------------------------------------
//   editColumn
//---------------------------------------------------------

void ListEdit::editColumn(Q3ListViewItem* i, const QPoint&, int col)
      {
      if (i == 0 || col == 2 || col == -1)
            return;
      returnPressed();        // close all old editors

      editItem = (LItem*)i;
      editCol  = col;

      int id = editItem->rtti();

      if (col == 1) {
            posEditor->setValue(editItem->tick());
            curEditor = posEditor;
            }
      else if (id == L_TIMESIG && col == 3) {
            sigEditor->setValue(i->text(col));
            curEditor = sigEditor;
            }
      else if (id == L_NOTES && col == 3) {
            pitchEditor->setValue(editItem->event().pitch());
            curEditor = pitchEditor;
            }
      else if (id == L_SYSEX) {
#if 0 //TODO3
            MidiTrack* track = (MidiTrack*)(editItem->part()->track());
            MidiPort* mp = &midiPorts[track->outPort()];
            MidiInstrument* instr = mp->instrument();
            Event oEvent = editItem->event();
            EditSysexDialog dl(editItem->tick(), instr, oEvent, list->viewport());
            int rv = dl.exec();
            if (rv == QDialog::Accepted) {
                  Event event = dl.event();
                  audio->msgChangeEvent(oEvent, event, editItem->part(), true);
                  }
#endif
            editItem = 0;
            editCol  = -1;
            return;
            }
      else if (id == L_META) {
            Event oEvent = editItem->event();
//TD            EditMetaDialog dl(editItem->tick(), oEvent, list->viewport());
//            int rv = dl.exec();
//            if (rv == QDialog::Accepted) {
//                  Event event = dl.event();
//                  audio->msgChangeEvent(oEvent, event, editItem->part(), true);
//                  }
            editItem = 0;
            editCol  = -1;
            return;
            }
      else if (id == L_CTRL) {
            ListCtrl ctrl(editItem->tick(), editItem->lli()->track(),
               editItem->lli()->ctrl(), editItem->val());

            EditCtrlDialog dl(&ctrl, list->viewport());

            int rv = dl.exec();
            if (rv == QDialog::Accepted) {
                  song->addControllerVal(ctrl.track, ctrl.id, ctrl.tick, ctrl.val);
                  }
            editItem = 0;
            editCol  = -1;
            return;
            }
      else {
            textEditor->setText(i->text(col));
            textEditor->selectAll();
            curEditor = textEditor;
            }
      setEditorGeometry();
      curEditor->show();
      curEditor->setFocus();
      }

//---------------------------------------------------------
//   setEditorGeometry
//---------------------------------------------------------

void ListEdit::setEditorGeometry()
      {
      int x1 = 0;
      for (int i = 0; i < editCol; ++i)
            x1 += list->columnWidth(i);
      QRect r(list->itemRect(editItem));
      r.setX(x1);
      int w = list->columnWidth(editCol);
      int wHint = curEditor->sizeHint().width();
      if (wHint > w)
            w = curEditor->sizeHint().width();
      r.setWidth(w);
      curEditor->setGeometry(r);
      }

//---------------------------------------------------------
//   returnPressed
//---------------------------------------------------------

void ListEdit::returnPressed()
      {
      if (editItem == 0)
            return;
      switch(editItem->rtti()) {
            case L_TIMESIG:
                  if (editCol == 3) {
                        Awl::Sig newSig = sigEditor->sig();
                        int tick = editItem->tick();
                        song->startUndo();
                        audio->msgAddSig(tick, newSig.z, newSig.n, false);
                        song->endUndo(SC_SIG);
                        }
                  break;
            case L_TEMPOSIG:
                  if (editCol == 3) {
                        QString input = textEditor->text();
                        const AL::TEvent* t = editItem->tempo();
                        unsigned tick = t->tick;
                        bool conversionOK;
                        double dbl_input = input.toDouble(&conversionOK);
                        if (conversionOK && dbl_input < 250.0) {
                              int tempo = lrint((1000000.0 * 60.0)/dbl_input);
                              song->startUndo();
                              audio->msgAddTempo(tick, tempo, false);
                              song->endUndo(SC_TEMPO);
                              }
                        else {
                              QMessageBox::warning(this, tr("MusE: List Editor"),
                                 tr("Input error, conversion not OK or value out of range"),
                                 QMessageBox::Ok, QMessageBox::NoButton
                                 );
                              }
                        }
                  break;
            case L_NOTES:
                  {
                  }
                  break;
            case L_CTRL:
                  break;
            }
      escapePressed();
      }

//---------------------------------------------------------
//   escapePressed
//---------------------------------------------------------

void ListEdit::escapePressed()
      {
      if (editItem == 0)
            return;
      curEditor->hide();
      curEditor = 0;
      list->setFocus();
      editItem  = 0;
      editCol   = -1;
      }

//---------------------------------------------------------
//   colResized
//---------------------------------------------------------

void ListEdit::colResized()
      {
      if (curEditor)
            setEditorGeometry();
      }

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void ListEdit::readStatus(QDomNode node)
      {
      Q3ListViewItemIterator i(lists);

      while (!node.isNull()) {
            QDomElement e = node.toElement();
            QString tag(e.tagName());
            QString s(e.text());
            if (tag == "geometry") {
                  QRect r = AL::readGeometry(node);
                  setGeometry(r);
                  }
            else if (tag == "midiNumberBase") {
                  int base = s.toInt();
                  if (base == 16)
                        showHex = true;
                  menuView->setItemChecked(CMD_SET_HEX, showHex);
                  menuView->setItemChecked(CMD_SET_DEC, !showHex);
                  }
            else if (tag == "splitter")
                  ; // TD splitter->readStatus(node.firstChild());
            else if (tag == "list") {
                  QString name(e.attribute("name"));

                  if (i.current()) {
                        if (i.current()->text(0) == name) {
                              int open = e.attribute("open","-1").toInt();
                              int checked = e.attribute("checked","-1").toInt();
                              if (open == 1)
                                    i.current()->setOpen(true);
                              else if (checked == 1)
                                    ((Q3CheckListItem*)(i.current()))->setOn(true);
                              }
                        else
                              printf("ListEdit::readStatus(): name mismatch <%s><%s>\n",
                                 i.current()->text(0).latin1(), s.latin1());
                        ++i;
                        }
                  else {
                        printf("ListEdit::readStatus(): list too long\n");
                        }
                  }
            else
                  printf("MusE:List::Edit::readStatus(): unknown tag %s\n", e.tagName().latin1());
            node = node.nextSibling();
            }
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void ListEdit::writeStatus(Xml& xml) const
      {
      xml.tag("listeditor");
      xml.geometryTag("geometry", this);
      xml.intTag("midiNumberBase", showHex ? 16 : 10);
//TD      splitter->writeStatus(xml);

      Q3ListViewItemIterator i(lists);
      for (; i.current(); ++i) {
            Q3ListViewItem* item = i.current();
            int id = item->rtti();
            if (id < 1000)
                  xml.tagE("list name=\"%s\" open=\"%d\"",
                     item->text(0).latin1(), item->isOpen());
            else {
                  LLItem* ci = (LLItem*)item;
                  xml.tagE("list name=\"%s\" checked=\"%d\"",
                     item->text(0).latin1(), ci->isOn());
                  }
            }
      xml.etag("listeditor");
      }

//---------------------------------------------------------
//   cmd
//---------------------------------------------------------

void ListEdit::cmd(int cmd)
      {
      switch (cmd) {
            case CMD_SET_HEX:
                  showHex = true;
                  menuView->setItemChecked(CMD_SET_HEX, true);
                  menuView->setItemChecked(CMD_SET_DEC, false);
                  updateList();
                  break;

            case CMD_SET_DEC:
                  showHex = false;
                  menuView->setItemChecked(CMD_SET_HEX, false);
                  menuView->setItemChecked(CMD_SET_DEC, true);
                  updateList();
                  break;

            case CMD_DELETE:
                  {
                  Q3ListViewItem* l = list->selectedItem();
                  if (!l)
                     return;
                  LItem* li = (LItem*)l;
                  switch(li->rtti()) {
                        case L_TIMESIG:
                              break;
                        case L_TEMPOSIG:
                              break;
                        case L_NOTES:
                        case L_SYSEX:
                        case L_PAFTER:
                        case L_CAFTER:
                        case L_META:
                              audio->msgDeleteEvent(li->event(), li->part(), true);
                              break;
                        case L_CTRL:
                              song->removeControllerVal(li->lli()->track(),
                                 li->lli()->ctrl(), li->tick());
                              break;
                        }
                  break;
                  }
            case CMD_INSERT_TEMPO:
                  break;
            case CMD_INSERT_SIG:
                  break;
            case CMD_INSERT_NOTE:
                  {
                  if (curPart == 0) {
                        printf("first select part\n");
                        break;
                        }
//TD                  Event event = EditNoteDialog::getEvent(0, Event(), this);
//                  if (!event.empty())
//                        audio->msgAddEvent(event, curPart);
                  }
                  break;
            case CMD_INSERT_SYSEX:
                  {
                  if (curPart == 0) {
                        printf("first select part\n");
                        break;
                        }
#if 0 // TODO3
                  MidiTrack* track = (MidiTrack*)(curPart->track());
                  MidiPort* mp = &midiPorts[track->outPort()];
                  MidiInstrument* mi = mp->instrument();
                  Event event = EditSysexDialog::getEvent(0, mi, Event(), this);
                  if (!event.empty())
                        audio->msgAddEvent(event, curPart);
#endif
                  }
                  break;
            case CMD_INSERT_PAFTER:
            case CMD_INSERT_CAFTER:
            case CMD_INSERT_META:
            case CMD_INSERT_CTRL:
                  {
//                  ListCtrl ctrl(0, 0, 0, 0, 0);
//                  if (EditCtrlDialog::editCtrl(&ctrl, this)) {
//                        printf("ADD CONTROLLER\n");
//                        }
                  }
                  break;
            }
      }

//---------------------------------------------------------
//   listSelectionChanged
//---------------------------------------------------------

void ListEdit::listSelectionChanged()
      {
      Q3ListViewItem* item = list->selectedItem();
      if (item == 0 || item->rtti() < 1000) {
            curPartChanged(0);
            return;
            }
      menuEdit->setItemEnabled(CMD_DELETE, true);
      LItem* i = (LItem*)item;
      switch(item->rtti()) {
            case L_NOTES:
            case L_SYSEX:
            case L_PAFTER:
            case L_CAFTER:
            case L_META:
                  curPartChanged((MidiPart*)(i->part()));
                  break;
            case L_TIMESIG:
            case L_TEMPOSIG:
            case L_CTRL:
                  curPartChanged(0);
                  break;
            }
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void ListEdit::songChanged(int flags)
      {
      if (flags & (SC_EVENT_INSERTED | SC_EVENT_REMOVED | SC_EVENT_MODIFIED | SC_SIG | SC_TEMPO)) {
            updateList();
            }
      }

//---------------------------------------------------------
//   EditCtrlDialog
//    PosEdit* timePos;
//    QSlider* valSlider;
//    QSpinBox* valSpinBox;
//    QLabel* controllerName;
//    QListBox* ctrlList;
//    QPushButton* buttonNewController;
//---------------------------------------------------------

EditCtrlDialog::EditCtrlDialog(ListCtrl* c, QWidget* parent)
//   : EditCtrlBase(parent)
   : QDialog(parent)
      {
#if 0
      ctrl = c;

      pop = new Q3PopupMenu(this);
      pop->setCheckable(false);

      bool isDrum = false;
      if (c->track->type() == Track::MIDI_CHANNEL)
            isDrum = ((MidiChannel*)c->track)->useDrumMap();

      if (c->track->isMidiTrack()) {
//TODO3            midiPort->setValue(((MidiTrack*)ctrl->track)->outPort() + 1);
//TODO3            midiChannel->setValue(((MidiTrack*)ctrl->track)->outChannel() + 1);
            }
      else {
            midiPort->setEnabled(false);
            midiChannel->setEnabled(false);
            }

      ctrlList->clear();
      ctrlList->setSelectionMode(Q3ListBox::Single);

      //
      // populate list of available controller
      //

      ControllerNameList* cnl = ctrl->track->controllerNames();

      int idx = 0;
      for (iControllerName i = cnl->begin(); i != cnl->end(); ++i, ++idx) {
            int num  = i->id;
            // dont show drum specific controller if not a drum track
            if ((num & 0xff) == 0xff) {
                  if (!isDrum)
                        continue;
                  }
            ctrlList->insertItem(i->name);
            if (num == ctrl->id)
                  ctrlList->setSelected(idx, true);
            }

      Ctrl* ct = ctrl->track->getController(ctrl->id);
      valSlider->setRange(ct->minVal().i, ct->maxVal().i);
      valSpinBox->setRange(ct->minVal().i, ct->maxVal().i);
      controllerName->setText(ct->name());

      connect(ctrlList, SIGNAL(clicked(Q3ListBoxItem*)), SLOT(ctrlListClicked(Q3ListBoxItem*)));
//TD      connect(buttonNewController, SIGNAL(pressed()), SLOT(newController()));
//      connect(hbank,   SIGNAL(valueChanged(int)), SLOT(programChanged()));
//      connect(lbank,   SIGNAL(valueChanged(int)), SLOT(programChanged()));
//      connect(program, SIGNAL(valueChanged(int)), SLOT(programChanged()));
//      connect(patchName, SIGNAL(released()), SLOT(instrPopup()));

      ctrlListClicked(ctrlList->selectedItem());

      timePos->setValue(ctrl->tick);
      if (ctrl->id != CTRL_PROGRAM)
            valSlider->setValue(ctrl->val.i);
#endif
      }

//---------------------------------------------------------
//   newController
//---------------------------------------------------------

void EditCtrlDialog::newController()
      {
      if (!ctrl->track->isMidiTrack())
            return;
      Q3PopupMenu* pop = new Q3PopupMenu(this);
      pop->setCheckable(this);
      //
      // populate popup with all controllers available for
      // current instrument
      //
#if 0 //TODO3
      int portn               = ((MidiTrack*)ctrl->track)->outPort();
      MidiPort* port          = &midiPorts[portn];
      MidiInstrument* instr   = port->instrument();
      MidiControllerList* mcl = instr->controller();
      for (iMidiController ci = mcl->begin(); ci != mcl->end(); ++ci)
            pop->insertItem((*ci)->name());
      int rv = pop->exec(buttonNewController->mapToGlobal(QPoint(0,0)));
      if (rv != -1) {
            QString s = pop->text(rv);
            for (iMidiController ci = mcl->begin(); ci != mcl->end(); ++ci) {
                  if ((*ci)->name() == s) {
                        for (int idx = 0; ;++idx) {
                              QString str = ctrlList->text(idx);
                              if (s == str)
                                    break;
                              if (str.isNull()) {
                                    ctrlList->insertItem(s);
                                    break;
                                    }
                              }
                        }
                  }
            }
#endif
      delete pop;
      }

//---------------------------------------------------------
//   ctrlListClicked
//---------------------------------------------------------

void EditCtrlDialog::ctrlListClicked(Q3ListBoxItem* item)
      {
#if 0 //TD
      if (item == 0)
            return;
      QString s(item->text());

      ControllerNameList* cnl = ctrl->track->controllerNames();
      for (iControllerName i = cnl->begin(); i != cnl->end(); ++i) {
            if (s != i->name)
                  continue;
            int num = i->id;
            Ctrl* c = ctrl->track->getController(num);

            if (num == CTRL_PROGRAM) {
                  widgetStack->raiseWidget(1);
                  updatePatch();
                  }
            else {
                  widgetStack->raiseWidget(0);
                  valSlider->setRange(c->minVal().i, c->maxVal().i);
                  valSpinBox->setRange(c->minVal().i, c->maxVal().i);
                  controllerName->setText(s);
                  ctrl->val = c->getDefault();
                  valSlider->setValue(ctrl->val.i);
                  }
            break;
            }
#endif
      }

//---------------------------------------------------------
//   setPatch
//---------------------------------------------------------

void EditCtrlDialog::updatePatch()
      {
#if 0 //TODO3
      int port              = ((MidiTrack*)ctrl->track)->outPort();
      int channel           = ((MidiTrack*)ctrl->track)->outChannel();
      MidiInstrument* instr = midiPorts[port].instrument();
      int val = ctrl->val.i;
      const char* name = instr->getPatchName(channel, val);
      patchName->setText(QString(name));

      int hb = ((val >> 16) & 0xff) + 1;
      if (hb == 0x100)
            hb = 0;
      int lb = ((val >> 8) & 0xff) + 1;
            if (lb == 0x100)
      lb = 0;
      int pr = (val & 0xff) + 1;
      if (pr == 0x100)
            pr = 0;

      hbank->blockSignals(true);
      lbank->blockSignals(true);
      program->blockSignals(true);

      hbank->setValue(hb);
      lbank->setValue(lb);
      program->setValue(pr);

      hbank->blockSignals(false);
      lbank->blockSignals(false);
      program->blockSignals(false);
#endif
      }

//---------------------------------------------------------
//   instrPopup
//---------------------------------------------------------

void EditCtrlDialog::instrPopup()
      {
#if 0 //TODO3
      int channel = ((MidiTrack*)ctrl->track)->outChannel();
      int port    = ((MidiTrack*)ctrl->track)->outPort();
      MidiInstrument* instr = midiPorts[port].instrument();
      instr->populatePatchPopup(pop, channel);

      int rv = pop->exec(patchName->mapToGlobal(QPoint(10,5)));
      if (rv != -1) {
            ctrl->val.i = rv;
            updatePatch();
            }
#endif
      }

//---------------------------------------------------------
//   programChanged
//---------------------------------------------------------

void EditCtrlDialog::programChanged()
      {
#if 0 //TD
      int hb      = hbank->value();
      int lb      = lbank->value();
      int prog    = program->value();

      if (hb > 0 && hb < 129)
            hb -= 1;
      else
            hb = 0xff;
      if (lb > 0 && lb < 129)
            lb -= 1;
      else
            lb = 0xff;
      if (prog > 0 && prog < 129)
            prog -= 1;
      else
            prog = 0xff;

      ctrl->val.i = (hb << 16) + (lb << 8) + prog;
      updatePatch();
#endif
      }

//---------------------------------------------------------
//   trackAdded
//---------------------------------------------------------

void ListEdit::trackAdded(Track* track, int idx)
      {
      Q3ListViewItem* item;
      if (idx != 0) {
            Q3ListViewItem* item = lists->firstChild();
            int i = 1;
            while (item) {
                  if (i == idx) {
                        item = new Q3ListViewItem(lists, item);
                        item->setText(0, track->name());
                        break;
                        }
                  item = item->nextSibling();
                  ++i;
                  }
            }
      else
            item = new Q3ListViewItem(lists, track->name());
      genListsTrack(track, item);
      }

//---------------------------------------------------------
//   trackRemoved
//---------------------------------------------------------

void ListEdit::trackRemoved(Track* t)
      {
      Q3ListViewItemIterator i(lists);
      for (; i.current(); ++i) {
            int id = i.current()->rtti();
            if (id == 0 && t->name() == i.current()->text(0)) {
                  delete i.current();
                  break;
                  }
            }
      updateList();
      }

//---------------------------------------------------------
//   genListsTrack
//---------------------------------------------------------

void ListEdit::genListsTrack(Track* track, Q3ListViewItem* it)
      {
      LLItem* lit;
      ControllerNameList* cn = track->controllerNames();
      for (iControllerName i = cn->begin(); i != cn->end(); ++i) {
            lit = new LLItem(it, i->name, L_CTRL);
            lit->setTrack(track);
            lit->setCtrl(i->id);
            connect(lit, SIGNAL(clicked(LLItem*)), SLOT(updateList()));
            connect(track, SIGNAL(controllerChanged(int)), SLOT(updateList()));
            }
      PartList* pl = track->parts();
      for (iPart ip = pl->begin(); ip != pl->end(); ++ip) {
            Part* part = ip->second;
            LLItem* item = new LLItem(it, tr("Part") + part->name(), L_PART);
            item->setPart(part);
            if (track->isMidiTrack()) {
                  lit = new LLItem(item, tr("Note On/Off"), L_NOTES);
                  lit->setTrack(track);
                  connect(lit, SIGNAL(clicked(LLItem*)), SLOT(updateList()));
                  lit = new LLItem(item, tr("Aftertouch"), L_CAFTER);
                  lit->setTrack(track);
                  connect(lit, SIGNAL(clicked(LLItem*)), SLOT(updateList()));
                  lit = new LLItem(item, tr("PolyAftertouch"), L_PAFTER);
                  lit->setTrack(track);
                  connect(lit, SIGNAL(clicked(LLItem*)), SLOT(updateList()));
                  lit = new LLItem(item, tr("Sysex"), L_SYSEX);
                  lit->setTrack(track);
                  connect(lit, SIGNAL(clicked(LLItem*)), SLOT(updateList()));
                  lit = new LLItem(item, tr("Meta"), L_META);
                  lit->setTrack(track);
                  connect(lit, SIGNAL(clicked(LLItem*)), SLOT(updateList()));
                  }
            else if (track->type() == Track::WAVE) {
                  lit = new LLItem(item, tr("Waves"), L_WAVE);
                  lit->setTrack(track);
                  connect(lit, SIGNAL(clicked(LLItem*)), SLOT(updateList()));
                  }
            }
      }

//---------------------------------------------------------
//   curPartChanged
//---------------------------------------------------------

void ListEdit::curPartChanged(MidiPart* part)
      {
      curPart = part;

      if (curPart) {
            tb_p->setText(curPart->name());
            tb_t->setText(curPart->track()->name());
            }
      else {
            tb_p->setText("----");
            tb_t->setText("----");
            }
      }
#endif

