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
//#include "popupmenu.h"
#include "routepopup.h"

namespace MusEGui {

enum { KNOB_PAN, KNOB_VAR_SEND, KNOB_REV_SEND, KNOB_CHO_SEND };

//---------------------------------------------------------
//   addKnob
//---------------------------------------------------------

void MidiStrip::addKnob(int idx, const QString& tt, const QString& label,
   const char* slot, bool enabled)
      {
      int ctl = MusECore::CTRL_PANPOT, mn, mx, v;
      int chan  = ((MusECore::MidiTrack*)track)->outChannel();
      switch(idx)
      {
        //case KNOB_PAN:
        //  ctl = MusECore::CTRL_PANPOT;
        //break;
        case KNOB_VAR_SEND:
          ctl = MusECore::CTRL_VARIATION_SEND;
        break;
        case KNOB_REV_SEND:
          ctl = MusECore::CTRL_REVERB_SEND;
        break;
        case KNOB_CHO_SEND:
          ctl = MusECore::CTRL_CHORUS_SEND;
        break;
      }
      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[((MusECore::MidiTrack*)track)->outPort()];
      MusECore::MidiController* mc = mp->midiController(ctl);
      mn = mc->minVal();
      mx = mc->maxVal();
      
      MusEGui::Knob* knob = new MusEGui::Knob(this);
      knob->setRange(double(mn), double(mx), 1.0);
      knob->setId(ctl);
      
      controller[idx].knob = knob;
      knob->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
      knob->setBackgroundRole(QPalette::Mid);
      knob->setToolTip(tt);
      knob->setEnabled(enabled);

      MusEGui::DoubleLabel* dl = new MusEGui::DoubleLabel(0.0, double(mn), double(mx), this);
      dl->setId(idx);
      dl->setSpecialText(tr("off"));
      dl->setToolTip(tr("ctrl-double-click on/off"));
      controller[idx].dl = dl;
      ///dl->setFont(MusEGlobal::config.fonts[1]);
      dl->setBackgroundRole(QPalette::Mid);
      dl->setFrame(true);
      dl->setPrecision(0);
      dl->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
      dl->setEnabled(enabled);

      double dlv;
      v = mp->hwCtrlState(chan, ctl);
      if(v == MusECore::CTRL_VAL_UNKNOWN)
      {
        //v = mc->initVal();
        //if(v == MusECore::CTRL_VAL_UNKNOWN)
        //  v = 0;
//        v = mn - 1;
        int lastv = mp->lastValidHWCtrlState(chan, ctl);
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
        dlv = dl->off() - 1.0;
      }  
      else
      {
        // Auto bias...
        v -= mc->bias();
        dlv = double(v);
      }
      
      knob->setValue(double(v));
      dl->setValue(dlv);
      //}
      //else
      //      knob->setRange(0.0, 127.0);
      
      QLabel* lb = new QLabel(label, this);
      controller[idx].lb = lb;
      ///lb->setFont(MusEGlobal::config.fonts[1]);
      lb->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
      lb->setAlignment(Qt::AlignCenter);
      lb->setEnabled(enabled);

      grid->addWidget(lb, _curGridRow, 0);
      grid->addWidget(dl, _curGridRow+1, 0);
      grid->addWidget(knob, _curGridRow, 1, 2, 1);
      _curGridRow += 2;
      
      connect(knob, SIGNAL(sliderMoved(double,int)), slot);
      connect(knob, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(controlRightClicked(const QPoint &, int)));
      connect(dl, SIGNAL(valueChanged(double, int)), slot);
      connect(dl, SIGNAL(ctrlDoubleClicked(int)), SLOT(labelDoubleClicked(int)));
      }

//---------------------------------------------------------
//   MidiStrip
//---------------------------------------------------------

MidiStrip::MidiStrip(QWidget* parent, MusECore::MidiTrack* t)
   : Strip(parent, t)
      {
      inHeartBeat = true;

      // Set the whole strip's font, except for the label.    p4.0.45
      setFont(MusEGlobal::config.fonts[1]);
      
      // Clear so the meters don't start off by showing stale values.
      t->setActivity(0);
      t->setLastActivity(0);
      
      volume      = MusECore::CTRL_VAL_UNKNOWN;
      pan         = MusECore::CTRL_VAL_UNKNOWN;
      variSend    = MusECore::CTRL_VAL_UNKNOWN;
      chorusSend  = MusECore::CTRL_VAL_UNKNOWN;
      reverbSend  = MusECore::CTRL_VAL_UNKNOWN;
      
      addKnob(KNOB_VAR_SEND, tr("VariationSend"), tr("Var"), SLOT(setVariSend(double)), false);
      addKnob(KNOB_REV_SEND, tr("ReverbSend"), tr("Rev"), SLOT(setReverbSend(double)), false);
      addKnob(KNOB_CHO_SEND, tr("ChorusSend"), tr("Cho"), SLOT(setChorusSend(double)), false);
      ///int auxsSize = MusEGlobal::song->auxs()->size();
      ///if (auxsSize)
            //layout->addSpacing((STRIP_WIDTH/2 + 1) * auxsSize);
            ///grid->addSpacing((STRIP_WIDTH/2 + 1) * auxsSize);  // ??

      //---------------------------------------------------
      //    slider, label, meter
      //---------------------------------------------------

      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[t->outPort()];
      MusECore::MidiController* mc = mp->midiController(MusECore::CTRL_VOLUME);
      int chan  = t->outChannel();
      int mn = mc->minVal();
      int mx = mc->maxVal();
      
      slider = new MusEGui::Slider(this, "vol", Qt::Vertical, MusEGui::Slider::None,
                          QColor(100, 255, 100));
      slider->setCursorHoming(true);
      slider->setRange(double(mn), double(mx), 1.0);
      slider->setFixedWidth(20);
      ///slider->setFont(MusEGlobal::config.fonts[1]);
      slider->setId(MusECore::CTRL_VOLUME);

      meter[0] = new MusEGui::Meter(this, MusEGui::Meter::LinMeter);
      meter[0]->setRange(0, 127.0);
      meter[0]->setFixedWidth(15);
      connect(meter[0], SIGNAL(mousePress()), this, SLOT(resetPeaks()));
      
      sliderGrid = new QGridLayout(); 
      sliderGrid->setRowStretch(0, 100);
      sliderGrid->addWidget(slider, 0, 0, Qt::AlignHCenter);
      sliderGrid->addWidget(meter[0], 0, 1, Qt::AlignHCenter);
      grid->addLayout(sliderGrid, _curGridRow++, 0, 1, 2); 

      sl = new MusEGui::DoubleLabel(0.0, -98.0, 0.0, this);
      ///sl->setFont(MusEGlobal::config.fonts[1]);
      sl->setBackgroundRole(QPalette::Mid);
      sl->setSpecialText(tr("off"));
      sl->setSuffix(tr("dB"));
      sl->setToolTip(tr("ctrl-double-click on/off"));
      sl->setFrame(true);
      sl->setPrecision(0);
      sl->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum));
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
        

