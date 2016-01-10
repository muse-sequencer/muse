//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: astrip.cpp,v 1.23.2.17 2009/11/16 01:55:55 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011-2013 Tim E. Real (terminator356 on sourceforge)
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
#include <stdio.h>
#include <stdlib.h>

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
// #include "knob.h" // REMOVE Tim. Trackinfo. Changed.
#include "compact_slider.h"
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
#include "ctrl.h"
#include "utils.h"

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
// REMOVE Tim. Trackinfo. Removed.
//    double clipperVal = 0.0f;
   const int tch = track->channels();
   for (int ch = 0; ch < tch; ++ch) {
      if (meter[ch]) {
         //int meterVal = track->meter(ch);
         //int peak  = track->peak(ch);
         //meter[ch]->setVal(meterVal, peak, false);
         meter[ch]->setVal(track->meter(ch), track->peak(ch), false);
      }
// REMOVE Tim. Trackinfo. Changed.      
//       clipperVal += track->peak(ch);
      if(_clipperLabel[ch])
      {
        _clipperLabel[ch]->setVal(track->peak(ch));
        _clipperLabel[ch]->setClipped(track->isClipped(ch));
      }
   }
// REMOVE Tim. Trackinfo. Removed.
//    clipperVal /= track->channels();
//    _clipperLabel->setVal(clipperVal);
   updateVolume();
   updatePan();

// REMOVE Tim. Trackinfo. Removed.
//    _clipperLabel->setClipper(track->isClipped());

}

// REMOVE Tim. Trackinfo. Changed.      
// //---------------------------------------------------------
// //   configChanged
// //   Catch when label font, or configuration min slider and meter values change, or viewable tracks etc.
// //---------------------------------------------------------
// 
// void AudioStrip::configChanged()    
// { 
//   // Set the whole strip's font, except for the label.
//   if(font() != MusEGlobal::config.fonts[1])
//     setFont(MusEGlobal::config.fonts[1]);
//   
//   // Set the strip label's font.
//   setLabelFont();
//   setLabelText();        
//   
//   // Adjust minimum volume slider and label values.
//   slider->setRange(MusEGlobal::config.minSlider-0.1, 10.0);
//   sl->setRange(MusEGlobal::config.minSlider, 10.0);
//   
//   // Adjust minimum aux knob and label values.
//   int n = auxKnob.size();
//   for (int idx = 0; idx < n; ++idx) 
//   {
//     auxKnob[idx]->blockSignals(true);
//     auxLabel[idx]->blockSignals(true);
//     auxKnob[idx]->setRange(MusEGlobal::config.minSlider-0.1, 10.0);
//     auxLabel[idx]->setRange(MusEGlobal::config.minSlider, 10.1);
//     auxKnob[idx]->blockSignals(false);
//     auxLabel[idx]->blockSignals(false);
//   }
//   
//   // Adjust minimum meter values.
//   for(int c = 0; c < channel; ++c) 
//     meter[c]->setRange(MusEGlobal::config.minMeter, 10.0);
// }

//---------------------------------------------------------
//   configChanged
//   Catch when label font, or configuration min slider and meter values change, or viewable tracks etc.
//---------------------------------------------------------

