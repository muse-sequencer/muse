//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: organgui.cpp,v 1.21 2005/12/16 15:36:51 wschweer Exp $
//
//    This is a simple GUI implemented with QT for
//    organ software synthesizer.
//
//  (C) Copyright 2001-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include "organgui.h"
#include "muse/midi.h"
#include "muse/midictrl.h"

//---------------------------------------------------------
//   OrganGui
//---------------------------------------------------------

OrganGui::OrganGui()
//   : QWidget(0, "organgui", Qt::WType_TopLevel),
   : QWidget(0),
     MessGui()
      {
      setupUi(this);
      QSocketNotifier* s = new QSocketNotifier(readFd, QSocketNotifier::Read);
      connect(s, SIGNAL(activated(int)), SLOT(readMessage(int)));

      dctrl[0]  = SynthGuiCtrl(p1,  lcd1,  SynthGuiCtrl::SLIDER);
      dctrl[1]  = SynthGuiCtrl(p2,  lcd2,  SynthGuiCtrl::SLIDER);
      dctrl[2]  = SynthGuiCtrl(p3,  lcd3,  SynthGuiCtrl::SLIDER);
      dctrl[3]  = SynthGuiCtrl(p4,  lcd4,  SynthGuiCtrl::SLIDER);
      dctrl[4]  = SynthGuiCtrl(p5,  lcd5,  SynthGuiCtrl::SLIDER);
      dctrl[5]  = SynthGuiCtrl(p6,  lcd6,  SynthGuiCtrl::SLIDER);
      dctrl[6]  = SynthGuiCtrl(p7,  lcd7,  SynthGuiCtrl::SLIDER);
      dctrl[7]  = SynthGuiCtrl(p8,  lcd8,  SynthGuiCtrl::SLIDER);
      dctrl[8]  = SynthGuiCtrl(p9,  lcd9,  SynthGuiCtrl::SLIDER);
      dctrl[9]  = SynthGuiCtrl(p10, lcd10, SynthGuiCtrl::SLIDER);
      dctrl[10] = SynthGuiCtrl(p11, lcd11, SynthGuiCtrl::SLIDER);
      dctrl[11] = SynthGuiCtrl(p12, lcd12, SynthGuiCtrl::SLIDER);
      dctrl[12] = SynthGuiCtrl(p13, lcd13, SynthGuiCtrl::SLIDER);
      dctrl[13] = SynthGuiCtrl(p14, lcd14, SynthGuiCtrl::SLIDER);
      dctrl[14] = SynthGuiCtrl(sw1,    0,  SynthGuiCtrl::SWITCH);
      dctrl[15] = SynthGuiCtrl(sw3,    0,  SynthGuiCtrl::SWITCH);
      dctrl[16] = SynthGuiCtrl(sw2,    0,  SynthGuiCtrl::SWITCH);
      dctrl[17] = SynthGuiCtrl(sw4,    0,  SynthGuiCtrl::SWITCH);

      map = new QSignalMapper(this);
      for (int i = 0; i < NUM_GUI_CONTROLLER; ++i) {
            map->setMapping(dctrl[i].editor, i);
            if (dctrl[i].type == SynthGuiCtrl::SLIDER)
                  connect((QSlider*)(dctrl[i].editor), SIGNAL(valueChanged(int)), map, SLOT(map()));
            else if (dctrl[i].type == SynthGuiCtrl::SWITCH)
                  connect((QCheckBox*)(dctrl[i].editor), SIGNAL(toggled(bool)), map, SLOT(map()));
            }
      connect(map, SIGNAL(mapped(int)), this, SLOT(ctrlChanged(int)));
      }

//---------------------------------------------------------
//   ctrlChanged
//---------------------------------------------------------

void OrganGui::ctrlChanged(int idx)
      {
      SynthGuiCtrl* ctrl = &dctrl[idx];
      int val = 0;
      if (ctrl->type == SynthGuiCtrl::SLIDER) {
            QSlider* slider = (QSlider*)(ctrl->editor);
            val = slider->value();
            }
      else if (ctrl->type == SynthGuiCtrl::SWITCH) {
            val = ((QCheckBox*)(ctrl->editor))->isChecked();
            }
      sendController(0, idx + CTRL_RPN14_OFFSET, val);
      }

//---------------------------------------------------------
//   setParam
//    set param in gui
//---------------------------------------------------------

void OrganGui::setParam(int param, int val)
      {
      param &= 0xfff;
      if (param >= int(sizeof(dctrl)/sizeof(*dctrl))) {
            fprintf(stderr, "OrganGui: set unknown Ctrl 0x%x to 0x%x\n", param, val);
            return;
            }
      SynthGuiCtrl* ctrl = &dctrl[param];
      ctrl->editor->blockSignals(true);
      if (ctrl->type == SynthGuiCtrl::SLIDER) {
            QSlider* slider = (QSlider*)(ctrl->editor);
            slider->setValue(val);
            if (ctrl->label)
                  ((QSpinBox*)(ctrl->label))->setValue(val);
            }
      else if (ctrl->type == SynthGuiCtrl::SWITCH) {
            ((QCheckBox*)(ctrl->editor))->setChecked(val);
            }
      ctrl->editor->blockSignals(false);
      }

//---------------------------------------------------------
//   processEvent
//---------------------------------------------------------

void OrganGui::processEvent(const MidiEvent& ev)
      {
      if (ev.type() == ME_CONTROLLER)
            setParam(ev.dataA(), ev.dataB());
      else
            printf("OrganGui::illegal event type received\n");
      }

//---------------------------------------------------------
//   readMessage
//---------------------------------------------------------

void OrganGui::readMessage(int)
      {
      MessGui::readMessage();
      }

