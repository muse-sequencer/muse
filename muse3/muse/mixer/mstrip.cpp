//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mstrip.cpp,v 1.9.2.13 2009/11/14 03:37:48 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Tim E. Real (terminator356 on sourceforge)
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

#include <fastlog.h>

#include <QLayout>
#include <QAction>
#include <QApplication>
//#include <QDialog>
#include <QToolButton>
#include <QLabel>
#include <QComboBox>
#include <QToolTip>
#include <QTimer>
//#include <QPopupMenu>
#include <QCursor>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QScrollArea>

#include <math.h>

#include "app.h"
#include "midi.h"
#include "midictrl.h"
#include "mstrip.h"
#include "midiport.h"
#include "globals.h"
#include "audio.h"
#include "song.h"
#include "slider.h"
#include "knob.h"
#include "combobox.h"
#include "meter.h"
#include "track.h"
#include "doublelabel.h"
#include "rack.h"
#include "node.h"
#include "amixer.h"
#include "icons.h"
#include "gconfig.h"
#include "ttoolbutton.h"
//#include "utils.h"
#include "popupmenu.h"
#include "routepopup.h"

#include "minstrument.h"
#include "midievent.h"
#include "compact_slider.h"
#include "compact_patch_edit.h"
#include "scroll_area.h"
#include "elided_label.h"
#include "utils.h"

#include "synth.h"
#ifdef LV2_SUPPORT
#include "lv2host.h"
#endif