void AudioStrip::configChanged()    
{ 
  // Set the whole strip's font, except for the label.
  if(font() != MusEGlobal::config.fonts[1])
  {
// REMOVE Tim. Trackinfo. Changed.  
//     setFont(MusEGlobal::config.fonts[1]);
    //fprintf(stderr, "AudioStrip::configChanged changing font: current size:%d\n", font().pointSize()); // REMOVE Tim. Trackinfo.
    setStyleSheet(MusECore::font2StyleSheet(MusEGlobal::config.fonts[1]));
  }
  
  // Set the strip label's font.
  setLabelFont();
  setLabelText();        
  
  // Adjust minimum volume slider and label values.
  slider->setRange(MusEGlobal::config.minSlider-0.1, 10.0);
  sl->setRange(MusEGlobal::config.minSlider, 10.0);
  
  // Adjust minimum aux slider values.
  int n = auxControl.size();
  for (int idx = 0; idx < n; ++idx) 
  {
    auxControl[idx]->blockSignals(true);
    auxControl[idx]->setRange(MusEGlobal::config.minSlider-0.1, 10.0);
    auxControl[idx]->blockSignals(false);
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

// REMOVE Tim. Trackinfo. Changed.      
// //---------------------------------------------------------
// //   songChanged
// //---------------------------------------------------------
// 
// void AudioStrip::songChanged(MusECore::SongChangedFlags_t val)
//       {
//       // Is it simply a midi controller value adjustment? Forget it.
//       if (val == SC_MIDI_CONTROLLER)
//         return;
// 
//       updateRouteButtons();
//     
//       MusECore::AudioTrack* src = (MusECore::AudioTrack*)track;
//       gain->setValue(src->gain());
// 
//       // Do channels before MusEGlobal::config...
//       if (val & SC_CHANNELS)
//         updateChannels();
//       
//       // Catch when label font, or configuration min slider and meter values change.
//       if (val & SC_CONFIG)
//       {
//         // So far only 1 instance of sending SC_CONFIG in the entire app, in instrument editor when a new instrument is saved.
//       }
//       
//       if (mute && (val & SC_MUTE)) {      // mute && off
//             mute->blockSignals(true);
//             mute->setChecked(src->mute());
//             mute->blockSignals(false);
//             mute->setIcon(src->mute() ? QIcon(*muteIconOff) : QIcon(*muteIconOn));
//             //mute->setIconSize(muteIconOn->size());  
//             updateOffState();
//             }
//       if (solo && (val & (SC_SOLO | SC_ROUTE))) {
//             solo->blockSignals(true);
//             solo->setChecked(track->solo());
//             solo->blockSignals(false);
//             if(track->internalSolo())
//               solo->setIcon(track->solo() ? QIcon(*soloblksqIconOn) : QIcon(*soloblksqIconOff));
//             else
//               solo->setIcon(track->solo() ? QIcon(*soloIconOn) : QIcon(*soloIconOff));
//             //solo->setIconSize(soloIconOn->size());  
//             }
//       if (val & SC_RECFLAG)
//             setRecordFlag(track->recordFlag());
//       if (val & SC_TRACK_MODIFIED)
//       {
//             setLabelText();
//             setLabelFont();
//             
//       }      
//       //if (val & SC_CHANNELS)
//       //      updateChannels();
//       if (val & SC_ROUTE) {
//             if (pre) {
//                   pre->blockSignals(true);
//                   pre->setChecked(src->prefader());
//                   pre->blockSignals(false);
//                   }
//             
//             // Are there any Aux Track routing paths to this track? Then we cannot process aux for this track! 
//             // Hate to do this, but as a quick visual reminder, seems most logical to disable Aux knobs and labels. 
//             int rc = track->auxRefCount();
//             int n = auxKnob.size();
//             for (int idx = 0; idx < n; ++idx) 
//             {  
//               auxKnob[idx]->setEnabled( rc == 0 );
//               auxLabel[idx]->setEnabled( rc == 0 );
//             }  
//           }
//       if (val & SC_AUX) {
//             int n = auxKnob.size();
//             for (int idx = 0; idx < n; ++idx) {
//                   double val = MusECore::fast_log10(src->auxSend(idx)) * 20.0;
//                   auxKnob[idx]->blockSignals(true);
//                   auxLabel[idx]->blockSignals(true);
//                   auxKnob[idx]->setValue(val);
//                   auxLabel[idx]->setValue(val);
//                   auxKnob[idx]->blockSignals(false);
//                   auxLabel[idx]->blockSignals(false);
//                   }
//             }
//       if (autoType && (val & SC_AUTOMATION)) {
//             autoType->blockSignals(true);
//             autoType->setCurrentItem(track->automationType());
//             QPalette palette;
//             //QLinearGradient gradient(autoType->geometry().topLeft(), autoType->geometry().bottomLeft());
//             if(track->automationType() == AUTO_TOUCH || track->automationType() == AUTO_WRITE)
//                   {
//                   palette.setColor(QPalette::Button, QColor(215, 76, 39)); // red
//                   //palette.setColor(QPalette::Window, QColor(215, 76, 39)); // red
//                   /*QLinearGradient gradient(autoType->geometry().topLeft(), autoType->geometry().bottomLeft());
//                   QColor c(Qt::red);
//                   //QColor c(215, 76, 39);       // red
//                   gradient.setColorAt(0, c.darker());
//                   gradient.setColorAt(0.5, c);
//                   gradient.setColorAt(1, c.darker());
//                   palette.setBrush(QPalette::Button, gradient);
//                   //palette.setBrush(autoType->backgroundRole(), gradient);
//                   //palette.setBrush(QPalette::Window, gradient);   */
//                   autoType->setPalette(palette);
//                   }
//             else if(track->automationType() == AUTO_READ)
//                   {
//                   palette.setColor(QPalette::Button, QColor(100, 172, 49)); // green
//                   //palette.setColor(QPalette::Window, QColor(100, 172, 49)); // green
//                   /*QLinearGradient gradient(autoType->geometry().topLeft(), autoType->geometry().bottomLeft());
//                   QColor c(Qt::green);
//                   //QColor c(100, 172, 49);     // green
//                   gradient.setColorAt(0, c.darker());
//                   gradient.setColorAt(0.5, c);
//                   gradient.setColorAt(1, c.darker());
//                   palette.setBrush(QPalette::Button, gradient);
//                   //palette.setBrush(autoType->backgroundRole(), gradient);
//                   //palette.setBrush(QPalette::Window, gradient);  */
//                   autoType->setPalette(palette);
//                   }
//             else  
//                   {
//                   palette.setColor(QPalette::Button, qApp->palette().color(QPalette::Active, QPalette::Background));
//                   //QColor c(qApp->palette().color(QPalette::Active, QPalette::Background));
//                   //gradient.setColorAt(0, c);
//                   //gradient.setColorAt(1, c.darker());
//                   //palette.setBrush(QPalette::Button, gradient);
//                   autoType->setPalette(palette);
//                   }
//       
//             autoType->blockSignals(false);
//             }
//       }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void AudioStrip::songChanged(MusECore::SongChangedFlags_t val)
      {
      // Is it simply a midi controller value adjustment? Forget it.
      if (val == SC_MIDI_CONTROLLER)
        return;

      updateRouteButtons();
    
      MusECore::AudioTrack* src = (MusECore::AudioTrack*)track;
      gain->setValue(src->gain());

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
      if (solo && (val & (SC_SOLO | SC_ROUTE))) {
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
            int n = auxControl.size();
            for (int idx = 0; idx < n; ++idx) 
              auxControl[idx]->setEnabled( rc == 0 );
          }
      if (val & SC_AUX) {
            int n = auxControl.size();
            for (int idx = 0; idx < n; ++idx) {
                  double val = MusECore::fast_log10(src->auxSend(idx)) * 20.0;
                  auxControl[idx]->blockSignals(true);
                  auxControl[idx]->setValue(val);
                  auxControl[idx]->blockSignals(false);
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
      if(_volPressed) // Inhibit the controller stream if control is currently pressed.
        return;
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

// REMOVE Tim. Trackinfo. Changed.      
// //---------------------------------------------------------
// //   updatePan
// //---------------------------------------------------------
// 
// void AudioStrip::updatePan()
// {
//       if(_panPressed) // Inhibit the controller stream if control is currently pressed.
//         return;
//       double v = ((MusECore::AudioTrack*)track)->pan();
//       if (v != panVal)
//       {
//           //printf("AudioStrip::updatePan setting slider and label\n");
//           pan->blockSignals(true);
//           panl->blockSignals(true);
//           pan->setValue(v);
//           panl->setValue(v);
//           panl->blockSignals(false);
//           pan->blockSignals(false);
//           panVal = v;
//           }
// }

//---------------------------------------------------------
//   updatePan
//---------------------------------------------------------

void AudioStrip::updatePan()
{
      if(_panPressed) // Inhibit the controller stream if control is currently pressed.
        return;
      double v = ((MusECore::AudioTrack*)track)->pan();
      if (v != panVal)
      {
          //printf("AudioStrip::updatePan setting slider and label\n");
          pan->blockSignals(true);
          pan->setValue(v);
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

// REMOVE Tim. Trackinfo. Changed.      
// //---------------------------------------------------------
// //   updateOffState
// //---------------------------------------------------------
// 
// void AudioStrip::updateOffState()
//       {
//       bool val = !track->off();
//       slider->setEnabled(val);
//       sl->setEnabled(val);
//       pan->setEnabled(val);
//       panl->setEnabled(val);
//       if (track->type() != MusECore::Track::AUDIO_SOFTSYNTH)
//             stereo->setEnabled(val);
//       label->setEnabled(val);
//       
//       // Are there any Aux Track routing paths to this track? Then we cannot process aux for this track! 
//       // Hate to do this, but as a quick visual reminder, seems most logical to disable Aux knobs and labels. 
//       bool ae = track->auxRefCount() == 0 && val;
//       int n = auxKnob.size();
//       for (int i = 0; i < n; ++i) 
//       {
//         auxKnob[i]->setEnabled(ae);
//         auxLabel[i]->setEnabled(ae);
//       }
//             
//       if (pre)
//             pre->setEnabled(val);
//       if (record)
//             record->setEnabled(val);
//       if (solo)
//             solo->setEnabled(val);
//       if (mute)
//             mute->setEnabled(val);
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

void AudioStrip::updateOffState()
      {
      bool val = !track->off();
      slider->setEnabled(val);
      sl->setEnabled(val);
      pan->setEnabled(val);
      if (track->type() != MusECore::Track::AUDIO_SOFTSYNTH)
            stereo->setEnabled(val);
      label->setEnabled(val);
      
      // Are there any Aux Track routing paths to this track? Then we cannot process aux for this track! 
      // Hate to do this, but as a quick visual reminder, seems most logical to disable Aux knobs and labels. 
      bool ae = track->auxRefCount() == 0 && val;
      int n = auxControl.size();
      for (int i = 0; i < n; ++i) 
        auxControl[i]->setEnabled(ae);
            
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
//   gainChanged
//---------------------------------------------------------

void AudioStrip::gainChanged(double val)
      {
      ((MusECore::AudioTrack*)track)->setGain(val);
      }

// REMOVE Tim. Trackinfo. Removed.
// REMOVE Tim. Trackinfo. Changed.
// //---------------------------------------------------------
// //   auxLabelChanged
// //---------------------------------------------------------
// 
// void AudioStrip::auxLabelChanged(double val, unsigned int idx) 
//       {
//         if(idx >= auxKnob.size())
//           return;    
//         auxKnob[idx]->setValue(val);
//       }
// //---------------------------------------------------------
// //   auxLabelChanged
// //---------------------------------------------------------
// 
// void AudioStrip::auxLabelChanged(double val, unsigned int idx) 
//       {
//         if(idx >= auxControl.size())
//           return;    
//         auxControl[idx]->setValue(val);
//       }

//---------------------------------------------------------
//   volumeChanged
//---------------------------------------------------------

void AudioStrip::volumeChanged(double val, int, bool shift_pressed)
      {
      if(track->isMidiTrack())
        return;
      MusECore::AudioTrack* t = static_cast<MusECore::AudioTrack*>(track);
      double vol;
      if (val <= MusEGlobal::config.minSlider) {
            vol = 0.0;
            val -= 1.0; // display special value "off"
            }
      else
            vol = pow(10.0, val/20.0);
      volume = vol;
      if (!shift_pressed) t->recordAutomation(MusECore::AC_VOLUME, vol);  // with shift, we get straight lines :)
      t->setParam(MusECore::AC_VOLUME, vol);                              // Schedules a timed control change.
      t->enableController(MusECore::AC_VOLUME, false);
      }

//---------------------------------------------------------
//   volumePressed
//---------------------------------------------------------

void AudioStrip::volumePressed()
      {
      if(track->isMidiTrack())
        return;
      _volPressed = true;
      MusECore::AudioTrack* t = static_cast<MusECore::AudioTrack*>(track);
      double val = slider->value();
      double vol;
      if (val <= MusEGlobal::config.minSlider) {
            vol = 0.0;
            //val -= 1.0; // display special value "off"
            }
      else
            vol = pow(10.0, val/20.0);
      volume = vol;
      t->startAutoRecord(MusECore::AC_VOLUME, vol);
      t->setVolume(vol);
      t->enableController(MusECore::AC_VOLUME, false);
      }

//---------------------------------------------------------
//   volumeReleased
//---------------------------------------------------------

void AudioStrip::volumeReleased()
      {
      if(track->isMidiTrack())
        return;
      MusECore::AudioTrack* t = static_cast<MusECore::AudioTrack*>(track);
      AutomationType at = t->automationType();
      t->stopAutoRecord(MusECore::AC_VOLUME, volume);
      if(at == AUTO_OFF ||
        at == AUTO_TOUCH)
        t->enableController(MusECore::AC_VOLUME, true);
      _volPressed = false;
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
      if(track->isMidiTrack())
        return;
      MusECore::AudioTrack* t = static_cast<MusECore::AudioTrack*>(track);
      double vol;
      if (val <= MusEGlobal::config.minSlider) {
            vol = 0.0;
            val -= 1.0; // display special value "off"
            }
      else
            vol = pow(10.0, val/20.0);
      volume = vol;
      slider->blockSignals(true);
      slider->setValue(val);                   
      slider->blockSignals(false);
      t->startAutoRecord(MusECore::AC_VOLUME, vol);
      t->setParam(MusECore::AC_VOLUME, vol);  // Schedules a timed control change.
      t->enableController(MusECore::AC_VOLUME, false);
      }

//---------------------------------------------------------
//   panChanged
//---------------------------------------------------------

void AudioStrip::panChanged(double val, int, bool shift_pressed)
      {
      if(track->isMidiTrack())
        return;
      MusECore::AudioTrack* t = static_cast<MusECore::AudioTrack*>(track);
      panVal = val;
      if (!shift_pressed) t->recordAutomation(MusECore::AC_PAN, val);  // with shift, we get straight lines :)
      t->setParam(MusECore::AC_PAN, val);                              // Schedules a timed control change.
      t->enableController(MusECore::AC_PAN, false);
      }

//---------------------------------------------------------
//   panPressed
//---------------------------------------------------------

void AudioStrip::panPressed()
      {
      if(track->isMidiTrack())
        return;
      _panPressed = true;
      MusECore::AudioTrack* t = static_cast<MusECore::AudioTrack*>(track);
      panVal = pan->value();
      t->startAutoRecord(MusECore::AC_PAN, panVal);
      t->setPan(panVal);
      t->enableController(MusECore::AC_PAN, false);
      }

//---------------------------------------------------------
//   panReleased
//---------------------------------------------------------

void AudioStrip::panReleased()
      {
      if(track->isMidiTrack())
        return;
      MusECore::AudioTrack* t = static_cast<MusECore::AudioTrack*>(track);
      AutomationType at = t->automationType();
      t->stopAutoRecord(MusECore::AC_PAN, panVal);
      if(at == AUTO_OFF ||
         at == AUTO_TOUCH)
        t->enableController(MusECore::AC_PAN, true);
      _panPressed = false;
      }

//---------------------------------------------------------
//   panRightClicked
//---------------------------------------------------------
void AudioStrip::panRightClicked(const QPoint &p)
{
   MusEGlobal::song->execAutomationCtlPopup((MusECore::AudioTrack*)track, p, MusECore::AC_PAN);
}

void AudioStrip::resetClipper()
{
   if(track)
   {
      track->resetClipper();
      resetPeaks();
   }
}

// REMOVE Tim. Trackinfo. Removed.
// //---------------------------------------------------------
// //   panLabelChanged
// //---------------------------------------------------------
// 
// void AudioStrip::panLabelChanged(double val)
//       {
//       if(track->isMidiTrack())
//         return;
//       MusECore::AudioTrack* t = static_cast<MusECore::AudioTrack*>(track);
//       panVal = val;
//       pan->blockSignals(true);
//       pan->setValue(val);
//       pan->blockSignals(false);
//       t->startAutoRecord(MusECore::AC_PAN, val);
//       t->setParam(MusECore::AC_PAN, val);     // Schedules a timed control change.
//       t->enableController(MusECore::AC_PAN, false);
//       }

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
                  // REMOVE Tim. Trackinfo. Added.
                  _clipperLabel[cc] = new ClipperLabel();
                  setClipperTooltip(cc);
                  _clipperLayout->addWidget(_clipperLabel[cc]);
                  connect(_clipperLabel[cc], SIGNAL(clicked()), SLOT(resetClipper()));
            
                  meter[cc] = new MusEGui::Meter(this);
                  //meter[cc]->setRange(MusEGlobal::config.minSlider, 10.0);
                  meter[cc]->setRange(MusEGlobal::config.minMeter, 10.0);
                  meter[cc]->setFixedWidth(FIXED_METER_WIDTH);
//                   connect(meter[cc], SIGNAL(mousePress()), this, SLOT(resetPeaks())); // REMOVE Tim. Trackinfo. Changed.
                  connect(meter[cc], SIGNAL(mousePress()), this, SLOT(resetClipper()));
                  sliderGrid->addWidget(meter[cc], 2, cc+1, Qt::AlignLeft);
                  sliderGrid->setColumnStretch(cc, 50);
                  meter[cc]->show();
                  }
            }
      else if (c < channel) {
            for (int cc = channel-1; cc >= c; --cc) {
                  // REMOVE Tim. Trackinfo. Added.
                  if(_clipperLabel[cc])
                    delete _clipperLabel[cc];
                  _clipperLabel[cc] = 0;
                  
                  if(meter[cc])
                    delete meter[cc];
                  meter[cc] = 0;
                  }
            }
      channel = c;
// REMOVE Tim. Trackinfo. Removed.
//       sliderGrid->removeWidget(_clipperLabel);
//       sliderGrid->addWidget(_clipperLabel, 0, 0, 1, -1);
      stereo->blockSignals(true);
      stereo->setChecked(channel == 2);
      stereo->blockSignals(false);
      stereo->setIcon(channel == 2 ? QIcon(*stereoIcon) : QIcon(*monoIcon));
      //stereo->setIconSize(stereoIcon->size());  
      }

// REMOVE Tim. Trackinfo. Changed.
// //---------------------------------------------------------
// //   addKnob
// //    type = 0 - panorama
// //           1 - aux send
// //---------------------------------------------------------
// 
// MusEGui::Knob* AudioStrip::addKnob(Knob::KnobType type, int id, MusEGui::DoubleLabel** dlabel, QLabel *name)
//       {
//       MusEGui::Knob* knob = NULL;
//       MusEGui::DoubleLabel* knobLabel = NULL;
//       switch(type)
//       {
//         case Knob::panType:
//           knob = new Knob(this);
//           knob->setRange(-1.0, +1.0);
//           knob->setToolTip(tr("panorama"));
//           knobLabel = new MusEGui::DoubleLabel(0, -1.0, +1.0, this);
//           knobLabel->setPrecision(2);
//         break;
//         case Knob::auxType:
//           knob = new Knob(this);
//           knob->setRange(MusEGlobal::config.minSlider-0.1, 10.0);
//           knob->setToolTip(tr("aux send level"));
//           knob->setFaceColor(Qt::blue);
//           knobLabel = new MusEGui::DoubleLabel(0.0, MusEGlobal::config.minSlider, 10.1, this);
//           knobLabel->setPrecision(0);
//         break;
//         case Knob::gainType:
//           knob = new Knob(this);
//           knob->setRange(1.0, 20.0);
//           knob->setFaceColor(Qt::yellow);
//           knob->setToolTip(tr("calibration gain"));
//           knobLabel = new MusEGui::DoubleLabel(1.0, 1.0, 30.0, this);
//           knobLabel->setPrecision(1);
//         break;
//         default:
//           fprintf(stderr, "FIXME: AudioStrip::addKnob(): Unknown type. Aborting!\n");
//           abort();
//       }
//         
//       knob->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
//       knob->setBackgroundRole(QPalette::Mid);
//             
//       if (dlabel)
//             *dlabel = knobLabel;
//       knobLabel->setSlider(knob);
//       knobLabel->setBackgroundRole(QPalette::Mid);
//       knobLabel->setFrame(true);
//       knobLabel->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
//       
//       name->setParent(this);
//       name->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
//       name->setAlignment(Qt::AlignCenter);
// 
//       grid->addWidget(name, _curGridRow, 0);
//       grid->addWidget(knobLabel, _curGridRow+1, 0);
//       grid->addWidget(knob, _curGridRow, 1, 2, 1);
//       _curGridRow += 2;
// 
//       connect(knob, SIGNAL(valueChanged(double,int)), knobLabel, SLOT(setValue(double)));
// 
//       if (type == Knob::panType) {
//             connect(knobLabel, SIGNAL(valueChanged(double, int)), SLOT(panLabelChanged(double)));
//             connect(knob, SIGNAL(sliderMoved(double,int,bool)), SLOT(panChanged(double,int,bool)));
//             connect(knob, SIGNAL(sliderPressed(int)), SLOT(panPressed()));
//             connect(knob, SIGNAL(sliderReleased(int)), SLOT(panReleased()));
//             connect(knob, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(panRightClicked(const QPoint &)));
//             }
//       else if (type == Knob::auxType){
//             knobLabel->setReadOnly(true);
//             knob->setId(id);
//             connect(knobLabel, SIGNAL(valueChanged(double, int)), knob,  SLOT(setValue(double)));
//             connect(knob, SIGNAL(sliderMoved(double, int)), SLOT(auxChanged(double, int)));
//             }
//       else if (type == Knob::gainType){
//             knobLabel->setReadOnly(true);
//             knob->setId(id);
//             connect(knobLabel, SIGNAL(valueChanged(double, int)), knob,  SLOT(setValue(double)));
//             connect(knob, SIGNAL(sliderMoved(double, int)), SLOT(gainChanged(double)));
//             }
//       return knob;
//       }

//---------------------------------------------------------
//   addKnob
//    type = 0 - panorama
//           1 - aux send
//---------------------------------------------------------

CompactSlider* AudioStrip::addController(ControllerType type, int id, const QString& label)
      {
      CompactSlider* control = NULL;
      QPalette pal(palette());
      switch(type)
      {


// REMOVE Tim. Trackinfo.
//         CompactSlider(QWidget *parent = 0, const char *name = 0,
//           Qt::Orientation orient = Qt::Horizontal,
//           ScalePos scalePos = None,
//           const QString& labelText = QString(), 
//           const QString& valPrefix = QString(), 
//           const QString& valSuffix = QString(),
//           const QString& specialValueText = QString(), 
//           QColor thumbColor = QColor(255, 255, 0));

        case panType:
          control = new CompactSlider(this, "MixerStripAudioPan", Qt::Horizontal, CompactSlider::None, label);
          control->setToolTip(tr("panorama"));
          control->setRange(-1.0, +1.0);
          control->setValueDecimals(2);
          //control->setValue(0);
          pal.setColor(QPalette::Active, QPalette::Button, Qt::darkYellow); // Border
          pal.setColor(QPalette::Inactive, QPalette::Button, Qt::darkYellow); // Border
          control->setPalette(pal);
        break;
        case auxType:
          control = new CompactSlider(this, "MixerStripAudioAux", Qt::Horizontal, CompactSlider::None, label);
          control->setToolTip(tr("aux send level"));
          control->setRange(MusEGlobal::config.minSlider-0.1, 10.0);
          control->setValueDecimals(0);
          //control->setValue(0);
          pal.setColor(QPalette::Active, QPalette::Button, Qt::blue); // Border
          pal.setColor(QPalette::Inactive, QPalette::Button, Qt::blue); // Border
          control->setPalette(pal);
        break;
        case gainType:
          control = new CompactSlider(this, "MixerStripAudioGain", Qt::Horizontal, CompactSlider::None, label);
          control->setToolTip(tr("calibration gain"));
          control->setRange(1.0, 20.0);
          control->setValueDecimals(1);
          pal.setColor(QPalette::Active, QPalette::Button, Qt::yellow); // Border
          pal.setColor(QPalette::Inactive, QPalette::Button, Qt::yellow); // Border
          control->setPalette(pal);
        break;
        default:
          fprintf(stderr, "FIXME: AudioStrip::addKnob(): Unknown type. Aborting!\n");
          abort();
      }
        
      control->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
//       control->setBackgroundRole(QPalette::Mid);
            
      grid->addWidget(control, _curGridRow++, 0, 1, 2);

//       connect(knob, SIGNAL(valueChanged(double,int)), knobLabel, SLOT(setValue(double)));

//       if (type == Knob::panType) {
//             connect(knobLabel, SIGNAL(valueChanged(double, int)), SLOT(panLabelChanged(double)));
//             connect(knob, SIGNAL(sliderMoved(double,int,bool)), SLOT(panChanged(double,int,bool)));
//             connect(knob, SIGNAL(sliderPressed(int)), SLOT(panPressed()));
//             connect(knob, SIGNAL(sliderReleased(int)), SLOT(panReleased()));
//             connect(knob, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(panRightClicked(const QPoint &)));
//             }
//       else if (type == Knob::auxType){
//             knobLabel->setReadOnly(true);
//             knob->setId(id);
//             connect(knobLabel, SIGNAL(valueChanged(double, int)), knob,  SLOT(setValue(double)));
//             connect(knob, SIGNAL(sliderMoved(double, int)), SLOT(auxChanged(double, int)));
//             }
//       else if (type == Knob::gainType){
//             knobLabel->setReadOnly(true);
//             knob->setId(id);
//             connect(knobLabel, SIGNAL(valueChanged(double, int)), knob,  SLOT(setValue(double)));
//             connect(knob, SIGNAL(sliderMoved(double, int)), SLOT(gainChanged(double)));
//             }
      if (type == panType) {
//             connect(knobLabel, SIGNAL(valueChanged(double, int)), SLOT(panLabelChanged(double)));
            connect(control, SIGNAL(sliderMoved(double,int,bool)), SLOT(panChanged(double,int,bool)));
            connect(control, SIGNAL(sliderPressed(int)), SLOT(panPressed()));
            connect(control, SIGNAL(sliderReleased(int)), SLOT(panReleased()));
            connect(control, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(panRightClicked(const QPoint &)));
            }
      else if (type == auxType){
//             knobLabel->setReadOnly(true);
            control->setId(id);
//             connect(knobLabel, SIGNAL(valueChanged(double, int)), knob,  SLOT(setValue(double)));
            connect(control, SIGNAL(sliderMoved(double, int)), SLOT(auxChanged(double, int)));
            }
      else if (type == gainType){
//             knobLabel->setReadOnly(true);
            control->setId(id);
//             connect(knobLabel, SIGNAL(valueChanged(double, int)), knob,  SLOT(setValue(double)));
            connect(control, SIGNAL(sliderMoved(double, int)), SLOT(gainChanged(double)));
            }
      return control;
      }

//---------------------------------------------------------
//   AudioStrip
//---------------------------------------------------------

AudioStrip::~AudioStrip()
      {
      }


// REMOVE Tim. Trackinfo. Changed.
// //---------------------------------------------------------
// //   AudioStrip
// //    create mixer strip
// //---------------------------------------------------------
// 
// AudioStrip::AudioStrip(QWidget* parent, MusECore::AudioTrack* at)
//    : Strip(parent, at)
//       {
//       volume        = -1.0;
//       panVal        = 0;
//       _volPressed   = false;
//       _panPressed   = false;
//       
//       record        = 0;
//       off           = 0;
//       
//       // Set the whole strip's font, except for the label.    p4.0.45
//       setFont(MusEGlobal::config.fonts[1]);
// 
//       
//       MusECore::AudioTrack* t = (MusECore::AudioTrack*)track;
//       channel       = at->channels();
//       ///setMinimumWidth(STRIP_WIDTH);
//       
//       int ch = 0;
//       for (; ch < channel; ++ch)
//             meter[ch] = new MusEGui::Meter(this);
//       for (; ch < MAX_CHANNELS; ++ch)
//             meter[ch] = 0;
// 
//       //---------------------------------------------------
//       //    plugin rack
//       //---------------------------------------------------
// 
//       rack = new EffectRack(this, t);
//       rack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
//       grid->addWidget(rack, _curGridRow++, 0, 1, 2);
// 
//       //---------------------------------------------------
//       //    mono/stereo  pre/post
//       //---------------------------------------------------
// 
//       stereo  = new QToolButton();
//       ///stereo->setFont(MusEGlobal::config.fonts[1]);
//       stereo->setFocusPolicy(Qt::NoFocus);
//       stereo->setCheckable(true);
//       stereo->setToolTip(tr("1/2 channel"));
//       stereo->setChecked(channel == 2);
//       stereo->setIcon(channel == 2 ? QIcon(*stereoIcon) : QIcon(*monoIcon));
//       stereo->setIconSize(monoIcon->size());  
//       stereo->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
//       connect(stereo, SIGNAL(clicked(bool)), SLOT(stereoToggled(bool)));
// 
//       // disable mono/stereo for Synthesizer-Plugins
//       if (t->type() == MusECore::Track::AUDIO_SOFTSYNTH)
//             stereo->setEnabled(false);
// 
//       pre = new QToolButton();
//       ///pre->setFont(MusEGlobal::config.fonts[1]);
//       pre->setFocusPolicy(Qt::NoFocus);
//       pre->setCheckable(true);
//       pre->setText(tr("Pre"));
//       pre->setToolTip(tr("pre fader - post fader"));
//       pre->setChecked(t->prefader());
//       pre->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
//       connect(pre, SIGNAL(clicked(bool)), SLOT(preToggled(bool)));
// 
//       grid->addWidget(stereo, _curGridRow, 0);
//       grid->addWidget(pre, _curGridRow++, 1);
// 
//       //---------------------------------------------------
//       //    Gain
//       //---------------------------------------------------
// 
//       gain = addKnob(MusEGui::Knob::gainType, 0, &gainLabel, new QLabel("Gain", this));
//       gain->setValue(t->gain());
// 
//       //---------------------------------------------------
//       //    aux send
//       //---------------------------------------------------
// 
//       int auxsSize = MusEGlobal::song->auxs()->size();
//       if (t->hasAuxSend()) {
//             for (int idx = 0; idx < auxsSize; ++idx) {
//                   MusEGui::DoubleLabel* al; // the thought was to aquire the correct Aux name for each Aux
//                                             // now they are only called Aux1, Aux2, which isn't too usable.
//                   QString title = ((MusECore::AudioAux*)(MusEGlobal::song->auxs()->at(idx)))->auxName();
//                   if (title.length() > 8) { // shorten name
//                       title = title.mid(0,8) + ".";
//                   }
//                   QLabel *name = new QLabel(title,this);
//                   MusEGui::Knob* ak = addKnob(MusEGui::Knob::auxType, idx, &al, name);
// 
//                   auxKnob.push_back(ak);
//                   auxLabel.push_back(al);
//                   double val = MusECore::fast_log10(t->auxSend(idx))*20.0;
//                   ak->setValue(val);
//                   al->setValue(val);
//                   
//                   // Are there any Aux Track routing paths to this track? Then we cannot process aux for this track! 
//                   // Hate to do this, but as a quick visual reminder, seems most logical to disable Aux knobs and labels. 
//                   int rc = track->auxRefCount();
//                   ak->setEnabled( rc == 0 );
//                   al->setEnabled( rc == 0 );
//                   }
//             }
//       else {
//             ///if (auxsSize)
//                   //layout->addSpacing((STRIP_WIDTH/2 + 2) * auxsSize);
//                   ///grid->addSpacing((STRIP_WIDTH/2 + 2) * auxsSize);  // ???
//             }
// 
//       //---------------------------------------------------
//       //    slider, label, meter
//       //---------------------------------------------------
// 
//       sliderGrid = new QGridLayout(); 
//       sliderGrid->setRowStretch(0, 100);
//       sliderGrid->setContentsMargins(0, 0, 0, 0);
//       sliderGrid->setSpacing(0);
//       
//       slider = new MusEGui::Slider(this, "vol", Qt::Vertical, MusEGui::Slider::None);
// 
//       slider->setCursorHoming(true);
//       slider->setRange(MusEGlobal::config.minSlider-0.1, 10.0);
//       slider->setFixedWidth(20);
//       ///slider->setFont(MusEGlobal::config.fonts[1]);
//       slider->setValue(MusECore::fast_log10(t->volume())*20.0);
// 
//       sliderGrid->addWidget(slider, 0, 0, Qt::AlignHCenter);
// 
//       for (int i = 0; i < channel; ++i) {
//             //meter[i]->setRange(MusEGlobal::config.minSlider, 10.0);
//             meter[i]->setRange(MusEGlobal::config.minMeter, 10.0);
//             meter[i]->setFixedWidth(15);
//             connect(meter[i], SIGNAL(mousePress()), this, SLOT(resetPeaks()));
//             sliderGrid->addWidget(meter[i], 0, i+1, Qt::AlignHCenter);
//             sliderGrid->setColumnStretch(i, 50);
//             }
//       sliderGrid->addItem(new QSpacerItem(2,0),0,3);
//       grid->addLayout(sliderGrid, _curGridRow++, 0, 1, 2); 
// 
//       sl = new MusEGui::DoubleLabel(0.0, MusEGlobal::config.minSlider, 10.0, this);
//       sl->setSlider(slider);
//       ///sl->setFont(MusEGlobal::config.fonts[1]);
//       sl->setBackgroundRole(QPalette::Mid);
//       sl->setSuffix(tr("dB"));
//       sl->setFrame(true);
//       sl->setPrecision(0);
//       sl->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum));
//       sl->setValue(MusECore::fast_log10(t->volume()) * 20.0);
// 
//       connect(sl, SIGNAL(valueChanged(double,int)), SLOT(volLabelChanged(double)));
//       connect(slider, SIGNAL(valueChanged(double,int)), sl, SLOT(setValue(double)));
//       connect(slider, SIGNAL(sliderMoved(double,int,bool)), SLOT(volumeChanged(double,int,bool)));
//       connect(slider, SIGNAL(sliderPressed(int)), SLOT(volumePressed()));
//       connect(slider, SIGNAL(sliderReleased(int)), SLOT(volumeReleased()));
//       connect(slider, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(volumeRightClicked(const QPoint &)));
//       grid->addWidget(sl, _curGridRow++, 0, 1, 2, Qt::AlignCenter);
// 
//       //---------------------------------------------------
//       //    pan, balance
//       //---------------------------------------------------
// 
//       pan = addKnob(MusEGui::Knob::panType, 0, &panl, new QLabel("Pan", this));
//       pan->setValue(t->pan());
//       
//       //---------------------------------------------------
//       //    mute, solo, record
//       //---------------------------------------------------
// 
//       if (track->canRecord()) {
//             record  = new MusEGui::TransparentToolButton(this);
// 	    record->setFocusPolicy(Qt::NoFocus);
//             record->setCheckable(true);
//             record->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
//             record->setBackgroundRole(QPalette::Mid);
//             record->setToolTip(tr("record"));
//             record->setChecked(t->recordFlag());
//             record->setIcon(t->recordFlag() ? QIcon(*record_on_Icon) : QIcon(*record_off_Icon));
//             ///record->setIconSize(record_on_Icon->size());  
//             connect(record, SIGNAL(clicked(bool)), SLOT(recordToggled(bool)));
//             }
// 
//       MusECore::Track::TrackType type = t->type();
// 
//       mute  = new QToolButton();
//       mute->setFocusPolicy(Qt::NoFocus);
//       mute->setCheckable(true);
//       mute->setToolTip(tr("mute"));
//       mute->setChecked(t->mute());
//       mute->setIcon(t->mute() ? QIcon(*muteIconOff) : QIcon(*muteIconOn));
//       ///mute->setIconSize(muteIconOn->size());  
//       mute->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
//       connect(mute, SIGNAL(clicked(bool)), SLOT(muteToggled(bool)));
// 
//       solo  = new QToolButton();
//       solo->setFocusPolicy(Qt::NoFocus);
//       solo->setCheckable(true);
//       solo->setChecked(t->solo());
//       if(t->internalSolo())
//         solo->setIcon(t->solo() ? QIcon(*soloblksqIconOn) : QIcon(*soloblksqIconOff));
//       else
//         solo->setIcon(t->solo() ? QIcon(*soloIconOn) : QIcon(*soloIconOff));
//       ///solo->setIconSize(soloIconOn->size());  
//       solo->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
//       connect(solo, SIGNAL(clicked(bool)), SLOT(soloToggled(bool)));
//       if (type == MusECore::Track::AUDIO_OUTPUT) {
//             record->setToolTip(tr("record downmix"));
//             //solo->setToolTip(tr("solo mode (monitor)"));
//             solo->setToolTip(tr("solo mode"));
//             }
//       else {
//             //solo->setToolTip(tr("pre fader listening"));
//             solo->setToolTip(tr("solo mode"));
//             }
// 
//       off  = new MusEGui::TransparentToolButton(this);
//       off->setFocusPolicy(Qt::NoFocus);
//       off->setBackgroundRole(QPalette::Mid);
//       off->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
//       off->setCheckable(true);
//       off->setToolTip(tr("off"));
//       off->setChecked(t->off());
//       off->setIcon(t->off() ? QIcon(*exit1Icon) : QIcon(*exitIcon));
//       ///off->setIconSize(exit1Icon->size());  
//       connect(off, SIGNAL(clicked(bool)), SLOT(offToggled(bool)));
// 
//       grid->addWidget(off, _curGridRow, 0);
//       if (record)
//             grid->addWidget(record, _curGridRow, 1);
//       ++_curGridRow;      
//       grid->addWidget(mute, _curGridRow, 0);
//       grid->addWidget(solo, _curGridRow++, 1);
// 
//       //---------------------------------------------------
//       //    routing
//       //---------------------------------------------------
// 
//       if (type != MusECore::Track::AUDIO_AUX) {
//             iR = new QToolButton();
// 	    iR->setFocusPolicy(Qt::NoFocus);
//             ///iR->setFont(MusEGlobal::config.fonts[1]);
//             iR->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum));
//             ///iR->setText(tr("iR"));
//             iR->setIcon(QIcon(*routesInIcon));
//             iR->setIconSize(routesInIcon->size());  
//             iR->setCheckable(false);
//             iR->setToolTip(tr("input routing"));
//             grid->addWidget(iR, _curGridRow, 0);
//             connect(iR, SIGNAL(pressed()), SLOT(iRoutePressed()));
//             }
//       
//       oR = new QToolButton();
//       oR->setFocusPolicy(Qt::NoFocus);
//       ///oR->setFont(MusEGlobal::config.fonts[1]);
//       oR->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum));
//       ///oR->setText(tr("oR"));
//       oR->setIcon(QIcon(*routesOutIcon));
//       oR->setIconSize(routesOutIcon->size());  
//       oR->setCheckable(false);
//       oR->setToolTip(tr("output routing"));
//       grid->addWidget(oR, _curGridRow++, 1);
//       connect(oR, SIGNAL(pressed()), SLOT(oRoutePressed()));
// 
//       //---------------------------------------------------
//       //    automation type
//       //---------------------------------------------------
// 
//       autoType = new MusEGui::ComboBox();
//       autoType->setFocusPolicy(Qt::NoFocus);
//       ///autoType->setFont(MusEGlobal::config.fonts[1]);
//       autoType->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
//       //autoType->setAutoFillBackground(true);
//       
//       autoType->addAction(tr("Off"), AUTO_OFF);
//       autoType->addAction(tr("Read"), AUTO_READ);
//       autoType->addAction(tr("Touch"), AUTO_TOUCH);
//       autoType->addAction(tr("Write"), AUTO_WRITE);
//       autoType->setCurrentItem(t->automationType());
// 
//       QPalette palette;
//       //QLinearGradient gradient(autoType->geometry().topLeft(), autoType->geometry().bottomLeft());
//       if(t->automationType() == AUTO_TOUCH || t->automationType() == AUTO_WRITE)
//             {
//             palette.setColor(QPalette::Button, QColor(215, 76, 39));  // red
//             /* QLinearGradient gradient(autoType->geometry().topLeft(), autoType->geometry().bottomLeft());
//             QColor c(Qt::red);
//             //QColor c(215, 76, 39);       // red
//             gradient.setColorAt(0, c.darker());
//             gradient.setColorAt(0.5, c);
//             gradient.setColorAt(1, c.darker());
//             palette.setBrush(QPalette::Button, gradient);
//             //palette.setBrush(autoType->backgroundRole(), gradient);
//             //palette.setBrush(QPalette::Window, gradient);  */
//             autoType->setPalette(palette);
//             }
//       else if(t->automationType() == AUTO_READ)
//             {
//             palette.setColor(QPalette::Button, QColor(100, 172, 49));  // green
//             /*QLinearGradient gradient(autoType->geometry().topLeft(), autoType->geometry().bottomLeft());
//             QColor c(Qt::green);
//             //QColor c(100, 172, 49);     // green
//             gradient.setColorAt(0, c.darker());
//             gradient.setColorAt(0.5, c);
//             gradient.setColorAt(1, c.darker());
//             palette.setBrush(QPalette::Button, gradient);
//             //palette.setBrush(autoType->backgroundRole(), gradient);
//             //palette.setBrush(QPalette::Window, gradient);  */
//             autoType->setPalette(palette);
//             }
//       else  
//             {
//             palette.setColor(QPalette::Button, qApp->palette().color(QPalette::Active, QPalette::Background));
//             //QColor c(qApp->palette().color(QPalette::Active, QPalette::Background));
//             //gradient.setColorAt(0, c);
//             //gradient.setColorAt(1, c.darker());
//             //palette.setBrush(QPalette::Button, gradient);
//             autoType->setPalette(palette);
//             }
// 
//       autoType->setToolTip(tr("automation type"));
//       connect(autoType, SIGNAL(activated(int)), SLOT(setAutomationType(int)));
//       grid->addWidget(autoType, _curGridRow++, 0, 1, 2);
// 
//       if (off) {
//             off->blockSignals(true);
//             updateOffState();   // init state
//             off->blockSignals(false);
//             }
//       connect(MusEGlobal::heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
// 
//       updateRouteButtons();
// 
//       }

//---------------------------------------------------------
//   AudioStrip
//    create mixer strip
//---------------------------------------------------------

AudioStrip::AudioStrip(QWidget* parent, MusECore::AudioTrack* at)
   : Strip(parent, at)
      {
      volume        = -1.0;
      panVal        = 0;
      _volPressed   = false;
      _panPressed   = false;
      
      record        = 0;
      off           = 0;
      
      // Set the whole strip's font, except for the label.    p4.0.45
      //setFont(MusEGlobal::config.fonts[1]);  // REMOVE Tim. Trackinfo. Changed.
      setStyleSheet(MusECore::font2StyleSheet(MusEGlobal::config.fonts[1]));

      MusECore::AudioTrack* t = (MusECore::AudioTrack*)track;
      channel       = at->channels();
      ///setMinimumWidth(STRIP_WIDTH);
      
      int ch = 0;
      for (; ch < channel; ++ch)
      {
            meter[ch] = new MusEGui::Meter(this);
// REMOVE Tim. Trackinfo. Added.
            _clipperLabel[ch] = new ClipperLabel(this);
            setClipperTooltip(ch);
            connect(_clipperLabel[ch], SIGNAL(clicked()), SLOT(resetClipper()));
            
      }
      for (; ch < MAX_CHANNELS; ++ch)
      {
            meter[ch] = 0;
// REMOVE Tim. Trackinfo. Removed.
            _clipperLabel[ch] = 0;
      }

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
      //    Gain
      //---------------------------------------------------

      gain = addController(gainType, 0, tr("Gain"));
      gain->setValue(t->gain());

      //---------------------------------------------------
      //    aux send
      //---------------------------------------------------

      int auxsSize = MusEGlobal::song->auxs()->size();
      if (t->hasAuxSend()) {
            for (int idx = 0; idx < auxsSize; ++idx) {
                  // the thought was to aquire the correct Aux name for each Aux
                  // now they are only called Aux1, Aux2, which isn't too usable.
                  QString title = ((MusECore::AudioAux*)(MusEGlobal::song->auxs()->at(idx)))->auxName();
                  if (title.length() > 8) { // shorten name
                      title = title.mid(0,8) + ".";
                  }
                  CompactSlider* control = addController(auxType, idx, title);

                  auxControl.push_back(control);
                  double val = MusECore::fast_log10(t->auxSend(idx))*20.0;
                  control->setValue(val);
                  
                  // Are there any Aux Track routing paths to this track? Then we cannot process aux for this track! 
                  // Hate to do this, but as a quick visual reminder, seems most logical to disable Aux knobs and labels. 
                  int rc = track->auxRefCount();
                  control->setEnabled( rc == 0 );
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
//       sliderGrid->setRowStretch(2, 100);  // REMOVE Tim. Trackinfo. Removed. TEST
//       sliderGrid->setContentsMargins(0, 1, 0, 0);   // REMOVE Tim. Trackinfo. Changed. TEST
      sliderGrid->setContentsMargins(0, 0, 0, 0);
      sliderGrid->setSpacing(0);

      /*-------------- clipper label -------------------*/      
// REMOVE Tim. Trackinfo. Changed.      
//       _clipperLabel = new ClipperLabel(this);
//       connect(_clipperLabel, SIGNAL(clicked()), SLOT(resetClipper()));
//       sliderGrid->addWidget(_clipperLabel, 0, 0, 1, -1);
//       sliderGrid->addItem(new QSpacerItem(0, 1), 1, 0, 1, -1);
      _clipperLayout = new QHBoxLayout();
      _clipperLayout->setSpacing(0);
      for(int ch = 0; ch < channel; ++ch)
        _clipperLayout->addWidget(_clipperLabel[ch]);
      sliderGrid->addLayout(_clipperLayout, 0, 0, 1, -1, Qt::AlignCenter);
      sliderGrid->addItem(new QSpacerItem(0, 1), 1, 0, 1, -1);
      
   
// REMOVE Tim. Trackinfo. Changed.      
//       slider = new MusEGui::Slider(this, "vol", Qt::Vertical, MusEGui::Slider::None);
//       slider = new MusEGui::Slider(this, "vol", Qt::Vertical, MusEGui::Slider::InsideVertical, 14, QColor(62, 37, 255));
      slider = new MusEGui::Slider(this, "vol", Qt::Vertical, MusEGui::Slider::InsideVertical, 14, QColor(128, 128, 255), ScaleDraw::TextHighlightSplitAndShadow);
//       QFont fnt = font();
//       fnt.setPointSize(8);
//       QFont fnt;
//       fnt.setFamily("Sans");
//       fnt.setPointSize(font().pointSize());
//       slider->setFont(fnt);

      slider->setCursorHoming(true);
      slider->setThumbLength(1);
      slider->setRange(MusEGlobal::config.minSlider-0.1, 10.0);
//       slider->setScaleMaxMinor(5);
      slider->setScale(MusEGlobal::config.minSlider-0.1, 10.0, 6.0, false);
      slider->setScaleBackBone(false);
      
// REMOVE Tim. Trackinfo. Changed.      
//       slider->setFixedWidth(20);
//       slider->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
      slider->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
      
      ///slider->setFont(MusEGlobal::config.fonts[1]);
      slider->setValue(MusECore::fast_log10(t->volume())*20.0);

      sliderGrid->addWidget(slider, 2, 0, Qt::AlignHCenter);

      for (int i = 0; i < channel; ++i) {
            //meter[i]->setRange(MusEGlobal::config.minSlider, 10.0);
            meter[i]->setRange(MusEGlobal::config.minMeter, 10.0);
            meter[i]->setFixedWidth(Strip::FIXED_METER_WIDTH);
            meter[i]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding); // REMOVE Tim. Trackinfo. Added.
//             connect(meter[i], SIGNAL(mousePress()), this, SLOT(resetPeaks())); // REMOVE Tim. Trackinfo. Changed.
            connect(meter[i], SIGNAL(mousePress()), this, SLOT(resetClipper()));
            sliderGrid->addWidget(meter[i], 2, i+1, Qt::AlignHCenter);
//             sliderGrid->setColumnStretch(i, 50); // REMOVE Tim. Trackinfo. Removed.
            }
      sliderGrid->addItem(new QSpacerItem(2,0),2,3);
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

      pan = addController(panType, 0, tr("Pan"));
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

// REMOVE Tim. Trackinfo. Added.
void AudioStrip::setClipperTooltip(int ch)
{
  QString clip_tt;
  switch(ch)
  {
    case 0:
      clip_tt = tr("L meter peak/clip");
    break;
    case 1:
      clip_tt = tr("R meter peak/clip");
    break;
    default:
      clip_tt = locale().toString(ch);
    break;
  }
  _clipperLabel[ch]->setToolTip(clip_tt);
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
