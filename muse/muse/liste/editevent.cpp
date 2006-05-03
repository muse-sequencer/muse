//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: editevent.cpp,v 1.26 2005/11/04 12:03:47 wschweer Exp $
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include "song.h"
#include "event.h"
#include "midictrl.h"
#include "editevent.h"
#include "awl/pitchlabel.h"
#include "awl/pitchedit.h"
// #include "widgets/intlabel.h"
#include "globals.h"
#include "awl/posedit.h"
#include "gconfig.h"
#include "midiport.h"
#include "midiedit/drummap.h"
#include "instruments/minstrument.h"

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
            d += s.sprintf("0x%02x", data[i]);
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
            long val =  strtol(src, &ep, 0);
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

Event EditSysexDialog::getEvent(int tick, MidiInstrument* mi, const Event& event, QWidget* parent)
      {
      EditSysexDialog* dlg = new EditSysexDialog(tick, mi, event, parent);
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
//   : QDialog(parent, "edit event", true)
   : QDialog(parent)
      {
      QVBoxLayout* xlayout = new QVBoxLayout(this);
      layout1 = new QGridLayout;
      xlayout->addLayout(layout1);

      //---------------------------------------------------
      //  Ok, Cancel
      //---------------------------------------------------

      QBoxLayout* w5 = new QHBoxLayout;
      xlayout->addLayout(w5);
      QPushButton* okB = new QPushButton(tr("Ok"), this);
      okB->setDefault(true);
      QPushButton* cancelB = new QPushButton(tr("Cancel"), this);
      okB->setFixedWidth(80);
      cancelB->setFixedWidth(80);
      w5->addWidget(okB);
      w5->addSpacing(12);
      w5->addWidget(cancelB);
      w5->addStretch(1);
      connect(cancelB, SIGNAL(clicked()), SLOT(reject()));
      connect(okB, SIGNAL(clicked()), SLOT(accept()));
      }

//---------------------------------------------------------
//   EditNoteDialog
//---------------------------------------------------------

EditNoteDialog::EditNoteDialog(int tick, const Event& /*event*/, QWidget* parent)
//   : EditNoteDialogBase(parent)
   : QDialog(parent)
      {
//      if (!event.empty()) {
//TD            pos->setValue(tick);
//            il1->setValue(event.lenTick());
//            pl->setValue(event.pitch());
//            il2->setValue(event.velo());
//            il3->setValue(event.veloOff());
//            }
//      else {
//TD            pos->setValue(0);
//            il1->setValue(96);
//            pl->setValue(64);
//            il2->setValue(100);
//            il3->setValue(0);
//            }
      }

//---------------------------------------------------------
//   EditNoteDialog::event
//---------------------------------------------------------

Event EditNoteDialog::event()
      {
      Event event(Note);
//TD      event.setTick(pos->pos().tick());
//      event.setA(pl->value());
//      event.setB(il2->value());
//      event.setC(il3->value());
//      event.setLenTick(il1->value());
      return event;
      }

//---------------------------------------------------------
//   EditSysExDialog
//---------------------------------------------------------

EditSysexDialog::EditSysexDialog(int tick, MidiInstrument* m, const Event& /*event*/, QWidget* parent)
//   : EditSysexDialogBase(parent)
   : QDialog(parent)
      {
#if 0 //TD
      sysex = 0;
      mi = m;
      if (mi) {
            std::vector<SysEx>* sl = mi->sysexList();
            for (std::vector<SysEx>::iterator i = sl->begin(); i != sl->end(); ++i) {
                  SysEx se = *i;
                  sysexList->insertItem(se.name);
                  }
            }
      if (!event.empty()) {
            pos->setValue(tick);
            edit->setText(string2hex(event.data(), event.dataLen()));
            }
      else {
            pos->setValue(0);
            }
      listSelectionChanged(); // initialize Add button
      connect(sysexList, SIGNAL(selectionChanged()), SLOT(listSelectionChanged()));
      connect(buttonAdd, SIGNAL(clicked()), SLOT(addPressed()));
#endif
      }

//---------------------------------------------------------
//   addPressed
//---------------------------------------------------------

void EditSysexDialog::addPressed()
      {
#if 0 //TD
      Q3ListBoxItem* item = sysexList->selectedItem();
      if (!item)
            return;
      std::vector<SysEx>* sl = mi->sysexList();
      for (std::vector<SysEx>::iterator i = sl->begin(); i != sl->end(); ++i) {
            SysEx se = *i;
            if (se.name == item->text()) {
                  QString s = edit->text();
                  if (!s.isEmpty())
                        s += " ";
                  s += se.data;
                  edit->setText(s);
                  comment->setText(se.comment);
                  break;
                  }
            }
#endif
      }

//---------------------------------------------------------
//   listSelectionChanged
//---------------------------------------------------------

void EditSysexDialog::listSelectionChanged()
      {
//TD      Q3ListBoxItem* item = sysexList->selectedItem();
//      buttonAdd->setEnabled(item);
      }

//---------------------------------------------------------
//   ~EditSysexDialog
//---------------------------------------------------------

EditSysexDialog::~EditSysexDialog()
      {
//TD      if (sysex)
//            delete sysex;
      }

//---------------------------------------------------------
//   EditSysExDialog::event
//---------------------------------------------------------

Event EditSysexDialog::event()
      {
      Event event(Sysex);
//TD      event.setTick(pos->pos().tick());
      event.setData(sysex, len);
      return event;
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void EditSysexDialog::accept()
      {
//TD      QString qsrc = edit->text();
//      const char* src = qsrc.toLatin1().data();

//      sysex = (unsigned char*)hex2string(this, src, len);
//      if (sysex)
//            QDialog::accept();
      }

//---------------------------------------------------------
//   EditMetaDialog
//---------------------------------------------------------

EditMetaDialog::EditMetaDialog(int tick, const Event& ev, QWidget* parent)
   : EditEventDialog(parent)
      {
#if 0
      meta = 0;
      setWindowTitle(tr("MusE: Enter Meta Event"));

      QLabel* l1 = new QLabel(tr("Time Position"), this);
      pos = new PosEdit;

      QLabel* l2 = new QLabel(tr("Meta Type"), this);
      il2 = new IntLabel(-1, 0, 127, this, -1);
      il2->setFrame(true);
      il2->setDark();

      hexButton = new QRadioButton(tr("Enter Hex"), this, "hextoggle");
      hexButton->setChecked(true);
      connect(hexButton, SIGNAL(toggled(bool)), SLOT(toggled(bool)));

      edit = new Q3MultiLineEdit(this);
      edit->setFont(*config.fonts[5]);

      if (!ev.empty()) {
            pos->setValue(tick);
            il2->setValue(ev.dataA());
            toggled(true);
            edit->setText(string2hex(ev.data(), ev.dataLen()));
            }
      else {
            pos->setValue(0);
            il2->setValue(0);
            }

      layout1->addWidget(l1,  0, 0);
      layout1->addWidget(pos, 0, 1, Qt::AlignLeft);
      layout1->addWidget(l2,  1, 0);
      layout1->addWidget(il2, 1, 1, Qt::AlignLeft);
      layout1->addMultiCellWidget(hexButton, 2, 2, 0, 1);
      layout1->addMultiCellWidget(edit, 3, 3, 0, 1);
#endif
      }

//---------------------------------------------------------
//   toggled
//---------------------------------------------------------

void EditMetaDialog::toggled(bool flag)
      {
#if 0
      QString qsrc    = edit->text();
      const char* src = qsrc.toLatin1().data();
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
#endif
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
#if 0
      event.setTick(pos->pos().tick());
      event.setA(il2->value());
      event.setData(meta, len);  // TODO ??
#endif
      return event;
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void EditMetaDialog::accept()
      {
#if 0
      QString qsrc = edit->text();
      const char* src = qsrc.toLatin1().data();
      if (!hexButton->isChecked()) {
            meta = (unsigned char*)strdup(src);
            len  = strlen(src);
            QDialog::accept();
            return;
            }

      meta = (unsigned char*)hex2string(this, src, len);
      if (meta)
            QDialog::accept();
#endif
      }

//---------------------------------------------------------
//   EditCAfterDialog
//---------------------------------------------------------

EditCAfterDialog::EditCAfterDialog(int tick, const Event& event,
   QWidget* parent)
   : EditEventDialog(parent)
      {
#if 0
      setCaption(tr("MusE: Enter Channel Aftertouch"));

      QLabel* l1 = new QLabel(tr("Time Position"), this);
      pos = new PosEdit;

      QLabel* l2 = new QLabel(tr("Pressure"), this);
      il2  = new IntLabel(-1, 0, 127, this, -1);
      il2->setFrame(true);
      il2->setDark();

      QSlider* slider = new QSlider(0, 127, 1, 0, Qt::Horizontal, this);
      connect(slider, SIGNAL(valueChanged(int)), il2, SLOT(setValue(int)));
      connect(il2, SIGNAL(valueChanged(int)), slider, SLOT(setValue(int)));

      if (!event.empty()) {
            pos->setValue(tick);
            il2->setValue(event.dataA());
            slider->setValue(event.dataA());
            }
      else {
            pos->setValue(0);
            il2->setValue(64);
            slider->setValue(64);
            }

      layout1->addWidget(l1,   0, 0);
      layout1->addWidget(pos,  0, 1, Qt::AlignLeft);
      layout1->addWidget(l2,   1, 0);
      layout1->addWidget(il2,  1, 1, Qt::AlignLeft);
      layout1->addMultiCellWidget(slider, 2, 2, 0, 1);
#endif
      }

//---------------------------------------------------------
//   EditCAfterDialog::event
//---------------------------------------------------------

Event EditCAfterDialog::event()
      {
      Event event(CAfter);
#if 0
      event.setTick(pos->pos().tick());
      event.setA(il2->value());
#endif
      return event;
      }

//---------------------------------------------------------
//   EditPAfterDialog
//---------------------------------------------------------

EditPAfterDialog::EditPAfterDialog(int tick, const Event& event,
   QWidget* parent)
   : EditEventDialog(parent)
      {
#if 0
      setCaption(tr("MusE: Enter Poly Aftertouch"));

      QLabel* l1 = new QLabel(tr("Time Position"), this);
      pos = new PosEdit;

      QLabel* l2 = new QLabel(tr("Pitch"), this);
      pl = new PitchLabel;

      QLabel* l3 = new QLabel(tr("Pressure"), this);
      il2  = new IntLabel(-1, 0, 127, this, -1);
      il2->setFrame(true);
      il2->setDark();

      QSlider* slider = new QSlider(0, 127, 1, 0, Qt::Horizontal, this);
      connect(slider, SIGNAL(valueChanged(int)), il2, SLOT(setValue(int)));
      connect(il2, SIGNAL(valueChanged(int)), slider, SLOT(setValue(int)));

      if (!event.empty()) {
            pos->setValue(tick);
            pl->setValue(event.pitch());
            il2->setValue(event.dataB());
            slider->setValue(event.dataB());
            }
      else {
            pos->setValue(0);
            pl->setValue(64);
            il2->setValue(64);
            slider->setValue(64);
            }

      layout1->addWidget(l1,  0, 0);
      layout1->addWidget(pos, 0, 1, Qt::AlignLeft);
      layout1->addWidget(l2,  1, 0);
      layout1->addWidget(pl,  1, 1, Qt::AlignLeft);
      layout1->addWidget(l3,  2, 0);
      layout1->addWidget(il2, 2, 1, Qt::AlignLeft);
      layout1->addMultiCellWidget(slider, 3, 3, 0, 1);
#endif
      }

//---------------------------------------------------------
//   EditPAfterDialog::event
//---------------------------------------------------------

Event EditPAfterDialog::event()
      {
      Event event(CAfter);
#if 0 //TD
      event.setTick(pos->pos().tick());
      event.setA(pl->value());
      event.setB(il2->value());
#endif
      return event;
      }