namespace MusEGui {

// REMOVE Tim. Trackinfo. Added.
const int MidiStrip::xMarginHorSlider = 1;
const int MidiStrip::yMarginHorSlider = 1;
const int MidiStrip::upperRackSpacerHeight = 2;
const int MidiStrip::rackFrameWidth = 1;


// REMOVE Tim. Trackinfo. Changed. Moved into class.
//enum { KNOB_PAN = 0, KNOB_VAR_SEND, KNOB_REV_SEND, KNOB_CHO_SEND };

// REMOVE Tim. Trackinfo. Changed.
// //---------------------------------------------------------
// //   addKnob
// //---------------------------------------------------------
// 
// void MidiStrip::addKnob(int idx, const QString& tt, const QString& label,
//    const char* slot, bool enabled)
//       {
//       int ctl = MusECore::CTRL_PANPOT, mn, mx, v;
//       int chan  = ((MusECore::MidiTrack*)track)->outChannel();
//       switch(idx)
//       {
//         //case KNOB_PAN:
//         //  ctl = MusECore::CTRL_PANPOT;
//         //break;
//         case KNOB_VAR_SEND:
//           ctl = MusECore::CTRL_VARIATION_SEND;
//         break;
//         case KNOB_REV_SEND:
//           ctl = MusECore::CTRL_REVERB_SEND;
//         break;
//         case KNOB_CHO_SEND:
//           ctl = MusECore::CTRL_CHORUS_SEND;
//         break;
//       }
//       MusECore::MidiPort* mp = &MusEGlobal::midiPorts[((MusECore::MidiTrack*)track)->outPort()];
//       MusECore::MidiController* mc = mp->midiController(ctl);
//       mn = mc->minVal();
//       mx = mc->maxVal();
//       
//       MusEGui::Knob* knob = new MusEGui::Knob(this);
//       knob->setRange(double(mn), double(mx), 1.0);
//       knob->setId(ctl);
//       
//       controller[idx].knob = knob;
//       knob->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
//       knob->setBackgroundRole(QPalette::Mid);
//       knob->setToolTip(tt);
//       knob->setEnabled(enabled);
// 
//       MusEGui::DoubleLabel* dl = new MusEGui::DoubleLabel(0.0, double(mn), double(mx), this);
//       dl->setId(idx);
//       dl->setSpecialText(tr("off"));
//       dl->setToolTip(tr("ctrl-double-click on/off"));
//       controller[idx].dl = dl;
//       ///dl->setFont(MusEGlobal::config.fonts[1]);
//       dl->setBackgroundRole(QPalette::Mid);
//       dl->setFrame(true);
//       dl->setPrecision(0);
//       dl->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
//       dl->setEnabled(enabled);
// 
//       double dlv;
//       v = mp->hwCtrlState(chan, ctl);
//       if(v == MusECore::CTRL_VAL_UNKNOWN)
//       {
//         //v = mc->initVal();
//         //if(v == MusECore::CTRL_VAL_UNKNOWN)
//         //  v = 0;
// //        v = mn - 1;
//         int lastv = mp->lastValidHWCtrlState(chan, ctl);
//         if(lastv == MusECore::CTRL_VAL_UNKNOWN)
//         {
//           if(mc->initVal() == MusECore::CTRL_VAL_UNKNOWN)
//             v = 0;
//           else  
//             v = mc->initVal();
//         }
//         else  
//           v = lastv - mc->bias();
//         //dlv = mn - 1;
//         dlv = dl->off() - 1.0;
//       }  
//       else
//       {
//         // Auto bias...
//         v -= mc->bias();
//         dlv = double(v);
//       }
//       
//       knob->setValue(double(v));
//       dl->setValue(dlv);
//       //}
//       //else
//       //      knob->setRange(0.0, 127.0);
//       
//       QLabel* lb = new QLabel(label, this);
//       controller[idx].lb = lb;
//       ///lb->setFont(MusEGlobal::config.fonts[1]);
//       lb->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
//       lb->setAlignment(Qt::AlignCenter);
//       lb->setEnabled(enabled);
// 
//       grid->addWidget(lb, _curGridRow, 0);
//       grid->addWidget(dl, _curGridRow+1, 0);
//       grid->addWidget(knob, _curGridRow, 1, 2, 1);
//       _curGridRow += 2;
//       
//       connect(knob, SIGNAL(sliderMoved(double,int)), slot);
//       connect(knob, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(controlRightClicked(const QPoint &, int)));
//       connect(dl, SIGNAL(valueChanged(double, int)), slot);
//       connect(dl, SIGNAL(ctrlDoubleClicked(int)), SLOT(labelDoubleClicked(int)));
//       }

//---------------------------------------------------------
//   addController
//---------------------------------------------------------

void MidiStrip::addController(QVBoxLayout* rackLayout, ControlType idx, int midiCtrlNum, const QString& tt, const QString& label,
                              const char* slot, bool enabled, double initVal)
      {
      //int ctl = MusECore::CTRL_PANPOT, mn, mx, v;
      int mn, mx, v;
      const int chan  = ((MusECore::MidiTrack*)track)->outChannel();
//       switch(idx)
//       {
//         //case KNOB_PAN:
//         //  ctl = MusECore::CTRL_PANPOT;
//         //break;
//         case KNOB_VAR_SEND:
//           ctl = MusECore::CTRL_VARIATION_SEND;
//         break;
//         case KNOB_REV_SEND:
//           ctl = MusECore::CTRL_REVERB_SEND;
//         break;
//         case KNOB_CHO_SEND:
//           ctl = MusECore::CTRL_CHORUS_SEND;
//         break;
//         
//        // REMOVE Tim. Trackinfo. Added.
//         case KNOB_PROGRAM:
//           ctl = MusECore::CTRL_PROGRAM;
//         break;
//       }
      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[((MusECore::MidiTrack*)track)->outPort()];
      MusECore::MidiController* mc = mp->midiController(midiCtrlNum);
      mn = mc->minVal();
      mx = mc->maxVal();

// REMOVE Tim. Trackinfo. Added.
      if(midiCtrlNum == MusECore::CTRL_PROGRAM)
      {
//         CompactPatchEdit* control = new CompactPatchEdit(this, 0, Qt::Horizontal, CompactSlider::None);
        CompactPatchEdit* control = new CompactPatchEdit(0, 0, Qt::Horizontal, CompactSlider::None);
        control->setContentsMargins(0, 0, 0, 0);
        control->setId(midiCtrlNum);
        controller[idx]._patchControl = control;
        controller[idx]._cachedVal = initVal;
        control->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
//         control->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
//         control->setToolTip(tt); // Remove. Has its own tooltips.
        control->setEnabled(enabled);

        bool off = false;
        v = mp->hwCtrlState(chan, midiCtrlNum);
        if(v == MusECore::CTRL_VAL_UNKNOWN)
        {
          int lastv = mp->lastValidHWCtrlState(chan, midiCtrlNum);
          if(lastv == MusECore::CTRL_VAL_UNKNOWN)
          {
            if(mc->initVal() == MusECore::CTRL_VAL_UNKNOWN)
              v = 0;
            else  
              v = mc->initVal();
          }
          off = true;
        }  
        
        control->setValueState(double(v), off);

        rackLayout->addWidget(control);
   
        connect(control, SIGNAL(patchNameClicked()), SLOT(patchPopup()));
        connect(control, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(controlRightClicked(const QPoint &, int)));
        connect(control, SIGNAL(valueStateChanged(double,bool,int)), slot);
      }
      else
      {
        
      
//         CompactSlider* control = new CompactSlider(this, 0, Qt::Horizontal, CompactSlider::None, label);
        CompactSlider* control = new CompactSlider(0, 0, Qt::Horizontal, CompactSlider::None, label);
        control->setHasOffMode(true);
  //       CompactSlider* control = new CompactSlider(0, 0, Qt::Horizontal, CompactSlider::None, label);
        control->setContentsMargins(0, 0, 0, 0);
        control->setMargins(xMarginHorSlider, yMarginHorSlider);
        //control->setTextHighlightMode(CompactSlider::TextHighlightFocus);

  //       control->setFont(MusEGlobal::config.fonts[1]);
        control->setRange(double(mn), double(mx), 1.0);
        control->setValueDecimals(0);
        control->setId(midiCtrlNum);
        controller[idx]._control = control;
        controller[idx]._cachedVal = initVal;
        control->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
//         control->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
  //       control->setBackgroundRole(QPalette::Mid);
        control->setToolTip(tt);
        control->setEnabled(enabled);
        //control->setVisible(false); //

        bool off = false;
        v = mp->hwCtrlState(chan, midiCtrlNum);
        if(v == MusECore::CTRL_VAL_UNKNOWN)
        {
          //v = mc->initVal();
          //if(v == MusECore::CTRL_VAL_UNKNOWN)
          //  v = 0;
          //v = mn - 1;
          int lastv = mp->lastValidHWCtrlState(chan, midiCtrlNum);
          if(lastv == MusECore::CTRL_VAL_UNKNOWN)
          {
            if(mc->initVal() == MusECore::CTRL_VAL_UNKNOWN)
              v = 0;
            else  
              v = mc->initVal();
          }
          else  
            v = lastv - mc->bias();
          //dlv = mn - 1;
  //         dlv = dl->off() - 1.0;
          off = true;
        }  
        else
        {
          // Auto bias...
          v -= mc->bias();
  //         dlv = double(v);
        }
        
  //       knob->setValue(double(v));
  //       dl->setValue(dlv);
        control->setValueState(double(v), off);
        
        //}
        //else
        //      knob->setRange(0.0, 127.0);
        
  //       QLabel* lb = new QLabel(label, this);
  //       controller[idx].lb = lb;
  //       ///lb->setFont(MusEGlobal::config.fonts[1]);
  //       lb->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
  //       lb->setAlignment(Qt::AlignCenter);
  //       lb->setEnabled(enabled);

  //       grid->addWidget(lb, _curGridRow, 0);
  //       grid->addWidget(dl, _curGridRow+1, 0);
  //       grid->addWidget(knob, _curGridRow, 1, 2, 1);
  //       _curGridRow += 2;
        
  //       grid->addWidget(control, _curGridRow++, 0, 1, 2);
        rackLayout->addWidget(control); // REMOVE Tim. Trackinfo. Changed. TEST
        
  //       connect(knob, SIGNAL(sliderMoved(double,int)), slot);
  //       connect(knob, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(controlRightClicked(const QPoint &, int)));
  //       connect(dl, SIGNAL(valueChanged(double, int)), slot);
  //       connect(dl, SIGNAL(ctrlDoubleClicked(int)), SLOT(labelDoubleClicked(int)));
        //connect(control, SIGNAL(sliderMoved(double,int)), slot);
        connect(control, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(controlRightClicked(const QPoint &, int)));
        //connect(control, SIGNAL(valueChanged(double, int)), slot);
        connect(control, SIGNAL(valueStateChanged(double,bool,int)), slot);
        //connect(control, SIGNAL(ctrlDoubleClicked(int)), SLOT(labelDoubleClicked(int)));
      }
      
      }

CompactSlider* MidiStrip::addProperty(QVBoxLayout* rackLayout, 
                            PropertyType idx, 
                            const QString& toolTipText, 
                            const QString& label, 
                            const char* slot, 
                            bool enabled,
                            double min,
                            double max, 
                            double initVal)
{
//   CompactSlider* control = new CompactSlider(this, 0, Qt::Horizontal, CompactSlider::None, label);
  CompactSlider* control = new CompactSlider(0, 0, Qt::Horizontal, CompactSlider::None, label);
  control->setContentsMargins(0, 0, 0, 0);
  control->setMargins(xMarginHorSlider, yMarginHorSlider);
  //control->setTextHighlightMode(CompactSlider::TextHighlightFocus);

//       control->setFont(MusEGlobal::config.fonts[1]);
  control->setRange(min, max, 1.0);
  control->setValueDecimals(0);
  control->setId(idx);
//   controller[idx]._control = control;
//   controller[idx]._cachedVal = initVal;
  _properties[idx] = control;
  control->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
//   control->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
//       control->setBackgroundRole(QPalette::Mid);
  control->setToolTip(toolTipText);
  control->setEnabled(enabled);

  control->setValueState(initVal);
  
  rackLayout->addWidget(control); // REMOVE Tim. Trackinfo. Changed. TEST
  
  connect(control, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(propertyRightClicked(const QPoint &, int)));
  connect(control, SIGNAL(valueStateChanged(double,bool,int)), slot);
  
  return control;
}
      
      
// REMOVE Tim. Trackinfo. Changed.
// //---------------------------------------------------------
// //   MidiStrip
// //---------------------------------------------------------
// 
// MidiStrip::MidiStrip(QWidget* parent, MusECore::MidiTrack* t)
//    : Strip(parent, t)
//       {
//       inHeartBeat = true;
// 
//       // Set the whole strip's font, except for the label.    p4.0.45
//       setFont(MusEGlobal::config.fonts[1]);
//       
//       // Clear so the meters don't start off by showing stale values.
//       t->setActivity(0);
//       t->setLastActivity(0);
//       
//       volume      = MusECore::CTRL_VAL_UNKNOWN;
//       pan         = MusECore::CTRL_VAL_UNKNOWN;
//       variSend    = MusECore::CTRL_VAL_UNKNOWN;
//       chorusSend  = MusECore::CTRL_VAL_UNKNOWN;
//       reverbSend  = MusECore::CTRL_VAL_UNKNOWN;
//       
//       addKnob(KNOB_VAR_SEND, tr("VariationSend"), tr("Var"), SLOT(setVariSend(double)), false);
//       addKnob(KNOB_REV_SEND, tr("ReverbSend"), tr("Rev"), SLOT(setReverbSend(double)), false);
//       addKnob(KNOB_CHO_SEND, tr("ChorusSend"), tr("Cho"), SLOT(setChorusSend(double)), false);
//       ///int auxsSize = MusEGlobal::song->auxs()->size();
//       ///if (auxsSize)
//             //layout->addSpacing((STRIP_WIDTH/2 + 1) * auxsSize);
//             ///grid->addSpacing((STRIP_WIDTH/2 + 1) * auxsSize);  // ??
// 
//       //---------------------------------------------------
//       //    slider, label, meter
//       //---------------------------------------------------
// 
//       MusECore::MidiPort* mp = &MusEGlobal::midiPorts[t->outPort()];
//       MusECore::MidiController* mc = mp->midiController(MusECore::CTRL_VOLUME);
//       int chan  = t->outChannel();
//       int mn = mc->minVal();
//       int mx = mc->maxVal();
//       
//       slider = new MusEGui::Slider(this, "vol", Qt::Vertical, MusEGui::Slider::None,
//                           QColor(100, 255, 100));
//       slider->setCursorHoming(true);
//       slider->setRange(double(mn), double(mx), 1.0);
//       slider->setFixedWidth(20);
//       ///slider->setFont(MusEGlobal::config.fonts[1]);
//       slider->setId(MusECore::CTRL_VOLUME);
// 
//       meter[0] = new MusEGui::Meter(this, MusEGui::Meter::LinMeter);
//       meter[0]->setRange(0, 127.0);
//       meter[0]->setFixedWidth(15);
//       connect(meter[0], SIGNAL(mousePress()), this, SLOT(resetPeaks()));
//       
//       sliderGrid = new QGridLayout(); 
//       sliderGrid->setRowStretch(0, 100);
//       sliderGrid->addWidget(slider, 0, 0, Qt::AlignHCenter);
//       sliderGrid->addWidget(meter[0], 0, 1, Qt::AlignHCenter);
//       grid->addLayout(sliderGrid, _curGridRow++, 0, 1, 2); 
// 
//       sl = new MusEGui::DoubleLabel(0.0, -98.0, 0.0, this);
//       ///sl->setFont(MusEGlobal::config.fonts[1]);
//       sl->setBackgroundRole(QPalette::Mid);
//       sl->setSpecialText(tr("off"));
//       sl->setSuffix(tr("dB"));
//       sl->setToolTip(tr("ctrl-double-click on/off"));
//       sl->setFrame(true);
//       sl->setPrecision(0);
//       sl->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum));
//       // Set the label's slider 'buddy'.
//       sl->setSlider(slider);
//       
//       double dlv;
//       int v = mp->hwCtrlState(chan, MusECore::CTRL_VOLUME);
//       if(v == MusECore::CTRL_VAL_UNKNOWN)
//       {
//         int lastv = mp->lastValidHWCtrlState(chan, MusECore::CTRL_VOLUME);
//         if(lastv == MusECore::CTRL_VAL_UNKNOWN)
//         {
//           if(mc->initVal() == MusECore::CTRL_VAL_UNKNOWN)
//             v = 0;
//           else  
//             v = mc->initVal();
//         }
//         else  
//           v = lastv - mc->bias();
//         dlv = sl->off() - 1.0;
//       }  
//       else  
//       {
//         if(v == 0)
//           dlv = sl->minValue() - 0.5 * (sl->minValue() - sl->off());
//         else
//         {  
//           dlv = -MusECore::fast_log10(float(127*127)/float(v*v))*20.0;
//           if(dlv > sl->maxValue())
//             dlv = sl->maxValue();
//         }    
//         // Auto bias...
//         v -= mc->bias();
//       }      
//       slider->setValue(double(v));
//       sl->setValue(dlv);
//         
// 
// //      connect(sl, SIGNAL(valueChanged(double,int)), slider, SLOT(setValue(double)));
// //      connect(slider, SIGNAL(valueChanged(double,int)), sl, SLOT(setValue(double)));
//       connect(slider, SIGNAL(sliderMoved(double,int)), SLOT(setVolume(double)));
//       connect(slider, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(controlRightClicked(const QPoint &, int)));
//       connect(sl, SIGNAL(valueChanged(double, int)), SLOT(volLabelChanged(double)));
//       connect(sl, SIGNAL(ctrlDoubleClicked(int)), SLOT(labelDoubleClicked(int)));
//       
//       grid->addWidget(sl, _curGridRow++, 0, 1, 2, Qt::AlignCenter); 
// 
//       //---------------------------------------------------
//       //    pan, balance
//       //---------------------------------------------------
// 
//       addKnob(KNOB_PAN, tr("Pan/Balance"), tr("Pan"), SLOT(setPan(double)), true);
// 
//       updateControls();
//       
//       //---------------------------------------------------
//       //    mute, solo
//       //    or
//       //    record, mixdownfile
//       //---------------------------------------------------
// 
//       record  = new MusEGui::TransparentToolButton(this);
//       record->setFocusPolicy(Qt::NoFocus);
//       record->setBackgroundRole(QPalette::Mid);
//       record->setCheckable(true);
//       record->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
//       record->setToolTip(tr("record"));
//       record->setChecked(track->recordFlag());
//       record->setIcon(track->recordFlag() ? QIcon(*record_on_Icon) : QIcon(*record_off_Icon));
//       ///record->setIconSize(record_on_Icon->size());  
//       connect(record, SIGNAL(clicked(bool)), SLOT(recordToggled(bool)));
// 
//       mute  = new QToolButton();
//       mute->setFocusPolicy(Qt::NoFocus);
//       mute->setCheckable(true);
//       mute->setToolTip(tr("mute"));
//       mute->setChecked(track->mute());
//       mute->setIcon(track->mute() ? QIcon(*muteIconOff) : QIcon(*muteIconOn));
//       ///mute->setIconSize(muteIconOn->size());  
//       mute->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
//       connect(mute, SIGNAL(clicked(bool)), SLOT(muteToggled(bool)));
// 
//       solo  = new QToolButton();
//       solo->setFocusPolicy(Qt::NoFocus);
//       solo->setToolTip(tr("solo mode"));
//       solo->setCheckable(true);
//       solo->setChecked(track->solo());
//       solo->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
//       if(track->internalSolo())
//         solo->setIcon(track->solo() ? QIcon(*soloblksqIconOn) : QIcon(*soloblksqIconOff));
//       else
//         solo->setIcon(track->solo() ? QIcon(*soloIconOn) : QIcon(*soloIconOff));
//       ///solo->setIconSize(soloIconOn->size());  
//       connect(solo, SIGNAL(clicked(bool)), SLOT(soloToggled(bool)));
//       
//       off  = new MusEGui::TransparentToolButton(this);
//       off->setFocusPolicy(Qt::NoFocus);
//       off->setBackgroundRole(QPalette::Mid);
//       off->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
//       off->setCheckable(true);
//       off->setToolTip(tr("off"));
//       off->setChecked(track->off());
//       off->setIcon(track->off() ? QIcon(*exit1Icon) : QIcon(*exitIcon));
//       ///off->setIconSize(exit1Icon->size());  
//       connect(off, SIGNAL(clicked(bool)), SLOT(offToggled(bool)));
// 
//       grid->addWidget(off, _curGridRow, 0);
//       grid->addWidget(record, _curGridRow++, 1);
//       grid->addWidget(mute, _curGridRow, 0);
//       grid->addWidget(solo, _curGridRow++, 1);
// 
//       //---------------------------------------------------
//       //    routing
//       //---------------------------------------------------
// 
//       iR = new QToolButton();
//       iR->setFocusPolicy(Qt::NoFocus);
//       ///iR->setFont(MusEGlobal::config.fonts[1]);
//       iR->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum));
//       ///iR->setText(tr("iR"));
//       iR->setIcon(QIcon(*routesMidiInIcon));
//       iR->setIconSize(routesMidiInIcon->size());  
//       iR->setCheckable(false);
//       iR->setToolTip(tr("input routing"));
//       grid->addWidget(iR, _curGridRow, 0);
//       connect(iR, SIGNAL(pressed()), SLOT(iRoutePressed()));
//       
//       oR = new QToolButton();
//       oR->setFocusPolicy(Qt::NoFocus);
//       ///oR->setFont(MusEGlobal::config.fonts[1]);
//       oR->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum));
//       ///oR->setText(tr("oR"));
//       oR->setIcon(QIcon(*routesMidiOutIcon));
//       oR->setIconSize(routesMidiOutIcon->size());  
//       oR->setCheckable(false);
//       // TODO: Works OK, but disabled for now, until we figure out what to do about multiple out routes and display values...
//       // Enabled (for Midi Port to Audio Input routing). p4.0.14 Tim.
//       //oR->setEnabled(false);
//       oR->setToolTip(tr("output routing"));
//       grid->addWidget(oR, _curGridRow++, 1);
//       connect(oR, SIGNAL(pressed()), SLOT(oRoutePressed()));
// 
//       //---------------------------------------------------
//       //    automation mode
//       //---------------------------------------------------
// 
//       autoType = new MusEGui::ComboBox();
//       autoType->setFocusPolicy(Qt::NoFocus);
//       ///autoType->setFont(MusEGlobal::config.fonts[1]);
//       autoType->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
//       autoType->setEnabled(false);
//       
//       // Removed by T356. 
//       // Disabled for now. There is no midi automation mechanism yet...
//       //autoType->addAction(tr("Off"), AUTO_OFF);
//       //autoType->addAction(tr("Read"), AUTO_READ);
//       //autoType->addAction(tr("Touch"), AUTO_TOUCH);
//       //autoType->addAction(tr("Write"), AUTO_WRITE);
//       //autoType->setCurrentItem(t->automationType());
//       //autoType->setToolTip(tr("automation type"));      
//       //connect(autoType, SIGNAL(activated(int)), SLOT(setAutomationType(int)));
//       autoType->addAction(" ", AUTO_OFF);  // Just a dummy text to fix sizing problems. REMOVE later if full automation added.
//       autoType->setCurrentItem(AUTO_OFF);    //
// 
//       grid->addWidget(autoType, _curGridRow++, 0, 1, 2);
//       connect(MusEGlobal::heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
//       inHeartBeat = false;
//       }

//---------------------------------------------------------
//   MidiStrip
//---------------------------------------------------------

MidiStrip::MidiStrip(QWidget* parent, MusECore::MidiTrack* t)
   : Strip(parent, t)
      {
      inHeartBeat = true;
      _heartBeatCounter = 0;

      // Start the layout in mode A (normal, racks on left).
      _isExpanded = false;
      
      // Set the whole strip's font, except for the label.    p4.0.45
// REMOVE Tim. Trackinfo. Changed.  
//       setFont(MusEGlobal::config.fonts[1]);
      setStyleSheet(MusECore::font2StyleSheet(MusEGlobal::config.fonts[1]));
      
      // Clear so the meters don't start off by showing stale values.
      t->setActivity(0);
      t->setLastActivity(0);
      
      _preScrollAreaPos_A  = GridPosStruct(_curGridRow,     0, 1, 3);
      
      _preScrollAreaPos_B  = GridPosStruct(_curGridRow + 1, 2, 1, 1);
      _sliderPos           = GridPosStruct(_curGridRow + 1, 0, 4, 2);

      _infoSpacerTop       = GridPosStruct(_curGridRow + 2, 2, 1, 1);
      
      _propertyRackPos     = GridPosStruct(_curGridRow + 3, 2, 1, 1);
      
      _infoSpacerBottom    = GridPosStruct(_curGridRow + 4, 2, 1, 1);
      
      _sliderLabelPos      = GridPosStruct(_curGridRow + 5, 0, 1, 2);
      _postScrollAreaPos_B = GridPosStruct(_curGridRow + 5, 2, 1, 1);
      
      _postScrollAreaPos_A = GridPosStruct(_curGridRow + 6, 0, 1, 3);
      
      _offPos              = GridPosStruct(_curGridRow + 7, 0, 1, 1);
      _recPos              = GridPosStruct(_curGridRow + 7, 1, 1, 1);
      
      _mutePos             = GridPosStruct(_curGridRow + 8, 0, 1, 1);
      _soloPos             = GridPosStruct(_curGridRow + 8, 1, 1, 1);
      
      //_inRoutesPos         = GridPosStruct(_curGridRow + 9, 0, 1, 1);
      //_outRoutesPos        = GridPosStruct(_curGridRow + 9, 1, 1, 1);
      _routesPos        = GridPosStruct(_curGridRow + 9, 0, 1, 2);
      
      _automationPos       = GridPosStruct(_curGridRow + 10, 0, 1, 2);
      
      _rightSpacerPos      = GridPosStruct(_curGridRow + 10, 2, 1, 1);

      volume      = MusECore::CTRL_VAL_UNKNOWN;
// REMOVE Tim. Trackinfo. Removed.
//       pan         = MusECore::CTRL_VAL_UNKNOWN;
//       variSend    = MusECore::CTRL_VAL_UNKNOWN;
//       chorusSend  = MusECore::CTRL_VAL_UNKNOWN;
//       reverbSend  = MusECore::CTRL_VAL_UNKNOWN;
// REMOVE Tim. Trackinfo. Added.
//       program     = MusECore::CTRL_VAL_UNKNOWN;

      
// REMOVE Tim. Trackinfo. Added.
//       _infoRack = new QFrame(this);
      _infoRack = new QFrame();
//       _infoRack->setVisible(false); // Not visible unless expanded.
      
      // FIXME For some reason StyledPanel has trouble, intermittent sometimes panel is drawn, sometimes not. 
//       _infoRack->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
      _infoRack->setFrameStyle(QFrame::Box | QFrame::Sunken);
      
      _infoRack->setLineWidth(rackFrameWidth);
      _infoRack->setMidLineWidth(0);
//       _infoRack->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
      _infoRack->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
//       _infoRack->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
      _infoRack->setContentsMargins(rackFrameWidth, rackFrameWidth, rackFrameWidth, rackFrameWidth);

      _infoLayout = new QVBoxLayout(_infoRack);
      _infoLayout->setSpacing(0);
      _infoLayout->setContentsMargins(0, 0, 0, 0);
//       _infoLayout->setContentsMargins(rackFrameWidth, rackFrameWidth, rackFrameWidth, rackFrameWidth);

      addProperty(_infoLayout, PropertyTransp, tr("Transpose notes up or down"), 
                  tr("Tran"), SLOT(propertyChanged(double, bool, int)), true, -127, 127, t->transposition);
      
      addProperty(_infoLayout, PropertyDelay, tr("Offset playback of notes before or after actual note"), 
                  tr("Dly"), SLOT(propertyChanged(double, bool, int)), true, -1000, 1000, t->delay);
      
      addProperty(_infoLayout, PropertyLen, tr("Change note length in percent of actual length"), 
                  tr("Len"), SLOT(propertyChanged(double, bool, int)), true, 25, 200, t->len)->setValSuffix("%");
      
      addProperty(_infoLayout, PropertyVelo, tr("<html><head/><body><p>Add or substract velocity to notes"
                                     " on track.</p><p><span style="" font-style:italic;"">Since"
                                     " the midi note range is 0-127 this <br/>might mean that the"
                                     " notes do not reach <br/>the combined velocity, note + "
                                     " Velocity.</span></p></body></html>"), 
                  tr("Vel"), SLOT(propertyChanged(double, bool, int)), true, -127, 127, t->velocity);

      addProperty(_infoLayout, PropertyCompr, tr("Compress the notes velocity range, in percent of actual velocity"), 
                  tr("Cmp"), SLOT(propertyChanged(double, bool, int)), true, 25, 200, t->compression)->setValSuffix("%");

      _infoLayout->addStretch();
      addGridWidget(_infoRack, _propertyRackPos);
                  
//       grid->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Expanding), 
      grid->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::Expanding), 
                    _infoSpacerTop._row, _infoSpacerTop._col, _infoSpacerTop._rowSpan, _infoSpacerTop._colSpan);

      
