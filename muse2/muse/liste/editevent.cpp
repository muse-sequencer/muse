//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: editevent.cpp,v 1.12.2.6 2009/02/02 21:38:00 terminator356 Exp $
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>

#include <QBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QTextEdit>
#include <QVBoxLayout>

#include "awl/posedit.h"

#include "song.h"
#include "event.h"
#include "midictrl.h"
#include "editevent.h"
#include "pitchedit.h"
#include "intlabel.h"
#include "globals.h"
///#include "posedit.h"
#include "gconfig.h"
#include "midiport.h"
#include "midiedit/drummap.h"
#include "instruments/minstrument.h"
#include "midi.h"

//---------------------------------------------------------
//   string2qhex
//---------------------------------------------------------

QString string2hex(const unsigned char* data, int len)
      {
      QString d;
      QString s;
      for (int i = 0; i < len; ++i) {
            if ((i > 0) && ((i % 8)==0)) {
                  d += "\n";
                  }
            else if (i)
                  d += " ";
            d += s.sprintf("%02x", data[i]);
            }
      return d;
      }

//---------------------------------------------------------
//   hex2string
//---------------------------------------------------------

char* hex2string(QWidget* parent, const char* src, int& len)
      {
      char buffer[2048];
      char* dst = buffer;

      while (*src) {
            while (*src == ' ' || *src == '\n')
                  ++src;
            char* ep;
            long val =  strtol(src, &ep, 16);
            if (ep == src) {
                  QMessageBox::information(parent,
                     QString("MusE"),
                     QWidget::tr("Cannot convert sysex string"));
                  return 0;
                  }
            src    = ep;
            *dst++ = val;
            if (dst - buffer >= 2048) {
                  QMessageBox::information(parent,
                     QString("MusE"),
                     QWidget::tr("Hex String too long (2048 bytes limit)"));
                  return 0;
                  }
            }
      len = dst - buffer;
      char* b = new char[len+1];
      memcpy(b, buffer, len);
      b[len] = 0;
      return b;
      }

//---------------------------------------------------------
//   getEvent
//---------------------------------------------------------

Event EditNoteDialog::getEvent(int tick, const Event& event, QWidget* parent)
      {
      EditNoteDialog* dlg = new EditNoteDialog(tick, event, parent);
      Event nevent;
      if (dlg->exec() == QDialog::Accepted) {
            nevent = dlg->event();
            }
      delete dlg;
      return nevent;
      }

Event EditSysexDialog::getEvent(int tick, const Event& event, QWidget* parent)
      {
      EditSysexDialog* dlg = new EditSysexDialog(tick, event, parent);
      Event nevent;
      if (dlg->exec() == QDialog::Accepted) {
            nevent = dlg->event();
            }
      delete dlg;
      return nevent;
      }

Event EditMetaDialog::getEvent(int tick, const Event& event, QWidget* parent)
      {
      EditEventDialog* dlg = new EditMetaDialog(tick, event, parent);
      Event nevent;
      if (dlg->exec() == QDialog::Accepted) {
            nevent = dlg->event();
            }
      delete dlg;
      return nevent;
      }

Event EditCAfterDialog::getEvent(int tick, const Event& event, QWidget* parent)
      {
      EditEventDialog* dlg = new EditCAfterDialog(tick, event, parent);
      Event nevent;
      if (dlg->exec() == QDialog::Accepted) {
            nevent = dlg->event();
            }
      delete dlg;
      return nevent;
      }

