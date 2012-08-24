//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: organgui.cpp,v 1.16.2.3 2009/11/16 04:30:46 terminator356 Exp $
//
//    This is a simple GUI implemented with QT for
//    organ software synthesizer.
//
//  (C) Copyright 2001-2004 Werner Schweer (ws@seh.de)
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

#include <unistd.h>
#include <stdlib.h>
#include <list>

#include <QCheckBox>
#include <QSignalMapper>
#include <QSlider>
#include <QSocketNotifier>
#include <QSpinBox>

#include "common_defs.h"
#include "organgui.h"
#include "muse/midi.h"
#include "muse/midictrl.h"

//#define ORGANGUI_DEBUG

//---------------------------------------------------------
//   OrganGui
//---------------------------------------------------------

OrganGui::OrganGui()
   : QWidget(0, Qt::Window), MessGui()
      {
      setupUi(this);  // p4.0.17
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

      // work around for probable QT/WM interaction bug.
      // for certain window managers, e.g xfce, this window is
      // is displayed although not specifically set to show();
      // bug: 2811156  	 Softsynth GUI unclosable with XFCE4 (and a few others)
      show();
      hide();
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
            // By T356. Apply auto-bias center value.
            if(slider->minimum() < 0)
              val += 8192;
            }
      else if (ctrl->type == SynthGuiCtrl::SWITCH) {
            val = ((QCheckBox*)(ctrl->editor))->isChecked();
            }
      sendController(0, idx + MusECore::CTRL_RPN14_OFFSET, val);
      }

//---------------------------------------------------------
//   getControllerInfo
//    return min max values for controllers
//---------------------------------------------------------
int OrganGui::getControllerMinMax(int id, int* min, int* max) const
      {
      if (id >= NUM_GUI_CONTROLLER)
            return 0;

      const SynthGuiCtrl* ctrl = (const SynthGuiCtrl*)&dctrl[id];
      //int val = 0;
      if (ctrl->type == SynthGuiCtrl::SLIDER) {
            QSlider* slider = (QSlider*)(ctrl->editor);
            *max = slider->maximum();
            *min = slider->minimum();
            //val = (slider->value() * 16383 + max/2) / max;
            
            //val = 16383 + 1/2 
            }
      else if (ctrl->type == SynthGuiCtrl::SWITCH) {
            //val = ((QCheckBox*)(ctrl->editor))->isOn();
            *min=0;
            *max=1;
            }
      return ++id;
      }

//---------------------------------------------------------
//   setParam
//    set param in gui
//---------------------------------------------------------

void OrganGui::setParam(int param, int val)
      {
      #ifdef ORGANGUI_DEBUG
      fprintf(stderr, "OrganGui:setParam param:%d val:%d\n", param, val);
      #endif
      
      param &= 0xfff;
      if (param >= int(sizeof(dctrl)/sizeof(*dctrl))) {
            #ifdef ORGANGUI_DEBUG
            fprintf(stderr, "OrganGui: set unknown Ctrl 0x%x to 0x%x\n", param, val);
            #endif
            return;
            }
      SynthGuiCtrl* ctrl = &dctrl[param];
      ctrl->editor->blockSignals(true);
      if (ctrl->type == SynthGuiCtrl::SLIDER) {
            QSlider* slider = (QSlider*)(ctrl->editor);
//             int max = slider->maximum();
//             if(val < 0) val = (val * max + 8191) / 16383 - 1;
//             else val = (val * max + 8191) / 16383;
            
            // By T356. Apply auto-bias center value.
            if(slider->minimum() < 0)
              val -= 8192;
            
            #ifdef ORGANGUI_DEBUG
            fprintf(stderr, "OrganGui:setParam setting slider val:%d\n", val);
            #endif
            
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

void OrganGui::processEvent(const MusECore::MidiPlayEvent& ev)
      {
      if (ev.type() == MusECore::ME_CONTROLLER)
            setParam(ev.dataA(), ev.dataB());
      else
      {
            #ifdef ORGANGUI_DEBUG
            printf("OrganGui::illegal event type received\n");
            #endif
      }      
      }

//---------------------------------------------------------
//   readMessage
//---------------------------------------------------------

void OrganGui::readMessage(int)
      {
      MessGui::readMessage();
      }

