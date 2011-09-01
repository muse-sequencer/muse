//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: astrip.cpp,v 1.23.2.17 2009/11/16 01:55:55 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
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

#include "app.h"
#include "globals.h"
#include "audio.h"
#include "driver/audiodev.h"
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
//---------------------------------------------------------

void AudioStrip::configChanged()    
{ 
  songChanged(SC_CONFIG); 
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void AudioStrip::songChanged(int val)
      {
      // Is it simply a midi controller value adjustment? Forget it.
      if (val == SC_MIDI_CONTROLLER)
        return;
    
      AudioTrack* src = (AudioTrack*)track;
      
      // Do channels before config...
      if (val & SC_CHANNELS)
        updateChannels();
      
      // Catch when label font, or configuration min slider and meter values change.
      if (val & SC_CONFIG)
      {
        // Added by Tim. p3.3.9
        
        // Set the strip label's font.
        //label->setFont(config.fonts[1]);
        setLabelFont();
        
        // Adjust minimum volume slider and label values.
        slider->setRange(config.minSlider-0.1, 10.0);
        sl->setRange(config.minSlider, 10.0);
        
        // Adjust minimum aux knob and label values.
        int n = auxKnob.size();
        for (int idx = 0; idx < n; ++idx) 
        {
          auxKnob[idx]->blockSignals(true);
          auxLabel[idx]->blockSignals(true);
          auxKnob[idx]->setRange(config.minSlider-0.1, 10.0);
          auxLabel[idx]->setRange(config.minSlider, 10.1);
          auxKnob[idx]->blockSignals(false);
          auxLabel[idx]->blockSignals(false);
        }
        
        // Adjust minimum meter values.
        for(int c = 0; c < channel; ++c) 
          meter[c]->setRange(config.minMeter, 10.0);
      }
      
      if (mute && (val & SC_MUTE)) {      // mute && off
            mute->blockSignals(true);
            mute->setChecked(src->mute());
            mute->blockSignals(false);
            updateOffState();
            }
      if (solo && (val & SC_SOLO)) {
            if((bool)track->internalSolo())
            {
              if(!useSoloIconSet2)
              {
                solo->setIcon(*soloIconSet2);
                solo->setIconSize(soloIconOn->size());  
                useSoloIconSet2 = true;
              }  
            }  
            else if(useSoloIconSet2)
            {
              solo->setIcon(*soloIconSet1);
              solo->setIconSize(soloblksqIconOn->size());  
              useSoloIconSet2 = false;
            }  
            
            solo->blockSignals(true);
            solo->setChecked(track->solo());
            solo->blockSignals(false);
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
            }
      if (val & SC_AUX) {
            int n = auxKnob.size();
            for (int idx = 0; idx < n; ++idx) {
                  double val = fast_log10(src->auxSend(idx)) * 20.0;
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
            if(track->automationType() == AUTO_TOUCH || track->automationType() == AUTO_WRITE)
                  {
                  palette.setColor(QPalette::Button, QColor(Qt::red));
                  autoType->setPalette(palette);
                  }
            else if(track->automationType() == AUTO_READ)
                  {
                  palette.setColor(QPalette::Button, QColor(Qt::green));
                  autoType->setPalette(palette);
                  }
            else  
                  {
                  palette.setColor(QPalette::Button, qApp->palette().color(QPalette::Active, QPalette::Background));
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
      double vol = ((AudioTrack*)track)->volume();
        if (vol != volume) 
        {
            //printf("AudioStrip::updateVolume setting slider and label\n");
          
            slider->blockSignals(true);
            sl->blockSignals(true);
            double val = fast_log10(vol) * 20.0;
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
      double v = ((AudioTrack*)track)->pan();
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
      song->update(SC_MUTE);
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
      if (track->type() != Track::AUDIO_SOFTSYNTH)
            stereo->setEnabled(val);
      label->setEnabled(val);
      
      int n = auxKnob.size();
      for (int i = 0; i < n; ++i) 
      {
        auxKnob[i]->setEnabled(val);
        auxLabel[i]->setEnabled(val);
      }
            
      if (pre)
            pre->setEnabled(val);
      if (record)
            record->setEnabled(val);
      if (solo)
            solo->setEnabled(val);
      if (mute)
            mute->setEnabled(val);
      if (autoType)
            autoType->setEnabled(val);
      if (iR)
            iR->setEnabled(val);
      if (oR)
            oR->setEnabled(val);
      if (off) {
            off->blockSignals(true);
            off->setChecked(track->off());
            off->blockSignals(false);
            }
      }

//---------------------------------------------------------
//   preToggled
//---------------------------------------------------------

void AudioStrip::preToggled(bool val)
      {
      audio->msgSetPrefader((AudioTrack*)track, val);
      resetPeaks();
      song->update(SC_ROUTE);
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
      audio->msgSetChannels((AudioTrack*)track, nc);
      song->update(SC_CHANNELS);
      }

//---------------------------------------------------------
//   auxChanged
//---------------------------------------------------------

void AudioStrip::auxChanged(double val, int idx)
      {
      double vol;
      if (val <= config.minSlider) {
            vol = 0.0;
            val -= 1.0; // display special value "off"
            }
      else
            vol = pow(10.0, val/20.0);
      audio->msgSetAux((AudioTrack*)track, idx, vol);
      song->update(SC_AUX);
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

void AudioStrip::volumeChanged(double val)
      {
      AutomationType at = ((AudioTrack*)track)->automationType();
      if(at == AUTO_WRITE || (audio->isPlaying() && at == AUTO_TOUCH))
        track->enableVolumeController(false);
      
      double vol;
      if (val <= config.minSlider) {
            vol = 0.0;
            val -= 1.0; // display special value "off"
            }
      else
            vol = pow(10.0, val/20.0);
      volume = vol;
      audio->msgSetVolume((AudioTrack*)track, vol);
      // p4.0.21 audio->msgXXX waits. Do we really need to?
      //((AudioTrack*)track)->setVolume(vol);
      
      ((AudioTrack*)track)->recordAutomation(AC_VOLUME, vol);

      //song->update(SC_TRACK_MODIFIED); // for graphical automation update
      //song->controllerChange(track);
      }

//---------------------------------------------------------
//   volumePressed
//---------------------------------------------------------

void AudioStrip::volumePressed()
      {
      AutomationType at = ((AudioTrack*)track)->automationType();
      if(at == AUTO_WRITE || (at == AUTO_READ || at == AUTO_TOUCH))
        track->enableVolumeController(false);
      
      double val = slider->value();
      double vol;
      if (val <= config.minSlider) {
            vol = 0.0;
            //val -= 1.0; // display special value "off"
            }
      else
            vol = pow(10.0, val/20.0);
      volume = vol;
      audio->msgSetVolume((AudioTrack*)track, volume);
      // p4.0.21 audio->msgXXX waits. Do we really need to?
      //((AudioTrack*)track)->setVolume(volume);
      
      ((AudioTrack*)track)->startAutoRecord(AC_VOLUME, volume);
      }

//---------------------------------------------------------
//   volumeReleased
//---------------------------------------------------------

void AudioStrip::volumeReleased()
      {
      if(track->automationType() != AUTO_WRITE)
        track->enableVolumeController(true);
      
      ((AudioTrack*)track)->stopAutoRecord(AC_VOLUME, volume);
      }

//---------------------------------------------------------
//   volumeRightClicked
//---------------------------------------------------------
void AudioStrip::volumeRightClicked(const QPoint &p)
{
  song->execAutomationCtlPopup((AudioTrack*)track, p, AC_VOLUME);
}

//---------------------------------------------------------
//   volLabelChanged
//---------------------------------------------------------

void AudioStrip::volLabelChanged(double val)
      {
      AutomationType at = ((AudioTrack*)track)->automationType();
      if(at == AUTO_WRITE || (audio->isPlaying() && at == AUTO_TOUCH))
        track->enableVolumeController(false);
      
      double vol;
      if (val <= config.minSlider) {
            vol = 0.0;
            val -= 1.0; // display special value "off"
            }
      else
            vol = pow(10.0, val/20.0);
      volume = vol;
      slider->setValue(val);
      audio->msgSetVolume((AudioTrack*)track, vol);
      // p4.0.21 audio->msgXXX waits. Do we really need to?
      //((AudioTrack*)track)->setVolume(vol);
      
      ((AudioTrack*)track)->startAutoRecord(AC_VOLUME, vol);
      }

//---------------------------------------------------------
//   panChanged
//---------------------------------------------------------

void AudioStrip::panChanged(double val)
      {
      AutomationType at = ((AudioTrack*)track)->automationType();
      if(at == AUTO_WRITE || (audio->isPlaying() && at == AUTO_TOUCH))
        track->enablePanController(false);
      
      panVal = val;  
      audio->msgSetPan(((AudioTrack*)track), val);
      // p4.0.21 audio->msgXXX waits. Do we really need to?
      //((AudioTrack*)track)->setPan(val);
      
      ((AudioTrack*)track)->recordAutomation(AC_PAN, val);
      }

//---------------------------------------------------------
//   panPressed
//---------------------------------------------------------

void AudioStrip::panPressed()
      {
      AutomationType at = ((AudioTrack*)track)->automationType();
      if(at == AUTO_WRITE || (at == AUTO_READ || at == AUTO_TOUCH))
        track->enablePanController(false);
      
      panVal = pan->value();  
      audio->msgSetPan(((AudioTrack*)track), panVal);
      // p4.0.21 audio->msgXXX waits. Do we really need to?
      //((AudioTrack*)track)->setPan(panVal);
      ((AudioTrack*)track)->startAutoRecord(AC_PAN, panVal);
      }

//---------------------------------------------------------
//   panReleased
//---------------------------------------------------------

void AudioStrip::panReleased()
      {
      if(track->automationType() != AUTO_WRITE)
        track->enablePanController(true);
      ((AudioTrack*)track)->stopAutoRecord(AC_PAN, panVal);
      }

//---------------------------------------------------------
//   panRightClicked
//---------------------------------------------------------
void AudioStrip::panRightClicked(const QPoint &p)
{
  song->execAutomationCtlPopup((AudioTrack*)track, p, AC_PAN);
}

//---------------------------------------------------------
//   panLabelChanged
//---------------------------------------------------------

void AudioStrip::panLabelChanged(double val)
      {
      AutomationType at = ((AudioTrack*)track)->automationType();
      if(at == AUTO_WRITE || (audio->isPlaying() && at == AUTO_TOUCH))
        track->enablePanController(false);
      
      panVal = val;
      pan->setValue(val);
      audio->msgSetPan((AudioTrack*)track, val);
      // p4.0.21 audio->msgXXX waits. Do we really need to?
      //((AudioTrack*)track)->setPan(val);
      ((AudioTrack*)track)->startAutoRecord(AC_PAN, val);
      }

//---------------------------------------------------------
//   updateChannels
//---------------------------------------------------------
                                       
void AudioStrip::updateChannels()
      {
      AudioTrack* t = (AudioTrack*)track;
      int c = t->channels();
      //printf("AudioStrip::updateChannels track channels:%d current channels:%d\n", c, channel);
      
      if (c > channel) {
            for (int cc = channel; cc < c; ++cc) {
                  meter[cc] = new Meter(this);
                  //meter[cc]->setRange(config.minSlider, 10.0);
                  meter[cc]->setRange(config.minMeter, 10.0);
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
      }

//---------------------------------------------------------
//   addKnob
//    type = 0 - panorama
//           1 - aux send
//---------------------------------------------------------

Knob* AudioStrip::addKnob(int type, int id, DoubleLabel** dlabel)
      {
      Knob* knob = new Knob(this);
      knob->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
      if (type == 0)
            knob->setRange(-1.0, +1.0);
      else
            knob->setRange(config.minSlider-0.1, 10.0);
      knob->setBackgroundRole(QPalette::Mid);

      if (type == 0)
            knob->setToolTip(tr("panorama"));
      else
            knob->setToolTip(tr("aux send level"));

      DoubleLabel* pl;
      if (type == 0)
            pl = new DoubleLabel(0, -1.0, +1.0, this);
      else
            pl = new DoubleLabel(0.0, config.minSlider, 10.1, this);
            
      if (dlabel)
            *dlabel = pl;
      pl->setSlider(knob);
      pl->setFont(config.fonts[1]);
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
      plb->setFont(config.fonts[1]);
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
      //connect(pl, SIGNAL(valueChanged(double, int)), SLOT(panChanged(double)));

      if (type == 0) {
            connect(pl, SIGNAL(valueChanged(double, int)), SLOT(panLabelChanged(double)));
            connect(knob, SIGNAL(sliderMoved(double,int)), SLOT(panChanged(double)));
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

AudioStrip::AudioStrip(QWidget* parent, AudioTrack* at)
   : Strip(parent, at)
      {
      volume        = -1.0;
      panVal        = 0;
      
      record        = 0;
      off           = 0;
      
      AudioTrack* t = (AudioTrack*)track;
      channel       = at->channels();
      ///setMinimumWidth(STRIP_WIDTH);
      
      int ch = 0;
      for (; ch < channel; ++ch)
            meter[ch] = new Meter(this);
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
      stereo->setFont(config.fonts[1]);
      QIcon stereoSet;
      stereoSet.addPixmap(*monoIcon, QIcon::Normal, QIcon::Off);
      stereoSet.addPixmap(*stereoIcon, QIcon::Normal, QIcon::On);
      stereo->setIcon(stereoSet);
      stereo->setIconSize(monoIcon->size());  

      stereo->setCheckable(true);
      stereo->setToolTip(tr("1/2 channel"));
      stereo->setChecked(channel == 2);
      stereo->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
      connect(stereo, SIGNAL(clicked(bool)), SLOT(stereoToggled(bool)));

      // disable mono/stereo for Synthesizer-Plugins
      if (t->type() == Track::AUDIO_SOFTSYNTH)
            stereo->setEnabled(false);

      pre = new QToolButton();
      pre->setFont(config.fonts[1]);
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

      int auxsSize = song->auxs()->size();
      if (t->hasAuxSend()) {
            for (int idx = 0; idx < auxsSize; ++idx) {
                  DoubleLabel* al;
                  Knob* ak = addKnob(1, idx, &al);
                  auxKnob.push_back(ak);
                  auxLabel.push_back(al);
                  double val = fast_log10(t->auxSend(idx))*20.0;
                  ak->setValue(val);
                  al->setValue(val);
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
      
      slider = new Slider(this, "vol", Qt::Vertical, Slider::None,
         Slider::BgTrough | Slider::BgSlot);
      slider->setCursorHoming(true);
      slider->setRange(config.minSlider-0.1, 10.0);
      slider->setFixedWidth(20);
      slider->setFont(config.fonts[1]);
      slider->setValue(fast_log10(t->volume())*20.0);

      sliderGrid->addWidget(slider, 0, 0, Qt::AlignHCenter);

      for (int i = 0; i < channel; ++i) {
            //meter[i]->setRange(config.minSlider, 10.0);
            meter[i]->setRange(config.minMeter, 10.0);
            meter[i]->setFixedWidth(15);
            connect(meter[i], SIGNAL(mousePress()), this, SLOT(resetPeaks()));
            sliderGrid->addWidget(meter[i], 0, i+1, Qt::AlignHCenter);
            sliderGrid->setColumnStretch(i, 50);
            }
      sliderGrid->addItem(new QSpacerItem(2,0),0,3);
      grid->addLayout(sliderGrid, _curGridRow++, 0, 1, 2); 

      sl = new DoubleLabel(0.0, config.minSlider, 10.0, this);
      sl->setSlider(slider);
      sl->setFont(config.fonts[1]);
      sl->setBackgroundRole(QPalette::Mid);
      sl->setSuffix(tr("dB"));
      sl->setFrame(true);
      sl->setPrecision(0);
      sl->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum));
      sl->setValue(fast_log10(t->volume()) * 20.0);

      connect(sl, SIGNAL(valueChanged(double,int)), SLOT(volLabelChanged(double)));
      //connect(sl, SIGNAL(valueChanged(double,int)), SLOT(volumeChanged(double)));
      connect(slider, SIGNAL(valueChanged(double,int)), sl, SLOT(setValue(double)));
      connect(slider, SIGNAL(sliderMoved(double,int)), SLOT(volumeChanged(double)));
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
            record  = new TransparentToolButton(this);
            record->setCheckable(true);
            record->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
            record->setBackgroundRole(QPalette::Mid);
            QIcon iconSet;
            iconSet.addPixmap(*record_on_Icon, QIcon::Normal, QIcon::On);
            iconSet.addPixmap(*record_off_Icon, QIcon::Normal, QIcon::Off);
            record->setIcon(iconSet);
            record->setIconSize(record_on_Icon->size());  
            record->setToolTip(tr("record"));
            record->setChecked(t->recordFlag());
            connect(record, SIGNAL(clicked(bool)), SLOT(recordToggled(bool)));
            }

      Track::TrackType type = t->type();

      mute  = new QToolButton();
      QIcon muteSet;
      muteSet.addPixmap(*muteIconOn, QIcon::Normal, QIcon::Off);
      muteSet.addPixmap(*muteIconOff, QIcon::Normal, QIcon::On);
      mute->setIcon(muteSet);
      mute->setIconSize(muteIconOn->size());  
      mute->setCheckable(true);
      mute->setToolTip(tr("mute"));
      mute->setChecked(t->mute());
      mute->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
      connect(mute, SIGNAL(clicked(bool)), SLOT(muteToggled(bool)));

      solo  = new QToolButton();
      
      if((bool)t->internalSolo())
      {
        solo->setIcon(*soloIconSet2);
        solo->setIconSize(soloIconOn->size());  
        useSoloIconSet2 = true;
      }  
      else  
      {
        solo->setIcon(*soloIconSet1);
        solo->setIconSize(soloblksqIconOn->size());  
        useSoloIconSet2 = false;
      }  
              
      solo->setCheckable(true);
      solo->setChecked(t->solo());
      solo->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
      connect(solo, SIGNAL(clicked(bool)), SLOT(soloToggled(bool)));
      if (type == Track::AUDIO_OUTPUT) {
            record->setToolTip(tr("record downmix"));
            //solo->setToolTip(tr("solo mode (monitor)"));
            solo->setToolTip(tr("solo mode"));
            }
      else {
            //solo->setToolTip(tr("pre fader listening"));
            solo->setToolTip(tr("solo mode"));
            }

      off  = new TransparentToolButton(this);
      QIcon iconSet;
      iconSet.addPixmap(*exit1Icon, QIcon::Normal, QIcon::On);
      iconSet.addPixmap(*exitIcon, QIcon::Normal, QIcon::Off);
      off->setIcon(iconSet);
      off->setIconSize(exit1Icon->size());  
      off->setBackgroundRole(QPalette::Mid);
      off->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
      off->setCheckable(true);
      off->setToolTip(tr("off"));
      off->setChecked(t->off());
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

      if (type != Track::AUDIO_AUX) {
            iR = new QToolButton();
            iR->setFont(config.fonts[1]);
            iR->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
            iR->setText(tr("iR"));
            iR->setCheckable(false);
            iR->setToolTip(tr("input routing"));
            grid->addWidget(iR, _curGridRow, 0);
            connect(iR, SIGNAL(pressed()), SLOT(iRoutePressed()));
            }
      
      oR = new QToolButton();
      oR->setFont(config.fonts[1]);
      oR->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
      oR->setText(tr("oR"));
      oR->setCheckable(false);
      oR->setToolTip(tr("output routing"));
      grid->addWidget(oR, _curGridRow++, 1);
      connect(oR, SIGNAL(pressed()), SLOT(oRoutePressed()));

      //---------------------------------------------------
      //    automation type
      //---------------------------------------------------

      autoType = new ComboBox();
      autoType->setFont(config.fonts[1]);
      autoType->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
      
      autoType->addAction(tr("Off"), AUTO_OFF);
      autoType->addAction(tr("Read"), AUTO_READ);
      autoType->addAction(tr("Touch"), AUTO_TOUCH);
      autoType->addAction(tr("Write"), AUTO_WRITE);
      autoType->setCurrentItem(t->automationType());

      QPalette palette;
      if(t->automationType() == AUTO_TOUCH || t->automationType() == AUTO_WRITE)
            {
            palette.setColor(QPalette::Button, QColor(Qt::red));
            autoType->setPalette(palette);
            }
      else if(t->automationType() == AUTO_READ)
            {
            palette.setColor(QPalette::Button, QColor(Qt::green));
            autoType->setPalette(palette);
            }
      else  
            {
            palette.setColor(QPalette::Button, qApp->palette().color(QPalette::Active, QPalette::Background));
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
      connect(heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
      }

//---------------------------------------------------------
//   iRoutePressed
//---------------------------------------------------------

void AudioStrip::iRoutePressed()
      {
      RoutePopupMenu* pup = muse->getRoutingPopupMenu();
      iR->setDown(false);     
      pup->exec(QCursor::pos(), track, false);
      }
      
//---------------------------------------------------------
//   oRoutePressed
//---------------------------------------------------------

void AudioStrip::oRoutePressed()
{
      RoutePopupMenu* pup = muse->getRoutingPopupMenu();
      oR->setDown(false);     
      pup->exec(QCursor::pos(), track, true);
}