//       QWidget* upperScrollW = new QWidget(this);
//       _upperRack = new QFrame(this);
      _upperRack = new QFrame();
      
      // FIXME For some reason StyledPanel has trouble, intermittent sometimes panel is drawn, sometimes not. 
//       _upperRack->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
      _upperRack->setFrameStyle(QFrame::Box | QFrame::Sunken);
      
      _upperRack->setLineWidth(rackFrameWidth);
      _upperRack->setMidLineWidth(0);
      // We do set a minimum height on this widget. Tested: Must be on fixed. Thankfully, it'll expand if more controls are added.
      _upperRack->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
//       _upperRack->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
      _upperRack->setContentsMargins(rackFrameWidth, rackFrameWidth, rackFrameWidth, rackFrameWidth);

      
//       _upperScrollLayout = new QVBoxLayout(upperScrollW);
      _upperScrollLayout = new QVBoxLayout(_upperRack);
//       _upperScrollLayout = new QVBoxLayout(this);
//       _upperScrollLayout = new RackLayout();
      _upperScrollLayout->setSpacing(0);
      _upperScrollLayout->setContentsMargins(0, 0, 0, 0);
      //_upperScrollLayout->setSizeConstraint(QLayout::SetNoConstraint);
      //upperScrollW->setLayout(_upperScrollLayout);
      //addGridWidget(_upperRack, _preScrollAreaPos_A);

// REMOVE Tim. Trackinfo. Added.      
//       _instrLabel = new ElidedLabel(this, Qt::ElideMiddle);
      _instrLabel = new ElidedLabel(0, Qt::ElideMiddle);
      _instrLabel->setObjectName("MidiStripInstrumentLabel");
      _instrLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
//       _instrLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
      _instrLabel->setContentsMargins(0, 0, 0, 0);
      _instrLabel->setToolTip(tr("Instrument"));
      if(MusECore::MidiInstrument* minstr = MusEGlobal::midiPorts[t->outPort()].instrument())
      {
        _instrLabel->setText(minstr->iname());
        if(minstr->isSynti())
          _instrLabel->setEnabled(false);
        else
          _instrLabel->setEnabled(true);
      }
      else
        _instrLabel->setText(tr("<unknown>"));
      _upperScrollLayout->addWidget(_instrLabel);
      connect(_instrLabel, SIGNAL(pressed()), SLOT(instrPopup()));
      
// REMOVE Tim. Trackinfo. Added.
      addController(_upperScrollLayout, KNOB_PROGRAM, MusECore::CTRL_PROGRAM, tr("Program"), tr("Prg"), 
                    SLOT(ctrlChanged(double, bool, int)), false, MusECore::CTRL_VAL_UNKNOWN);
      _upperScrollLayout->addSpacing(upperRackSpacerHeight);
      
// REMOVE Tim. Trackinfo. Changed.
//       addKnob(KNOB_VAR_SEND, tr("VariationSend"), tr("Var"), SLOT(setVariSend(double)), false);
//       addKnob(KNOB_REV_SEND, tr("ReverbSend"), tr("Rev"), SLOT(setReverbSend(double)), false);
//       addKnob(KNOB_CHO_SEND, tr("ChorusSend"), tr("Cho"), SLOT(setChorusSend(double)), false);
      addController(_upperScrollLayout, KNOB_VAR_SEND, MusECore::CTRL_VARIATION_SEND, tr("VariationSend"), tr("Var"),
                    SLOT(ctrlChanged(double, bool, int)), false, MusECore::CTRL_VAL_UNKNOWN);
      addController(_upperScrollLayout, KNOB_REV_SEND, MusECore::CTRL_REVERB_SEND, tr("ReverbSend"), tr("Rev"),
                    SLOT(ctrlChanged(double, bool, int)), false, MusECore::CTRL_VAL_UNKNOWN);
      addController(_upperScrollLayout, KNOB_CHO_SEND, MusECore::CTRL_CHORUS_SEND, tr("ChorusSend"), tr("Cho"),
                    SLOT(ctrlChanged(double, bool, int)), false, MusECore::CTRL_VAL_UNKNOWN);
//       controller[KNOB_PROGRAM]._patchControl = 0;
      
      ///int auxsSize = MusEGlobal::song->auxs()->size();
      ///if (auxsSize)
            //layout->addSpacing((STRIP_WIDTH/2 + 1) * auxsSize);
            ///grid->addSpacing((STRIP_WIDTH/2 + 1) * auxsSize);  // ??

      // REMOVE Tim. Trackinfo. Just a test.
//       QSpinBox* spinbox = new QSpinBox(this);
//       spinbox->setContentsMargins(0, 0, 0, 0);
//       spinbox->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum));
//       _upperScrollLayout->addWidget(spinbox);
      
// Keep this if dynamic layout (flip to right side) is desired.
      _upperScrollLayout->addStretch();
//       _upperScrollLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));

      
      
//       _upperScrollArea = new ScrollArea(this); // Must give parent so min height can be found.
// //       _upperScrollArea = new CompactControllerRack(this, 3); // Must give parent so min height can be found.
//       //_upperScrollArea = new CompactControllerRack(this);
//       _upperScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//       _upperScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
//       
//       // Must set to Ignored otherwise it refuses to shrink below a certain amount (too much for us!)
//       // Minimum height is also set in ::resizeEvent(). Width is ignored, it is floating.
// //       _upperScrollArea->setMinimumHeight(_upperScrollLayout->minimumSize().height());
//       //_upperScrollArea->setMinimumHeight(70);
// //       _upperScrollArea->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum));
//       _upperScrollArea->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
//        
//       _upperScrollArea->setWidget(upperScrollW);
//       _upperScrollArea->setWidgetResizable(true);
//       //_upperScrollArea->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
//       _upperScrollArea->setContentsMargins(0, 0, 0, 0);
//       //_upperScrollArea->setMinItems(3);
// //       _upperScrollArea->setItemMargins(1, 1);
// //       _upperScrollArea->setFixedHeight(3 * CompactSlider::getMinimumSizeHint(fontMetrics(), 
// //                                                                              Qt::Horizontal, 
// //                                                                              CompactSlider::None, 
// //                                                                              1, 1).height() + 2 * _upperScrollArea->frameWidth());
      updateRackSizes(true, false);

      
      //grid->addLayout(_upperScrollLayout, _curGridRow++, 0, 1, 3);  // REMOVE Tim. Trackinfo. Changed. TEST
      //grid->addWidget(_upperScrollArea, _curGridRow++, 0, 1, 3);  // REMOVE Tim. Trackinfo. Changed. TEST
//       addGridWidget(_upperScrollArea, _preScrollAreaPos_A);
      //addGridLayout(_upperScrollLayout, _preScrollAreaPos_A);
      
      addGridWidget(_upperRack, _preScrollAreaPos_A);
      
      //---------------------------------------------------
      //    slider, label, meter
      //---------------------------------------------------

      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[t->outPort()];
      MusECore::MidiController* mc = mp->midiController(MusECore::CTRL_VOLUME);
      int chan  = t->outChannel();
      int mn = mc->minVal();
      int mx = mc->maxVal();
      
// REMOVE Tim. Trackinfo. Changed.      
//       slider = new MusEGui::Slider(this, "vol", Qt::Vertical, MusEGui::Slider::None,
//       slider = new MusEGui::Slider(this, "vol", Qt::Vertical, MusEGui::Slider::InsideVertical, 14,
//       slider = new Slider(0, "vol", Qt::Vertical, MusEGui::Slider::InsideVertical, 14, QColor(100, 255, 100));
//       slider = new Slider(0, "vol", Qt::Vertical, MusEGui::Slider::InsideVertical, 14, QColor(62, 37, 255));
      slider = new Slider(0, "vol", Qt::Vertical, Slider::InsideVertical, 14, QColor(128, 128, 255), ScaleDraw::TextHighlightSplitAndShadow);
      slider->setContentsMargins(0, 0, 0, 0);  // REMOVE Tim. Trackinfo. Changed. TEST
      slider->setCursorHoming(true);
      slider->setThumbLength(1);
      slider->setRange(double(mn), double(mx), 1.0);
//       slider->setScaleMaxMinor(5);
      slider->setScale(double(mn), double(mx), 10.0, false);
      slider->setScaleBackBone(false);
      
// REMOVE Tim. Trackinfo. Changed.      
//       slider->setFixedWidth(20);
//       QFont fnt = font();
//       fnt.setPointSize(8);
//       QFont fnt;
//       fnt.setFamily("Sans");
//       fnt.setPointSize(font().pointSize());
//       slider->setFont(fnt);
      
//       slider->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
      slider->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
      
      ///slider->setFont(MusEGlobal::config.fonts[1]);
      slider->setId(MusECore::CTRL_VOLUME);
      //slider->setVisible(false);  // REMOVE Tim. Trackinfo. Changed. TEST

//       meter[0] = new MusEGui::Meter(this, MusEGui::Meter::LinMeter);
      meter[0] = new MusEGui::Meter(0, MusEGui::Meter::LinMeter);
      meter[0]->setContentsMargins(0, 0, 0, 0);  // REMOVE Tim. Trackinfo. Changed. TEST
      meter[0]->setRange(0, 127.0);
//       meter[0]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
      meter[0]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
// REMOVE Tim. Trackinfo. Changed.      
//       meter[0]->setFixedWidth(15);
      meter[0]->setFixedWidth(FIXED_METER_WIDTH);
      connect(meter[0], SIGNAL(mousePress()), this, SLOT(resetPeaks()));
      
      sliderGrid = new QGridLayout(); 
      sliderGrid->setSpacing(0);  // REMOVE Tim. Trackinfo. Changed. TEST
      //sliderGrid->setHorizontalSpacing(0);  // REMOVE Tim. Trackinfo. Changed. TEST
      sliderGrid->setContentsMargins(0, 0, 0, 0);  // REMOVE Tim. Trackinfo. Changed. TEST
//       sliderGrid->setRowStretch(0, 100);
      sliderGrid->addWidget(slider, 0, 0, Qt::AlignHCenter);
      sliderGrid->addWidget(meter[0], 0, 1, Qt::AlignHCenter);
      
//       grid->addLayout(sliderGrid, _curGridRow++, 0, 1, 2); 
//       grid->addLayout(sliderGrid, _curGridRow++, 0, 1, 2); // REMOVE Tim. Trackinfo. Changed. TEST
      addGridLayout(sliderGrid, _sliderPos); // REMOVE Tim. Trackinfo. Changed. TEST

//       sl = new MusEGui::DoubleLabel(0.0, -98.0, 0.0, this);
      sl = new MusEGui::DoubleLabel(0.0, -98.0, 0.0);
      sl->setContentsMargins(0, 0, 0, 0);  // REMOVE Tim. Trackinfo. Changed. TEST
      ///sl->setFont(MusEGlobal::config.fonts[1]);
      sl->setBackgroundRole(QPalette::Mid);
      sl->setSpecialText(tr("off"));
      sl->setSuffix(tr("dB"));
      sl->setToolTip(tr("ctrl-double-click on/off"));
      sl->setFrame(true);
      sl->setPrecision(0);
      sl->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
      //sl->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum));
      // Set the label's slider 'buddy'.
      sl->setSlider(slider);
      
      double dlv;
      int v = mp->hwCtrlState(chan, MusECore::CTRL_VOLUME);
      if(v == MusECore::CTRL_VAL_UNKNOWN)
      {
        int lastv = mp->lastValidHWCtrlState(chan, MusECore::CTRL_VOLUME);
        if(lastv == MusECore::CTRL_VAL_UNKNOWN)
        {
          if(mc->initVal() == MusECore::CTRL_VAL_UNKNOWN)
            v = 0;
          else  
            v = mc->initVal();
        }
        else  
          v = lastv - mc->bias();
        dlv = sl->off() - 1.0;
      }  
      else  
      {
        if(v == 0)
          dlv = sl->minValue() - 0.5 * (sl->minValue() - sl->off());
        else
        {  
          dlv = -MusECore::fast_log10(float(127*127)/float(v*v))*20.0;
          if(dlv > sl->maxValue())
            dlv = sl->maxValue();
        }    
        // Auto bias...
        v -= mc->bias();
      }      
      slider->setValue(double(v));
      sl->setValue(dlv);
      //sl->setVisible(false);  // REMOVE Tim. Trackinfo. Changed. TEST
        
//       grid->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Expanding), 
      grid->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::Expanding), 
                    _infoSpacerBottom._row, _infoSpacerBottom._col, _infoSpacerBottom._rowSpan, _infoSpacerBottom._colSpan);

//      connect(sl, SIGNAL(valueChanged(double,int)), slider, SLOT(setValue(double)));
//      connect(slider, SIGNAL(valueChanged(double,int)), sl, SLOT(setValue(double)));
      connect(slider, SIGNAL(sliderMoved(double,int)), SLOT(setVolume(double)));
      connect(slider, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(controlRightClicked(const QPoint &, int)));
      connect(sl, SIGNAL(valueChanged(double, int)), SLOT(volLabelChanged(double)));
//       connect(sl, SIGNAL(ctrlDoubleClicked(int)), SLOT(labelDoubleClicked(int)));
      connect(sl, SIGNAL(ctrlDoubleClicked(int)), SLOT(volLabelDoubleClicked()));
      
//       grid->addWidget(sl, _curGridRow++, 0, 1, 2, Qt::AlignCenter); 
//       grid->addWidget(sl, _curGridRow++, 0, 1, 2, Qt::AlignCenter);  // REMOVE Tim. Trackinfo. Changed. TEST
      addGridWidget(sl, _sliderLabelPos, Qt::AlignCenter);  // REMOVE Tim. Trackinfo. Changed. TEST

      //---------------------------------------------------
      //    pan, balance
      //---------------------------------------------------