//      connect(sl, SIGNAL(valueChanged(double,int)), slider, SLOT(setValue(double)));
//      connect(slider, SIGNAL(valueChanged(double,int)), sl, SLOT(setValue(double)));
      connect(slider, SIGNAL(sliderMoved(double,int)), SLOT(setVolume(double)));
      connect(slider, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(controlRightClicked(const QPoint &, int)));
      connect(sl, SIGNAL(valueChanged(double, int)), SLOT(volLabelChanged(double)));
      connect(sl, SIGNAL(ctrlDoubleClicked(int)), SLOT(labelDoubleClicked(int)));
      
      grid->addWidget(sl, _curGridRow++, 0, 1, 2, Qt::AlignCenter); 

      //---------------------------------------------------
      //    pan, balance
      //---------------------------------------------------

      addKnob(KNOB_PAN, tr("Pan/Balance"), tr("Pan"), SLOT(setPan(double)), true);

      updateControls();
      
      //---------------------------------------------------
      //    mute, solo
      //    or
      //    record, mixdownfile
      //---------------------------------------------------

      record  = new MusEGui::TransparentToolButton(this);
      record->setFocusPolicy(Qt::NoFocus);
      record->setBackgroundRole(QPalette::Mid);
      record->setCheckable(true);
      record->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
      record->setToolTip(tr("record"));
      record->setChecked(track->recordFlag());
      record->setIcon(track->recordFlag() ? QIcon(*record_on_Icon) : QIcon(*record_off_Icon));
      ///record->setIconSize(record_on_Icon->size());  
      connect(record, SIGNAL(clicked(bool)), SLOT(recordToggled(bool)));

      mute  = new QToolButton();
      mute->setFocusPolicy(Qt::NoFocus);
      mute->setCheckable(true);
      mute->setToolTip(tr("mute"));
      mute->setChecked(track->mute());
      mute->setIcon(track->mute() ? QIcon(*muteIconOff) : QIcon(*muteIconOn));
      ///mute->setIconSize(muteIconOn->size());  
      mute->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
      connect(mute, SIGNAL(clicked(bool)), SLOT(muteToggled(bool)));

      solo  = new QToolButton();
      solo->setFocusPolicy(Qt::NoFocus);
      solo->setToolTip(tr("solo mode"));
      solo->setCheckable(true);
      solo->setChecked(track->solo());
      solo->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
      if(track->internalSolo())
        solo->setIcon(track->solo() ? QIcon(*soloblksqIconOn) : QIcon(*soloblksqIconOff));
      else
        solo->setIcon(track->solo() ? QIcon(*soloIconOn) : QIcon(*soloIconOff));
      ///solo->setIconSize(soloIconOn->size());  
      connect(solo, SIGNAL(clicked(bool)), SLOT(soloToggled(bool)));
      
      off  = new MusEGui::TransparentToolButton(this);
      off->setFocusPolicy(Qt::NoFocus);
      off->setBackgroundRole(QPalette::Mid);
      off->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
      off->setCheckable(true);
      off->setToolTip(tr("off"));
      off->setChecked(track->off());
      off->setIcon(track->off() ? QIcon(*exit1Icon) : QIcon(*exitIcon));
      ///off->setIconSize(exit1Icon->size());  
      connect(off, SIGNAL(clicked(bool)), SLOT(offToggled(bool)));

      grid->addWidget(off, _curGridRow, 0);
      grid->addWidget(record, _curGridRow++, 1);
      grid->addWidget(mute, _curGridRow, 0);
      grid->addWidget(solo, _curGridRow++, 1);

      //---------------------------------------------------
      //    routing
      //---------------------------------------------------

      iR = new QToolButton();
      iR->setFocusPolicy(Qt::NoFocus);
      ///iR->setFont(MusEGlobal::config.fonts[1]);
      iR->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum));
      ///iR->setText(tr("iR"));
      iR->setIcon(QIcon(*routesMidiInIcon));
      iR->setIconSize(routesMidiInIcon->size());  
      iR->setCheckable(false);
      iR->setToolTip(tr("input routing"));
      grid->addWidget(iR, _curGridRow, 0);
      connect(iR, SIGNAL(pressed()), SLOT(iRoutePressed()));
      
      oR = new QToolButton();
      oR->setFocusPolicy(Qt::NoFocus);
      ///oR->setFont(MusEGlobal::config.fonts[1]);
      oR->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum));
      ///oR->setText(tr("oR"));
      oR->setIcon(QIcon(*routesMidiOutIcon));
      oR->setIconSize(routesMidiOutIcon->size());  
      oR->setCheckable(false);
      // TODO: Works OK, but disabled for now, until we figure out what to do about multiple out routes and display values...
      // Enabled (for Midi Port to Audio Input routing). p4.0.14 Tim.
      //oR->setEnabled(false);
      oR->setToolTip(tr("output routing"));
      grid->addWidget(oR, _curGridRow++, 1);
      connect(oR, SIGNAL(pressed()), SLOT(oRoutePressed()));

      //---------------------------------------------------
      //    automation mode
      //---------------------------------------------------

      autoType = new MusEGui::ComboBox();
      autoType->setFocusPolicy(Qt::NoFocus);
      ///autoType->setFont(MusEGlobal::config.fonts[1]);
      autoType->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
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

      grid->addWidget(autoType, _curGridRow++, 0, 1, 2);
      connect(MusEGlobal::heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
      inHeartBeat = false;
      }

