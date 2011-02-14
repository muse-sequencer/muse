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
#include "popupmenu.h"

//---------------------------------------------------------
//   MenuTitleItem
//---------------------------------------------------------

MenuTitleItem::MenuTitleItem(const QString& ss, QWidget* parent)
  : QWidgetAction(parent)
      {
        s = ss;
        // Don't allow to click on it.
        setEnabled(false);
        // Just to be safe, set to -1 instead of default 0.
        setData(-1);
      }

QWidget* MenuTitleItem::createWidget(QWidget *parent)
{
  QLabel* l = new QLabel(s, parent);
  l->setAlignment(Qt::AlignCenter);
  l->setAutoFillBackground(true);
  //QPalette palette;
  //palette.setColor(label->backgroundRole(), c);
  //l->setPalette(palette);
  l->setBackgroundRole(QPalette::Dark);
  return l;
}

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
      
      // p3.3.47
      // Update the routing popup menu if anything relevant changed.
      if (val & (SC_ROUTE | SC_CHANNELS | SC_CONFIG))
      { 
        //updateRouteMenus();
        muse->updateRouteMenus(track, this);      // p3.3.50 Use this handy shared routine.
      }
      
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
            // Added by Tim. p3.3.9
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
            if(track->automationType() == AUTO_TOUCH || track->automationType() == AUTO_WRITE)
                  {
                  //autoType->setPaletteBackgroundColor(Qt::red);
                  QPalette palette;
                  palette.setColor(autoType->backgroundRole(), QColor(Qt::red));
                  autoType->setPalette(palette);
                  }
            else  
                  {
                  //autoType->setPaletteBackgroundColor(qApp->palette().active().background());
                  QPalette palette;
                  palette.setColor(autoType->backgroundRole(), qApp->palette().color(QPalette::Active, QPalette::Background));
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
      ((AudioTrack*)track)->recordAutomation(AC_VOLUME, vol);

      song->update(SC_TRACK_MODIFIED); // for graphical automation update
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
                  sliderGrid->addWidget(meter[cc], 0, cc+1, Qt::AlignHCenter);
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

      autoType = new ComboBox(this);
      autoType->setFont(config.fonts[1]);
      autoType->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
      autoType->setAlignment(Qt::AlignCenter);
      
      autoType->insertItem(tr("Off"), AUTO_OFF);
      autoType->insertItem(tr("Read"), AUTO_READ);
      autoType->insertItem(tr("Touch"), AUTO_TOUCH);
      autoType->insertItem(tr("Write"), AUTO_WRITE);
      autoType->setCurrentItem(t->automationType());
      // FIXME: TODO: Convert ComboBox to QT4
      //autoType->insertItem(AUTO_OFF, tr("Off"));
      //autoType->insertItem(AUTO_READ, tr("Read"));
      //autoType->insertItem(AUTO_TOUCH, tr("Touch"));
      //autoType->insertItem(AUTO_WRITE, tr("Write"));
      //autoType->setCurrentIndex(t->automationType());
      
      if(t->automationType() == AUTO_TOUCH || t->automationType() == AUTO_WRITE)
            {
            // FIXME:
            //autoType->setPaletteBackgroundColor(Qt::red);
	    QPalette palette;
	    palette.setColor(autoType->backgroundRole(), QColor(Qt::red));
	    autoType->setPalette(palette);
            }
      else  
            {
            // FIXME:
            //autoType->setPaletteBackgroundColor(qApp->palette().active().background());
            QPalette palette;
            palette.setColor(autoType->backgroundRole(), qApp->palette().color(QPalette::Active, QPalette::Background));
            autoType->setPalette(palette);
            }
      autoType->setToolTip(tr("automation type"));
      connect(autoType, SIGNAL(activated(int,int)), SLOT(setAutomationType(int,int)));
      grid->addWidget(autoType, _curGridRow++, 0, 1, 2);

      if (off) {
            off->blockSignals(true);
            updateOffState();   // init state
            off->blockSignals(false);
            }
      connect(heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
      }

//---------------------------------------------------------
//   addMenuItem
//---------------------------------------------------------

static int addMenuItem(AudioTrack* track, Track* route_track, PopupMenu* lb, int id, RouteMenuMap& mm, int channel, int channels, bool isOutput)
{
  // totalInChannels is only used by syntis.
  int toch = ((AudioTrack*)track)->totalOutChannels();
  // If track channels = 1, it must be a mono synth. And synti channels cannot be changed by user.
  if(track->channels() == 1)
    toch = 1;
  
  // Don't add the last stray mono route if the track is stereo.
  //if(route_track->channels() > 1 && (channel+1 == chans))
  //  return id;
    
  RouteList* rl = isOutput ? track->outRoutes() : track->inRoutes();
  
  QAction* act;
  
  QString s(route_track->name());
  
  act = lb->addAction(s);
  act->setData(id);
  act->setCheckable(true);
  
  int ach = channel;
  int bch = -1;
  
  Route r(route_track, isOutput ? ach : bch, channels);
  
  r.remoteChannel = isOutput ? bch : ach;
  
  mm.insert( pRouteMenuMap(id, r) );
  
  for(iRoute ir = rl->begin(); ir != rl->end(); ++ir) 
  {
    if(ir->type == Route::TRACK_ROUTE && ir->track == route_track && ir->remoteChannel == r.remoteChannel)
    {
      int tcompch = r.channel;
      if(tcompch == -1)
        tcompch = 0;
      int tcompchs = r.channels;
      if(tcompchs == -1)
        tcompchs = isOutput ? track->channels() : route_track->channels();
      
      int compch = ir->channel;
      if(compch == -1)
        compch = 0;
      int compchs = ir->channels;
      if(compchs == -1)
        compchs = isOutput ? track->channels() : ir->track->channels();
      
      if(compch == tcompch && compchs == tcompchs) 
      {
        act->setChecked(true);
        break;
      }
    }  
  }
  return ++id;      
}

//---------------------------------------------------------
//   addAuxPorts
//---------------------------------------------------------

static int addAuxPorts(AudioTrack* t, PopupMenu* lb, int id, RouteMenuMap& mm, int channel, int channels, bool isOutput)
      {
      AuxList* al = song->auxs();
      for (iAudioAux i = al->begin(); i != al->end(); ++i) {
            Track* track = *i;
            if (t == track)
                  continue;
            id = addMenuItem(t, track, lb, id, mm, channel, channels, isOutput);
            }
      return id;      
      }

//---------------------------------------------------------
//   addInPorts
//---------------------------------------------------------

static int addInPorts(AudioTrack* t, PopupMenu* lb, int id, RouteMenuMap& mm, int channel, int channels, bool isOutput)
      {
      InputList* al = song->inputs();
      for (iAudioInput i = al->begin(); i != al->end(); ++i) {
            Track* track = *i;
            if (t == track)
                  continue;
            id = addMenuItem(t, track, lb, id, mm, channel, channels, isOutput);
            }
      return id;      
      }

//---------------------------------------------------------
//   addOutPorts
//---------------------------------------------------------

static int addOutPorts(AudioTrack* t, PopupMenu* lb, int id, RouteMenuMap& mm, int channel, int channels, bool isOutput)
      {
      OutputList* al = song->outputs();
      for (iAudioOutput i = al->begin(); i != al->end(); ++i) {
            Track* track = *i;
            if (t == track)
                  continue;
            id = addMenuItem(t, track, lb, id, mm, channel, channels, isOutput);
            }
      return id;      
      }

//---------------------------------------------------------
//   addGroupPorts
//---------------------------------------------------------

static int addGroupPorts(AudioTrack* t, PopupMenu* lb, int id, RouteMenuMap& mm, int channel, int channels, bool isOutput)
      {
      GroupList* al = song->groups();
      for (iAudioGroup i = al->begin(); i != al->end(); ++i) {
            Track* track = *i;
            if (t == track)
                  continue;
            id = addMenuItem(t, track, lb, id, mm, channel, channels, isOutput);
            }
      return id;      
      }

//---------------------------------------------------------
//   addWavePorts
//---------------------------------------------------------

static int addWavePorts(AudioTrack* t, PopupMenu* lb, int id, RouteMenuMap& mm, int channel, int channels, bool isOutput)
      {
      WaveTrackList* al = song->waves();
      for (iWaveTrack i = al->begin(); i != al->end(); ++i) {
            Track* track = *i;
            if (t == track)
                  continue;
            id = addMenuItem(t, track, lb, id, mm, channel, channels, isOutput);
            }
      return id;      
      }

//---------------------------------------------------------
//   addSyntiPorts
//---------------------------------------------------------

static int addSyntiPorts(AudioTrack* t, PopupMenu* lb, int id, 
                         RouteMenuMap& mm, int channel, int channels, bool isOutput)
{
      RouteList* rl = isOutput ? t->outRoutes() : t->inRoutes();
      
      QAction* act;
      
      SynthIList* al = song->syntis();
      for (iSynthI i = al->begin(); i != al->end(); ++i) 
      {
            Track* track = *i;
            if (t == track)
                  continue;
            int toch = ((AudioTrack*)track)->totalOutChannels();
            // If track channels = 1, it must be a mono synth. And synti channels cannot be changed by user.
            if(track->channels() == 1)
              toch = 1;
            
            // totalInChannels is only used by syntis.
            int chans = (!isOutput || track->type() != Track::AUDIO_SOFTSYNTH) ? toch : ((AudioTrack*)track)->totalInChannels();
            
            int tchans = (channels != -1) ? channels: t->channels();
            if(tchans == 2)
            {
              // Ignore odd numbered left-over mono channel.
              //chans = chans & ~1;
              //if(chans != 0)
                chans -= 1;
            }
            
            if(chans > 0)
            {
              PopupMenu* chpup = new PopupMenu(lb);
              chpup->setTitle(track->name());
              for(int ch = 0; ch < chans; ++ch)
              {
                char buffer[128];
                if(tchans == 2)
                  snprintf(buffer, 128, "%s %d,%d", chpup->tr("Channel").toLatin1().constData(), ch+1, ch+2);
                else  
                  snprintf(buffer, 128, "%s %d", chpup->tr("Channel").toLatin1().constData(), ch+1);
                act = chpup->addAction(QString(buffer));
                act->setData(id);
                act->setCheckable(true);
                
                int ach = (channel == -1) ? ch : channel;
                int bch = (channel == -1) ? -1 : ch;
                
                Route rt(track, (t->type() != Track::AUDIO_SOFTSYNTH || isOutput) ? ach : bch, tchans);
                //Route rt(track, ch);
                //rt.remoteChannel = -1;
                rt.remoteChannel = (t->type() != Track::AUDIO_SOFTSYNTH || isOutput) ? bch : ach;
                
                mm.insert( pRouteMenuMap(id, rt) );
                
                for(iRoute ir = rl->begin(); ir != rl->end(); ++ir) 
                {
                  if(ir->type == Route::TRACK_ROUTE && ir->track == track && ir->remoteChannel == rt.remoteChannel)
                  {
                    int tcompch = rt.channel;
                    if(tcompch == -1)
                      tcompch = 0;
                    int tcompchs = rt.channels;
                    if(tcompchs == -1)
                      tcompchs = isOutput ? t->channels() : track->channels();
                    
                    int compch = ir->channel;
                    if(compch == -1)
                      compch = 0;
                    int compchs = ir->channels;
                    if(compchs == -1)
                      compchs = isOutput ? t->channels() : ir->track->channels();
                    
                    if(compch == tcompch && compchs == tcompchs) 
                    {
                      act->setChecked(true);
                      break;
                    }
                  }
                }  
                ++id;
              }
            
              lb->addMenu(chpup);
            }
      }
      return id;      
}

//---------------------------------------------------------
//   addMultiChannelOutPorts
//---------------------------------------------------------

static int addMultiChannelPorts(AudioTrack* t, PopupMenu* pup, int id, RouteMenuMap& mm, bool isOutput)
{
  int toch = t->totalOutChannels();
  // If track channels = 1, it must be a mono synth. And synti channels cannot be changed by user.
  if(t->channels() == 1)
    toch = 1;
  
  // Number of allocated buffers is always MAX_CHANNELS or more, even if _totalOutChannels is less. 
  // totalInChannels is only used by syntis.
  int chans = (isOutput || t->type() != Track::AUDIO_SOFTSYNTH) ? toch : t->totalInChannels();

  if(chans > 1)
    pup->addAction(new MenuTitleItem("<Mono>", pup)); 
  
  //
  // If it's more than one channel, create a sub-menu. If it's just one channel, don't bother with a sub-menu...
  //

  PopupMenu* chpup = pup;
  
  for(int ch = 0; ch < chans; ++ch)
  {
    // If more than one channel, create the sub-menu.
    if(chans > 1)
      chpup = new PopupMenu(pup);
    
    if(isOutput)
    {
      switch(t->type()) 
      {
        
        case Track::AUDIO_INPUT:
              id = addWavePorts(t, chpup, id, mm, ch, 1, isOutput);
        case Track::WAVE:
        case Track::AUDIO_GROUP:
        case Track::AUDIO_SOFTSYNTH:
              id = addOutPorts(t, chpup, id, mm, ch, 1, isOutput);
              id = addGroupPorts(t, chpup, id, mm, ch, 1, isOutput);
              id = addSyntiPorts(t, chpup, id, mm, ch, 1, isOutput);
              break;
        case Track::AUDIO_AUX:
              id = addOutPorts(t, chpup, id, mm, ch, 1, isOutput);
              break;
        default:
              break;
      }
    }
    else
    {
      switch(t->type()) 
      {
        
        case Track::AUDIO_OUTPUT:
              id = addWavePorts(t, chpup, id, mm, ch, 1, isOutput);
              id = addInPorts(t, chpup, id, mm, ch, 1, isOutput);
              id = addGroupPorts(t, chpup, id, mm, ch, 1, isOutput);
              id = addAuxPorts(t, chpup, id, mm, ch, 1, isOutput);
              id = addSyntiPorts(t, chpup, id, mm, ch, 1, isOutput);
              break;
        case Track::WAVE:
              id = addInPorts(t, chpup, id, mm, ch, 1, isOutput);
              break;
        case Track::AUDIO_SOFTSYNTH:
        case Track::AUDIO_GROUP:
              id = addWavePorts(t, chpup, id, mm, ch, 1, isOutput);
              id = addInPorts(t, chpup, id, mm, ch, 1, isOutput);
              id = addGroupPorts(t, chpup, id, mm, ch, 1, isOutput);
              id = addSyntiPorts(t, chpup, id, mm, ch, 1, isOutput);
              break;
        default:
              break;
      }
    }
      
    // If more than one channel, add the created sub-menu.
    if(chans > 1)
    {
      char buffer[128];
      snprintf(buffer, 128, "%s %d", pup->tr("Channel").toLatin1().constData(), ch+1);
      chpup->setTitle(QString(buffer));
      pup->addMenu(chpup);
    }  
  } 
       
  // For stereo listing, ignore odd numbered left-over channels.
  chans -= 1;
  if(chans > 0)
  {
    // Ignore odd numbered left-over channels.
    //int schans = (chans & ~1) - 1;
    
    pup->addSeparator();
    pup->addAction(new MenuTitleItem("<Stereo>", pup));
  
    //
    // If it's more than two channels, create a sub-menu. If it's just two channels, don't bother with a sub-menu...
    //
    
    chpup = pup;
    if(chans <= 2)
      // Just do one iteration.
      chans = 1;
    
    for(int ch = 0; ch < chans; ++ch)
    {
      // If more than two channels, create the sub-menu.
      if(chans > 2)
        chpup = new PopupMenu(pup);
      
      if(isOutput)
      {
        switch(t->type()) 
        {
          case Track::AUDIO_INPUT:
                id = addWavePorts(t, chpup, id, mm, ch, 2, isOutput);
          case Track::WAVE:
          case Track::AUDIO_GROUP:
          case Track::AUDIO_SOFTSYNTH:
                id = addOutPorts(t, chpup, id, mm, ch, 2, isOutput);
                id = addGroupPorts(t, chpup, id, mm, ch, 2, isOutput);
                id = addSyntiPorts(t, chpup, id, mm, ch, 2, isOutput);
                break;
          case Track::AUDIO_AUX:
                id = addOutPorts(t, chpup, id, mm, ch, 2, isOutput);
                break;
          default:
                break;
        }
      }    
      else
      {
        switch(t->type()) 
        {
          case Track::AUDIO_OUTPUT:
                id = addWavePorts(t, chpup, id, mm, ch, 2, isOutput);
                id = addInPorts(t, chpup, id, mm, ch, 2, isOutput);
                id = addGroupPorts(t, chpup, id, mm, ch, 2, isOutput);
                id = addAuxPorts(t, chpup, id, mm, ch, 2, isOutput);
                id = addSyntiPorts(t, chpup, id, mm, ch, 2, isOutput);
                break;
          case Track::WAVE:
                id = addInPorts(t, chpup, id, mm, ch, 2, isOutput);
                break;
          case Track::AUDIO_SOFTSYNTH:
          case Track::AUDIO_GROUP:
                id = addWavePorts(t, chpup, id, mm, ch, 2, isOutput);
                id = addInPorts(t, chpup, id, mm, ch, 2, isOutput);
                id = addGroupPorts(t, chpup, id, mm, ch, 2, isOutput);
                id = addSyntiPorts(t, chpup, id, mm, ch, 2, isOutput);
                break;
          default:
                break;
        }
      }
      
      // If more than two channels, add the created sub-menu.
      if(chans > 2)
      {
        char buffer[128];
        snprintf(buffer, 128, "%s %d,%d", pup->tr("Channel").toLatin1().constData(), ch+1, ch+2);
        chpup->setTitle(QString(buffer));
        pup->addMenu(chpup);
      }  
    } 
  }
  
  return id;
}

//---------------------------------------------------------
//   nonSyntiTrackAddSyntis
//---------------------------------------------------------

static int nonSyntiTrackAddSyntis(AudioTrack* t, PopupMenu* lb, int id, RouteMenuMap& mm, bool isOutput)
{
      RouteList* rl = isOutput ? t->outRoutes() : t->inRoutes();
      
      QAction* act;
      SynthIList* al = song->syntis();
      for (iSynthI i = al->begin(); i != al->end(); ++i) 
      {
            Track* track = *i;
            if (t == track)
                  continue;
            
            int toch = ((AudioTrack*)track)->totalOutChannels();
            // If track channels = 1, it must be a mono synth. And synti channels cannot be changed by user.
            if(track->channels() == 1)
              toch = 1;
            
            // totalInChannels is only used by syntis.
            int chans = (!isOutput || track->type() != Track::AUDIO_SOFTSYNTH) ? toch : ((AudioTrack*)track)->totalInChannels();
            
            //int schans = synti->channels();
            //if(schans < chans)
            //  chans = schans;
//            int tchans = (channels != -1) ? channels: t->channels();
//            if(tchans == 2)
//            {
              // Ignore odd numbered left-over mono channel.
              //chans = chans & ~1;
              //if(chans != 0)
//                chans -= 1;
//            }
            //int tchans = (channels != -1) ? channels: t->channels();
            
            if(chans > 0)
            {
              PopupMenu* chpup = new PopupMenu(lb);
              chpup->setTitle(track->name());
              if(chans > 1)
                chpup->addAction(new MenuTitleItem("<Mono>", chpup));
              
              for(int ch = 0; ch < chans; ++ch)
              {
                char buffer[128];
                snprintf(buffer, 128, "%s %d", chpup->tr("Channel").toLatin1().constData(), ch+1);
                act = chpup->addAction(QString(buffer));
                act->setData(id);
                act->setCheckable(true);
                
                int ach = ch;
                int bch = -1;
                
                Route rt(track, isOutput ? bch : ach, 1);
                
                rt.remoteChannel = isOutput ? ach : bch;
                
                mm.insert( pRouteMenuMap(id, rt) );
                
                for(iRoute ir = rl->begin(); ir != rl->end(); ++ir) 
                {
                  if(ir->type == Route::TRACK_ROUTE && ir->track == track && ir->remoteChannel == rt.remoteChannel)
                  {
                    int tcompch = rt.channel;
                    if(tcompch == -1)
                      tcompch = 0;
                    int tcompchs = rt.channels;
                    if(tcompchs == -1)
                      tcompchs = isOutput ? t->channels() : track->channels();
                    
                    int compch = ir->channel;
                    if(compch == -1)
                      compch = 0;
                    int compchs = ir->channels;
                    if(compchs == -1)
                      compchs = isOutput ? t->channels() : ir->track->channels();
                    
                    if(compch == tcompch && compchs == tcompchs) 
                    {
                      act->setChecked(true);
                      break;
                    }
                  }
                }
                ++id;
              }
            
              chans -= 1;
              if(chans > 0)
              {
                // Ignore odd numbered left-over channels.
                //int schans = (chans & ~1) - 1;
                
                chpup->addSeparator();
                chpup->addAction(new MenuTitleItem("<Stereo>", chpup)); 
              
                for(int ch = 0; ch < chans; ++ch)
                {
                  char buffer[128];
                  snprintf(buffer, 128, "%s %d,%d", chpup->tr("Channel").toLatin1().constData(), ch+1, ch+2);
                  act = chpup->addAction(QString(buffer));
                  act->setData(id);
                  act->setCheckable(true);
                  
                  int ach = ch;
                  int bch = -1;
                  
                  Route rt(track, isOutput ? bch : ach, 2);
                  
                  rt.remoteChannel = isOutput ? ach : bch;
                  
                  mm.insert( pRouteMenuMap(id, rt) );
                  
                  for(iRoute ir = rl->begin(); ir != rl->end(); ++ir) 
                  {
                    if(ir->type == Route::TRACK_ROUTE && ir->track == track && ir->remoteChannel == rt.remoteChannel)
                    {
                      int tcompch = rt.channel;
                      if(tcompch == -1)
                        tcompch = 0;
                      int tcompchs = rt.channels;
                      if(tcompchs == -1)
                        tcompchs = isOutput ? t->channels() : track->channels();
                      
                      int compch = ir->channel;
                      if(compch == -1)
                        compch = 0;
                      int compchs = ir->channels;
                      if(compchs == -1)
                        compchs = isOutput ? t->channels() : ir->track->channels();
                      
                      if(compch == tcompch && compchs == tcompchs) 
                      {
                        act->setChecked(true);
                        break;
                      }
                    }  
                  }
                  ++id;
                }
              }
              
              lb->addMenu(chpup);
            }
      }
      return id;      
}

//---------------------------------------------------------
//   addMidiPorts
//---------------------------------------------------------

static int addMidiPorts(AudioTrack* t, PopupMenu* pup, int id, RouteMenuMap& mm, bool isOutput)
{
  QAction* act;
  for(int i = 0; i < MIDI_PORTS; ++i)
  {
    MidiPort* mp = &midiPorts[i];
    MidiDevice* md = mp->device();
    
    // This is desirable, but could lead to 'hidden' routes unless we add more support
    //  such as removing the existing routes when user changes flags.
    // So for now, just list all valid ports whether read or write.
    if(!md)
      continue;
    //if(!(md->rwFlags() & (isOutput ? 1 : 2)))
    //  continue;
          
    RouteList* rl = isOutput ? t->outRoutes() : t->inRoutes();
    
    PopupMenu* subp = new PopupMenu(pup);
    subp->setTitle(md->name()); 
    
    int chanmask = 0;
    // To reduce number of routes required, from one per channel to just one containing a channel mask. 
    // Look for the first route to this midi port. There should always be only a single route for each midi port, now.
    for(iRoute ir = rl->begin(); ir != rl->end(); ++ir)   
    {
      if(ir->type == Route::MIDI_PORT_ROUTE && ir->midiPort == i) 
      {
        // We have a route to the midi port. Grab the channel mask.
        chanmask = ir->channel;
        break;
      }
    }
    
    for(int ch = 0; ch < MIDI_CHANNELS; ++ch) 
    {
      act = subp->addAction(QString("Channel %1").arg(ch+1));
      act->setCheckable(true);
      act->setData(id);
      
      int chbit = 1 << ch;
      Route srcRoute(i, chbit);    // In accordance with new channel mask, use the bit position.
      
      mm.insert( pRouteMenuMap(id, srcRoute) );
      
      if(chanmask & chbit)                  // Is the channel already set? Show item check mark.
        act->setChecked(true);
      
      ++id;
    }
    
    //gid = MIDI_PORTS * MIDI_CHANNELS + i;           // Make sure each 'toggle' item gets a unique id.
    act = subp->addAction(QString("Toggle all"));
    //act->setCheckable(true);
    act->setData(id);
    Route togRoute(i, (1 << MIDI_CHANNELS) - 1);    // Set all channel bits.
    mm.insert( pRouteMenuMap(id, togRoute) );
    ++id;
    
    pup->addMenu(subp);
  }    
  return id;      
}

//---------------------------------------------------------
//   routingPopupMenuActivated
//---------------------------------------------------------

void AudioStrip::routingPopupMenuActivated(QAction* act)
{
      if(!track || gRoutingPopupMenuMaster != this || track->isMidiTrack())
        return;
      
      PopupMenu* pup = muse->getRoutingPopupMenu();
      
      if(pup->actions().isEmpty())
        return;
        
      AudioTrack* t = (AudioTrack*)track;
      RouteList* rl = gIsOutRoutingPopupMenu ? t->outRoutes() : t->inRoutes();
      
      int n = act->data().toInt();
      if (n == -1) 
        return;
      
      iRouteMenuMap imm = gRoutingMenuMap.find(n);
      if(imm == gRoutingMenuMap.end())
        return;
        
      if(gIsOutRoutingPopupMenu)
      {  
        Route srcRoute(t, imm->second.channel, imm->second.channels);
        srcRoute.remoteChannel = imm->second.remoteChannel;
        
        Route &dstRoute = imm->second;

        // check if route src->dst exists:
        iRoute irl = rl->begin();
        for (; irl != rl->end(); ++irl) {
              if (*irl == dstRoute)
                    break;
              }
        if (irl != rl->end()) {
              // disconnect if route exists
              audio->msgRemoveRoute(srcRoute, dstRoute);
              }
        else {
              // connect if route does not exist
              audio->msgAddRoute(srcRoute, dstRoute);
              }
        audio->msgUpdateSoloStates();
        song->update(SC_ROUTE);
      }  
      else
      {
        Route &srcRoute = imm->second;
        
        // Support Midi Port to Audio Input routes. p4.0.14 Tim.
        if(track->type() == Track::AUDIO_INPUT && srcRoute.type == Route::MIDI_PORT_ROUTE)
        {
          int chbit = srcRoute.channel;
          Route dstRoute(t, chbit);
          int mdidx = srcRoute.midiPort;
          int chmask = 0;                   
          iRoute iir = rl->begin();
          for (; iir != rl->end(); ++iir) 
          {
            if(iir->type == Route::MIDI_PORT_ROUTE && iir->midiPort == mdidx)    // Is there already a route to this port?
            {
              chmask = iir->channel;  // Grab the channel mask.
              break;
            }      
          }
          
          if ((chmask & chbit) == chbit)             // Is the channel's bit(s) set?
          {
            //printf("astrip: removing src route ch:%d dst route ch:%d\n", srcRoute.channel, dstRoute.channel); 
            audio->msgRemoveRoute(srcRoute, dstRoute);
          }
          else 
          {
            //printf("astrip: adding src route ch:%d dst route ch:%d\n", srcRoute.channel, dstRoute.channel); 
            audio->msgAddRoute(srcRoute, dstRoute);
          }
          
          audio->msgUpdateSoloStates();
          song->update(SC_ROUTE);
          return;
        }
        
        Route dstRoute(t, imm->second.channel, imm->second.channels);
        dstRoute.remoteChannel = imm->second.remoteChannel;

        iRoute irl = rl->begin();
        for (; irl != rl->end(); ++irl) {
              if (*irl == srcRoute)
                    break;
              }
        if (irl != rl->end()) {
              // disconnect
              audio->msgRemoveRoute(srcRoute, dstRoute);
              }
        else {
              // connect
              audio->msgAddRoute(srcRoute, dstRoute);
              }
        audio->msgUpdateSoloStates();
        song->update(SC_ROUTE);
      }
}

//---------------------------------------------------------
//   iRoutePressed
//---------------------------------------------------------

void AudioStrip::iRoutePressed()
      {
      //if(track->isMidiTrack() || (track->type() == Track::AUDIO_AUX) || (track->type() == Track::AUDIO_SOFTSYNTH))
      if(!track || track->isMidiTrack() || track->type() == Track::AUDIO_AUX)
      {
        gRoutingPopupMenuMaster = 0;
        return;
      }
        
      QPoint ppt = QCursor::pos();
      
      PopupMenu* pup = muse->getRoutingPopupMenu();
      pup->disconnect();
      
      AudioTrack* t = (AudioTrack*)track;
      RouteList* irl = t->inRoutes();

      QAction* act = 0;
      int gid = 0;
      //int id = 0;
      
      pup->clear();
      gRoutingMenuMap.clear();
      gid = 0;
      
      switch(track->type()) 
      {
        case Track::AUDIO_INPUT:
        {
          for(int i = 0; i < channel; ++i) 
          {
            char buffer[128];
            snprintf(buffer, 128, "%s %d", tr("Channel").toLatin1().constData(), i+1);
            MenuTitleItem* titel = new MenuTitleItem(QString(buffer), pup);
            pup->addAction(titel); 
  
            if(!checkAudioDevice())
            { 
              gRoutingPopupMenuMaster = 0;
              pup->clear();
              gRoutingMenuMap.clear();
              iR->setDown(false);     
              return;
            }
            std::list<QString> ol = audioDevice->outputPorts();
            for(std::list<QString>::iterator ip = ol.begin(); ip != ol.end(); ++ip) 
            {
              //id = gid * 16 + i;        // IDs removed p4.0.14 Tim.
              act = pup->addAction(*ip);
              //act->setData(id);
              act->setData(gid);
              act->setCheckable(true);
              
              Route dst(*ip, true, i, Route::JACK_ROUTE);
              //gRoutingMenuMap.insert( pRouteMenuMap(id, dst) );
              gRoutingMenuMap.insert( pRouteMenuMap(gid, dst) );
              ++gid;
              for(iRoute ir = irl->begin(); ir != irl->end(); ++ir) 
              {
                if(*ir == dst) 
                {
                  act->setChecked(true);
                  break;
                }
              }
            }
            if(i+1 != channel)
              pup->addSeparator();
          }
          
          // p4.0.14
          //
          // Display using separate menus for midi ports and audio outputs:
          //
          pup->addSeparator();
          pup->addAction(new MenuTitleItem(tr("Soloing chain"), pup)); 
          PopupMenu* subp = new PopupMenu(pup);
          subp->setTitle(tr("Audio sends")); 
          pup->addMenu(subp);
          gid = addOutPorts(t, subp, gid, gRoutingMenuMap, -1, -1, false);  
          subp = new PopupMenu(pup);
          subp->setTitle(tr("Midi port sends")); 
          pup->addMenu(subp);
          addMidiPorts(t, subp, gid, gRoutingMenuMap, false);
          //
          // Display all in the same menu:
          //
          //pup->addAction(new MenuTitleItem(tr("Audio sends"), pup)); 
          //gid = addOutPorts(t, pup, gid, gRoutingMenuMap, -1, -1, false);  
          //pup->addSeparator();
          //pup->addAction(new MenuTitleItem(tr("Midi sends"), pup)); 
          //addMidiPorts(t, pup, gid, gRoutingMenuMap, false);
        }
        break;
        //case Track::AUDIO_OUTPUT:
        //case Track::WAVE:
        //case Track::AUDIO_GROUP:
        
        case Track::AUDIO_OUTPUT:
              gid = addWavePorts( t, pup, gid, gRoutingMenuMap, -1, -1, false);
              gid = addInPorts(   t, pup, gid, gRoutingMenuMap, -1, -1, false);
              gid = addGroupPorts(t, pup, gid, gRoutingMenuMap, -1, -1, false);
              gid = addAuxPorts(  t, pup, gid, gRoutingMenuMap, -1, -1, false);
              gid = nonSyntiTrackAddSyntis(t, pup, gid, gRoutingMenuMap, false);
              break;
        case Track::WAVE:
              gid = addInPorts(   t, pup, gid, gRoutingMenuMap, -1, -1, false);
              break;
        case Track::AUDIO_GROUP:
              gid = addWavePorts( t, pup, gid, gRoutingMenuMap, -1, -1, false);
              gid = addInPorts(   t, pup, gid, gRoutingMenuMap, -1, -1, false);
              gid = addGroupPorts(t, pup, gid, gRoutingMenuMap, -1, -1, false);
              gid = nonSyntiTrackAddSyntis(t, pup, gid, gRoutingMenuMap, false);
              break;
        
        case Track::AUDIO_SOFTSYNTH:
              gid = addMultiChannelPorts(t, pup, gid, gRoutingMenuMap, false);
              break;
        default:
              gRoutingPopupMenuMaster = 0;
              pup->clear();
              gRoutingMenuMap.clear();
              iR->setDown(false);     
              return;
      }  
      
      if(pup->actions().isEmpty())
      {
        gRoutingPopupMenuMaster = 0;
        gRoutingMenuMap.clear();
        iR->setDown(false);     
        return;
      }
      
      gIsOutRoutingPopupMenu = false;
      gRoutingPopupMenuMaster = this;
      connect(pup, SIGNAL(triggered(QAction*)), SLOT(routingPopupMenuActivated(QAction*)));
      connect(pup, SIGNAL(aboutToHide()), muse, SLOT(routingPopupMenuAboutToHide()));
      pup->popup(ppt);
      iR->setDown(false);     
      }
      
//---------------------------------------------------------
//   oRoutePressed
//---------------------------------------------------------

void AudioStrip::oRoutePressed()
{
      if(!track || track->isMidiTrack())
      {
        gRoutingPopupMenuMaster = 0;
        return;
      }
        
      QPoint ppt = QCursor::pos();
      
      PopupMenu* pup = muse->getRoutingPopupMenu();
      pup->disconnect();
      
      AudioTrack* t = (AudioTrack*)track;
      RouteList* orl = t->outRoutes();

      QAction* act = 0;
      int gid = 0;
      //int id = 0;
      
      pup->clear();
      gRoutingMenuMap.clear();
      gid = 0;
      
      switch(track->type()) 
      {
        case Track::AUDIO_OUTPUT:
        {
          for(int i = 0; i < channel; ++i) 
          {
            char buffer[128];
            snprintf(buffer, 128, "%s %d", tr("Channel").toLatin1().constData(), i+1);
            MenuTitleItem* titel = new MenuTitleItem(QString(buffer), pup);
            pup->addAction(titel); 
  
            if(!checkAudioDevice())
            { 
              gRoutingPopupMenuMaster = 0;
              pup->clear();
              gRoutingMenuMap.clear();
              oR->setDown(false);     
              return;
            }
            std::list<QString> ol = audioDevice->inputPorts();
            for(std::list<QString>::iterator ip = ol.begin(); ip != ol.end(); ++ip) 
            {
              //id = gid * 16 + i;        // IDs removed p4.0.14 Tim.
              act = pup->addAction(*ip);
              //act->setData(id);
              act->setData(gid);
              act->setCheckable(true);
              
              Route dst(*ip, true, i, Route::JACK_ROUTE);
              //gRoutingMenuMap.insert( pRouteMenuMap(id, dst) );
              gRoutingMenuMap.insert( pRouteMenuMap(gid, dst) );
              ++gid;
              for(iRoute ir = orl->begin(); ir != orl->end(); ++ir) 
              {
                if(*ir == dst) 
                {
                  act->setChecked(true);
                  break;
                }
              }
            }
            if(i+1 != channel)
              pup->addSeparator();
          }      
          
          // p4.0.14
          //
          // Display using separate menu for audio inputs:
          //
          pup->addSeparator();
          pup->addAction(new MenuTitleItem(tr("Soloing chain"), pup)); 
          PopupMenu* subp = new PopupMenu(pup);
          subp->setTitle(tr("Audio returns")); 
          pup->addMenu(subp);
          gid = addInPorts(t, subp, gid, gRoutingMenuMap, -1, -1, true);  
          //
          // Display all in the same menu:
          //
          //pup->addSeparator();
          //MenuTitleItem* title = new MenuTitleItem(tr("Audio returns"), pup);
          //pup->addAction(title); 
          //gid = addInPorts(t, pup, gid, gRoutingMenuMap, -1, -1, true);  
        }
        break;
        //case Track::AUDIO_INPUT:
        //case Track::WAVE:
        //case Track::AUDIO_GROUP:

        case Track::AUDIO_SOFTSYNTH:
              gid = addMultiChannelPorts(t, pup, gid, gRoutingMenuMap, true);
        break;
        
        case Track::AUDIO_INPUT:
              gid = addWavePorts(        t, pup, gid, gRoutingMenuMap, -1, -1, true);
        case Track::WAVE:
        case Track::AUDIO_GROUP:
        case Track::AUDIO_AUX:
        //case Track::AUDIO_SOFTSYNTH:
              gid = addOutPorts(         t, pup, gid, gRoutingMenuMap, -1, -1, true);
              gid = addGroupPorts(       t, pup, gid, gRoutingMenuMap, -1, -1, true);
              gid = nonSyntiTrackAddSyntis(t, pup, gid, gRoutingMenuMap, true);
        break;
        //case Track::AUDIO_AUX:
        //      gid = addOutPorts(         t, pup, gid, gRoutingMenuMap, -1, -1, true);
        //break;
        
        default:
              gRoutingPopupMenuMaster = 0;
              pup->clear();
              gRoutingMenuMap.clear();
              oR->setDown(false);     
              return;
      }
      
      if(pup->actions().isEmpty())
      {
        gRoutingPopupMenuMaster = 0;
        gRoutingMenuMap.clear();
        oR->setDown(false);     
        return;
      }
      
      gIsOutRoutingPopupMenu = true;
      gRoutingPopupMenuMaster = this;
      connect(pup, SIGNAL(triggered(QAction*)), SLOT(routingPopupMenuActivated(QAction*)));
      connect(pup, SIGNAL(aboutToHide()), muse, SLOT(routingPopupMenuAboutToHide()));
      pup->popup(ppt);
      oR->setDown(false);     
}

