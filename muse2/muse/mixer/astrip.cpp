//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: astrip.cpp,v 1.23.2.17 2009/11/16 01:55:55 terminator356 Exp $
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
#include <QApplication>
//#include <QDialog>
#include <QToolButton>
#include <QLabel>
#include <QComboBox>
#include <QToolTip>
#include <QTimer>
//#include <QPopupMenu>
#include <QCursor>
#include <QPainter>
#include <QString>
#include <QPoint>
#include <QEvent>
#include <QWidget>
#include <QVariant>
#include <QAction>
#include <QGridLayout>
//#include <QLinearGradient>

#include "app.h"
#include "globals.h"
#include "audio.h"
//#include "driver/audiodev.h"
#include "song.h"
#include "slider.h"
#include "knob.h"
#include "combobox.h"
#include "meter.h"
#include "astrip.h"
#include "track.h"
#include "synth.h"
//#include "route.h"
#include "doublelabel.h"
#include "rack.h"
#include "node.h"
#include "amixer.h"
#include "icons.h"
#include "gconfig.h"
#include "ttoolbutton.h"
#include "menutitleitem.h"
//#include "popupmenu.h"
#include "routepopup.h"

namespace MusEGui {

/*
//---------------------------------------------------------
//   minimumSizeHint
//---------------------------------------------------------

QSize AudioStrip::minimumSizeHint () const
{
    // We force the width of the size hint to be what we want
    //return QWidget::minimumSizeHint();
    ///return QSize(66,QWidget::minimumSizeHint().height());
}

//---------------------------------------------------------
//   sizeHint
//---------------------------------------------------------

QSize AudioStrip::sizeHint () const
{
    // We force the width of the size hint to be what we want
    //return QWidget::minimumSizeHint();
    //return QSize(66,QWidget::minimumSizeHint().height());
    return minimumSizeHint();
}
*/

//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void AudioStrip::heartBeat()
      {
        for (int ch = 0; ch < track->channels(); ++ch) {
          if (meter[ch]) {
            //int meterVal = track->meter(ch);
            //int peak  = track->peak(ch);
            //meter[ch]->setVal(meterVal, peak, false);
            meter[ch]->setVal(track->meter(ch), track->peak(ch), false);
          }
        }
        Strip::heartBeat();
                  updateVolume();
                  updatePan();
            }

//---------------------------------------------------------
//   configChanged
//   Catch when label font, or configuration min slider and meter values change, or viewable tracks etc.
//---------------------------------------------------------

void AudioStrip::configChanged()    
{ 
  // Set the whole strip's font, except for the label.
  if(font() != MusEGlobal::config.fonts[1])
    setFont(MusEGlobal::config.fonts[1]);
  
  // Set the strip label's font.
  setLabelFont();
  setLabelText();        
  
  // Adjust minimum volume slider and label values.
  slider->setRange(MusEGlobal::config.minSlider-0.1, 10.0);
  sl->setRange(MusEGlobal::config.minSlider, 10.0);
  
  // Adjust minimum aux knob and label values.
  int n = auxKnob.size();
  for (int idx = 0; idx < n; ++idx) 
  {
    auxKnob[idx]->blockSignals(true);
    auxLabel[idx]->blockSignals(true);
    auxKnob[idx]->setRange(MusEGlobal::config.minSlider-0.1, 10.0);
    auxLabel[idx]->setRange(MusEGlobal::config.minSlider, 10.1);
    auxKnob[idx]->blockSignals(false);
    auxLabel[idx]->blockSignals(false);
  }
  
  // Adjust minimum meter values.
  for(int c = 0; c < channel; ++c) 
    meter[c]->setRange(MusEGlobal::config.minMeter, 10.0);
}

void AudioStrip::updateRouteButtons()
{
    if (iR)
    {
        if (track->noInRoute())
          iR->setStyleSheet("background-color:darkgray;");
        else
          iR->setStyleSheet("");
    }

    if (track->noOutRoute())
      oR->setStyleSheet("background-color:red;");
    else
      oR->setStyleSheet("");
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void AudioStrip::songChanged(int val)
      {
      // Is it simply a midi controller value adjustment? Forget it.
      if (val == SC_MIDI_CONTROLLER)
        return;

      updateRouteButtons();
    
      MusECore::AudioTrack* src = (MusECore::AudioTrack*)track;
      
      // Do channels before MusEGlobal::config...
      if (val & SC_CHANNELS)
        updateChannels();
      
      // Catch when label font, or configuration min slider and meter values change.
      if (val & SC_CONFIG)
      {
        // So far only 1 instance of sending SC_CONFIG in the entire app, in instrument editor when a new instrument is saved.
      }
      
      if (mute && (val & SC_MUTE)) {      // mute && off
            mute->blockSignals(true);
            mute->setChecked(src->mute());
            mute->blockSignals(false);
            mute->setIcon(src->mute() ? QIcon(*muteIconOff) : QIcon(*muteIconOn));
            //mute->setIconSize(muteIconOn->size());  
            updateOffState();
            }
      if (solo && (val & SC_SOLO)) {
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
      //if (val & SC_CHANNELS)
      //      updateChannels();
      if (val & SC_ROUTE) {
            if (pre) {
                  pre->blockSignals(true);
                  pre->setChecked(src->prefader());
                  pre->blockSignals(false);
                  }
            
            // Are there any Aux Track routing paths to this track? Then we cannot process aux for this track! 
            // Hate to do this, but as a quick visual reminder, seems most logical to disable Aux knobs and labels. 
            int rc = track->auxRefCount();
            int n = auxKnob.size();
            for (int idx = 0; idx < n; ++idx) 
            {  
              auxKnob[idx]->setEnabled( rc == 0 );
              auxLabel[idx]->setEnabled( rc == 0 );
            }  
          }
      if (val & SC_AUX) {
            int n = auxKnob.size();
            for (int idx = 0; idx < n; ++idx) {
                  double val = MusECore::fast_log10(src->auxSend(idx)) * 20.0;
                  auxKnob[idx]->blockSignals(true);
                  auxLabel[idx]->blockSignals(true);
                  auxKnob[idx]->setValue(val);
                  auxLabel[idx]->setValue(val);
                  auxKnob[idx]->blockSignals(false);
                  auxLabel[idx]->blockSignals(false);
                  }
            }
      if (autoType && (val & SC_AUTOMATION)) {
            autoType->blockSignals(true);
            autoType->setCurrentItem(track->automationType());
            QPalette palette;
            //QLinearGradient gradient(autoType->geometry().topLeft(), autoType->geometry().bottomLeft());
            if(track->automationType() == AUTO_TOUCH || track->automationType() == AUTO_WRITE)
                  {
                  palette.setColor(QPalette::Button, QColor(215, 76, 39)); // red
                  //palette.setColor(QPalette::Window, QColor(215, 76, 39)); // red
                  /*QLinearGradient gradient(autoType->geometry().topLeft(), autoType->geometry().bottomLeft());
                  QColor c(Qt::red);
                  //QColor c(215, 76, 39);       // red
                  gradient.setColorAt(0, c.darker());
                  gradient.setColorAt(0.5, c);
                  gradient.setColorAt(1, c.darker());
                  palette.setBrush(QPalette::Button, gradient);
                  //palette.setBrush(autoType->backgroundRole(), gradient);
                  //palette.setBrush(QPalette::Window, gradient);   */
                  autoType->setPalette(palette);
                  }
            else if(track->automationType() == AUTO_READ)
                  {
                  palette.setColor(QPalette::Button, QColor(100, 172, 49)); // green
                  //palette.setColor(QPalette::Window, QColor(100, 172, 49)); // green
                  /*QLinearGradient gradient(autoType->geometry().topLeft(), autoType->geometry().bottomLeft());
                  QColor c(Qt::green);
                  //QColor c(100, 172, 49);     // green
                  gradient.setColorAt(0, c.darker());
                  gradient.setColorAt(0.5, c);
                  gradient.setColorAt(1, c.darker());
                  palette.setBrush(QPalette::Button, gradient);
                  //palette.setBrush(autoType->backgroundRole(), gradient);
                  //palette.setBrush(QPalette::Window, gradient);  */
                  autoType->setPalette(palette);
                  }
            else  
                  {
                  palette.setColor(QPalette::Button, qApp->palette().color(QPalette::Active, QPalette::Background));
                  //QColor c(qApp->palette().color(QPalette::Active, QPalette::Background));
                  //gradient.setColorAt(0, c);
                  //gradient.setColorAt(1, c.darker());
                  //palette.setBrush(QPalette::Button, gradient);
                  autoType->setPalette(palette);
                  }
      
            autoType->blockSignals(false);
            }
      }

//---------------------------------------------------------
//   updateVolume
//---------------------------------------------------------

void AudioStrip::updateVolume()
{
      double vol = ((MusECore::AudioTrack*)track)->volume();
        if (vol != volume) 
        {
            //printf("AudioStrip::updateVolume setting slider and label\n");
          
            slider->blockSignals(true);
            sl->blockSignals(true);
            double val = MusECore::fast_log10(vol) * 20.0;
            slider->setValue(val);
            sl->setValue(val);
            sl->blockSignals(false);
            slider->blockSignals(false);
            volume = vol;
            }
}

//---------------------------------------------------------
//   updatePan
//---------------------------------------------------------

void AudioStrip::updatePan()
{
      double v = ((MusECore::AudioTrack*)track)->pan();
        if (v != panVal) 
        {
            //printf("AudioStrip::updatePan setting slider and label\n");
            
            pan->blockSignals(true);
            panl->blockSignals(true);
            pan->setValue(v);
            panl->setValue(v);
            panl->blockSignals(false);
            pan->blockSignals(false);
            panVal = v;
            }
}       

//---------------------------------------------------------
//   offToggled
//---------------------------------------------------------

void AudioStrip::offToggled(bool val)
      {
      track->setOff(val);
      MusEGlobal::song->update(SC_MUTE);
      }

//---------------------------------------------------------
//   updateOffState
//---------------------------------------------------------

void AudioStrip::updateOffState()
      {
      bool val = !track->off();
      slider->setEnabled(val);
      sl->setEnabled(val);
      pan->setEnabled(val);
      panl->setEnabled(val);
      if (track->type() != MusECore::Track::AUDIO_SOFTSYNTH)
            stereo->setEnabled(val);
      label->setEnabled(val);
      
      // Are there any Aux Track routing paths to this track? Then we cannot process aux for this track! 
      // Hate to do this, but as a quick visual reminder, seems most logical to disable Aux knobs and labels. 
      bool ae = track->auxRefCount() == 0 && val;
      int n = auxKnob.size();
      for (int i = 0; i < n; ++i) 
      {
        auxKnob[i]->setEnabled(ae);
        auxLabel[i]->setEnabled(ae);
      }
            
      if (pre)
            pre->setEnabled(val);
      if (record)
            record->setEnabled(val);
      if (solo)
            solo->setEnabled(val);
      if (mute)
            mute->setEnabled(val);
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
//   preToggled
//---------------------------------------------------------

void AudioStrip::preToggled(bool val)
      {
      MusEGlobal::audio->msgSetPrefader((MusECore::AudioTrack*)track, val);
      resetPeaks();
      MusEGlobal::song->update(SC_ROUTE);
      }

//---------------------------------------------------------
//   stereoToggled
//---------------------------------------------------------

void AudioStrip::stereoToggled(bool val)
      {
      int oc = track->channels();
      int nc = val ? 2 : 1;
//      stereo->setIcon(nc == 2 ? *stereoIcon : *monoIcon);
      if (oc == nc)
            return;
      MusEGlobal::audio->msgSetChannels((MusECore::AudioTrack*)track, nc);
      MusEGlobal::song->update(SC_CHANNELS);
      }

//---------------------------------------------------------
//   auxChanged
//---------------------------------------------------------

void AudioStrip::auxChanged(double val, int idx)
      {
      double vol;
      if (val <= MusEGlobal::config.minSlider) {
            vol = 0.0;
            val -= 1.0; // display special value "off"
            }
      else
            vol = pow(10.0, val/20.0);
      MusEGlobal::audio->msgSetAux((MusECore::AudioTrack*)track, idx, vol);
      MusEGlobal::song->update(SC_AUX);
      }

//---------------------------------------------------------
//   auxLabelChanged
//---------------------------------------------------------

void AudioStrip::auxLabelChanged(double val, unsigned int idx) 
      {
        if(idx >= auxKnob.size())
          return;    
        auxKnob[idx]->setValue(val);
      }

//---------------------------------------------------------
//   volumeChanged
//---------------------------------------------------------

void AudioStrip::volumeChanged(double val, int, bool shift_pressed)
      {
      AutomationType at = ((MusECore::AudioTrack*)track)->automationType();
      if ( (at == AUTO_WRITE) ||
           (at == AUTO_TOUCH && MusEGlobal::audio->isPlaying()) )
        track->enableVolumeController(false);
      
      double vol;
      if (val <= MusEGlobal::config.minSlider) {
            vol = 0.0;
            val -= 1.0; // display special value "off"
            }
      else
            vol = pow(10.0, val/20.0);
      volume = vol;
      //MusEGlobal::audio->msgSetVolume((MusECore::AudioTrack*)track, vol);
      // p4.0.21 MusEGlobal::audio->msgXXX waits. Do we really need to?
      ((MusECore::AudioTrack*)track)->setVolume(vol);
      if (!shift_pressed) ((MusECore::AudioTrack*)track)->recordAutomation(MusECore::AC_VOLUME, vol);
      }

//---------------------------------------------------------
//   volumePressed
//---------------------------------------------------------

void AudioStrip::volumePressed()
      {
      AutomationType at = ((MusECore::AudioTrack*)track)->automationType();
      if (at == AUTO_READ || at == AUTO_TOUCH || at == AUTO_WRITE)
        track->enableVolumeController(false);
      
      double val = slider->value();
      double vol;
      if (val <= MusEGlobal::config.minSlider) {
            vol = 0.0;
            //val -= 1.0; // display special value "off"
            }
      else
            vol = pow(10.0, val/20.0);
      volume = vol;
      //MusEGlobal::audio->msgSetVolume((MusECore::AudioTrack*)track, volume);
      // p4.0.21 MusEGlobal::audio->msgXXX waits. Do we really need to?
      ((MusECore::AudioTrack*)track)->setVolume(volume);
      ((MusECore::AudioTrack*)track)->startAutoRecord(MusECore::AC_VOLUME, volume);
      }

//---------------------------------------------------------
//   volumeReleased
//---------------------------------------------------------

void AudioStrip::volumeReleased()
      {
      AutomationType at = track->automationType();
      if (at == AUTO_OFF || at == AUTO_READ || at == AUTO_TOUCH)
        track->enableVolumeController(true);
      
      ((MusECore::AudioTrack*)track)->stopAutoRecord(MusECore::AC_VOLUME, volume);
      }

//---------------------------------------------------------
//   volumeRightClicked
//---------------------------------------------------------
void AudioStrip::volumeRightClicked(const QPoint &p)
{
  MusEGlobal::song->execAutomationCtlPopup((MusECore::AudioTrack*)track, p, MusECore::AC_VOLUME);
}

//---------------------------------------------------------
//   volLabelChanged
//---------------------------------------------------------

void AudioStrip::volLabelChanged(double val)
      {
      AutomationType at = ((MusECore::AudioTrack*)track)->automationType();
      if ( (at == AUTO_WRITE) ||
           (at == AUTO_TOUCH && MusEGlobal::audio->isPlaying()) )
        track->enableVolumeController(false);
      
      double vol;
      if (val <= MusEGlobal::config.minSlider) {
            vol = 0.0;
            val -= 1.0; // display special value "off"
            }
      else
            vol = pow(10.0, val/20.0);
      volume = vol;
      slider->setValue(val);
      //audio->msgSetVolume((MusECore::AudioTrack*)track, vol);
      // p4.0.21 audio->msgXXX waits. Do we really need to?
      ((MusECore::AudioTrack*)track)->setVolume(vol);
      ((MusECore::AudioTrack*)track)->startAutoRecord(MusECore::AC_VOLUME, vol);
      }

//---------------------------------------------------------
//   panChanged
//---------------------------------------------------------

void AudioStrip::panChanged(double val, int, bool shift_pressed)
      {
      AutomationType at = ((MusECore::AudioTrack*)track)->automationType();
      if ( (at == AUTO_WRITE) ||
           (at == AUTO_TOUCH && MusEGlobal::audio->isPlaying()) )
        track->enablePanController(false);
      
      panVal = val;  
      //MusEGlobal::audio->msgSetPan(((MusECore::AudioTrack*)track), val);
      // p4.0.21 MusEGlobal::audio->msgXXX waits. Do we really need to?
      ((MusECore::AudioTrack*)track)->setPan(val);
      if (!shift_pressed) ((MusECore::AudioTrack*)track)->recordAutomation(MusECore::AC_PAN, val);
      }

//---------------------------------------------------------
//   panPressed
//---------------------------------------------------------

void AudioStrip::panPressed()
      {
      AutomationType at = ((MusECore::AudioTrack*)track)->automationType();
      if (at == AUTO_READ || at == AUTO_TOUCH || at == AUTO_WRITE)
        track->enablePanController(false);
      
      panVal = pan->value();  
      //MusEGlobal::audio->msgSetPan(((MusECore::AudioTrack*)track), panVal);
      // p4.0.21 MusEGlobal::audio->msgXXX waits. Do we really need to?
      ((MusECore::AudioTrack*)track)->setPan(panVal);
      ((MusECore::AudioTrack*)track)->startAutoRecord(MusECore::AC_PAN, panVal);
      }

//---------------------------------------------------------
//   panReleased
//---------------------------------------------------------

void AudioStrip::panReleased()
      {
      AutomationType at = track->automationType();
      if (at == AUTO_OFF || at == AUTO_READ || at == AUTO_TOUCH)
        track->enablePanController(true);
      ((MusECore::AudioTrack*)track)->stopAutoRecord(MusECore::AC_PAN, panVal);
      }

//---------------------------------------------------------
//   panRightClicked
//---------------------------------------------------------
void AudioStrip::panRightClicked(const QPoint &p)
{
  MusEGlobal::song->execAutomationCtlPopup((MusECore::AudioTrack*)track, p, MusECore::AC_PAN);
}

//---------------------------------------------------------
//   panLabelChanged
//---------------------------------------------------------

void AudioStrip::panLabelChanged(double val)
      {
      AutomationType at = ((MusECore::AudioTrack*)track)->automationType(); 
      if ( (at == AUTO_WRITE) ||
           (at == AUTO_TOUCH && MusEGlobal::audio->isPlaying()) )
        track->enablePanController(false);
      
      panVal = val;
      pan->setValue(val);
      //MusEGlobal::audio->msgSetPan((MusECore::AudioTrack*)track, val);
      // p4.0.21 MusEGlobal::audio->msgXXX waits. Do we really need to?
      ((MusECore::AudioTrack*)track)->setPan(val);
      ((MusECore::AudioTrack*)track)->startAutoRecord(MusECore::AC_PAN, val);
      }

//---------------------------------------------------------
//   updateChannels
//---------------------------------------------------------
                                       
void AudioStrip::updateChannels()
      {
      MusECore::AudioTrack* t = (MusECore::AudioTrack*)track;
      int c = t->channels();
      //printf("AudioStrip::updateChannels track channels:%d current channels:%d\n", c, channel);
      
      if (c > channel) {
            for (int cc = channel; cc < c; ++cc) {
                  meter[cc] = new MusEGui::Meter(this);
                  //meter[cc]->setRange(MusEGlobal::config.minSlider, 10.0);
                  meter[cc]->setRange(MusEGlobal::config.minMeter, 10.0);
                  meter[cc]->setFixedWidth(15);
                  connect(meter[cc], SIGNAL(mousePress()), this, SLOT(resetPeaks()));
                  sliderGrid->addWidget(meter[cc], 0, cc+1, Qt::AlignLeft);
                  sliderGrid->setColumnStretch(cc, 50);
                  meter[cc]->show();
                  }
            }
      else if (c < channel) {
            for (int cc = channel-1; cc >= c; --cc) {
                  delete meter[cc];
                  meter[cc] = 0;
                  }
            }
      channel = c;
      stereo->blockSignals(true);
      stereo->setChecked(channel == 2);
      stereo->blockSignals(false);
      stereo->setIcon(channel == 2 ? QIcon(*stereoIcon) : QIcon(*monoIcon));
      //stereo->setIconSize(stereoIcon->size());  
      }

//---------------------------------------------------------
//   addKnob
//    type = 0 - panorama
//           1 - aux send
//---------------------------------------------------------

MusEGui::Knob* AudioStrip::addKnob(int type, int id, MusEGui::DoubleLabel** dlabel)
      {
      MusEGui::Knob* knob = new MusEGui::Knob(this);
      knob->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
      if (type == 0)
            knob->setRange(-1.0, +1.0);
      else
            knob->setRange(MusEGlobal::config.minSlider-0.1, 10.0);
      knob->setBackgroundRole(QPalette::Mid);

      if (type == 0)
            knob->setToolTip(tr("panorama"));
      else
            knob->setToolTip(tr("aux send level"));

      MusEGui::DoubleLabel* pl;
      if (type == 0)
            pl = new MusEGui::DoubleLabel(0, -1.0, +1.0, this);
      else
            pl = new MusEGui::DoubleLabel(0.0, MusEGlobal::config.minSlider, 10.1, this);
            
      if (dlabel)
            *dlabel = pl;
      pl->setSlider(knob);
      ///pl->setFont(MusEGlobal::config.fonts[1]);
      pl->setBackgroundRole(QPalette::Mid);
      pl->setFrame(true);
      if (type == 0)
            pl->setPrecision(2);
      else {
            pl->setPrecision(0);
            }
      pl->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
      
      QString label;
      if (type == 0)
            label = tr("Pan");
      else
            label.sprintf("Aux%d", id+1);

      QLabel* plb = new QLabel(label, this);
      ///plb->setFont(MusEGlobal::config.fonts[1]);
      plb->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
      plb->setAlignment(Qt::AlignCenter);

      grid->addWidget(plb, _curGridRow, 0);
      grid->addWidget(pl, _curGridRow+1, 0);
      grid->addWidget(knob, _curGridRow, 1, 2, 1);
      //grid->addWidget(plb, _curGridRow, 0, Qt::AlignCenter);
      //grid->addWidget(pl, _curGridRow+1, 0, Qt::AlignCenter);
      //grid->addWidget(knob, _curGridRow, 1, 2, 1, Qt::AlignCenter);
      _curGridRow += 2;

      connect(knob, SIGNAL(valueChanged(double,int)), pl, SLOT(setValue(double)));

      if (type == 0) {
            connect(pl, SIGNAL(valueChanged(double, int)), SLOT(panLabelChanged(double)));
            connect(knob, SIGNAL(sliderMoved(double,int,bool)), SLOT(panChanged(double,int,bool)));
            connect(knob, SIGNAL(sliderPressed(int)), SLOT(panPressed()));
            connect(knob, SIGNAL(sliderReleased(int)), SLOT(panReleased()));
            connect(knob, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(panRightClicked(const QPoint &)));
            }
      else {
            knob->setId(id);
            
            connect(pl, SIGNAL(valueChanged(double, int)), knob,  SLOT(setValue(double)));
            // Not used yet. Switch if/when necessary.
            //connect(pl, SIGNAL(valueChanged(double, int)), SLOT(auxLabelChanged(double, int)));
            
            connect(knob, SIGNAL(sliderMoved(double, int)), SLOT(auxChanged(double, int)));
            }
      return knob;
      }

//---------------------------------------------------------
//   AudioStrip
//---------------------------------------------------------

AudioStrip::~AudioStrip()
      {
      }


//---------------------------------------------------------
//   AudioStrip
//    create mixer strip
//---------------------------------------------------------

AudioStrip::AudioStrip(QWidget* parent, MusECore::AudioTrack* at)
   : Strip(parent, at)
      {
      volume        = -1.0;
      panVal        = 0;
      
      record        = 0;
      off           = 0;
      
      // Set the whole strip's font, except for the label.    p4.0.45
      setFont(MusEGlobal::config.fonts[1]);

      
      MusECore::AudioTrack* t = (MusECore::AudioTrack*)track;
      channel       = at->channels();
      ///setMinimumWidth(STRIP_WIDTH);
      
      int ch = 0;
      for (; ch < channel; ++ch)
            meter[ch] = new MusEGui::Meter(this);
      for (; ch < MAX_CHANNELS; ++ch)
            meter[ch] = 0;

      //---------------------------------------------------
      //    plugin rack
      //---------------------------------------------------

      rack = new EffectRack(this, t);
      rack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
      grid->addWidget(rack, _curGridRow++, 0, 1, 2);

      //---------------------------------------------------
      //    mono/stereo  pre/post
      //---------------------------------------------------

      stereo  = new QToolButton();
      ///stereo->setFont(MusEGlobal::config.fonts[1]);
      stereo->setFocusPolicy(Qt::NoFocus);
      stereo->setCheckable(true);
      stereo->setToolTip(tr("1/2 channel"));
      stereo->setChecked(channel == 2);
      stereo->setIcon(channel == 2 ? QIcon(*stereoIcon) : QIcon(*monoIcon));
      stereo->setIconSize(monoIcon->size());  
      stereo->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
      connect(stereo, SIGNAL(clicked(bool)), SLOT(stereoToggled(bool)));

      // disable mono/stereo for Synthesizer-Plugins
      if (t->type() == MusECore::Track::AUDIO_SOFTSYNTH)
            stereo->setEnabled(false);

      pre = new QToolButton();
      ///pre->setFont(MusEGlobal::config.fonts[1]);
      pre->setFocusPolicy(Qt::NoFocus);
      pre->setCheckable(true);
      pre->setText(tr("Pre"));
      pre->setToolTip(tr("pre fader - post fader"));
      pre->setChecked(t->prefader());
      pre->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
      connect(pre, SIGNAL(clicked(bool)), SLOT(preToggled(bool)));

      grid->addWidget(stereo, _curGridRow, 0);
      grid->addWidget(pre, _curGridRow++, 1);

      //---------------------------------------------------
      //    aux send
      //---------------------------------------------------

      int auxsSize = MusEGlobal::song->auxs()->size();
      if (t->hasAuxSend()) {
            for (int idx = 0; idx < auxsSize; ++idx) {
                  MusEGui::DoubleLabel* al;
                  MusEGui::Knob* ak = addKnob(1, idx, &al);
                  auxKnob.push_back(ak);
                  auxLabel.push_back(al);
                  double val = MusECore::fast_log10(t->auxSend(idx))*20.0;
                  ak->setValue(val);
                  al->setValue(val);
                  
                  // Are there any Aux Track routing paths to this track? Then we cannot process aux for this track! 
                  // Hate to do this, but as a quick visual reminder, seems most logical to disable Aux knobs and labels. 
                  int rc = track->auxRefCount();
                  ak->setEnabled( rc == 0 );
                  al->setEnabled( rc == 0 );
                  }
            }
      else {
            ///if (auxsSize)
                  //layout->addSpacing((STRIP_WIDTH/2 + 2) * auxsSize);
                  ///grid->addSpacing((STRIP_WIDTH/2 + 2) * auxsSize);  // ???
            }

      //---------------------------------------------------
      //    slider, label, meter
      //---------------------------------------------------

      sliderGrid = new QGridLayout(); 
      sliderGrid->setRowStretch(0, 100);
      sliderGrid->setContentsMargins(0, 0, 0, 0);
      sliderGrid->setSpacing(0);
      
      slider = new MusEGui::Slider(this, "vol", Qt::Vertical, MusEGui::Slider::None);

      slider->setCursorHoming(true);
      slider->setRange(MusEGlobal::config.minSlider-0.1, 10.0);
      slider->setFixedWidth(20);
      ///slider->setFont(MusEGlobal::config.fonts[1]);
      slider->setValue(MusECore::fast_log10(t->volume())*20.0);

      sliderGrid->addWidget(slider, 0, 0, Qt::AlignHCenter);

      for (int i = 0; i < channel; ++i) {
            //meter[i]->setRange(MusEGlobal::config.minSlider, 10.0);
            meter[i]->setRange(MusEGlobal::config.minMeter, 10.0);
            meter[i]->setFixedWidth(15);
            connect(meter[i], SIGNAL(mousePress()), this, SLOT(resetPeaks()));
            sliderGrid->addWidget(meter[i], 0, i+1, Qt::AlignHCenter);
            sliderGrid->setColumnStretch(i, 50);
            }
      sliderGrid->addItem(new QSpacerItem(2,0),0,3);
      grid->addLayout(sliderGrid, _curGridRow++, 0, 1, 2); 

      sl = new MusEGui::DoubleLabel(0.0, MusEGlobal::config.minSlider, 10.0, this);
      sl->setSlider(slider);
      ///sl->setFont(MusEGlobal::config.fonts[1]);
      sl->setBackgroundRole(QPalette::Mid);
      sl->setSuffix(tr("dB"));
      sl->setFrame(true);
      sl->setPrecision(0);
      sl->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum));
      sl->setValue(MusECore::fast_log10(t->volume()) * 20.0);

      connect(sl, SIGNAL(valueChanged(double,int)), SLOT(volLabelChanged(double)));
      connect(slider, SIGNAL(valueChanged(double,int)), sl, SLOT(setValue(double)));
      connect(slider, SIGNAL(sliderMoved(double,int,bool)), SLOT(volumeChanged(double,int,bool)));
      connect(slider, SIGNAL(sliderPressed(int)), SLOT(volumePressed()));
      connect(slider, SIGNAL(sliderReleased(int)), SLOT(volumeReleased()));
      connect(slider, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(volumeRightClicked(const QPoint &)));
      grid->addWidget(sl, _curGridRow++, 0, 1, 2, Qt::AlignCenter);

      //---------------------------------------------------
      //    pan, balance
      //---------------------------------------------------

      pan = addKnob(0, 0, &panl);
      pan->setValue(t->pan());
      
      //---------------------------------------------------
      //    mute, solo, record
      //---------------------------------------------------

      if (track->canRecord()) {
            record  = new MusEGui::TransparentToolButton(this);
	    record->setFocusPolicy(Qt::NoFocus);
            record->setCheckable(true);
            record->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
            record->setBackgroundRole(QPalette::Mid);
            record->setToolTip(tr("record"));
            record->setChecked(t->recordFlag());
            record->setIcon(t->recordFlag() ? QIcon(*record_on_Icon) : QIcon(*record_off_Icon));
            ///record->setIconSize(record_on_Icon->size());  
            connect(record, SIGNAL(clicked(bool)), SLOT(recordToggled(bool)));
            }

      MusECore::Track::TrackType type = t->type();

      mute  = new QToolButton();
      mute->setFocusPolicy(Qt::NoFocus);
      mute->setCheckable(true);
      mute->setToolTip(tr("mute"));
      mute->setChecked(t->mute());
      mute->setIcon(t->mute() ? QIcon(*muteIconOff) : QIcon(*muteIconOn));
      ///mute->setIconSize(muteIconOn->size());  
      mute->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
      connect(mute, SIGNAL(clicked(bool)), SLOT(muteToggled(bool)));

      solo  = new QToolButton();
      solo->setFocusPolicy(Qt::NoFocus);
      solo->setCheckable(true);
      solo->setChecked(t->solo());
      if(t->internalSolo())
        solo->setIcon(t->solo() ? QIcon(*soloblksqIconOn) : QIcon(*soloblksqIconOff));
      else
        solo->setIcon(t->solo() ? QIcon(*soloIconOn) : QIcon(*soloIconOff));
      ///solo->setIconSize(soloIconOn->size());  
      solo->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
      connect(solo, SIGNAL(clicked(bool)), SLOT(soloToggled(bool)));
      if (type == MusECore::Track::AUDIO_OUTPUT) {
            record->setToolTip(tr("record downmix"));
            //solo->setToolTip(tr("solo mode (monitor)"));
            solo->setToolTip(tr("solo mode"));
            }
      else {
            //solo->setToolTip(tr("pre fader listening"));
            solo->setToolTip(tr("solo mode"));
            }

      off  = new MusEGui::TransparentToolButton(this);
      off->setFocusPolicy(Qt::NoFocus);
      off->setBackgroundRole(QPalette::Mid);
      off->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
      off->setCheckable(true);
      off->setToolTip(tr("off"));
      off->setChecked(t->off());
      off->setIcon(t->off() ? QIcon(*exit1Icon) : QIcon(*exitIcon));
      ///off->setIconSize(exit1Icon->size());  
      connect(off, SIGNAL(clicked(bool)), SLOT(offToggled(bool)));

      grid->addWidget(off, _curGridRow, 0);
      if (record)
            grid->addWidget(record, _curGridRow, 1);
      ++_curGridRow;      
      grid->addWidget(mute, _curGridRow, 0);
      grid->addWidget(solo, _curGridRow++, 1);

      //---------------------------------------------------
      //    routing
      //---------------------------------------------------

      if (type != MusECore::Track::AUDIO_AUX) {
            iR = new QToolButton();
	    iR->setFocusPolicy(Qt::NoFocus);
            ///iR->setFont(MusEGlobal::config.fonts[1]);
            iR->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum));
            ///iR->setText(tr("iR"));
            iR->setIcon(QIcon(*routesInIcon));
            iR->setIconSize(routesInIcon->size());  
            iR->setCheckable(false);
            iR->setToolTip(tr("input routing"));
            grid->addWidget(iR, _curGridRow, 0);
            connect(iR, SIGNAL(pressed()), SLOT(iRoutePressed()));
            }
      