Event EditPAfterDialog::getEvent(int tick, const Event& event, QWidget* parent)
      {
      EditEventDialog* dlg = new EditPAfterDialog(tick, event, parent);
      Event nevent;
      if (dlg->exec() == QDialog::Accepted) {
            nevent = dlg->event();
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

EditNoteDialog::EditNoteDialog(int tick, const Event& event,
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

Event EditNoteDialog::event()
      {
      Event event(Note);
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

EditSysexDialog::EditSysexDialog(int tick, const Event& event,
   QWidget* parent)
   : QDialog(parent)
      {
      setupUi(this);
      sysex = 0;
      if (!event.empty()) {
            epos->setValue(tick);
            edit->setText(string2hex(event.data(), event.dataLen()));
            }
      else {
            epos->setValue(tick);
            }
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

Event EditSysexDialog::event()
      {
      Event event(Sysex);
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
//   EditMetaDialog
//---------------------------------------------------------

EditMetaDialog::EditMetaDialog(int tick, const Event& ev,
   QWidget* parent)
   : EditEventDialog(parent)
      {
      meta = 0;
      setWindowTitle(tr("MusE: Enter Meta Event"));

      QLabel* l1 = new QLabel(tr("Time Position"));
      ///epos = new PosEdit;
      epos = new Awl::PosEdit;

      QLabel* l2 = new QLabel(tr("Meta Type"));
      il2 = new IntLabel(-1, 0, 127, this, -1);
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
      edit->setFont(config.fonts[5]);

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
  typeLabel->setText(midiMetaName(val));
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
            dst = string2hex((unsigned char*)src, strlen(src));
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

Event EditMetaDialog::event()
      {
      Event event(Meta);
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
            len  = strlen(src);
            QDialog::accept();
            return;
            }

      meta = (unsigned char*)hex2string(this, src, len);
      if (meta)
            QDialog::accept();
      }

//---------------------------------------------------------
//   EditCAfterDialog
//---------------------------------------------------------

EditCAfterDialog::EditCAfterDialog(int tick, const Event& event,
   QWidget* parent)
   : EditEventDialog(parent)
      {
      setWindowTitle(tr("MusE: Enter Channel Aftertouch"));

      QLabel* l1 = new QLabel(tr("Time Position"));
      ///epos = new PosEdit;
      epos = new Awl::PosEdit;

      QLabel* l2 = new QLabel(tr("Pressure"));
      il2  = new IntLabel(-1, 0, 127, this, -1);
      il2->setFrame(true);
      il2->setDark();

      QSlider* slider = new QSlider(Qt::Horizontal);
      slider->setMinimum(0);
      slider->setMaximum(127);
      slider->setPageStep(1);
      slider->setValue(0);

      connect(slider, SIGNAL(valueChanged(int)), il2, SLOT(setValue(int)));
      connect(il2, SIGNAL(valueChanged(int)), slider, SLOT(setValue(int)));

      if (!event.empty()) {
            epos->setValue(tick);
            il2->setValue(event.dataA());
            slider->setValue(event.dataA());
            }
      else {
            epos->setValue(tick);
            il2->setValue(64);
            slider->setValue(64);
            }

      layout1->addWidget(l1,   0, 0);
      layout1->addWidget(epos,  0, 1, Qt::AlignLeft);
      layout1->addWidget(l2,   1, 0);
      layout1->addWidget(il2,  1, 1, Qt::AlignLeft);
      //layout1->addMultiCellWidget(slider, 2, 2, 0, 1);
      layout1->addWidget(slider, 2, 0, 1, 2);
      }

//---------------------------------------------------------
//   EditCAfterDialog::event
//---------------------------------------------------------

Event EditCAfterDialog::event()
      {
      Event event(CAfter);
      event.setTick(epos->pos().tick());
      event.setA(il2->value());
      return event;
      }

//---------------------------------------------------------
//   EditPAfterDialog
//---------------------------------------------------------

EditPAfterDialog::EditPAfterDialog(int tick, const Event& event,
   QWidget* parent)
   : EditEventDialog(parent)
      {
      setWindowTitle(tr("MusE: Enter Poly Aftertouch"));

      QLabel* l1 = new QLabel(tr("Time Position"));
      ///epos = new PosEdit;
      epos = new Awl::PosEdit;

      QLabel* l2 = new QLabel(tr("Pitch"));
      pl = new PitchEdit;
      QLabel* l3 = new QLabel(tr("Pressure"));
      il2  = new IntLabel(-1, 0, 127, this, -1);
      il2->setFrame(true);
      il2->setDark();

      QSlider* slider = new QSlider(Qt::Horizontal);
      slider->setMinimum(0);
      slider->setMaximum(127);
      slider->setPageStep(1);
      slider->setValue(0);

      connect(slider, SIGNAL(valueChanged(int)), il2, SLOT(setValue(int)));
      connect(il2, SIGNAL(valueChanged(int)), slider, SLOT(setValue(int)));

      if (!event.empty()) {
            epos->setValue(tick);
            pl->setValue(event.pitch());
            il2->setValue(event.dataB());
            slider->setValue(event.dataB());
            }
      else {
            epos->setValue(tick);
            pl->setValue(64);
            il2->setValue(64);
            slider->setValue(64);
            }

      layout1->addWidget(l1,  0, 0);
      layout1->addWidget(epos, 0, 1, Qt::AlignLeft);
      layout1->addWidget(l2,  1, 0);
      layout1->addWidget(pl,  1, 1, Qt::AlignLeft);
      layout1->addWidget(l3,  2, 0);
      layout1->addWidget(il2, 2, 1, Qt::AlignLeft);
      //layout1->addMultiCellWidget(slider, 3, 3, 0, 1);
      layout1->addWidget(slider, 3, 0, 1, 2);
      }

//---------------------------------------------------------
//   EditPAfterDialog::event
//---------------------------------------------------------

Event EditPAfterDialog::event()
      {
      Event event(PAfter);
      event.setTick(epos->pos().tick());
      event.setA(pl->value());
      event.setB(il2->value());
      return event;
      }
//---------------------------------------------------------
//   getEvent
//---------------------------------------------------------

Event EditCtrlDialog::getEvent(int tick, const Event& event,
   const MidiPart* part, QWidget* parent)
      {
      EditCtrlDialog* dlg = new EditCtrlDialog(tick, event, part, parent);
      Event nevent;
      if (dlg->exec() == QDialog::Accepted) {
            nevent = dlg->event();
            }
      delete dlg;
      return nevent;
      }

//---------------------------------------------------------
//   EditCtrlDialog::event
//---------------------------------------------------------

Event EditCtrlDialog::event()
      {
      Event event(Controller);
      event.setTick(timePos->pos().tick());
      event.setA(num);
      if (num == CTRL_PROGRAM)
            event.setB(val);
      else
            event.setB(valSlider->value() + midiPorts[part->track()->outPort()].midiController(num)->bias());
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

EditCtrlDialog::EditCtrlDialog(int tick, const Event& event,
   const MidiPart* p, QWidget* parent)
   : QDialog(parent), part(p)
      {
      setupUi(this);
      widgetStack->setAutoFillBackground(true);
      val = 0;
      num = 0;
      if (!event.empty()) {
            num = event.dataA();
            val = event.dataB();
            }

      pop = new QMenu(this);
      //pop->setCheckable(false);//not necessary in Qt4

      MidiTrack* track   = part->track();
      int portn          = track->outPort();
      MidiPort* port     = &midiPorts[portn];
      bool isDrum        = track->type() == Track::DRUM;
      MidiCtrlValListList* cll = port->controller();

      ctrlList->clear();
      ctrlList->setSelectionMode(QAbstractItemView::SingleSelection);

      //
      // populate list of available controller
      //

      std::list<QString> sList;
      typedef std::list<QString>::iterator isList;

      for (iMidiCtrlValList i = cll->begin(); i != cll->end(); ++i) {
            MidiCtrlValList* cl = i->second;
            int num             = cl->num();

            // dont show drum specific controller if not a drum track
            if ((num & 0xff) == 0xff) {
                  if (!isDrum)
                        continue;
                  }
            MidiController* c = port->midiController(num);
            isList i = sList.begin();
            for (; i != sList.end(); ++i) {
                  if (*i == c->name())
                        break;
                  }
            if (i == sList.end())
                  sList.push_back(c->name());
            }
      MidiController* mc = port->midiController(num);
      int idx = 0;
      int selectionIndex = 0;
      for (isList i = sList.begin(); i != sList.end(); ++i, ++idx) {
            ctrlList->addItem(*i);
            if (mc->name() == *i)
                  selectionIndex = idx;
            }
      ctrlList->item(selectionIndex)->setSelected(true);

      valSlider->setRange(mc->minVal(), mc->maxVal());
      valSpinBox->setRange(mc->minVal(), mc->maxVal());
      
      controllerName->setText(mc->name());

      if(!event.empty())
      {
        if(num == CTRL_PROGRAM)
        {
          widgetStack->setCurrentIndex(1);
          updatePatch();
        }  
        else  
        {
          widgetStack->setCurrentIndex(0);
          valSlider->setValue(val - mc->bias());
        }  
      }
      else
        ctrlListClicked(ctrlList->selectedItems()[0]);
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
      QMenu* pop = new QMenu(this);
      //pop->setCheckable(this);//not necessary in Qt4
      //
      // populate popup with all controllers available for
      // current instrument
      //
      MidiTrack* track        = part->track();
      int portn               = track->outPort();
      MidiPort* port          = &midiPorts[portn];
      MidiInstrument* instr   = port->instrument();
      MidiControllerList* mcl = instr->controller();
      
      MidiCtrlValListList* cll = port->controller();
      int channel              = track->outChannel();
      int nn = 0;
      for (iMidiController ci = mcl->begin(); ci != mcl->end(); ++ci)
      {
            if(cll->find(channel, ci->second->num()) == cll->end())
            {
                    QAction* act = pop->addAction(ci->second->name());
		    act->setData(nn);
		    ++nn;
	    }
      }
      QAction* rv = pop->exec(buttonNewController->mapToGlobal(QPoint(0,0)));
      if (rv) {
            QString s = rv->text();
            for (iMidiController ci = mcl->begin(); ci != mcl->end(); ++ci) {
                  MidiController* mc = ci->second;
                  if (mc->name() == s) {
                        if(cll->find(channel, mc->num()) == cll->end())
                        {
                          MidiCtrlValList* vl = new MidiCtrlValList(mc->num());
                          cll->add(channel, vl);
                          //song->update(SC_MIDI_CONTROLLER_ADD);
                        }
                        for (int idx = 0; ;++idx) {
			  QString str = ctrlList->item(idx)->text();
                              if (s == str)
                              {
				ctrlList->item(idx)->setSelected(true);
                                    ctrlListClicked(ctrlList->item(idx));
                                    break;
                              }      
                              if (str.isNull()) {
                                    ctrlList->addItem(s);
                                    ctrlList->item(idx)->setSelected(true);
                                    ctrlListClicked(ctrlList->item(idx));
                                    break;
                                    }
                              }
                              
                        break;
                        }
                  }
            }
      delete pop;
      }
//---------------------------------------------------------
//   ctrlListClicked
//---------------------------------------------------------

void EditCtrlDialog::ctrlListClicked(QListWidgetItem* item)
      {
      if (item == 0)
            return;
      QString s(item->text());

      MidiTrack* track         = part->track();
      int portn                = track->outPort();
      MidiPort* port           = &midiPorts[portn];
      MidiCtrlValListList* cll = port->controller();
      
      iMidiCtrlValList i;
      for (i = cll->begin(); i != cll->end(); ++i) {
            MidiCtrlValList* cl = i->second;
            num                 = cl->num();
            MidiController* c   = port->midiController(num);
            if (s == c->name()) {
                  if (num == CTRL_PROGRAM) {
                        widgetStack->setCurrentIndex(1);
                        
                        val = c->initVal();
                        if(val == CTRL_VAL_UNKNOWN)
                          val = 0;
                        updatePatch();
                        }
                  else {
                        widgetStack->setCurrentIndex(0);
                        valSlider->setRange(c->minVal(), c->maxVal());
                        valSpinBox->setRange(c->minVal(), c->maxVal());
                        controllerName->setText(s);
                        val = c->initVal();
                        
                        if(val == CTRL_VAL_UNKNOWN || val == 0)
                        {
                          switch(num)
                          {
                            case CTRL_PANPOT:
                              val = 64 - c->bias();
                            break;
                            case CTRL_VOLUME:
                              val = 100;
                            break;
                            default:  
                              val = 0;
                            break;  
                          } 
                        }
                        valSlider->setValue(val);
                        }
                  break;
                  }
            }
      if (i == cll->end())
            printf("controller %s not found!\n", s.toLatin1().constData());
      }

//---------------------------------------------------------
//   updatePatch
//---------------------------------------------------------

void EditCtrlDialog::updatePatch()
      {
      MidiTrack* track      = part->track();
      int port              = track->outPort();
      int channel           = track->outChannel();
      MidiInstrument* instr = midiPorts[port].instrument();
      const char* name = instr->getPatchName(channel, val, song->mtype(), track->type() == Track::DRUM);
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
      }

//---------------------------------------------------------
//   instrPopup
//---------------------------------------------------------

void EditCtrlDialog::instrPopup()
      {
      MidiTrack* track = part->track();
      int channel = track->outChannel();
      int port    = track->outPort();
      MidiInstrument* instr = midiPorts[port].instrument();
      instr->populatePatchPopup(pop, channel, song->mtype(), track->type() == Track::DRUM);

      if(pop->actions().count() == 0)
        return;
      QAction* rv = new QAction(pop->exec(patchName->mapToGlobal(QPoint(10,5))));
      if (rv) {
            val = rv->data().toInt();
            updatePatch();
            }
      delete rv;
      }

//---------------------------------------------------------
//   programChanged
//---------------------------------------------------------

void EditCtrlDialog::programChanged()
      {
//      MidiTrack* track = part->track();
//      int channel = track->outChannel();
//      int port    = track->outPort();
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

      val = (hb << 16) + (lb << 8) + prog;
      updatePatch();
      }