// REMOVE Tim. Trackinfo. Changed. TEST      
//       addKnob(KNOB_PAN, tr("Pan/Balance"), tr("Pan"), SLOT(setPan(double)), true);

      
//       QWidget* lowerScrollW = new QWidget(this);
//       _lowerRack = new QFrame(this);
      _lowerRack = new QFrame();
      
      // FIXME For some reason StyledPanel has trouble, intermittent sometimes panel is drawn, sometimes not. 
//       _lowerRack->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
      _lowerRack->setFrameStyle(QFrame::Box | QFrame::Sunken);
      
      _lowerRack->setLineWidth(rackFrameWidth);
      _lowerRack->setMidLineWidth(0);
      // We do set a minimum height on this widget. Tested: Must be on fixed. Thankfully, it'll expand if more controls are added.
      _lowerRack->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
//       _lowerRack->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
      _lowerRack->setContentsMargins(rackFrameWidth, rackFrameWidth, rackFrameWidth, rackFrameWidth);

      
//       _lowerScrollLayout = new QVBoxLayout(lowerScrollW);
      _lowerScrollLayout = new QVBoxLayout(_lowerRack);
      //_lowerScrollLayout = new RackLayout();
      _lowerScrollLayout->setSpacing(0);
      _lowerScrollLayout->setContentsMargins(0, 0, 0, 0);
      //_lowerScrollLayout->setSizeConstraint(QLayout::SetNoConstraint);
      
//        addController(_lowerScrollLayout, KNOB_PAN, tr("Pan/Balance"), tr("Pan"), SLOT(setPan(double, bool)), true, MusECore::CTRL_VAL_UNKNOWN);
      addController(_lowerScrollLayout, KNOB_PAN, MusECore::CTRL_PANPOT, tr("Pan/Balance"), tr("Pan"), 
                    SLOT(ctrlChanged(double, bool, int)), true, MusECore::CTRL_VAL_UNKNOWN);

// Keep this if dynamic layout (flip to right side) is desired.
       _lowerScrollLayout->addStretch();
//       _lowerScrollLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
       

      
      
//       _lowerScrollArea = new ScrollArea(this);
// //       _lowerScrollArea = new CompactControllerRack(this, 1); // Must give parent so min height can be found.
//       //_lowerScrollArea = new CompactControllerRack(this);
//       _lowerScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//       _lowerScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
// 
//       // Must set to Ignored otherwise it refuses to shrink below a certain amount (too much for us!)
//       // Minimum height is also set in ::resizeEvent(). Width is ignored, it is floating.
// //       _lowerScrollArea->setMinimumHeight(_lowerScrollLayout->minimumSize().height());
// //       _lowerScrollArea->setMinimumHeight(24);
// //       _lowerScrollArea->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum));
//       _lowerScrollArea->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
//        
//       _lowerScrollArea->setWidget(lowerScrollW);
//       _lowerScrollArea->setWidgetResizable(true);
//       //_lowerScrollArea->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
//       _lowerScrollArea->setContentsMargins(0, 0, 0, 0);
//       //_lowerScrollArea->setMinItems(1);
// //       _lowerScrollArea->setItemMargins(1, 1);
//       //grid->addWidget(_lowerScrollArea, _curGridRow++, 0, 1, 3);
// //       _lowerScrollArea->setFixedHeight(1 * CompactSlider::getMinimumSizeHint(fontMetrics(), 
// //                                                                              Qt::Horizontal, 
// //                                                                              CompactSlider::None, 
// //                                                                              1, 1).height() + 2 * _lowerScrollArea->frameWidth());
      
      
      updateRackSizes(false, true);
//       addGridWidget(_lowerScrollArea, _postScrollAreaPos_A);
//       addGridLayout(_lowerScrollLayout, _postScrollAreaPos_A);
      addGridWidget(_lowerRack, _postScrollAreaPos_A);
      
      
      
      updateControls();
      
      //---------------------------------------------------
      //    mute, solo
      //    or
      //    record, mixdownfile
      //---------------------------------------------------

//       record  = new MusEGui::TransparentToolButton(this);  // REMOVE Tim. Trackinfo. Changed. TEST
//       record  = new CompactToolButton(0, track->recordFlag() ? record_on_Icon : record_off_Icon);
      record  = new CompactToolButton();
      record->setFocusPolicy(Qt::NoFocus);
//       record->setBackgroundRole(QPalette::Mid);  // REMOVE Tim. Trackinfo. Removed. TEST
      record->setCheckable(true);
      record->setContentsMargins(0, 0, 0, 0);  // REMOVE Tim. Trackinfo. Changed. TEST
//       record->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
      record->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum); // REMOVE Tim. Trackinfo. Changed. TEST
      record->setToolTip(tr("record"));
      QIcon rec_icon(*record_off_Icon);
      rec_icon.addPixmap(*record_on_Icon, QIcon::Normal, QIcon::On);
      record->setIcon(rec_icon);
      record->setIconSize(record_off_Icon->size());  
      record->setChecked(track->recordFlag());
//       record->setIcon(track->recordFlag() ? QIcon(*record_on_Icon) : QIcon(*record_off_Icon));
//       record->setVisible(false);  // REMOVE Tim. Trackinfo. Changed. TEST
      connect(record, SIGNAL(clicked(bool)), SLOT(recordToggled(bool)));

//       mute  = new QToolButton(); // REMOVE Tim. Trackinfo. Changed. TEST
//       mute  = new CompactToolButton(0, track->mute() ? muteIconOff : muteIconOn);
      mute  = new CompactToolButton();
      mute->setFocusPolicy(Qt::NoFocus);
      mute->setCheckable(true);
      mute->setContentsMargins(0, 0, 0, 0);  // REMOVE Tim. Trackinfo. Changed. TEST
      mute->setToolTip(tr("mute"));
      QIcon mute_icon(*muteIconOn);
      mute_icon.addPixmap(*muteIconOff, QIcon::Normal, QIcon::On);
      mute->setIcon(mute_icon);
      mute->setIconSize(muteIconOn->size());  
      mute->setChecked(track->mute());
//       mute->setIcon(track->mute() ? QIcon(*muteIconOff) : QIcon(*muteIconOn));
//       mute->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
      mute->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum); // REMOVE Tim. Trackinfo. Changed. TEST
//       mute->setVisible(false);  // REMOVE Tim. Trackinfo. Changed. TEST
      connect(mute, SIGNAL(clicked(bool)), SLOT(muteToggled(bool)));

//       solo  = new QToolButton();
      solo  = new CompactToolButton();
      solo->setFocusPolicy(Qt::NoFocus);
      solo->setToolTip(tr("solo mode"));
      solo->setContentsMargins(0, 0, 0, 0);  // REMOVE Tim. Trackinfo. Changed. TEST
      solo->setCheckable(true);
      QIcon solo_icon_A(*soloIconOff);
      solo_icon_A.addPixmap(*soloIconOn, QIcon::Normal, QIcon::On);
      solo->setIcon(solo_icon_A);
      QIcon solo_icon_B(*soloblksqIconOff);
      solo_icon_B.addPixmap(*soloblksqIconOn, QIcon::Normal, QIcon::On);
//       solo->setIcon(solo_icon_B);
      solo->setIconSize(soloIconOff->size());  
      solo->setChecked(track->solo());
//       solo->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
      solo->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum); // REMOVE Tim. Trackinfo. Changed. TEST
//       if(track->internalSolo())
//         solo->setIcon(track->solo() ? QIcon(*soloblksqIconOn) : QIcon(*soloblksqIconOff));
//       else
//         solo->setIcon(track->solo() ? QIcon(*soloIconOn) : QIcon(*soloIconOff));
      ///solo->setIconSize(soloIconOn->size());  
//       solo->setVisible(false);  // REMOVE Tim. Trackinfo. Changed. TEST
      connect(solo, SIGNAL(clicked(bool)), SLOT(soloToggled(bool)));
      
//       off  = new MusEGui::TransparentToolButton(this);
      off  = new CompactToolButton();
//       off  = new CompactToolButton(0, track->off() ? exit1Icon : exitIcon);
      off->setContentsMargins(0, 0, 0, 0);  // REMOVE Tim. Trackinfo. Changed. TEST
      off->setFocusPolicy(Qt::NoFocus);
//       off->setBackgroundRole(QPalette::Mid); // REMOVE Tim. Trackinfo. Removed. TEST
//       off->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
      off->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum); // REMOVE Tim. Trackinfo. Changed. TEST
      off->setCheckable(true);
      off->setToolTip(tr("off"));
      QIcon off_icon(*exitIcon);
      off_icon.addPixmap(*exit1Icon, QIcon::Normal, QIcon::On);
      off->setIcon(off_icon);
      off->setIconSize(exitIcon->size());  
      off->setChecked(track->off());
//       off->setIcon(track->off() ? QIcon(*exit1Icon) : QIcon(*exitIcon));
//       off->setVisible(false);  // REMOVE Tim. Trackinfo. Changed. TEST
      connect(off, SIGNAL(clicked(bool)), SLOT(offToggled(bool)));

//       grid->addWidget(off, _curGridRow, 0);
//       grid->addWidget(record, _curGridRow++, 1);
//       grid->addWidget(mute, _curGridRow, 0);
//       grid->addWidget(solo, _curGridRow++, 1);
// REMOVE Tim. Trackinfo. Changed. TEST
      addGridWidget(off, _offPos);
      addGridWidget(record, _recPos);
      addGridWidget(mute, _mutePos);
      addGridWidget(solo, _soloPos);
      
      //---------------------------------------------------
      //    routing
      //---------------------------------------------------

//       iR = new QToolButton();  // REMOVE Tim. Trackinfo. Changed. TEST
      iR = new CompactToolButton(0, *routesMidiInIcon);
      iR->setContentsMargins(0, 0, 0, 0);  // REMOVE Tim. Trackinfo. Changed. TEST
      iR->setFocusPolicy(Qt::NoFocus);
      ///iR->setFont(MusEGlobal::config.fonts[1]);
//       iR->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum));
//       iR->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum)); // REMOVE Tim. Trackinfo. Changed. TEST
      iR->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum); // REMOVE Tim. Trackinfo. Changed. TEST
      ///iR->setText(tr("iR"));
//       iR->setIcon(QIcon(*routesMidiInIcon)); // REMOVE Tim. Trackinfo. Changed. TEST
      // Give it a wee bit more height.
      iR->setIconSize(QSize(routesMidiInIcon->width(), routesMidiInIcon->height() + 5)); // REMOVE Tim. Trackinfo. Changed. TEST
      //iR->setFixedSize(iR->iconSize());
      iR->setCheckable(false);
      iR->setToolTip(tr("input routing"));
      //iR->setVisible(false);  // REMOVE Tim. Trackinfo. Changed. TEST
//       grid->addWidget(iR, _curGridRow, 0);
//       grid->addWidget(iR, _curGridRow, 0); // REMOVE Tim. Trackinfo. Changed. TEST
//       addGridWidget(iR, _inRoutesPos); // REMOVE Tim. Trackinfo. Changed. TEST
      connect(iR, SIGNAL(pressed()), SLOT(iRoutePressed()));
      
//       oR = new QToolButton(); // REMOVE Tim. Trackinfo. Changed. TEST
      oR = new CompactToolButton(0, *routesMidiOutIcon);
      oR->setContentsMargins(0, 0, 0, 0);  // REMOVE Tim. Trackinfo. Changed. TEST
      oR->setFocusPolicy(Qt::NoFocus);
      ///oR->setFont(MusEGlobal::config.fonts[1]);
//       oR->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum));
//       oR->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
      oR->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
      // REMOVE Tim. Trackinfo. Changed. TEST
      ///oR->setText(tr("oR"));
//       oR->setIcon(QIcon(*routesMidiOutIcon)); // REMOVE Tim. Trackinfo. Changed. TEST
      // Give it a wee bit more height.
      oR->setIconSize(QSize(routesMidiOutIcon->width(), routesMidiOutIcon->height() + 5));  // REMOVE Tim. Trackinfo. Changed. TEST
      //oR->setFixedSize(oR->iconSize());
      oR->setCheckable(false);
      // TODO: Works OK, but disabled for now, until we figure out what to do about multiple out routes and display values...
      // Enabled (for Midi Port to Audio Input routing). p4.0.14 Tim.
      //oR->setEnabled(false);
      oR->setToolTip(tr("output routing"));
      //oR->setVisible(false);  // REMOVE Tim. Trackinfo. Changed. TEST
//       grid->addWidget(oR, _curGridRow++, 1);
//       grid->addWidget(oR, _curGridRow++, 1); // REMOVE Tim. Trackinfo. Changed. TEST
//       addGridWidget(oR, _outRoutesPos); // REMOVE Tim. Trackinfo. Changed. TEST
      connect(oR, SIGNAL(pressed()), SLOT(oRoutePressed()));
   
      // REMOVE Tim. Trackinfo. Added. TEST
      _midiThru = new CompactToolButton();
      _midiThru->setFocusPolicy(Qt::NoFocus);
      _midiThru->setContentsMargins(0, 0, 0, 0);
      _midiThru->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
      _midiThru->setCheckable(true);
      _midiThru->setToolTip(tr("midi thru"));
      _midiThru->setWhatsThis(tr("Pass input events through ('thru') to output"));
      QIcon thruIcon(*midiThruOffIcon);
      thruIcon.addPixmap(*midiThruOnIcon, QIcon::Normal, QIcon::On);
      _midiThru->setIcon(thruIcon);
      _midiThru->setIconSize(midiThruOffIcon->size());  
      _midiThru->setChecked(t->recEcho());
      connect(_midiThru, SIGNAL(toggled(bool)), SLOT(midiThruToggled(bool)));
      QHBoxLayout* routesLayout = new QHBoxLayout();
      routesLayout->setContentsMargins(0, 0, 0, 0);
      routesLayout->setSpacing(0);
      routesLayout->addWidget(iR);
      routesLayout->addWidget(_midiThru);
      routesLayout->addWidget(oR);
      addGridLayout(routesLayout, _routesPos); 
      
      //---------------------------------------------------
      //    automation mode
      //---------------------------------------------------

      autoType = new ComboBox();
      autoType->setContentsMargins(0, 0, 0, 0);  // REMOVE Tim. Trackinfo. Changed. TEST
      autoType->setFocusPolicy(Qt::NoFocus);
      ///autoType->setFont(MusEGlobal::config.fonts[1]);
//       autoType->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
//       autoType->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum)); // REMOVE Tim. Trackinfo. Changed. TEST
      autoType->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum); // REMOVE Tim. Trackinfo. Changed. TEST