      oR = new QToolButton();
      oR->setFocusPolicy(Qt::NoFocus);
      ///oR->setFont(MusEGlobal::config.fonts[1]);
      oR->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum));
      ///oR->setText(tr("oR"));
      oR->setIcon(QIcon(*routesOutIcon));
      oR->setIconSize(routesOutIcon->size());  
      oR->setCheckable(false);
      oR->setToolTip(tr("output routing"));
      grid->addWidget(oR, _curGridRow++, 1);
      connect(oR, SIGNAL(pressed()), SLOT(oRoutePressed()));

      //---------------------------------------------------
      //    automation type
      //---------------------------------------------------

      autoType = new MusEGui::ComboBox();
      autoType->setFocusPolicy(Qt::NoFocus);
      ///autoType->setFont(MusEGlobal::config.fonts[1]);
      autoType->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
      //autoType->setAutoFillBackground(true);
      
      autoType->addAction(tr("Off"), AUTO_OFF);
      autoType->addAction(tr("Read"), AUTO_READ);
      autoType->addAction(tr("Touch"), AUTO_TOUCH);
      autoType->addAction(tr("Write"), AUTO_WRITE);
      autoType->setCurrentItem(t->automationType());

      QPalette palette;
      //QLinearGradient gradient(autoType->geometry().topLeft(), autoType->geometry().bottomLeft());
      if(t->automationType() == AUTO_TOUCH || t->automationType() == AUTO_WRITE)
            {
            palette.setColor(QPalette::Button, QColor(215, 76, 39));  // red
            /* QLinearGradient gradient(autoType->geometry().topLeft(), autoType->geometry().bottomLeft());
            QColor c(Qt::red);
            //QColor c(215, 76, 39);       // red
            gradient.setColorAt(0, c.darker());
            gradient.setColorAt(0.5, c);
            gradient.setColorAt(1, c.darker());
            palette.setBrush(QPalette::Button, gradient);
            //palette.setBrush(autoType->backgroundRole(), gradient);
            //palette.setBrush(QPalette::Window, gradient);  */
            autoType->setPalette(palette);
            }
      else if(t->automationType() == AUTO_READ)
            {
            palette.setColor(QPalette::Button, QColor(100, 172, 49));  // green
            /*QLinearGradient gradient(autoType->geometry().topLeft(), autoType->geometry().bottomLeft());
            QColor c(Qt::green);
            //QColor c(100, 172, 49);     // green
            gradient.setColorAt(0, c.darker());
            gradient.setColorAt(0.5, c);
            gradient.setColorAt(1, c.darker());
            palette.setBrush(QPalette::Button, gradient);
            //palette.setBrush(autoType->backgroundRole(), gradient);
            //palette.setBrush(QPalette::Window, gradient);  */
            autoType->setPalette(palette);
            }
      else  
            {
            palette.setColor(QPalette::Button, qApp->palette().color(QPalette::Active, QPalette::Background));
            //QColor c(qApp->palette().color(QPalette::Active, QPalette::Background));
            //gradient.setColorAt(0, c);
            //gradient.setColorAt(1, c.darker());
            //palette.setBrush(QPalette::Button, gradient);
            autoType->setPalette(palette);
            }

      autoType->setToolTip(tr("automation type"));
      connect(autoType, SIGNAL(activated(int)), SLOT(setAutomationType(int)));
      grid->addWidget(autoType, _curGridRow++, 0, 1, 2);

      if (off) {
            off->blockSignals(true);
            updateOffState();   // init state
            off->blockSignals(false);
            }
      connect(MusEGlobal::heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));

      updateRouteButtons();

      }

//---------------------------------------------------------
//   iRoutePressed
//---------------------------------------------------------

void AudioStrip::iRoutePressed()
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

void AudioStrip::oRoutePressed()
{
      //MusEGui::RoutePopupMenu* pup = MusEGlobal::muse->getRoutingPopupMenu();
      RoutePopupMenu* pup = new RoutePopupMenu();
      pup->exec(QCursor::pos(), track, true);
      delete pup;
      oR->setDown(false);     
}

} // namespace MusEGui