//---------------------------------------------------------
//   updateOffState
//---------------------------------------------------------

void MidiStrip::updateOffState()
      {
      bool val = !track->off();
      slider->setEnabled(val);
      sl->setEnabled(val);
      controller[KNOB_PAN].knob->setEnabled(val);         
      controller[KNOB_PAN].dl->setEnabled(val);         
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
            off->setIcon(track->off() ? QIcon(*exit1Icon) : QIcon(*exitIcon));
            //off->setIconSize(exit1Icon->size());  
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
    setFont(MusEGlobal::config.fonts[1]);
  
  // Set the strip label's font.
  setLabelFont();
  setLabelText();        
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MidiStrip::songChanged(int val)
      {
      if (mute && (val & SC_MUTE)) {      // mute && off
            mute->blockSignals(true);
            //mute->setChecked(track->isMute());  
            mute->setChecked(track->mute());
            mute->blockSignals(false);
            mute->setIcon(track->mute() ? QIcon(*muteIconOff) : QIcon(*muteIconOn));
            //mute->setIconSize(muteIconOn->size());  
            updateOffState();
            }
      if (solo && (val & SC_SOLO)) 
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
    }

//---------------------------------------------------------
//   controlRightClicked
//---------------------------------------------------------

void MidiStrip::controlRightClicked(const QPoint &p, int id)
{
  MusEGlobal::song->execMidiAutomationCtlPopup((MusECore::MidiTrack*)track, 0, p, id);
}

//---------------------------------------------------------
//   labelDoubleClicked
//---------------------------------------------------------

void MidiStrip::labelDoubleClicked(int idx)
{
  //int mn, mx, v;
  //int num = MusECore::CTRL_VOLUME;
  int num;
  switch(idx)
  {
    case KNOB_PAN:
      num = MusECore::CTRL_PANPOT;
    break;
    case KNOB_VAR_SEND:
      num = MusECore::CTRL_VARIATION_SEND;
    break;
    case KNOB_REV_SEND:
      num = MusECore::CTRL_REVERB_SEND;
    break;
    case KNOB_CHO_SEND:
      num = MusECore::CTRL_CHORUS_SEND;
    break;
    //case -1:
    default:
      num = MusECore::CTRL_VOLUME;
    break;  
  }
  int outport = ((MusECore::MidiTrack*)track)->outPort();
  int chan = ((MusECore::MidiTrack*)track)->outChannel();
  MusECore::MidiPort* mp = &MusEGlobal::midiPorts[outport];
  MusECore::MidiController* mc = mp->midiController(num);
  
  int lastv = mp->lastValidHWCtrlState(chan, num);
  int curv = mp->hwCtrlState(chan, num);
  
  if(curv == MusECore::CTRL_VAL_UNKNOWN)
  {
    // If no value has ever been set yet, use the current knob value 
    //  (or the controller's initial value?) to 'turn on' the controller.
    if(lastv == MusECore::CTRL_VAL_UNKNOWN)
    {
      //int kiv = _ctrl->initVal());
      int kiv;
      if(idx == -1)
        kiv = lrint(slider->value());
      else
        kiv = lrint(controller[idx].knob->value());
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

//---------------------------------------------------------
//   updateControls
//---------------------------------------------------------

void MidiStrip::updateControls()
      {
        bool en;
        int channel  = ((MusECore::MidiTrack*)track)->outChannel();
        MusECore::MidiPort* mp = &MusEGlobal::midiPorts[((MusECore::MidiTrack*)track)->outPort()];
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
        
        
          KNOB* gcon = &controller[KNOB_PAN];
          ctrl = mp->midiController(MusECore::CTRL_PANPOT);
          int npan = mp->hwCtrlState(channel, MusECore::CTRL_PANPOT);
          if(npan == MusECore::CTRL_VAL_UNKNOWN)
          {
            // MusEGui::DoubleLabel ignores the value if already set...
            //if(npan != pan) 
            //{
              gcon->dl->setValue(gcon->dl->off() - 1.0);
              //pan = npan;
            //}
            pan = MusECore::CTRL_VAL_UNKNOWN;
            npan = mp->lastValidHWCtrlState(channel, MusECore::CTRL_PANPOT);
            if(npan != MusECore::CTRL_VAL_UNKNOWN)
            {
              npan -= ctrl->bias();
              if(double(npan) != gcon->knob->value())
              {
                //printf("MidiStrip::updateControls setting pan knob\n");
                
                gcon->knob->setValue(double(npan));
              }  
            }
          }
          else
          {
            npan -= ctrl->bias();
            if(npan != pan) 
            {
                //printf("MidiStrip::updateControls setting pan label and knob\n");
                
                //controller[KNOB_PAN].knob->blockSignals(true);
                gcon->knob->setValue(double(npan));
                gcon->dl->setValue(double(npan));
                //controller[KNOB_PAN].knob->blockSignals(false);
                pan = npan;
            }
          }        
              
              
        icl = mc->find(channel, MusECore::CTRL_VARIATION_SEND);
        en = icl != mc->end();
        
        gcon = &controller[KNOB_VAR_SEND];
        if(gcon->knob->isEnabled() != en)
          gcon->knob->setEnabled(en);
        if(gcon->lb->isEnabled() != en)
          gcon->lb->setEnabled(en);
        if(gcon->dl->isEnabled() != en)
          gcon->dl->setEnabled(en);
          
        if(en)
        {
          ctrl = mp->midiController(MusECore::CTRL_VARIATION_SEND);
          int nvariSend = icl->second->hwVal();
          if(nvariSend == MusECore::CTRL_VAL_UNKNOWN)
          {
            // MusEGui::DoubleLabel ignores the value if already set...
            //if(nvariSend != variSend) 
            //{
              gcon->dl->setValue(gcon->dl->off() - 1.0);
              //variSend = nvariSend;
            //}
            variSend = MusECore::CTRL_VAL_UNKNOWN;
            nvariSend = mp->lastValidHWCtrlState(channel, MusECore::CTRL_VARIATION_SEND);
            if(nvariSend != MusECore::CTRL_VAL_UNKNOWN)
            {
              nvariSend -= ctrl->bias();
              if(double(nvariSend) != gcon->knob->value())
              {
                gcon->knob->setValue(double(nvariSend));
              }  
            }
          }
          else
          {
            nvariSend -= ctrl->bias();
            if(nvariSend != variSend) 
            {
              //controller[KNOB_VAR_SEND].knob->blockSignals(true);
              gcon->knob->setValue(double(nvariSend));
              gcon->dl->setValue(double(nvariSend));
              //controller[KNOB_VAR_SEND].knob->blockSignals(false);
              variSend = nvariSend;
            }  
          }  
        }
        
        icl = mc->find(channel, MusECore::CTRL_REVERB_SEND);
        en = icl != mc->end();
        
        gcon = &controller[KNOB_REV_SEND];
        if(gcon->knob->isEnabled() != en)
          gcon->knob->setEnabled(en);
        if(gcon->lb->isEnabled() != en)
          gcon->lb->setEnabled(en);
        if(gcon->dl->isEnabled() != en)
          gcon->dl->setEnabled(en);
        
        if(en)
        {
          ctrl = mp->midiController(MusECore::CTRL_REVERB_SEND);
          int nreverbSend = icl->second->hwVal();
          if(nreverbSend == MusECore::CTRL_VAL_UNKNOWN)
          {
            // MusEGui::DoubleLabel ignores the value if already set...
            //if(nreverbSend != reverbSend) 
            //{
              gcon->dl->setValue(gcon->dl->off() - 1.0);
              //reverbSend = nreverbSend;
            //}
            reverbSend = MusECore::CTRL_VAL_UNKNOWN;
            nreverbSend = mp->lastValidHWCtrlState(channel, MusECore::CTRL_REVERB_SEND);
            if(nreverbSend != MusECore::CTRL_VAL_UNKNOWN)
            {
              nreverbSend -= ctrl->bias();
              if(double(nreverbSend) != gcon->knob->value())
              {
                gcon->knob->setValue(double(nreverbSend));
              }  
            }
          }
          else
          {
            nreverbSend -= ctrl->bias();
            if(nreverbSend != reverbSend) 
            {
              //controller[KNOB_REV_SEND].knob->blockSignals(true);
              gcon->knob->setValue(double(nreverbSend));
              gcon->dl->setValue(double(nreverbSend));
              //controller[KNOB_REV_SEND].knob->blockSignals(false);
              reverbSend = nreverbSend;
            }
          }    
        }
        
        icl = mc->find(channel, MusECore::CTRL_CHORUS_SEND);
        en = icl != mc->end();
        
        gcon = &controller[KNOB_CHO_SEND];
        if(gcon->knob->isEnabled() != en)
          gcon->knob->setEnabled(en);
        if(gcon->lb->isEnabled() != en)
          gcon->lb->setEnabled(en);
        if(gcon->dl->isEnabled() != en)
          gcon->dl->setEnabled(en);
        
        if(en)
        {
          ctrl = mp->midiController(MusECore::CTRL_CHORUS_SEND);
          int nchorusSend = icl->second->hwVal();
          if(nchorusSend == MusECore::CTRL_VAL_UNKNOWN)
          {
            // MusEGui::DoubleLabel ignores the value if already set...
            //if(nchorusSend != chorusSend) 
            //{
              gcon->dl->setValue(gcon->dl->off() - 1.0);
              //chorusSend = nchorusSend;
            //}
            chorusSend = MusECore::CTRL_VAL_UNKNOWN;
            nchorusSend = mp->lastValidHWCtrlState(channel, MusECore::CTRL_CHORUS_SEND);
            if(nchorusSend != MusECore::CTRL_VAL_UNKNOWN)
            {
              nchorusSend -= ctrl->bias();
              if(double(nchorusSend) != gcon->knob->value())
              {
                gcon->knob->setValue(double(nchorusSend));
              }  
            }
          }
          else
          {
            nchorusSend -= ctrl->bias();
            if(nchorusSend != chorusSend) 
            {
              gcon->knob->setValue(double(nchorusSend));
              gcon->dl->setValue(double(nchorusSend));
              chorusSend = nchorusSend;
            }  
          }  
        }
      }
//---------------------------------------------------------
//   ctrlChanged
//---------------------------------------------------------

void MidiStrip::ctrlChanged(int num, int val)
    {
      if (inHeartBeat)
            return;
      
      MusECore::MidiTrack* t = (MusECore::MidiTrack*) track;
      int port     = t->outPort();
      
      int chan  = t->outChannel();
      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
      MusECore::MidiController* mctl = mp->midiController(num);
      if((val < mctl->minVal()) || (val > mctl->maxVal()))
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

//---------------------------------------------------------
//   volLabelChanged
//---------------------------------------------------------

void MidiStrip::volLabelChanged(double val)
      {
      val = sqrt( float(127*127) / pow(10.0, -val/20.0) );
      
      ctrlChanged(MusECore::CTRL_VOLUME, lrint(val));
      
      }
      
//---------------------------------------------------------
//   setVolume
//---------------------------------------------------------

void MidiStrip::setVolume(double val)
      {
      
// printf("Vol %d\n", lrint(val));
      ctrlChanged(MusECore::CTRL_VOLUME, lrint(val));
      }
      
//---------------------------------------------------------
//   setPan
//---------------------------------------------------------

void MidiStrip::setPan(double val)
      {
      
      ctrlChanged(MusECore::CTRL_PANPOT, lrint(val));
      }

//---------------------------------------------------------
//   setVariSend
//---------------------------------------------------------

void MidiStrip::setVariSend(double val)
      {
      ctrlChanged(MusECore::CTRL_VARIATION_SEND, lrint(val));
      }
      
//---------------------------------------------------------
//   setChorusSend
//---------------------------------------------------------

void MidiStrip::setChorusSend(double val)
      {
      ctrlChanged(MusECore::CTRL_CHORUS_SEND, lrint(val));
      }
      
//---------------------------------------------------------
//   setReverbSend
//---------------------------------------------------------

void MidiStrip::setReverbSend(double val)
      {
      ctrlChanged(MusECore::CTRL_REVERB_SEND, lrint(val));
      }
      
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

} // namespace MusEGui
