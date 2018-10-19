//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: editevent.cpp,v 1.12.2.6 2009/02/02 21:38:00 terminator356 Exp $
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
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

#include <stdio.h>

#include <QBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QTextEdit>
#include <QVBoxLayout>

#include "posedit.h"

#include "song.h"
#include "event.h"
#include "midictrl.h"
#include "editevent.h"
#include "pitchedit.h"
#include "intlabel.h"
#include "globals.h"
#include "gconfig.h"
#include "midiport.h"
#include "midiedit/drummap.h"
#include "instruments/minstrument.h"
#include "midi.h"
#include "popupmenu.h"
#include "choose_sysex.h"

namespace MusEGui {

//---------------------------------------------------------
//   string2qhex
//---------------------------------------------------------

QString string2hex(const unsigned char* data, int len)
      {
      QString d;
      for (int i = 0; i < len; ++i) {
            if ((i > 0) && ((i % 8)==0)) {
                  d += "\n";
                  }
            else if (i)
                  d += " ";
            // Strip all f0 and f7 (whether accidental or on purpose enclosing etc).
            if(data[i] == MusECore::ME_SYSEX || data[i] == MusECore::ME_SYSEX_END)
              continue;
            d += QString("%1").arg(data[i], 2, 16, QLatin1Char('0'));
            }
      return d;
      }

//---------------------------------------------------------
//   hex2string
//---------------------------------------------------------

char* hex2string(QWidget* parent, const char* src, int& len, bool warn = true)
      {
      char buffer[2048];
      char* dst = buffer;

      while (*src) {
            while (*src == ' ' || *src == '\n')
                  ++src;
            char* ep;
            long val =  strtol(src, &ep, 16);
            if (ep == src) {
                  if(warn)
                    QMessageBox::information(parent,
                      QString("MusE"),
                      QWidget::tr("Cannot convert sysex string"));
                  return 0;
                  }
            src    = ep;
            // Strip all f0 and f7 (whether accidental or on purpose enclosing etc).
            if(val == MusECore::ME_SYSEX || val == MusECore::ME_SYSEX_END)
              continue;
            *dst++ = val;
            if (dst - buffer >= 2048) {
                  if(warn)
                    QMessageBox::information(parent,
                      QString("MusE"),
                      QWidget::tr("Hex String too long (2048 bytes limit)"));
                  return 0;
                  }
            }
      len = dst - buffer;
      if(len == 0)
        return 0;
      char* b = new char[len+1];
      memcpy(b, buffer, len);
      b[len] = 0;
      return b;
      }

//---------------------------------------------------------
//   getEvent
//---------------------------------------------------------

MusECore::Event EditNoteDialog::getEvent(int tick, const MusECore::Event& event, QWidget* parent)
      {
      EditNoteDialog* dlg = new EditNoteDialog(tick, event, parent);
      MusECore::Event nevent;
      if (dlg->exec() == QDialog::Accepted) {
            nevent = dlg->getEvent();
            }
      delete dlg;
      return nevent;
      }

MusECore::Event EditSysexDialog::getEvent(int tick, const MusECore::Event& event, QWidget* parent, MusECore::MidiInstrument* instr)
      {
      EditSysexDialog* dlg = new EditSysexDialog(tick, event, parent, instr);
      MusECore::Event nevent;
      if (dlg->exec() == QDialog::Accepted) {
            nevent = dlg->getEvent();
            }
      delete dlg;
      return nevent;
      }

MusECore::Event EditMetaDialog::getEvent(int tick, const MusECore::Event& event, QWidget* parent)
      {
      EditEventDialog* dlg = new EditMetaDialog(tick, event, parent);
      MusECore::Event nevent;
      if (dlg->exec() == QDialog::Accepted) {
            nevent = dlg->getEvent();
            }
      delete dlg;
      return nevent;
      }

//---------------------------------------------------------
//   EditEventDialog
//---------------------------------------------------------

EditEventDialog::EditEventDialog(QWidget* parent)
   : QDialog(parent)
      {
      QVBoxLayout* xlayout = new QVBoxLayout;
      layout1 = new QGridLayout; // ddskrjo this
      xlayout->addLayout(layout1);

      //---------------------------------------------------
      //  Ok, Cancel
      //---------------------------------------------------

      QBoxLayout* w5 = new QHBoxLayout; // ddskrjo this
      QPushButton* okB = new QPushButton(tr("Ok"));
      okB->setDefault(true);
      QPushButton* cancelB = new QPushButton(tr("Cancel"));
      okB->setFixedWidth(80);
      cancelB->setFixedWidth(80);
      w5->addWidget(okB);
      w5->addSpacing(12);
      w5->addWidget(cancelB);
      w5->addStretch(1);
      xlayout->addLayout(w5);
      setLayout(xlayout);
      connect(cancelB, SIGNAL(clicked()), SLOT(reject()));
      connect(okB, SIGNAL(clicked()), SLOT(accept()));
      }

//---------------------------------------------------------
//   EditNoteDialog
//---------------------------------------------------------

EditNoteDialog::EditNoteDialog(int tick, const MusECore::Event& event,
   QWidget* parent)
   : QDialog(parent)
      {
      setupUi(this);
      if (!event.empty()) {
            epos->setValue(tick);
            il1->setValue(event.lenTick());
            pl->setValue(event.pitch());
            il2->setValue(event.velo());
            il3->setValue(event.veloOff());
            }
      else {
            epos->setValue(tick);
            il1->setValue(96);
            pl->setValue(64);
            il2->setValue(100);
            il3->setValue(0);
            }
      }

//---------------------------------------------------------
//   EditNoteDialog::event
//---------------------------------------------------------

MusECore::Event EditNoteDialog::getEvent()
      {
      MusECore::Event event(MusECore::Note);
      event.setTick(epos->pos().tick());
      event.setA(pl->value());
      event.setB(il2->value());
      event.setC(il3->value());
      event.setLenTick(il1->value());
      return event;
      }

//---------------------------------------------------------
//   EditSysExDialog
//---------------------------------------------------------

EditSysexDialog::EditSysexDialog(int tick, const MusECore::Event& event,
   QWidget* parent, MusECore::MidiInstrument* instr)
   : QDialog(parent)
      {
      setupUi(this);
      sysex = 0;
      _instr = instr;
      if (!event.empty()) {
            epos->setValue(tick);
            edit->setText(string2hex(event.data(), event.dataLen()));
            if(_instr)
            {
              typeLabel->setText(MusECore::nameSysex(event.dataLen(), event.data(), _instr));              
              commentLabel->setText(MusECore::sysexComment(event.dataLen(), event.data(), _instr));              
            }
            }
      else {
            epos->setValue(tick);
            }
      connect(edit, SIGNAL(textChanged()), SLOT(editChanged()));      
      connect(buttonSelect, SIGNAL(clicked(bool)), SLOT(selectSysex()));      
      }

//---------------------------------------------------------
//   ~EditSysexDialog
//---------------------------------------------------------

EditSysexDialog::~EditSysexDialog()
      {
      if (sysex)
            delete sysex;
      }

//---------------------------------------------------------
//   EditSysExDialog::event
//---------------------------------------------------------

MusECore::Event EditSysexDialog::getEvent()
      {
      MusECore::Event event(MusECore::Sysex);
      event.setTick(epos->pos().tick());
      event.setData(sysex, len);
      return event;
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void EditSysexDialog::accept()
      {
      QString qsrc = edit->toPlainText();
      QByteArray ba = qsrc.toLatin1();
      const char* src = ba.constData();

      sysex = (unsigned char*)hex2string(this, src, len);
      if (sysex)
            QDialog::accept();
      }

//---------------------------------------------------------
//   editChanged
//---------------------------------------------------------

void EditSysexDialog::editChanged()
{
  if(!_instr)
    return;
  
  QString qsrc = edit->toPlainText();
  QByteArray ba = qsrc.toLatin1();
  const char* src = ba.constData();

  int l;
  unsigned char* data = (unsigned char*)hex2string(this, src, l, false); // false = Don't warn with popups
  if(data && l > 0)
  {
    typeLabel->setText(MusECore::nameSysex(l, data, _instr));
    commentLabel->setText(MusECore::sysexComment(l, data, _instr));              
  }
  else
  {
   typeLabel->clear();
   commentLabel->clear();
  }
}
      
//---------------------------------------------------------
//   selectSysex
//---------------------------------------------------------

void EditSysexDialog::selectSysex()
{
  ChooseSysexDialog* dlg = new ChooseSysexDialog(this, _instr);
  if(dlg->exec() == QDialog::Accepted) 
  {
    MusECore::SysEx* s = dlg->sysex();
    if(s)
    {
      edit->setText(string2hex(s->data, s->dataLen));
      typeLabel->setText(s->name);
      commentLabel->setText(s->comment);              
    }
  }
  delete dlg;
}

//---------------------------------------------------------
//   EditMetaDialog
//---------------------------------------------------------

EditMetaDialog::EditMetaDialog(int tick, const MusECore::Event& ev,
   QWidget* parent)
   : EditEventDialog(parent)
      {
      meta = 0;
      setWindowTitle(tr("MusE: Enter Meta Event"));

      QLabel* l1 = new QLabel(tr("Time Position"));
      ///epos = new PosEdit;
      epos = new PosEdit;

      QLabel* l2 = new QLabel(tr("Meta Type"));
      il2 = new MusEGui::IntLabel(-1, 0, 127, this, -1);
      il2->setFixedWidth(100);
      il2->setFrame(true);
      il2->setDark();
      typeLabel = new QLabel;
      typeLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
      QHBoxLayout* typeLayout = new QHBoxLayout;
      typeLayout->addWidget(il2);
      typeLayout->addWidget(typeLabel);
      typeLayout->addStretch();

      hexButton = new QRadioButton(tr("Enter Hex"));
      hexButton->setChecked(true);
      connect(hexButton, SIGNAL(toggled(bool)), SLOT(toggled(bool)));

      edit = new QTextEdit;
      edit->setFont(MusEGlobal::config.fonts[0]);

      if (!ev.empty()) {
            epos->setValue(tick);
            il2->setValue(ev.dataA());
            toggled(true);
            edit->setText(string2hex(ev.data(), ev.dataLen()));
            }
      else {
            epos->setValue(tick);
            il2->setValue(0);
            }

      typeChanged(il2->value());
      connect(il2, SIGNAL(valueChanged(int)), SLOT(typeChanged(int)));
      
      layout1->addWidget(l1,  0, 0);
      layout1->addWidget(epos, 0, 1, Qt::AlignLeft);
      layout1->addWidget(l2,  1, 0);
      
      //layout1->addWidget(il2, 1, 1, AlignLeft);
      layout1->addLayout(typeLayout, 1, 1);

      //layout1->addMultiCellWidget(hexButton, 2, 2, 0, 1);
      //layout1->addMultiCellWidget(edit, 3, 3, 0, 1);
      layout1->addWidget(hexButton, 2, 0, 1, 2);
      layout1->addWidget(edit, 3, 0, 1, 2);
      }

//---------------------------------------------------------
//   typeChanged
//---------------------------------------------------------

void EditMetaDialog::typeChanged(int val)
{
  typeLabel->setText(MusECore::midiMetaName(val));
}

//---------------------------------------------------------
//   toggled
//---------------------------------------------------------

void EditMetaDialog::toggled(bool flag)
      {
      QString qsrc    = edit->toPlainText();
      QByteArray ba = qsrc.toLatin1();
      const char* src = ba.constData();
      edit->clear();

      QString dst;
      if (flag) {       // convert to hex
            dst = string2hex((unsigned char*)src, ba.length());
            }
      else {            // convert to string
            int len;
            dst = hex2string(this, src, len);
            }
      edit->setText(dst);
      }

//---------------------------------------------------------
//   ~EditMetaDialog
//---------------------------------------------------------

EditMetaDialog::~EditMetaDialog()
      {
      if (meta)
            delete meta;
      }

//---------------------------------------------------------
//   EditMetaDialog::event
//---------------------------------------------------------

MusECore::Event EditMetaDialog::getEvent()
      {
      MusECore::Event event(MusECore::Meta);
      event.setTick(epos->pos().tick());
      event.setA(il2->value());
      event.setData(meta, len);  // TODO ??
      return event;
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void EditMetaDialog::accept()
      {
      QString qsrc = edit->toPlainText();
      QByteArray ba = qsrc.toLatin1();
      const char* src = ba.constData();
      if (!hexButton->isChecked()) {
            meta = (unsigned char*)strdup(src);
            len  = ba.length();
            QDialog::accept();
            return;
            }

      meta = (unsigned char*)hex2string(this, src, len);
      if (meta)
            QDialog::accept();
      }

//---------------------------------------------------------
//   getEvent
//---------------------------------------------------------

MusECore::Event EditCtrlDialog::getEvent(int tick, const MusECore::Event& event,
    const MusECore::MidiPart* part, QWidget* parent)
      {
      EditCtrlDialog* dlg = new EditCtrlDialog(tick, event, part, parent);
      MusECore::Event nevent;
      if (dlg->exec() == QDialog::Accepted) {
            nevent = dlg->getEvent();
            }
      delete dlg;
      return nevent;
      }

//---------------------------------------------------------
//   EditCtrlDialog::event
//---------------------------------------------------------

MusECore::Event EditCtrlDialog::getEvent()
      {
      MusECore::Event event(MusECore::Controller);
      event.setTick(timePos->pos().tick());

      int cnum = 0;
      QListWidgetItem* item = ctrlList->currentItem();
      if(item != 0)
        cnum = item->data(Qt::UserRole).toInt();

      MusECore::MidiTrack* track  = part->track();
      bool isDrum                 = track->type() == MusECore::Track::DRUM;
      MusECore::MidiPort* port    = &MusEGlobal::midiPorts[track->outPort()];
      int channel                 = track->outChannel();
      bool isNewDrum              = track->type() == MusECore::Track::NEW_DRUM;

      int evnum = cnum;
      int num = cnum;
      if((cnum & 0xff) == 0xff)
      {
        evnum = (cnum & ~0xff) | (noteSpinBox->value() & 0x7f);
        num = evnum;
        if(isDrum)
        {
          MusECore::DrumMap* dm = &MusEGlobal::drumMap[noteSpinBox->value() & 0x7f];
          num     = (cnum & ~0xff) | dm->anote;
          // Default to track port if -1 and track channel if -1.
          if(dm->port != -1)
            port    = &MusEGlobal::midiPorts[dm->port];
          if(dm->channel != -1)
            channel = dm->channel;
        }
        else if(isNewDrum)
        {
          MusECore::DrumMap* dm = &track->drummap()[noteSpinBox->value() & 0x7f];
          num     = (cnum & ~0xff) | dm->anote;
          // Default to track port if -1 and track channel if -1.
          if(dm->port != -1)
            port    = &MusEGlobal::midiPorts[dm->port];
          if(dm->channel != -1)
            channel = dm->channel;
        }
      }

      MusECore::MidiController* c = port->midiController(cnum);
      MusECore::MidiCtrlValListList* cll = port->controller();

      if(cll->find(channel, num) == cll->end())
      {
        MusECore::MidiCtrlValList* vl = new MusECore::MidiCtrlValList(num);
        cll->add(channel, vl);
      }
                        
      event.setA(evnum);
      if(cnum == MusECore::CTRL_PROGRAM)
      {
        int hb   = hbank->value();
        int lb   = lbank->value();
        int prog = program->value();
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
        int val = (hb << 16) + (lb << 8) + prog;
        event.setB(val);
      }
      else
        event.setB(valSlider->value() + c->bias());
      return event;
      }

//---------------------------------------------------------
//   EditCtrlDialog
//    PosEdit* timePos;
//    QSlider* valSlider;
//    QSpinBox* valSpinBox;
//    QLabel* controllerName;
//    QListWidget* ctrlList;
//    QPushButton* buttonNewController;
//---------------------------------------------------------

struct CI {
            int num;
            QString s;
            bool used;
            bool off;
            bool instrument;
            CI(int n, const QString& ss, bool u, bool o, bool i) : num(n), s(ss), used(u), off(o), instrument(i) {}
            };

EditCtrlDialog::EditCtrlDialog(int tick, const MusECore::Event& event,
   const MusECore::MidiPart* p, QWidget* parent)
   : QDialog(parent), part(p)
      {
      setupUi(this);
      widgetStack->setAutoFillBackground(true);

      MusECore::MidiTrack* track   = part->track();
      MusECore::MidiPort* port   = &MusEGlobal::midiPorts[track->outPort()];
      bool isDrum        = track->type() == MusECore::Track::DRUM;
      bool isNewDrum     = track->type() == MusECore::Track::NEW_DRUM;
      bool isMidi        = track->type() == MusECore::Track::MIDI;
      MusECore::MidiCtrlValListList* cll = port->controller();
      int channel        = track->outChannel();
      MusECore::MidiInstrument* instr = port->instrument();
      MusECore::MidiControllerList* mcl = instr->controller();
      int val = 0;
      int ev_num = 0;
      int num = 0;
      int ev_cnum = 0;
      int ev_note = -1;
      if (!event.empty()) {
            ev_num = event.dataA();
            num = ev_num;
            ev_cnum = ev_num;
            val = event.dataB();
            if(port->drumController(ev_num))
            {
              ev_cnum |= 0xff;
              if(isDrum)
                num = (ev_num & ~0xff) | MusEGlobal::drumMap[ev_num & 0xff].anote;
              else if(isNewDrum)
                num = (ev_num & ~0xff) | track->drummap()[ev_num & 0xff].anote;

              ev_note = ev_num & 0xff;
            }
          }

      MusECore::MidiController* mc = port->midiController(ev_num);
      
      ctrlList->clear();
      ctrlList->setSelectionMode(QAbstractItemView::SingleSelection);

      //---------------------------------------------------
      // build list of midi controllers for current
      // MusECore::MidiPort/channel
      //---------------------------------------------------

      std::list<CI> sList;
      typedef std::list<CI>::iterator isList;
      std::set<int> already_added_nums;

      for (MusECore::iMidiCtrlValList it = cll->begin(); it != cll->end(); ++it) {
            MusECore::MidiCtrlValList* cl = it->second;
            int ch = it->first >> 24;
            if(ch != channel)
              continue;
            MusECore::MidiController* c   = port->midiController(cl->num());
            bool isDrumCtrl = (c->isPerNoteController());
            int show = c->showInTracks();
            int cnum = c->num();
            int clnum = cl->num();
            isList i = sList.begin();
            for (; i != sList.end(); ++i) {
                  if (i->num == cnum)
                        break;
                  }

            if (i == sList.end()) {
                  bool used = (clnum == num);
                  bool off = cl->hwVal() == MusECore::CTRL_VAL_UNKNOWN;  // Does it have a value or is it 'off'?
                  // Filter if not used and off. But if there's something there, we must show it.
                  //if(!used && off &&
                  if(!used && //off &&
                     (((isDrumCtrl || isNewDrum) && !(show & MusECore::MidiController::ShowInDrum)) ||
                     (isMidi && !(show & MusECore::MidiController::ShowInMidi))))
                    continue;
                  bool isinstr = mcl->find(cnum) != mcl->end();
                  // Need to distinguish between global default controllers and
                  //  instrument defined controllers. Instrument takes priority over global
                  //  ie they 'overtake' definition of a global controller such that the
                  //  global def is no longer available.
                  //sList.push_back(CI(num,
                  sList.push_back(CI(cnum,
                                  isinstr ? MusECore::midiCtrlNumString(cnum, true) + c->name() : MusECore::midiCtrlName(cnum, true),
                                  used, off, isinstr));
                  already_added_nums.insert(num);
                  }
            }

      // Add instrument-defined controllers:
      QListWidgetItem* sel_item = 0; 
      for (isList i = sList.begin(); i != sList.end(); ++i)
      {
        // Filter if not used and off. But if there's something there, we must show it.
        if(!i->instrument && !i->used && i->off)
          continue;
        QListWidgetItem* item = new QListWidgetItem(i->s, ctrlList);
        item->setData(Qt::UserRole, i->num);
        if(i->num == ev_cnum)
          sel_item = item;
      }
      if(sel_item)
        ctrlList->setCurrentItem(sel_item);  
      
      valSlider->setRange(mc->minVal(), mc->maxVal());
      valSpinBox->setRange(mc->minVal(), mc->maxVal());
      
      controllerName->setText(mc->name());

      if(!event.empty())
      {
        if(ev_num == MusECore::CTRL_PROGRAM)
        {
          widgetStack->setCurrentIndex(1);
          updatePatch(val);
        }  
        else  
        {
          widgetStack->setCurrentIndex(0);
          valSlider->setValue(val - mc->bias());
          
          if(mc->isPerNoteController())
          {
            noteSpinBox->setVisible(true);
            noteSpinBox->setEnabled(true);
            noteLabel->setVisible(true);
            noteLabel->setEnabled(true);
            if(ev_note != -1)
              noteSpinBox->setValue(ev_note);
          }
          else
          {
            noteSpinBox->setEnabled(false);
            noteSpinBox->setVisible(false);
            noteLabel->setEnabled(false);
            noteLabel->setVisible(false);
          }
        }  
      }
      else
      {
        noteSpinBox->setEnabled(false);
        noteSpinBox->setVisible(false);
        noteLabel->setEnabled(false);
        noteLabel->setVisible(false);
        if(sel_item)
          ctrlListClicked(sel_item);
      }
      connect(ctrlList, SIGNAL(itemClicked(QListWidgetItem*)), SLOT(ctrlListClicked(QListWidgetItem*)));
      connect(buttonNewController, SIGNAL(clicked()), SLOT(newController()));
      connect(hbank,   SIGNAL(valueChanged(int)), SLOT(programChanged()));
      connect(lbank,   SIGNAL(valueChanged(int)), SLOT(programChanged()));
      connect(program, SIGNAL(valueChanged(int)), SLOT(programChanged()));
      connect(patchName, SIGNAL(released()), SLOT(instrPopup()));
      connect(buttonCancel, SIGNAL(clicked()), SLOT(reject()));
      connect(buttonOk, SIGNAL(clicked()), SLOT(accept()));
      timePos->setValue(tick);
      
      }
//---------------------------------------------------------
//   newController
//---------------------------------------------------------

void EditCtrlDialog::newController()
      {
      MusEGui::PopupMenu* pup = new MusEGui::PopupMenu(this);

      // populate popup with all controllers available for
      // current instrument

      MusECore::MidiTrack* track        = part->track();
      int portn               = track->outPort();
      MusECore::MidiPort* port          = &MusEGlobal::midiPorts[portn];
      bool isDrum      = track->type() == MusECore::Track::DRUM;
      bool isNewDrum   = track->type() == MusECore::Track::NEW_DRUM;
      bool isMidi      = track->type() == MusECore::Track::MIDI;
      MusECore::MidiInstrument* instr   = port->instrument();
      MusECore::MidiControllerList* mcl = instr->controller();
      
      MusECore::MidiCtrlValListList* cll = port->controller();
      int channel              = track->outChannel();
      for (MusECore::iMidiController ci = mcl->begin(); ci != mcl->end(); ++ci)
      {
          MusECore::MidiController* c = ci->second;
          int cnum = c->num();
          int show = c->showInTracks();
          if(((isDrum || isNewDrum) && !(show & MusECore::MidiController::ShowInDrum)) ||
             (isMidi && !(show & MusECore::MidiController::ShowInMidi)))
            continue;
          // If it's not already in the parent menu...
          int idx = 0;
          for(; idx < ctrlList->count(); ++idx) {
            if(ctrlList->item(idx)->data(Qt::UserRole).toInt() == cnum)
              break;
          }
          if(idx >= ctrlList->count()) {
            QAction* act = pup->addAction(MusECore::midiCtrlNumString(cnum, true) + c->name());
            act->setData(cnum);
          }
      }
      
      QAction* act = pup->exec(buttonNewController->mapToGlobal(QPoint(0,0)));
      if (act && act->data().toInt() != -1) {
            int rv = act->data().toInt();
            int cnum = rv;
            for (MusECore::iMidiController ci = mcl->begin(); ci != mcl->end(); ++ci) {
                  MusECore::MidiController* mc = ci->second;
                  if (mc->num() == cnum) {
                        // Create a new controller list if it does not exist.
                        // FIXME: Sorry no per-pitch controller lists created here
                        //         (meaning you should only create one 'new' one at a time)
                        //         because the user has not had a chance to choose a pitch yet.
                        //        They are handled in accept(), where there are more checks and creations.
                        if(!mc->isPerNoteController() && cll->find(channel, rv) == cll->end())
                        {
                          MusECore::MidiCtrlValList* vl = new MusECore::MidiCtrlValList(rv);
                          cll->add(channel, vl);
                        }
                        int idx = 0;
                        for (; idx < ctrlList->count() ;++idx) {  
                              QListWidgetItem* item = ctrlList->item(idx);
                              int item_data = item->data(Qt::UserRole).toInt();
                              if(item_data == cnum)
                              {
                                    ctrlList->setCurrentItem(item);
                                    ctrlListClicked(item);
                                    break;
                              }      
                              }
                        if (idx >= ctrlList->count()) {                       
                              QListWidgetItem* new_item = new QListWidgetItem(act->text(), ctrlList);
                              new_item->setData(Qt::UserRole, cnum);
                              ctrlList->setCurrentItem(new_item);
                              ctrlListClicked(new_item);
                              break;
                              }
                        break;
                        }
                  }
            }
      delete pup;
      }
//---------------------------------------------------------
//   ctrlListClicked
//---------------------------------------------------------

void EditCtrlDialog::ctrlListClicked(QListWidgetItem* item)
      {
      if(item == 0)
        return;
      int cnum = item->data(Qt::UserRole).toInt();
      MusECore::MidiTrack* track  = part->track();
      int portn                   = track->outPort();
      MusECore::MidiPort* port    = &MusEGlobal::midiPorts[portn];
      MusECore::MidiController* c = port->midiController(cnum);
      int val;
      if (cnum == MusECore::CTRL_PROGRAM) {
            widgetStack->setCurrentIndex(1);

            val = c->initVal();
            if(val == MusECore::CTRL_VAL_UNKNOWN)
              val = 0;
            updatePatch(val);
            }
      else {
            widgetStack->setCurrentIndex(0);
            if(c->isPerNoteController())
            {
              noteSpinBox->setEnabled(true);
              noteSpinBox->setVisible(true);
              noteLabel->setEnabled(true);
              noteLabel->setVisible(true);
            }
            else
            {
              noteSpinBox->setEnabled(false);
              noteSpinBox->setVisible(false);
              noteLabel->setEnabled(false);
              noteLabel->setVisible(false);
            }
            valSlider->setRange(c->minVal(), c->maxVal());
            valSpinBox->setRange(c->minVal(), c->maxVal());
            controllerName->setText(c->name());
            val = c->initVal();

            if(val == MusECore::CTRL_VAL_UNKNOWN || val == 0)
            {
              switch(cnum)
              {
                case MusECore::CTRL_PANPOT:
                  val = 64 - c->bias();
                break;
                case MusECore::CTRL_VOLUME:
                  val = 100;
                break;
                default:
                  val = 0;
                break;
              }
            }
            valSlider->setValue(val);
            }
      }

//---------------------------------------------------------
//   updatePatch
//---------------------------------------------------------

void EditCtrlDialog::updatePatch(int val)
      {
      MusECore::MidiTrack* track      = part->track();
      int port              = track->outPort();
      int channel           = track->outChannel();
      MusECore::MidiInstrument* instr = MusEGlobal::midiPorts[port].instrument();
      patchName->setText(instr->getPatchName(channel, val, track->isDrumTrack(), true)); // Include default.

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
      }

//---------------------------------------------------------
//   instrPopup
//---------------------------------------------------------

void EditCtrlDialog::instrPopup()
      {
      MusECore::MidiTrack* track = part->track();
      int channel = track->outChannel();
      int port    = track->outPort();
      MusECore::MidiInstrument* instr = MusEGlobal::midiPorts[port].instrument();
      
      MusEGui::PopupMenu* pup = new MusEGui::PopupMenu(this);
      instr->populatePatchPopup(pup, channel, track->isDrumTrack());

      if(pup->actions().count() == 0)
      {
        delete pup;
        return;
      }  
      
      QAction* rv = pup->exec(patchName->mapToGlobal(QPoint(10,5)));
      if (rv) {
            updatePatch(rv->data().toInt());
            }
            
      delete pup;      
      }

//---------------------------------------------------------
//   programChanged
//---------------------------------------------------------

void EditCtrlDialog::programChanged()
      {
      int hb   = hbank->value();
      int lb   = lbank->value();
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

      int val = (hb << 16) + (lb << 8) + prog;
      updatePatch(val);
      }

} // namespace MusEGui