//       autoType->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum); // REMOVE Tim. Trackinfo. Changed. TEST
      autoType->setEnabled(false);
      
      // Removed by T356. 
      // Disabled for now. There is no midi automation mechanism yet...
      //autoType->addAction(tr("Off"), AUTO_OFF);
      //autoType->addAction(tr("Read"), AUTO_READ);
      //autoType->addAction(tr("Touch"), AUTO_TOUCH);
      //autoType->addAction(tr("Write"), AUTO_WRITE);
      //autoType->setCurrentItem(t->automationType());
      //autoType->setToolTip(tr("automation type"));      
      //connect(autoType, SIGNAL(activated(int)), SLOT(setAutomationType(int)));
      autoType->addAction(" ", AUTO_OFF);  // Just a dummy text to fix sizing problems. REMOVE later if full automation added.
      autoType->setCurrentItem(AUTO_OFF);    //

//       grid->addWidget(autoType, _curGridRow++, 0, 1, 2);
//       grid->addWidget(autoType, _curGridRow++, 0, 1, 2); // REMOVE Tim. Trackinfo. Changed. TEST
      addGridWidget(autoType, _automationPos); // REMOVE Tim. Trackinfo. Changed. TEST

      grid->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Ignored), 
                    _rightSpacerPos._row, _rightSpacerPos._col, _rightSpacerPos._rowSpan, _rightSpacerPos._colSpan);

      
      // TODO: Activate this. But owners want to marshall this signal and send it themselves. Change that.
      //connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedFlags_t)), SLOT(songChanged(MusECore::SongChangedFlags_t)));

      connect(MusEGlobal::heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
      inHeartBeat = false;
      }

// REMOVE Tim. Trackinfo. Changed.
// //---------------------------------------------------------
// //   updateOffState
// //---------------------------------------------------------
// 
// void MidiStrip::updateOffState()
//       {
//       bool val = !track->off();
//       slider->setEnabled(val);
//       sl->setEnabled(val);
//       controller[KNOB_PAN].knob->setEnabled(val);         
//       controller[KNOB_PAN].dl->setEnabled(val);         
//       label->setEnabled(val);
//       
//       if (record)
//             record->setEnabled(val);
//       if (solo)
//             solo->setEnabled(val);
//       if (mute)
//             mute->setEnabled(val);
//       // TODO: Disabled for now.
//       //if (autoType)
//       //      autoType->setEnabled(val);
//       //if (iR)
//       //      iR->setEnabled(val);
//       //if (oR)
//       //      oR->setEnabled(val);
//       if (off) {
//             off->blockSignals(true);
//             off->setChecked(track->off());
//             off->blockSignals(false);
//             off->setIcon(track->off() ? QIcon(*exit1Icon) : QIcon(*exitIcon));
//             //off->setIconSize(exit1Icon->size());  
//             }
//       }

//---------------------------------------------------------
//   updateOffState
//---------------------------------------------------------

void MidiStrip::updateOffState()
      {
      bool val = !track->off();
      slider->setEnabled(val);
      sl->setEnabled(val);
      controller[KNOB_PAN]._control->setEnabled(val);         
      label->setEnabled(val);
      
      if (record)
            record->setEnabled(val);
      if (solo)
            solo->setEnabled(val);
      if (mute)
            mute->setEnabled(val);
      // TODO: Disabled for now.
      //if (autoType)
      //      autoType->setEnabled(val);
      //if (iR)
      //      iR->setEnabled(val);
      //if (oR)
      //      oR->setEnabled(val);
      if (off) {
            off->blockSignals(true);
            off->setChecked(track->off());
            off->blockSignals(false);
//             off->setIcon(track->off() ? QIcon(*exit1Icon) : QIcon(*exitIcon)); // REMOVE Tim. Trackinfo. Removed. TEST
            //off->setIconSize(exit1Icon->size());  
            }
      }

void MidiStrip::updateRackSizes(bool upper, bool lower)
{
  const QFontMetrics fm = fontMetrics();
  if(upper)
  {
    // Make room for 3 CompactSliders and one CompactPatchEdit.
    // TODO: Add the instrument select label height!

// REMOVE Tim. Trackinfo.    
//     const int csh = CompactSlider::getMinimumSizeHint(fm,
//                                             Qt::Horizontal, 
//                                             CompactSlider::None, 
//                                             xMarginHorSlider, yMarginHorSlider).height();
//     const int cpeh = CompactPatchEdit::getMinimumSizeHint(fm, 
//                                             Qt::Horizontal, 
//                                             CompactSlider::None, 
//                                             xMarginHorSlider, yMarginHorSlider).height();
//     const int ilh = _instrLabel->sizeHint().height();
    
//     fprintf(stderr, "MidiStrip::updateRackSizes: CompactSlider h:%d CompactPatchEdit h:%d instrLabel h:%d upper frame w:%d \n", 
//                      csh, cpeh, ilh, _upperRack->frameWidth()); // REMOVE Tim. Trackinfo.
    
    _upperRack->setMinimumHeight(
      3 * CompactSlider::getMinimumSizeHint(fm,
                                            Qt::Horizontal, 
                                            CompactSlider::None, 
                                            xMarginHorSlider, yMarginHorSlider).height() + 
      1 * CompactPatchEdit::getMinimumSizeHint(fm, 
                                            Qt::Horizontal, 
                                            CompactSlider::None, 
                                            xMarginHorSlider, yMarginHorSlider).height() +
      upperRackSpacerHeight +
      
      _instrLabel->sizeHint().height() +
      
      2 * rackFrameWidth);
  }
  if(lower)
  {
    // Make room for 1 CompactSlider (Pan, so far).
    
    //fprintf(stderr, "MidiStrip::updateRackSizes: lower frame w:%d \n", _lowerRack->frameWidth()); // REMOVE Tim. Trackinfo.
    
    _lowerRack->setMinimumHeight(
      1 * CompactSlider::getMinimumSizeHint(fm, 
                                            Qt::Horizontal, 
                                            CompactSlider::None, 
                                            xMarginHorSlider, yMarginHorSlider).height() + 
      2 * rackFrameWidth);
  }
}
      
//---------------------------------------------------------
//   configChanged
//   Catch when config label font changes, viewable tracks etc.
//---------------------------------------------------------

void MidiStrip::configChanged()
{
  // Set the whole strip's font, except for the label.    p4.0.45
  if(font() != MusEGlobal::config.fonts[1])
  {
    //fprintf(stderr, "MidiStrip::configChanged changing font: current size:%d\n", font().pointSize()); // REMOVE Tim. Trackinfo.
// REMOVE Tim. Trackinfo. Changed.  
//     setFont(MusEGlobal::config.fonts[1]);
    setStyleSheet(MusECore::font2StyleSheet(MusEGlobal::config.fonts[1]));
    // Update in case font changed.
    updateRackSizes(true, true);
  }
  // Update always, in case style, stylesheet, or font changed.
  //updateRackSizes(true, true);
  
  // Set the strip label's font.
  setLabelFont();
  setLabelText();        
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MidiStrip::songChanged(MusECore::SongChangedFlags_t val)
      {
      // Is it simply a midi controller value adjustment? Forget it.
      if (val == SC_MIDI_CONTROLLER)
        return;
      
      if (mute && (val & SC_MUTE)) {      // mute && off
            mute->blockSignals(true);
            //mute->setChecked(track->isMute());  
            mute->setChecked(track->mute());
            mute->blockSignals(false);
//             mute->setIcon(track->mute() ? QIcon(*muteIconOff) : QIcon(*muteIconOn)); // REMOVE Tim. Trackinfo. Removed.
            //mute->setIconSize(muteIconOn->size());  
            updateOffState();
            }
      if (solo && (val & (SC_SOLO | SC_ROUTE))) 
      {
            solo->blockSignals(true);
            solo->setChecked(track->solo());
            solo->blockSignals(false);
            if(track->internalSolo())
              solo->setIcon(track->solo() ? QIcon(*soloblksqIconOn) : QIcon(*soloblksqIconOff));
            else
              solo->setIcon(track->solo() ? QIcon(*soloIconOn) : QIcon(*soloIconOff));
            //solo->setIconSize(soloIconOn->size());  
      }      
      
      if (val & SC_RECFLAG)
            setRecordFlag(track->recordFlag());
      if (val & SC_TRACK_MODIFIED)
      {
            setLabelText();
            setLabelFont();
            
      }      
      
      // Catch when label font changes. 
      if (val & SC_CONFIG)
      {
        // So far only 1 instance of sending SC_CONFIG in the entire app, in instrument editor when a new instrument is saved. 
      }  
      
      if(val & SC_MIDI_TRACK_PROP)
      {
        MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(track);
        const int outPort    = mt->outPort();
        if(MusECore::MidiInstrument* minstr = MusEGlobal::midiPorts[outPort].instrument())
        {
          _instrLabel->setText(minstr->iname()); // It ignores if already set.
          if(minstr->isSynti())
            _instrLabel->setEnabled(false);
          else
            _instrLabel->setEnabled(true);
        }
        else
          _instrLabel->setText(tr("<unknown>")); // It ignores if already set.
        
        // Set record echo.
        if(_midiThru->isChecked() != mt->recEcho())
        {
          _midiThru->blockSignals(true);
          _midiThru->setChecked(mt->recEcho());
          _midiThru->blockSignals(false);
        }
          
        if(_properties[PropertyTransp]->value() != mt->transposition)
        {
          _properties[PropertyTransp]->blockSignals(true);
          _properties[PropertyTransp]->setValue(mt->transposition);
          _properties[PropertyTransp]->blockSignals(false);
        }
        
        if(_properties[PropertyDelay]->value() != mt->delay)
        {
          _properties[PropertyDelay]->blockSignals(true);
          _properties[PropertyDelay]->setValue(mt->delay);
          _properties[PropertyDelay]->blockSignals(false);
        }
        
        if(_properties[PropertyLen]->value() != mt->len)
        {
          _properties[PropertyLen]->blockSignals(true);
          _properties[PropertyLen]->setValue(mt->len);
          _properties[PropertyLen]->blockSignals(false);
        }
        
        if(_properties[PropertyVelo]->value() != mt->velocity)
        {
          _properties[PropertyVelo]->blockSignals(true);
          _properties[PropertyVelo]->setValue(mt->velocity);
          _properties[PropertyVelo]->blockSignals(false);
        }
        
        if(_properties[PropertyCompr]->value() != mt->compression)
        {
          _properties[PropertyCompr]->blockSignals(true);
          _properties[PropertyCompr]->setValue(mt->compression);
          _properties[PropertyCompr]->blockSignals(false);
        }
      }
    }

//---------------------------------------------------------
//   controlRightClicked
//---------------------------------------------------------

void MidiStrip::controlRightClicked(const QPoint &p, int id)
{
  MusEGlobal::song->execMidiAutomationCtlPopup((MusECore::MidiTrack*)track, 0, p, id);
}

void MidiStrip::propertyRightClicked(const QPoint& /*p*/, int /*id*/)
{
  
}


// REMOVE Tim. Trackinfo. Changed.
// //---------------------------------------------------------
// //   labelDoubleClicked
// //---------------------------------------------------------
// 
// void MidiStrip::labelDoubleClicked(int idx)
// {
//   //int mn, mx, v;
//   //int num = MusECore::CTRL_VOLUME;
//   int num;
//   switch(idx)
//   {
//     case KNOB_PAN:
//       num = MusECore::CTRL_PANPOT;
//     break;
//     case KNOB_VAR_SEND:
//       num = MusECore::CTRL_VARIATION_SEND;
//     break;
//     case KNOB_REV_SEND:
//       num = MusECore::CTRL_REVERB_SEND;
//     break;
//     case KNOB_CHO_SEND:
//       num = MusECore::CTRL_CHORUS_SEND;
//     break;
//     //case -1:
//     default:
//       num = MusECore::CTRL_VOLUME;
//     break;  
//   }
//   int outport = ((MusECore::MidiTrack*)track)->outPort();
//   int chan = ((MusECore::MidiTrack*)track)->outChannel();
//   MusECore::MidiPort* mp = &MusEGlobal::midiPorts[outport];
//   MusECore::MidiController* mc = mp->midiController(num);
//   
//   int lastv = mp->lastValidHWCtrlState(chan, num);
//   int curv = mp->hwCtrlState(chan, num);
//   
//   if(curv == MusECore::CTRL_VAL_UNKNOWN)
//   {
//     // If no value has ever been set yet, use the current knob value 
//     //  (or the controller's initial value?) to 'turn on' the controller.
//     if(lastv == MusECore::CTRL_VAL_UNKNOWN)
//     {
//       //int kiv = _ctrl->initVal());
//       int kiv;
//       if(idx == -1)
//         kiv = lrint(slider->value());
//       else
//         kiv = lrint(controller[idx].knob->value());
//       if(kiv < mc->minVal())
//         kiv = mc->minVal();
//       if(kiv > mc->maxVal())
//         kiv = mc->maxVal();
//       kiv += mc->bias();
//       
//       //MusECore::MidiPlayEvent ev(MusEGlobal::song->cpos(), outport, chan, MusECore::ME_CONTROLLER, num, kiv);
//       MusECore::MidiPlayEvent ev(0, outport, chan, MusECore::ME_CONTROLLER, num, kiv);
//       MusEGlobal::audio->msgPlayMidiEvent(&ev);
//     }
//     else
//     {
//       //MidiPlayEvent ev(MusEGlobal::song->cpos(), outport, chan, MusECore::ME_CONTROLLER, num, lastv);
//       MusECore::MidiPlayEvent ev(0, outport, chan, MusECore::ME_CONTROLLER, num, lastv);
//       MusEGlobal::audio->msgPlayMidiEvent(&ev);
//     }
//   }  
//   else
//   {
//     if(mp->hwCtrlState(chan, num) != MusECore::CTRL_VAL_UNKNOWN)
//       MusEGlobal::audio->msgSetHwCtrlState(mp, chan, num, MusECore::CTRL_VAL_UNKNOWN);
//   }
//   MusEGlobal::song->update(SC_MIDI_CONTROLLER);
// }

//---------------------------------------------------------
//   midiThruToggled
//---------------------------------------------------------

void MidiStrip::midiThruToggled(bool v)
{
  if(MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(track))
  {
    mt->setRecEcho(v);
    MusEGlobal::song->update(SC_MIDI_TRACK_PROP);
  }
}

//---------------------------------------------------------
//   volLabelDoubleClicked
//---------------------------------------------------------

void MidiStrip::volLabelDoubleClicked()
{
  //int mn, mx, v;
  //int num = MusECore::CTRL_VOLUME;
  const int num = MusECore::CTRL_VOLUME;
  const int outport = ((MusECore::MidiTrack*)track)->outPort();
  const int chan = ((MusECore::MidiTrack*)track)->outChannel();
  MusECore::MidiPort* mp = &MusEGlobal::midiPorts[outport];
  MusECore::MidiController* mc = mp->midiController(num);
  
  const int lastv = mp->lastValidHWCtrlState(chan, num);
  const int curv = mp->hwCtrlState(chan, num);
  
  if(curv == MusECore::CTRL_VAL_UNKNOWN)
  {
    // If no value has ever been set yet, use the current knob value 
    //  (or the controller's initial value?) to 'turn on' the controller.
    if(lastv == MusECore::CTRL_VAL_UNKNOWN)
    {
      //int kiv = _ctrl->initVal());
      int kiv = lrint(slider->value());
      if(kiv < mc->minVal())
        kiv = mc->minVal();
      if(kiv > mc->maxVal())
        kiv = mc->maxVal();
      kiv += mc->bias();
      
      //MusECore::MidiPlayEvent ev(MusEGlobal::song->cpos(), outport, chan, MusECore::ME_CONTROLLER, num, kiv);
      MusECore::MidiPlayEvent ev(0, outport, chan, MusECore::ME_CONTROLLER, num, kiv);
      MusEGlobal::audio->msgPlayMidiEvent(&ev);
    }
    else
    {
      //MidiPlayEvent ev(MusEGlobal::song->cpos(), outport, chan, MusECore::ME_CONTROLLER, num, lastv);
      MusECore::MidiPlayEvent ev(0, outport, chan, MusECore::ME_CONTROLLER, num, lastv);
      MusEGlobal::audio->msgPlayMidiEvent(&ev);
    }
  }  
  else
  {
    if(mp->hwCtrlState(chan, num) != MusECore::CTRL_VAL_UNKNOWN)
      MusEGlobal::audio->msgSetHwCtrlState(mp, chan, num, MusECore::CTRL_VAL_UNKNOWN);
  }
  MusEGlobal::song->update(SC_MIDI_CONTROLLER);
}

//---------------------------------------------------------
//   offToggled
//---------------------------------------------------------

void MidiStrip::offToggled(bool val)
      {
      track->setOff(val);
      MusEGlobal::song->update(SC_MUTE);
      }

/*
//---------------------------------------------------------
//   routeClicked
//---------------------------------------------------------

void MidiStrip::routeClicked()
      {
      }
*/

//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void MidiStrip::heartBeat()
      {
      inHeartBeat = true;
      
      // Try to avoid calling MidiInstrument::getPatchName too often.
      if(++_heartBeatCounter >= 10)
        _heartBeatCounter = 0;
      
      int act = track->activity();
      double dact = double(act) * (slider->value() / 127.0);
      
      if((int)dact > track->lastActivity())
        track->setLastActivity((int)dact);
      
      if(meter[0]) 
        //meter[0]->setVal(int(double(act) * (slider->value() / 127.0)), 0, false);  
        meter[0]->setVal(dact, track->lastActivity(), false);  
      
      // Gives reasonable decay with gui update set to 20/sec.
      if(act)
        track->setActivity((int)((double)act * 0.8));
      
      Strip::heartBeat();
      updateControls();
            
      inHeartBeat = false;
      }

// REMOVE Tim. Trackinfo. Changed.
// //---------------------------------------------------------
// //   updateControls
// //---------------------------------------------------------
// 
// void MidiStrip::updateControls()
//       {
//         bool en;
//         int channel  = ((MusECore::MidiTrack*)track)->outChannel();
//         MusECore::MidiPort* mp = &MusEGlobal::midiPorts[((MusECore::MidiTrack*)track)->outPort()];
//         MusECore::MidiCtrlValListList* mc = mp->controller();
//         MusECore::ciMidiCtrlValList icl;
//         
//           MusECore::MidiController* ctrl = mp->midiController(MusECore::CTRL_VOLUME);
//           int nvolume = mp->hwCtrlState(channel, MusECore::CTRL_VOLUME);
//           if(nvolume == MusECore::CTRL_VAL_UNKNOWN)
//           {
//             //if(nvolume != volume) 
//             //{
//               // MusEGui::DoubleLabel ignores the value if already set...
//               sl->setValue(sl->off() - 1.0);
//               //volume = nvolume;
//             //}  
//             volume = MusECore::CTRL_VAL_UNKNOWN;
//             nvolume = mp->lastValidHWCtrlState(channel, MusECore::CTRL_VOLUME);
//             //if(nvolume != volume) 
//             if(nvolume != MusECore::CTRL_VAL_UNKNOWN)
//             {
//               nvolume -= ctrl->bias();
//               //slider->blockSignals(true);
//               if(double(nvolume) != slider->value())
//               {
//                 //printf("MidiStrip::updateControls setting volume slider\n");
//                 
//                 slider->setValue(double(nvolume));
//               }  
//             }  
//           }  
//           else  
//           {
//             int ivol = nvolume;
//             nvolume -= ctrl->bias();
//             if(nvolume != volume) {
//                 //printf("MidiStrip::updateControls setting volume slider\n");
//                 
//                 //slider->blockSignals(true);
//                 slider->setValue(double(nvolume));
//                 //sl->setValue(double(nvolume));
//                 if(ivol == 0)
//                 {
//                   //printf("MidiStrip::updateControls setting volume slider label\n");  
//                   
//                   sl->setValue(sl->minValue() - 0.5 * (sl->minValue() - sl->off()));
//                 }  
//                 else
//                 {  
//                   double v = -MusECore::fast_log10(float(127*127)/float(ivol*ivol))*20.0;
//                   if(v > sl->maxValue())
//                   {
//                     //printf("MidiStrip::updateControls setting volume slider label\n");
//                     
//                     sl->setValue(sl->maxValue());
//                   }  
//                   else  
//                   {
//                     //printf("MidiStrip::updateControls setting volume slider label\n");
//                     
//                     sl->setValue(v);
//                   }  
//                 }    
//                 //slider->blockSignals(false);
//                 volume = nvolume;
//                 }
//           }      
//         
//         
//           KNOB* gcon = &controller[KNOB_PAN];
//           ctrl = mp->midiController(MusECore::CTRL_PANPOT);
//           int npan = mp->hwCtrlState(channel, MusECore::CTRL_PANPOT);
//           if(npan == MusECore::CTRL_VAL_UNKNOWN)
//           {
//             // MusEGui::DoubleLabel ignores the value if already set...
//             //if(npan != pan) 
//             //{
//               gcon->dl->setValue(gcon->dl->off() - 1.0);
//               //pan = npan;
//             //}
//             pan = MusECore::CTRL_VAL_UNKNOWN;
//             npan = mp->lastValidHWCtrlState(channel, MusECore::CTRL_PANPOT);
//             if(npan != MusECore::CTRL_VAL_UNKNOWN)
//             {
//               npan -= ctrl->bias();
//               if(double(npan) != gcon->knob->value())
//               {
//                 //printf("MidiStrip::updateControls setting pan knob\n");
//                 
//                 gcon->knob->setValue(double(npan));
//               }  
//             }
//           }
//           else
//           {
//             npan -= ctrl->bias();
//             if(npan != pan) 
//             {
//                 //printf("MidiStrip::updateControls setting pan label and knob\n");
//                 
//                 //controller[KNOB_PAN].knob->blockSignals(true);
//                 gcon->knob->setValue(double(npan));
//                 gcon->dl->setValue(double(npan));
//                 //controller[KNOB_PAN].knob->blockSignals(false);
//                 pan = npan;
//             }
//           }        
//               
//               
//         icl = mc->find(channel, MusECore::CTRL_VARIATION_SEND);
//         en = icl != mc->end();
//         
//         gcon = &controller[KNOB_VAR_SEND];
//         if(gcon->knob->isEnabled() != en)
//           gcon->knob->setEnabled(en);
//         if(gcon->lb->isEnabled() != en)
//           gcon->lb->setEnabled(en);
//         if(gcon->dl->isEnabled() != en)
//           gcon->dl->setEnabled(en);
//           
//         if(en)
//         {
//           ctrl = mp->midiController(MusECore::CTRL_VARIATION_SEND);
//           int nvariSend = icl->second->hwVal();
//           if(nvariSend == MusECore::CTRL_VAL_UNKNOWN)
//           {
//             // MusEGui::DoubleLabel ignores the value if already set...
//             //if(nvariSend != variSend) 
//             //{
//               gcon->dl->setValue(gcon->dl->off() - 1.0);
//               //variSend = nvariSend;
//             //}
//             variSend = MusECore::CTRL_VAL_UNKNOWN;
//             nvariSend = mp->lastValidHWCtrlState(channel, MusECore::CTRL_VARIATION_SEND);
//             if(nvariSend != MusECore::CTRL_VAL_UNKNOWN)
//             {
//               nvariSend -= ctrl->bias();
//               if(double(nvariSend) != gcon->knob->value())
//               {
//                 gcon->knob->setValue(double(nvariSend));
//               }  
//             }
//           }
//           else
//           {
//             nvariSend -= ctrl->bias();
//             if(nvariSend != variSend) 
//             {
//               //controller[KNOB_VAR_SEND].knob->blockSignals(true);
//               gcon->knob->setValue(double(nvariSend));
//               gcon->dl->setValue(double(nvariSend));
//               //controller[KNOB_VAR_SEND].knob->blockSignals(false);
//               variSend = nvariSend;
//             }  
//           }  
//         }
//         
//         icl = mc->find(channel, MusECore::CTRL_REVERB_SEND);
//         en = icl != mc->end();
//         
//         gcon = &controller[KNOB_REV_SEND];
//         if(gcon->knob->isEnabled() != en)
//           gcon->knob->setEnabled(en);
//         if(gcon->lb->isEnabled() != en)
//           gcon->lb->setEnabled(en);
//         if(gcon->dl->isEnabled() != en)
//           gcon->dl->setEnabled(en);
//         
//         if(en)
//         {
//           ctrl = mp->midiController(MusECore::CTRL_REVERB_SEND);
//           int nreverbSend = icl->second->hwVal();
//           if(nreverbSend == MusECore::CTRL_VAL_UNKNOWN)
//           {
//             // MusEGui::DoubleLabel ignores the value if already set...
//             //if(nreverbSend != reverbSend) 
//             //{
//               gcon->dl->setValue(gcon->dl->off() - 1.0);
//               //reverbSend = nreverbSend;
//             //}
//             reverbSend = MusECore::CTRL_VAL_UNKNOWN;
//             nreverbSend = mp->lastValidHWCtrlState(channel, MusECore::CTRL_REVERB_SEND);
//             if(nreverbSend != MusECore::CTRL_VAL_UNKNOWN)
//             {
//               nreverbSend -= ctrl->bias();
//               if(double(nreverbSend) != gcon->knob->value())
//               {
//                 gcon->knob->setValue(double(nreverbSend));
//               }  
//             }
//           }
//           else
//           {
//             nreverbSend -= ctrl->bias();
//             if(nreverbSend != reverbSend) 
//             {
//               //controller[KNOB_REV_SEND].knob->blockSignals(true);
//               gcon->knob->setValue(double(nreverbSend));
//               gcon->dl->setValue(double(nreverbSend));
//               //controller[KNOB_REV_SEND].knob->blockSignals(false);
//               reverbSend = nreverbSend;
//             }
//           }    
//         }
//         
//         icl = mc->find(channel, MusECore::CTRL_CHORUS_SEND);
//         en = icl != mc->end();
//         
//         gcon = &controller[KNOB_CHO_SEND];
//         if(gcon->knob->isEnabled() != en)
//           gcon->knob->setEnabled(en);
//         if(gcon->lb->isEnabled() != en)
//           gcon->lb->setEnabled(en);
//         if(gcon->dl->isEnabled() != en)
//           gcon->dl->setEnabled(en);
//         
//         if(en)
//         {
//           ctrl = mp->midiController(MusECore::CTRL_CHORUS_SEND);
//           int nchorusSend = icl->second->hwVal();
//           if(nchorusSend == MusECore::CTRL_VAL_UNKNOWN)
//           {
//             // MusEGui::DoubleLabel ignores the value if already set...
//             //if(nchorusSend != chorusSend) 
//             //{
//               gcon->dl->setValue(gcon->dl->off() - 1.0);
//               //chorusSend = nchorusSend;
//             //}
//             chorusSend = MusECore::CTRL_VAL_UNKNOWN;
//             nchorusSend = mp->lastValidHWCtrlState(channel, MusECore::CTRL_CHORUS_SEND);
//             if(nchorusSend != MusECore::CTRL_VAL_UNKNOWN)
//             {
//               nchorusSend -= ctrl->bias();
//               if(double(nchorusSend) != gcon->knob->value())
//               {
//                 gcon->knob->setValue(double(nchorusSend));
//               }  
//             }
//           }
//           else
//           {
//             nchorusSend -= ctrl->bias();
//             if(nchorusSend != chorusSend) 
//             {
//               gcon->knob->setValue(double(nchorusSend));
//               gcon->dl->setValue(double(nchorusSend));
//               chorusSend = nchorusSend;
//             }  
//           }  
//         }
//       }

//---------------------------------------------------------
//   updateControls
//---------------------------------------------------------

void MidiStrip::updateControls()
      {
        bool en;
        MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(track);
        const int channel  = mt->outChannel();
        MusECore::MidiPort* mp = &MusEGlobal::midiPorts[mt->outPort()];
        MusECore::MidiCtrlValListList* mc = mp->controller();
        MusECore::ciMidiCtrlValList icl;
        
        MusECore::MidiController* ctrl = mp->midiController(MusECore::CTRL_VOLUME);
        int nvolume = mp->hwCtrlState(channel, MusECore::CTRL_VOLUME);
        if(nvolume == MusECore::CTRL_VAL_UNKNOWN)
        {
          //if(nvolume != volume) 
          //{
            // MusEGui::DoubleLabel ignores the value if already set...
            sl->setValue(sl->off() - 1.0);
            //volume = nvolume;
          //}  
          volume = MusECore::CTRL_VAL_UNKNOWN;
          nvolume = mp->lastValidHWCtrlState(channel, MusECore::CTRL_VOLUME);
          //if(nvolume != volume) 
          if(nvolume != MusECore::CTRL_VAL_UNKNOWN)
          {
            nvolume -= ctrl->bias();
            //slider->blockSignals(true);
            if(double(nvolume) != slider->value())
            {
              //printf("MidiStrip::updateControls setting volume slider\n");
              
              slider->setValue(double(nvolume));
            }  
          }  
        }  
        else  
        {
          int ivol = nvolume;
          nvolume -= ctrl->bias();
          if(nvolume != volume) {
              //printf("MidiStrip::updateControls setting volume slider\n");
              
              //slider->blockSignals(true);
              slider->setValue(double(nvolume));
              //sl->setValue(double(nvolume));
              if(ivol == 0)
              {
                //printf("MidiStrip::updateControls setting volume slider label\n");  
                
                sl->setValue(sl->minValue() - 0.5 * (sl->minValue() - sl->off()));
              }  
              else
              {  
                double v = -MusECore::fast_log10(float(127*127)/float(ivol*ivol))*20.0;
                if(v > sl->maxValue())
                {
                  //printf("MidiStrip::updateControls setting volume slider label\n");
                  
                  sl->setValue(sl->maxValue());
                }  
                else  
                {
                  //printf("MidiStrip::updateControls setting volume slider label\n");
                  
                  sl->setValue(v);
                }  
              }    
              //slider->blockSignals(false);
              volume = nvolume;
              }
        }      
      
      
        CONTROL* gcon = &controller[KNOB_PAN];
        ctrl = mp->midiController(MusECore::CTRL_PANPOT);
        int npan = mp->hwCtrlState(channel, MusECore::CTRL_PANPOT);
        if(npan == MusECore::CTRL_VAL_UNKNOWN)
        {
//             pan = MusECore::CTRL_VAL_UNKNOWN;
          gcon->_cachedVal = MusECore::CTRL_VAL_UNKNOWN;
          npan = mp->lastValidHWCtrlState(channel, MusECore::CTRL_PANPOT);
          if(npan == MusECore::CTRL_VAL_UNKNOWN)
          {
            if(!gcon->_control->isOff())
            {
              gcon->_control->blockSignals(true);
              gcon->_control->setOff(true);
              gcon->_control->blockSignals(false);
            }
          }
          else
          {
            npan -= ctrl->bias();
            if(!gcon->_control->isOff() || double(npan) != gcon->_control->value())
            {
              gcon->_control->blockSignals(true);
              gcon->_control->setValueState(double(npan), true);
              gcon->_control->blockSignals(false);
            }  
          }
        }
        else
        {
          npan -= ctrl->bias();
//             if(gcon->_control->isOff() || npan != pan) 
          if(gcon->_control->isOff() || npan != gcon->_cachedVal) 
          {
              gcon->_control->blockSignals(true);
              gcon->_control->setValueState(double(npan), false);
              gcon->_control->blockSignals(false);
//                 pan = npan;
              gcon->_cachedVal = npan;
          }
        }        
            
              
        icl = mc->find(channel, MusECore::CTRL_VARIATION_SEND);
        en = icl != mc->end();
        
        gcon = &controller[KNOB_VAR_SEND];
        if(gcon->_control->isEnabled() != en)
          gcon->_control->setEnabled(en);
          
        if(en)
        {
          ctrl = mp->midiController(MusECore::CTRL_VARIATION_SEND);
          int nvariSend = icl->second->hwVal();
          if(nvariSend == MusECore::CTRL_VAL_UNKNOWN)
          {
            // MusEGui::DoubleLabel ignores the value if already set...
            //if(nvariSend != variSend) 
            //{
//               gcon->dl->setValue(gcon->dl->off() - 1.0);
              //variSend = nvariSend;
            //}
//             variSend = MusECore::CTRL_VAL_UNKNOWN;
            gcon->_cachedVal = MusECore::CTRL_VAL_UNKNOWN;
            nvariSend = mp->lastValidHWCtrlState(channel, MusECore::CTRL_VARIATION_SEND);
            if(nvariSend == MusECore::CTRL_VAL_UNKNOWN)
            {
              if(!gcon->_control->isOff())
              {
                gcon->_control->blockSignals(true);
                gcon->_control->setOff(true);
                gcon->_control->blockSignals(false);
              }
            }
            else
            //if(nvariSend != MusECore::CTRL_VAL_UNKNOWN)
            {
              nvariSend -= ctrl->bias();
//               if(double(nvariSend) != gcon->knob->value())
              if(!gcon->_control->isOff() || double(nvariSend) != gcon->_control->value())
              {
//                 gcon->knob->setValue(double(nvariSend));
                gcon->_control->blockSignals(true);
                gcon->_control->setValueState(double(nvariSend), true);
                gcon->_control->blockSignals(false);
              }  
            }
          }
          else
          {
            nvariSend -= ctrl->bias();
//             if(nvariSend != variSend) 
//             if(gcon->_control->isOff() || nvariSend != variSend) 
            if(gcon->_control->isOff() || nvariSend != gcon->_cachedVal) 
            {
              //controller[KNOB_VAR_SEND].knob->blockSignals(true);
//               gcon->knob->setValue(double(nvariSend));
//               gcon->dl->setValue(double(nvariSend));
              //controller[KNOB_VAR_SEND].knob->blockSignals(false);
              gcon->_control->blockSignals(true);
              gcon->_control->setValueState(double(nvariSend), false);
              gcon->_control->blockSignals(false);
//               variSend = nvariSend;
              gcon->_cachedVal = nvariSend;
            }  
          }  
        }
        
        icl = mc->find(channel, MusECore::CTRL_REVERB_SEND);
        en = icl != mc->end();
        
        gcon = &controller[KNOB_REV_SEND];
//         if(gcon->knob->isEnabled() != en)
//           gcon->knob->setEnabled(en);
//         if(gcon->lb->isEnabled() != en)
//           gcon->lb->setEnabled(en);
//         if(gcon->dl->isEnabled() != en)
//           gcon->dl->setEnabled(en);
        if(gcon->_control->isEnabled() != en)
          gcon->_control->setEnabled(en);
        
        if(en)
        {
          ctrl = mp->midiController(MusECore::CTRL_REVERB_SEND);
          int nreverbSend = icl->second->hwVal();
          if(nreverbSend == MusECore::CTRL_VAL_UNKNOWN)
          {
            // MusEGui::DoubleLabel ignores the value if already set...
            //if(nreverbSend != reverbSend) 
            //{
//               gcon->dl->setValue(gcon->dl->off() - 1.0);
              //reverbSend = nreverbSend;
            //}
//             reverbSend = MusECore::CTRL_VAL_UNKNOWN;
            gcon->_cachedVal = MusECore::CTRL_VAL_UNKNOWN;
            nreverbSend = mp->lastValidHWCtrlState(channel, MusECore::CTRL_REVERB_SEND);
            if(nreverbSend == MusECore::CTRL_VAL_UNKNOWN)
            {
              if(!gcon->_control->isOff())
              {
                gcon->_control->blockSignals(true);
                gcon->_control->setOff(true);
                gcon->_control->blockSignals(false);
              }
            }
            else
//             if(nreverbSend != MusECore::CTRL_VAL_UNKNOWN)
            {
              nreverbSend -= ctrl->bias();
//               if(double(nreverbSend) != gcon->knob->value())
              if(!gcon->_control->isOff() || double(nreverbSend) != gcon->_control->value())
              {
//                 gcon->knob->setValue(double(nreverbSend));
                gcon->_control->blockSignals(true);
                gcon->_control->setValueState(double(nreverbSend), true);
                gcon->_control->blockSignals(false);
              }  
            }
          }
          else
          {
            nreverbSend -= ctrl->bias();
//             if(nreverbSend != reverbSend) 
//             if(gcon->_control->isOff() || nreverbSend != reverbSend) 
            if(gcon->_control->isOff() || nreverbSend != gcon->_cachedVal) 
            {
              //controller[KNOB_REV_SEND].knob->blockSignals(true);
//               gcon->knob->setValue(double(nreverbSend));
//               gcon->dl->setValue(double(nreverbSend));
              //controller[KNOB_REV_SEND].knob->blockSignals(false);
              gcon->_control->blockSignals(true);
              gcon->_control->setValueState(double(nreverbSend), false);
              gcon->_control->blockSignals(false);
              gcon->_cachedVal = nreverbSend;
            }
          }    
        }
        
        icl = mc->find(channel, MusECore::CTRL_CHORUS_SEND);
        en = icl != mc->end();
        
        gcon = &controller[KNOB_CHO_SEND];
//         if(gcon->knob->isEnabled() != en)
//           gcon->knob->setEnabled(en);
//         if(gcon->lb->isEnabled() != en)
//           gcon->lb->setEnabled(en);
//         if(gcon->dl->isEnabled() != en)
//           gcon->dl->setEnabled(en);
        if(gcon->_control->isEnabled() != en)
          gcon->_control->setEnabled(en);
        
        if(en)
        {
          ctrl = mp->midiController(MusECore::CTRL_CHORUS_SEND);
          int nchorusSend = icl->second->hwVal();
          if(nchorusSend == MusECore::CTRL_VAL_UNKNOWN)
          {
            // MusEGui::DoubleLabel ignores the value if already set...
            //if(nchorusSend != chorusSend) 
            //{
//               gcon->dl->setValue(gcon->dl->off() - 1.0);
              //chorusSend = nchorusSend;
            //}
//             chorusSend = MusECore::CTRL_VAL_UNKNOWN;
            gcon->_cachedVal = MusECore::CTRL_VAL_UNKNOWN;
            nchorusSend = mp->lastValidHWCtrlState(channel, MusECore::CTRL_CHORUS_SEND);
            if(nchorusSend == MusECore::CTRL_VAL_UNKNOWN)
            {
              if(!gcon->_control->isOff())
              {
                gcon->_control->blockSignals(true);
                gcon->_control->setOff(true);
                gcon->_control->blockSignals(false);
              }
            }
            else
//             if(nchorusSend != MusECore::CTRL_VAL_UNKNOWN)
            {
              nchorusSend -= ctrl->bias();
//               if(double(nchorusSend) != gcon->knob->value())
              if(!gcon->_control->isOff() || double(nchorusSend) != gcon->_control->value())
              {
//                 gcon->knob->setValue(double(nchorusSend));
                gcon->_control->blockSignals(true);
                gcon->_control->setValueState(double(nchorusSend), true);
                gcon->_control->blockSignals(false);
              }  
            }
          }
          else
          {
            nchorusSend -= ctrl->bias();
//             if(nchorusSend != chorusSend) 
//             if(gcon->_control->isOff() || nchorusSend != chorusSend) 
            if(gcon->_control->isOff() || nchorusSend != gcon->_cachedVal) 
            {
//               gcon->knob->setValue(double(nchorusSend));
//               gcon->dl->setValue(double(nchorusSend));
              gcon->_control->blockSignals(true);
              gcon->_control->setValueState(double(nchorusSend), false);
              gcon->_control->blockSignals(false);
//               chorusSend = nchorusSend;
              gcon->_cachedVal = nchorusSend;
            }  
          }  
        }
        
        
// REMOVE Tim. Trackinfo. Added.
        icl = mc->find(channel, MusECore::CTRL_PROGRAM);
        en = icl != mc->end();
        
        gcon = &controller[KNOB_PROGRAM];
        if(gcon->_patchControl)
        {
          if(gcon->_patchControl->isEnabled() != en)
            gcon->_patchControl->setEnabled(en);
          
          if(en)
          {
            ctrl = mp->midiController(MusECore::CTRL_PROGRAM);
            int nProgram = icl->second->hwVal();
            if(nProgram == MusECore::CTRL_VAL_UNKNOWN)
            {
//               program = MusECore::CTRL_VAL_UNKNOWN;
              gcon->_cachedVal = MusECore::CTRL_VAL_UNKNOWN;
              nProgram = mp->lastValidHWCtrlState(channel, MusECore::CTRL_PROGRAM);
              if(nProgram == MusECore::CTRL_VAL_UNKNOWN)
              {
                if(!gcon->_patchControl->isOff())
                {
                  gcon->_patchControl->blockSignals(true);
                  gcon->_patchControl->setOff(true);
                  gcon->_patchControl->blockSignals(false);
                }
              }
              else
              {
                if(!gcon->_patchControl->isOff() || double(nProgram) != gcon->_patchControl->value())
                {
                  gcon->_patchControl->blockSignals(true);
                  gcon->_patchControl->setValueState(double(nProgram), true);
                  gcon->_patchControl->blockSignals(false);
                }  
              }
            }
            else
            {
//               if(gcon->_patchControl->isOff() || nProgram != program) 
              if(gcon->_patchControl->isOff() || nProgram != gcon->_cachedVal) 
              {
                gcon->_patchControl->blockSignals(true);
                gcon->_patchControl->setValueState(double(nProgram), false);
                gcon->_patchControl->blockSignals(false);
//                 program = nProgram;
                gcon->_cachedVal = nProgram;
              }  
            }  
            
            if(nProgram == MusECore::CTRL_VAL_UNKNOWN)
            {
              const QString patchName(tr("<unknown>"));
              if(gcon->_patchControl->patchName() != patchName)
                gcon->_patchControl->setPatchName(patchName);
            }
            else
            {
              // Try to avoid calling MidiInstrument::getPatchName too often.
              if(_heartBeatCounter == 0)
              {
                MusECore::MidiInstrument* instr = mp->instrument();
                QString patchName(instr->getPatchName(channel, nProgram, track->isDrumTrack()));
                if(patchName.isEmpty())
                  patchName = QString("???");
                if(gcon->_patchControl->patchName() != patchName)
                  gcon->_patchControl->setPatchName(patchName);
              }
            }
          }
        }
        
//         if(_properties[PropertyTransp]->value() != mt->transposition)
//         {
//           _properties[PropertyTransp]->blockSignals(true);
//           _properties[PropertyTransp]->setValue(mt->transposition);
//           _properties[PropertyTransp]->blockSignals(false);
//         }
//         
//         if(_properties[PropertyDelay]->value() != mt->delay)
//         {
//           _properties[PropertyDelay]->blockSignals(true);
//           _properties[PropertyDelay]->setValue(mt->delay);
//           _properties[PropertyDelay]->blockSignals(false);
//         }
//         
//         if(_properties[PropertyLen]->value() != mt->len)
//         {
//           _properties[PropertyLen]->blockSignals(true);
//           _properties[PropertyLen]->setValue(mt->len);
//           _properties[PropertyLen]->blockSignals(false);
//         }
//         
//         if(_properties[PropertyVelo]->value() != mt->velocity)
//         {
//           _properties[PropertyVelo]->blockSignals(true);
//           _properties[PropertyVelo]->setValue(mt->velocity);
//           _properties[PropertyVelo]->blockSignals(false);
//         }
//         
//         if(_properties[PropertyCompr]->value() != mt->compression)
//         {
//           _properties[PropertyCompr]->blockSignals(true);
//           _properties[PropertyCompr]->setValue(mt->compression);
//           _properties[PropertyCompr]->blockSignals(false);
//         }
        
      }

// REMOVE Tim. Trackinfo. Changed.
// //---------------------------------------------------------
// //   ctrlChanged
// //---------------------------------------------------------
// 
// void MidiStrip::ctrlChanged(int num, int val)
//     {
//       if (inHeartBeat)
//             return;
//       
//       MusECore::MidiTrack* t = (MusECore::MidiTrack*) track;
//       int port     = t->outPort();
//       
//       int chan  = t->outChannel();
//       MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
//       MusECore::MidiController* mctl = mp->midiController(num);
//       if((val < mctl->minVal()) || (val > mctl->maxVal()))
//       {
//         if(mp->hwCtrlState(chan, num) != MusECore::CTRL_VAL_UNKNOWN)
//           MusEGlobal::audio->msgSetHwCtrlState(mp, chan, num, MusECore::CTRL_VAL_UNKNOWN);
//       }  
//       else
//       {
//         val += mctl->bias();
//         
//         int tick     = MusEGlobal::song->cpos();
//         
//         MusECore::MidiPlayEvent ev(tick, port, chan, MusECore::ME_CONTROLLER, num, val);
//         
//         MusEGlobal::audio->msgPlayMidiEvent(&ev);
//       }  
//       MusEGlobal::song->update(SC_MIDI_CONTROLLER);
//     }

//---------------------------------------------------------
//   ctrlChanged
//---------------------------------------------------------

//void MidiStrip::ctrlChanged(int num, int val, bool off)
void MidiStrip::ctrlChanged(double v, bool off, int num)
    {
      if (inHeartBeat)
            return;
      int val = lrint(v);
      MusECore::MidiTrack* t = (MusECore::MidiTrack*) track;
      int port     = t->outPort();
      int chan  = t->outChannel();
      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
      MusECore::MidiController* mctl = mp->midiController(num);
      if(off || (val < mctl->minVal()) || (val > mctl->maxVal()))
      {
        if(mp->hwCtrlState(chan, num) != MusECore::CTRL_VAL_UNKNOWN)
          MusEGlobal::audio->msgSetHwCtrlState(mp, chan, num, MusECore::CTRL_VAL_UNKNOWN);
      }  
      else
      {
        val += mctl->bias();
        int tick     = MusEGlobal::song->cpos();
        MusECore::MidiPlayEvent ev(tick, port, chan, MusECore::ME_CONTROLLER, num, val);
        MusEGlobal::audio->msgPlayMidiEvent(&ev);
      }  
      MusEGlobal::song->update(SC_MIDI_CONTROLLER);
    }

void MidiStrip::propertyChanged(double v, bool /*off*/, int num)
{
  if(!track)
    return;
  MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(track);
  int val = lrint(v);
  
  // FIXME ! This direct setting is probably not safe. Use a FIFO buffer or something. 
  //         A problem with using MidiPort::putEvent is that it does not appear to be safe
  //          when directly setting the hwValues.
  switch(num)
  {
    case PropertyTransp:
      mt->transposition = lrint(val);
    break;
    
    case PropertyDelay:
      mt->delay = lrint(val);
    break;
    
    case PropertyLen:
      mt->len = lrint(val);
    break;
    
    case PropertyVelo:
      mt->velocity = lrint(val);
    break;
    
    case PropertyCompr:
      mt->compression = lrint(val);
    break;
  }
  MusEGlobal::song->update(SC_MIDI_TRACK_PROP);
}

//---------------------------------------------------------
//   patchPopup
//---------------------------------------------------------

void MidiStrip::patchPopup()
{
  if(!track || !controller[KNOB_PROGRAM]._patchControl)
    return;
  MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(track);
  const int channel = mt->outChannel();
  const int port    = mt->outPort();
  MusECore::MidiInstrument* instr = MusEGlobal::midiPorts[port].instrument();
  PopupMenu* pup = new PopupMenu(true);
  
  instr->populatePatchPopup(pup, channel, mt->isDrumTrack());

  if(pup->actions().count() == 0)
  {
    delete pup;
    return;
  }  
  
  connect(pup, SIGNAL(triggered(QAction*)), SLOT(patchPopupActivated(QAction*)));

  pup->exec(controller[KNOB_PROGRAM]._patchControl->mapToGlobal(QPoint(10,5)));
  delete pup;      
}   

//---------------------------------------------------------
//   patchPopupActivated
//---------------------------------------------------------

void MidiStrip::patchPopupActivated(QAction* act)
{
  if(act && track) 
  {
    MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(track);
    const int channel = mt->outChannel();
    const int port    = mt->outPort();
    MusECore::MidiInstrument* instr = MusEGlobal::midiPorts[port].instrument();
    if(act->data().type() == QVariant::Int)
    {
      int rv = act->data().toInt();
      if(rv != -1)
      {
          //++_blockHeartbeatCount;
          MusECore::MidiPlayEvent ev(0, port, channel, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, rv);
          MusEGlobal::audio->msgPlayMidiEvent(&ev);
          //updateTrackInfo(-1);
          //--_blockHeartbeatCount;
      }
    }
    else if(instr->isSynti() && act->data().canConvert<void *>())
    {
      MusECore::SynthI *si = static_cast<MusECore::SynthI *>(instr);
      MusECore::Synth *s = si->synth();
#ifdef LV2_SUPPORT
      //only for lv2 synths call applyPreset function.
      if(s && s->synthType() == MusECore::Synth::LV2_SYNTH)
      {
          MusECore::LV2SynthIF *sif = static_cast<MusECore::LV2SynthIF *>(si->sif());
          //be pedantic about checks
          if(sif)
          {
            MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
            if(mp)
            {
                if(mp->hwCtrlState(channel, MusECore::CTRL_PROGRAM) != MusECore::CTRL_VAL_UNKNOWN)
                  MusEGlobal::audio->msgSetHwCtrlState(mp, channel, MusECore::CTRL_PROGRAM, MusECore::CTRL_VAL_UNKNOWN);
                sif->applyPreset(act->data().value<void *>());
            }
          }
      }
#endif
    }
  }
}

//---------------------------------------------------------
//   instrPopup
//---------------------------------------------------------

void MidiStrip::instrPopup()
{
  if(!track)
    return;
  const int port = static_cast<MusECore::MidiTrack*>(track)->outPort();
  MusECore::MidiInstrument* instr = MusEGlobal::midiPorts[port].instrument();
  PopupMenu* pup = new PopupMenu(false);
  
  MusECore::MidiInstrument::populateInstrPopup(pup, instr, false);

  if(pup->actions().count() == 0)
  {
    delete pup;
    return;
  }  
  
  QAction *act = pup->exec(_instrLabel->mapToGlobal(QPoint(10,5)));
  if(act) 
  {
    QString s = act->text();
    for (MusECore::iMidiInstrument i = MusECore::midiInstruments.begin(); i != MusECore::midiInstruments.end(); ++i) 
    {
      if((*i)->iname() == s) 
      {
        MusEGlobal::audio->msgIdle(true); // Make it safe to edit structures
        MusEGlobal::midiPorts[port].setInstrument(*i);
        MusEGlobal::audio->msgIdle(false);
        // Make sure device initializations are sent if necessary.
        MusEGlobal::audio->msgInitMidiDevices(false);  // false = Don't force
        MusEGlobal::song->update(); // TODO: Use a specific flag instead of brute force all.
        break;
      }
    }
  }
  delete pup;      
}

// REMOVE Tim. Trackinfo. Changed.
// //---------------------------------------------------------
// //   volLabelChanged
// //---------------------------------------------------------
// 
// void MidiStrip::volLabelChanged(double val)
//       {
//       val = sqrt( float(127*127) / pow(10.0, -val/20.0) );
//       
//       ctrlChanged(MusECore::CTRL_VOLUME, lrint(val));
//       
//       }

//---------------------------------------------------------
//   volLabelChanged
//---------------------------------------------------------

void MidiStrip::volLabelChanged(double val)
{
  val = sqrt( float(127*127) / pow(10.0, -val/20.0) );
  ctrlChanged(val, false, MusECore::CTRL_VOLUME);
}
      
// REMOVE Tim. Trackinfo. Changed.
// //---------------------------------------------------------
// //   setVolume
// //---------------------------------------------------------
// 
// void MidiStrip::setVolume(double val)
//       {
//       
// // printf("Vol %d\n", lrint(val));
//       ctrlChanged(MusECore::CTRL_VOLUME, lrint(val));
//       }

//---------------------------------------------------------
//   setVolume
//---------------------------------------------------------

void MidiStrip::setVolume(double val)
      {
      
// printf("Vol %d\n", lrint(val));
      ctrlChanged(val, false, MusECore::CTRL_VOLUME);
      }
      
// REMOVE Tim. Trackinfo. Changed.
// //---------------------------------------------------------
// //   setPan
// //---------------------------------------------------------
// 
// void MidiStrip::setPan(double val)
//       {
//       
//       ctrlChanged(MusECore::CTRL_PANPOT, lrint(val));
//       }
// 
// //---------------------------------------------------------
// //   setVariSend
// //---------------------------------------------------------
// 
// void MidiStrip::setVariSend(double val)
//       {
//       ctrlChanged(MusECore::CTRL_VARIATION_SEND, lrint(val));
//       }
//       
// //---------------------------------------------------------
// //   setChorusSend
// //---------------------------------------------------------
// 
// void MidiStrip::setChorusSend(double val)
//       {
//       ctrlChanged(MusECore::CTRL_CHORUS_SEND, lrint(val));
//       }
//       
// //---------------------------------------------------------
// //   setReverbSend
// //---------------------------------------------------------
// 
// void MidiStrip::setReverbSend(double val)
//       {
//       ctrlChanged(MusECore::CTRL_REVERB_SEND, lrint(val));
//       }

// //---------------------------------------------------------
// //   setPan
// //---------------------------------------------------------
// 
// void MidiStrip::setPan(double v, bool off)
//       {
//       
//       ctrlChanged(MusECore::CTRL_PANPOT, lrint(v), off);
//       }
// 
// //---------------------------------------------------------
// //   setVariSend
// //---------------------------------------------------------
// 
// void MidiStrip::setVariSend(double v, bool off)
//       {
//       ctrlChanged(MusECore::CTRL_VARIATION_SEND, lrint(v), off);
//       }
//       
// //---------------------------------------------------------
// //   setChorusSend
// //---------------------------------------------------------
// 
// void MidiStrip::setChorusSend(double v, bool off)
//       {
//       ctrlChanged(MusECore::CTRL_CHORUS_SEND, lrint(v), off);
//       }
//       
// //---------------------------------------------------------
// //   setReverbSend
// //---------------------------------------------------------
// 
// void MidiStrip::setReverbSend(double v, bool off)
//       {
//       ctrlChanged(MusECore::CTRL_REVERB_SEND, lrint(v), off);
//       }
// 
//       
// // REMOVE Tim. Trackinfo. Added.
// void MidiStrip::setProgram(double v, bool off)
// {
//   //ctrlChanged(MusECore::CTRL_PROGRAM, lrint(v), off);
//   
//   if (inHeartBeat)
//         return;
// 
//   const int val = lrint(v);
//   MusECore::MidiTrack* t = (MusECore::MidiTrack*) track;
//   int port     = t->outPort();
//   
//   int chan  = t->outChannel();
//   MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
//   MusECore::MidiController* mctl = mp->midiController(MusECore::CTRL_PROGRAM);
//   if(off || (val < mctl->minVal()) || (val > mctl->maxVal()))
// //   if(off || (val < mctl->minVal()))
//   {
//     if(mp->hwCtrlState(chan, MusECore::CTRL_PROGRAM) != MusECore::CTRL_VAL_UNKNOWN)
//       MusEGlobal::audio->msgSetHwCtrlState(mp, chan, MusECore::CTRL_PROGRAM, MusECore::CTRL_VAL_UNKNOWN);
//   }  
//   else
//   {
//     int tick     = MusEGlobal::song->cpos();
//     MusECore::MidiPlayEvent ev(tick, port, chan, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, val);
//     MusEGlobal::audio->msgPlayMidiEvent(&ev);
//   }  
//   MusEGlobal::song->update(SC_MIDI_CONTROLLER);
// }
      
//---------------------------------------------------------
//   iRoutePressed
//---------------------------------------------------------

void MidiStrip::iRoutePressed()
{
  //MusEGui::RoutePopupMenu* pup = MusEGlobal::muse->getRoutingPopupMenu();
  RoutePopupMenu* pup = new RoutePopupMenu();
  pup->exec(QCursor::pos(), track, false);
  delete pup;
  iR->setDown(false);     
}

//---------------------------------------------------------
//   oRoutePressed
//---------------------------------------------------------

void MidiStrip::oRoutePressed()
{
  //MusEGui::RoutePopupMenu* pup = MusEGlobal::muse->getRoutingPopupMenu();
  RoutePopupMenu* pup = new RoutePopupMenu();
  pup->exec(QCursor::pos(), track, true);
  delete pup;
  oR->setDown(false);     
}

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void MidiStrip::resizeEvent(QResizeEvent* ev)
{
  //printf("MidiStrip::resizeEvent\n");  
  
  ev->ignore();
  Strip::resizeEvent(ev);

// //   if(_upperScrollArea)
// //     //_upperScrollArea->setMinimumHeight(_upperScrollLayout->minimumSize().height());
// //     _upperScrollArea->setMinimumHeight(70);
// //   if(_lowerScrollArea)
// //     //_lowerScrollArea->setMinimumHeight(_lowerScrollLayout->minimumSize().height());
// //     _lowerScrollArea->setMinimumHeight(22);
//   
//   const QSize& oldSize = ev->oldSize();
//   const QSize& newSize = ev->size();
//   const int old_w = oldSize.width();
//   const int new_w = newSize.width();
//   // Get the original size hint (without the user width).
//   const int min_w = sizeHint().width() - userWidth();
//   // Flip at around 1.5 times the original size hint.
// //   const int flip_w = min_w + min_w / 2;
//   const int flip_w = min_w * 2;
//   
// //   if(old_w <= 100 && new_w > 100)
//   if(!_isExpanded && new_w > old_w && new_w >= flip_w)
//   {
// //     grid->removeWidget(_upperScrollArea);
// //     grid->removeWidget(_lowerScrollArea);
// //     addGridWidget(_upperScrollArea, _preScrollAreaPos_B);
// //     addGridWidget(_lowerScrollArea, _postScrollAreaPos_B);
// 
// //     grid->removeItem(_upperScrollLayout);
// //     grid->removeItem(_lowerScrollLayout);
// //     addGridLayout(_upperScrollLayout, _preScrollAreaPos_B);
// //     addGridLayout(_lowerScrollLayout, _postScrollAreaPos_B);
// 
//     _isExpanded = true;
//     grid->removeWidget(_upperRack);
//     grid->removeWidget(_lowerRack);
//     addGridWidget(_upperRack, _preScrollAreaPos_B);
//     //addGridWidget(_infoRack, _propertyRackPos);
//     addGridWidget(_lowerRack, _postScrollAreaPos_B);
// //     _infoRack->setVisible(true); // Visible if expanded.
//   }
// //   else if(old_w > 150 && new_w <= 150)
//   else if(_isExpanded && new_w < old_w && new_w <= flip_w)
//   {
// //     grid->removeWidget(_upperScrollArea);
// //     grid->removeWidget(_lowerScrollArea);
// //     addGridWidget(_upperScrollArea, _preScrollAreaPos_A);
// //     addGridWidget(_lowerScrollArea, _postScrollAreaPos_A);
//     
// //     grid->removeItem(_upperScrollLayout);
// //     grid->removeItem(_lowerScrollLayout);
// //     addGridLayout(_upperScrollLayout, _preScrollAreaPos_A);
// //     addGridLayout(_lowerScrollLayout, _postScrollAreaPos_A);
//    
//     _isExpanded = false;
// //     _infoRack->setVisible(false); // Not visible unless expanded.
//     grid->removeWidget(_upperRack);
//     //grid->removeWidget(_infoRack);
//     grid->removeWidget(_lowerRack);
//     addGridWidget(_upperRack, _preScrollAreaPos_A);
//     addGridWidget(_lowerRack, _postScrollAreaPos_A);
//   }
  
  
}  

} // namespace MusEGui
