//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: astrip.cpp,v 1.23.2.17 2009/11/16 01:55:55 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <fastlog.h>

#include <qlayout.h>
#include <qapplication.h>
#include <qdialog.h>
#include <qtoolbutton.h>
#include <qlabel.h>
#include <qdialog.h>
#include <qcombobox.h>
#include <qtooltip.h>
#include <qtimer.h>
#include <qpopupmenu.h>
#include <qcursor.h>
#include <qmenudata.h>
#include <qpainter.h>
#include <qstring.h>

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
#include "route.h"
#include "doublelabel.h"
#include "rack.h"
#include "node.h"
#include "amixer.h"
#include "icons.h"
#include "gconfig.h"
#include "ttoolbutton.h"
#include "menutitleitem.h"

/*
//---------------------------------------------------------
//   MenuTitleItem
//---------------------------------------------------------

class MenuTitleItem : public QCustomMenuItem {
      QString s;
      virtual bool fullSpan() const    { return true; }
      virtual bool isSeparator() const { return true; }
      virtual void paint(QPainter* p, const QColorGroup& cg, bool act,
         bool, int, int, int, int);
      virtual QSize sizeHint();

   public:
      MenuTitleItem(QString s);
      };
*/

//---------------------------------------------------------
//   MenuTitleItem
//---------------------------------------------------------

MenuTitleItem::MenuTitleItem(QString ss)
  : s(ss)
      {
      }

QSize MenuTitleItem::sizeHint()
      {
      return QSize(60, 20);
      }

//---------------------------------------------------------
//   drawItem
//---------------------------------------------------------

void MenuTitleItem::paint(QPainter* p, const QColorGroup&, bool,
   bool, int x, int y, int w, int h)
      {
      p->fillRect(x, y, w, h, QBrush(lightGray));
      p->drawText(x, y, w, h, AlignCenter, s);
      }

//---------------------------------------------------------
//   minimumSizeHint
//---------------------------------------------------------

QSize AudioStrip::minimumSizeHint () const
{
    // We force the width of the size hint to be what we want
    //return QWidget::minimumSizeHint();
    return QSize(66,QWidget::minimumSizeHint().height());
}

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
      if(val == SC_MIDI_CONTROLLER)
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
            mute->setOn(src->mute());
            mute->blockSignals(false);
            updateOffState();
            }
      if (solo && (val & SC_SOLO)) {
            if((bool)track->internalSolo())
            {
              if(!useSoloIconSet2)
              {
                solo->setIconSet(*soloIconSet2);
                useSoloIconSet2 = true;
              }  
            }  
            else if(useSoloIconSet2)
            {
              solo->setIconSet(*soloIconSet1);
              useSoloIconSet2 = false;
            }  
            
            solo->blockSignals(true);
            solo->setOn(track->solo());
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
                  pre->setOn(src->prefader());
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
              autoType->setPaletteBackgroundColor(red);
            else  
              autoType->setPaletteBackgroundColor(qApp->palette().active().background());
      
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
            // Added by Tim. p3.3.6
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
            // Added by Tim. p3.3.6
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
            off->setOn(track->off());
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
      // Added by Tim. p3.3.6
      //printf("AudioStrip::updateChannels track channels:%d current channels:%d\n", c, channel);
      
      if (c > channel) {
            for (int cc = channel; cc < c; ++cc) {
                  meter[cc] = new Meter(this);
                  //meter[cc]->setRange(config.minSlider, 10.0);
                  meter[cc]->setRange(config.minMeter, 10.0);
                  meter[cc]->setFixedWidth(15);
                  connect(meter[cc], SIGNAL(mousePress()), this, SLOT(resetPeaks()));
                  sliderGrid->addWidget(meter[cc], 0, cc+1, AlignHCenter);
                  sliderGrid->setColStretch(cc, 50);
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
      stereo->setOn(channel == 2);
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
      knob->setFixedWidth(STRIP_WIDTH/2);
      if (type == 0)
            knob->setRange(-1.0, +1.0);
      else
            knob->setRange(config.minSlider-0.1, 10.0);
      knob->setBackgroundMode(PaletteMid);

      if (type == 0)
            QToolTip::add(knob, tr("panorama"));
      else
            QToolTip::add(knob, tr("aux send level"));


      DoubleLabel* pl;
      if (type == 0)
            pl = new DoubleLabel(0, -1.0, +1.0, this);
      else
            pl = new DoubleLabel(0.0, config.minSlider, 10.1, this);
            
      if (dlabel)
            *dlabel = pl;
      pl->setSlider(knob);
      pl->setFont(config.fonts[1]);
      pl->setBackgroundMode(PaletteMid);
      pl->setFrame(true);
      if (type == 0)
            pl->setPrecision(2);
      else {
            pl->setPrecision(0);
            pl->setPrecision(0);
            }
      pl->setFixedWidth(STRIP_WIDTH/2);

      QString label;
      if (type == 0)
            label = tr("Pan");
      else
            label.sprintf("Aux%d", id+1);

      QLabel* plb = new QLabel(label, this);
      plb->setFont(config.fonts[1]);
      plb->setFixedWidth(STRIP_WIDTH/2);
      plb->setAlignment(AlignCenter);

      QGridLayout* pangrid = new QGridLayout(0, 2, 2, 0, 0, "pangrid");
      pangrid->addWidget(plb, 0, 0);
      pangrid->addWidget(pl, 1, 0);
      pangrid->addMultiCellWidget(knob, 0, 1, 1, 1);
      layout->addLayout(pangrid);

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
      iR            = 0;
      oR            = 0;
      off           = 0;
      
      volume        = -1.0;
      panVal        = 0;
      
      record        = 0;
      
      AudioTrack* t = (AudioTrack*)track;
      channel       = at->channels();
      setFixedWidth(STRIP_WIDTH);
      setMinimumWidth(STRIP_WIDTH);

      int ch = 0;
      for (; ch < channel; ++ch)
            meter[ch] = new Meter(this);
      for (; ch < MAX_CHANNELS; ++ch)
            meter[ch] = 0;

      //---------------------------------------------------
      //    plugin rack
      //---------------------------------------------------

      EffectRack* rack = new EffectRack(this, t);
      rack->setFixedWidth(STRIP_WIDTH);
      rack->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
      layout->addWidget(rack);

      //---------------------------------------------------
      //    mono/stereo  pre/post
      //---------------------------------------------------

      QHBoxLayout* ppBox = new QHBoxLayout(0);
      stereo  = new QToolButton(this);
      stereo->setFont(config.fonts[1]);
      QIconSet stereoSet;
      stereoSet.setPixmap(*monoIcon,   QIconSet::Automatic, QIconSet::Normal, QIconSet::Off);
      stereoSet.setPixmap(*stereoIcon, QIconSet::Automatic, QIconSet::Normal, QIconSet::On);
      stereo->setIconSet(stereoSet);

      stereo->setToggleButton(true);
      QToolTip::add(stereo, tr("1/2 channel"));
      stereo->setOn(channel == 2);
      stereo->setFixedWidth(STRIP_WIDTH/2);
      connect(stereo, SIGNAL(toggled(bool)), SLOT(stereoToggled(bool)));

      // disable mono/stereo for Synthesizer-Plugins
      if (t->type() == Track::AUDIO_SOFTSYNTH)
            stereo->setEnabled(false);

      pre = new QToolButton(this);
      pre->setFont(config.fonts[1]);
      pre->setToggleButton(true);
      pre->setText(tr("Pre"));
      QToolTip::add(pre, tr("pre fader - post fader"));
      pre->setOn(t->prefader());
      pre->setFixedWidth(STRIP_WIDTH/2);
      connect(pre, SIGNAL(toggled(bool)), SLOT(preToggled(bool)));

      ppBox->addWidget(stereo);
      ppBox->addWidget(pre);
      layout->addLayout(ppBox);

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
            if (auxsSize)
                  layout->addSpacing((STRIP_WIDTH/2 + 2) * auxsSize);
            }

      //---------------------------------------------------
      //    slider, label, meter
      //---------------------------------------------------

      sliderGrid = new QGridLayout;
      sliderGrid->setRowStretch(0, 100);

      //slider = new Slider(this);
      slider = new Slider(this, "vol", Slider::Vertical, Slider::None,
         Slider::BgTrough | Slider::BgSlot);
      slider->setCursorHoming(true);
      slider->setRange(config.minSlider-0.1, 10.0);
      slider->setFixedWidth(20);
      slider->setFont(config.fonts[1]);
      slider->setValue(fast_log10(t->volume())*20.0);

      sliderGrid->addWidget(slider, 0, 0, AlignHCenter);

      for (int i = 0; i < channel; ++i) {
            //meter[i]->setRange(config.minSlider, 10.0);
            meter[i]->setRange(config.minMeter, 10.0);
            meter[i]->setFixedWidth(15);
            connect(meter[i], SIGNAL(mousePress()), this, SLOT(resetPeaks()));
            sliderGrid->addWidget(meter[i], 0, i+1, AlignHCenter);
            sliderGrid->setColStretch(i, 50);
            }
      layout->addLayout(sliderGrid);

      sl = new DoubleLabel(0.0, config.minSlider, 10.0, this);
      sl->setSlider(slider);
      sl->setFont(config.fonts[1]);
      sl->setBackgroundMode(PaletteMid);
      sl->setSuffix(tr("dB"));
      sl->setFrame(true);
      sl->setPrecision(0);
      sl->setFixedWidth(STRIP_WIDTH);
      sl->setValue(fast_log10(t->volume()) * 20.0);

      connect(sl, SIGNAL(valueChanged(double,int)), SLOT(volLabelChanged(double)));
      //connect(sl, SIGNAL(valueChanged(double,int)), SLOT(volumeChanged(double)));
      connect(slider, SIGNAL(valueChanged(double,int)), sl, SLOT(setValue(double)));
      connect(slider, SIGNAL(sliderMoved(double,int)), SLOT(volumeChanged(double)));
      connect(slider, SIGNAL(sliderPressed(int)), SLOT(volumePressed()));
      connect(slider, SIGNAL(sliderReleased(int)), SLOT(volumeReleased()));
      connect(slider, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(volumeRightClicked(const QPoint &)));
      layout->addWidget(sl);

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
            record->setToggleButton(true);
            record->setFixedWidth(STRIP_WIDTH/2);
            record->setBackgroundMode(PaletteMid);
            QIconSet iconSet;
            iconSet.setPixmap(*record_on_Icon, QIconSet::Automatic, QIconSet::Normal, QIconSet::On);
            iconSet.setPixmap(*record_off_Icon, QIconSet::Automatic, QIconSet::Normal, QIconSet::Off);
            record->setIconSet(iconSet);
            QToolTip::add(record, tr("record"));
            record->setOn(t->recordFlag());
            connect(record, SIGNAL(toggled(bool)), SLOT(recordToggled(bool)));
            }

      Track::TrackType type = t->type();

      QHBoxLayout* smBox1 = new QHBoxLayout(0);
      QHBoxLayout* smBox2 = new QHBoxLayout(0);

      mute  = new QToolButton(this);
      
      QIconSet muteSet;
      muteSet.setPixmap(*muteIconOn,   QIconSet::Automatic, QIconSet::Normal, QIconSet::Off);
      muteSet.setPixmap(*muteIconOff, QIconSet::Automatic, QIconSet::Normal, QIconSet::On);
      mute->setIconSet(muteSet);
      mute->setToggleButton(true);
      QToolTip::add(mute, tr("mute"));
      mute->setOn(t->mute());
      mute->setFixedWidth(STRIP_WIDTH/2-2);
      connect(mute, SIGNAL(toggled(bool)), SLOT(muteToggled(bool)));
      smBox2->addWidget(mute);

      solo  = new QToolButton(this);
      
      if((bool)t->internalSolo())
      {
        solo->setIconSet(*soloIconSet2);
        useSoloIconSet2 = true;
      }  
      else  
      {
        solo->setIconSet(*soloIconSet1);
        useSoloIconSet2 = false;
      }  
              
      solo->setToggleButton(true);
      solo->setOn(t->solo());
      
      solo->setFixedWidth(STRIP_WIDTH/2-2);
      smBox2->addWidget(solo);
      connect(solo, SIGNAL(toggled(bool)), SLOT(soloToggled(bool)));
      if (type == Track::AUDIO_OUTPUT) {
            QToolTip::add(record, tr("record downmix"));
            //QToolTip::add(solo, tr("solo mode (monitor)"));
            QToolTip::add(solo, tr("solo mode"));
            }
      else {
            //QToolTip::add(solo, tr("pre fader listening"));
            QToolTip::add(solo, tr("solo mode"));
            }

      off  = new TransparentToolButton(this);
      QIconSet iconSet;
      iconSet.setPixmap(*exit1Icon, QIconSet::Automatic, QIconSet::Normal, QIconSet::On);
      iconSet.setPixmap(*exitIcon, QIconSet::Automatic, QIconSet::Normal, QIconSet::Off);
      off->setIconSet(iconSet);
      off->setBackgroundMode(PaletteMid);
      off->setFixedWidth(STRIP_WIDTH/2);
      off->setToggleButton(true);
      QToolTip::add(off, tr("off"));
      off->setOn(t->off());
      connect(off, SIGNAL(toggled(bool)), SLOT(offToggled(bool)));

      smBox1->addWidget(off);
      if (track->canRecord())
            smBox1->addWidget(record);
      else
            smBox1->addStretch(100);

      layout->addLayout(smBox1);
      layout->addLayout(smBox2);

      //---------------------------------------------------
      //    routing
      //---------------------------------------------------

      QHBoxLayout* rBox = new QHBoxLayout(0);
      if (type != Track::AUDIO_AUX) {
            iR = new QToolButton(this);
            iR->setFont(config.fonts[1]);
            iR->setFixedWidth((STRIP_WIDTH-4)/2);
            iR->setText(tr("iR"));
            iR->setToggleButton(false);
            QToolTip::add(iR, tr("input routing"));
            rBox->addWidget(iR);
            connect(iR, SIGNAL(pressed()), SLOT(iRoutePressed()));
            }
      else
            rBox->addSpacing((STRIP_WIDTH-4)/2);
      oR = new QToolButton(this);
      oR->setFont(config.fonts[1]);
      oR->setFixedWidth((STRIP_WIDTH-4)/2);
      oR->setText(tr("oR"));
      oR->setToggleButton(false);
      QToolTip::add(oR, tr("output routing"));
      rBox->addWidget(oR);
      connect(oR, SIGNAL(pressed()), SLOT(oRoutePressed()));

      layout->addLayout(rBox);

      //---------------------------------------------------
      //    automation type
      //---------------------------------------------------

      autoType = new ComboBox(this);
      autoType->setFont(config.fonts[1]);
      autoType->setFixedWidth(STRIP_WIDTH-4);
      autoType->insertItem(tr("Off"), AUTO_OFF);
      autoType->insertItem(tr("Read"), AUTO_READ);
      autoType->insertItem(tr("Touch"), AUTO_TOUCH);
      autoType->insertItem(tr("Write"), AUTO_WRITE);
      autoType->setCurrentItem(t->automationType());
      
      if(t->automationType() == AUTO_TOUCH || t->automationType() == AUTO_WRITE)
        autoType->setPaletteBackgroundColor(red);
      else  
        autoType->setPaletteBackgroundColor(qApp->palette().active().background());
      
      QToolTip::add(autoType, tr("automation type"));
      connect(autoType, SIGNAL(activated(int,int)), SLOT(setAutomationType(int,int)));
      layout->addWidget(autoType);

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

static int addMenuItem(QButton* /*parent*/, AudioTrack* track, Track* route_track, QPopupMenu* lb, int id, RouteMenuMap& mm, int channel, int channels, bool isOutput)
{
  // totalInChannels is only used by syntis.
  //int channels = (!isOutput || route_track->type() != Track::AUDIO_SOFTSYNTH) ? ((AudioTrack*)route_track)->totalOutChannels() : ((AudioTrack*)route_track)->totalInChannels();
  //int channels = (!isOutput || track->type() != Track::AUDIO_SOFTSYNTH) ? ((AudioTrack*)track)->totalOutChannels() : ((AudioTrack*)track)->totalInChannels();
  int toch = ((AudioTrack*)track)->totalOutChannels();
  // If track channels = 1, it must be a mono synth. And synti channels cannot be changed by user.
  if(track->channels() == 1)
    toch = 1;
  
  // totalInChannels is only used by syntis.
  int chans = (isOutput || track->type() != Track::AUDIO_SOFTSYNTH) ? toch : ((AudioTrack*)track)->totalInChannels();
  
  // Don't add the last stray mono route if the track is stereo.
  //if(route_track->channels() > 1 && (channel+1 == chans))
  //  return id;
    
  RouteList* rl = isOutput ? track->outRoutes() : track->inRoutes();
  
  QString s(route_track->name());
  //int trackchans = track->channels();
  //QString ns;
  
  //if(track->channels() > 1 && (channel+1 < channels))
  //if(track->channels() > 1)
  //if(route_track->type() == Track::AUDIO_SOFTSYNTH && channels > 2 && track->channels() > 1)
  ///if(track->type() == Track::AUDIO_SOFTSYNTH && chans > 2 && route_track->channels() > 1)
  ///  s += QString(" < [%1,%2]").arg(channel+1).arg(channel+2);
  
  //int it = lb->insertItem(s);
  lb->insertItem(s, id);
  
  int ach = channel;
  int bch = -1;
  
  Route r(route_track, isOutput ? ach : bch, channels);
  //Route r(route_track, channel);
  
  r.remoteChannel = isOutput ? bch : ach;
  
  mm.insert( pRouteMenuMap(id, r) );
  
  for(iRoute ir = rl->begin(); ir != rl->end(); ++ir) 
  {
    //if (ir->type == 0 && ir->track == track) {
    //if(ir->type == Route::TRACK_ROUTE && ir->track == track && ir->channels == channels) 
    //if(ir->type == Route::TRACK_ROUTE && ir->track == track && 
    //   (channel != -1 && ir->channel == channel) && (channels != -1 && ir->channels == channels)) 
    //if(ir->type == Route::TRACK_ROUTE && ir->track == track && ir->channel == channel) 
    //printf("addMenuItem: ir->type:%d ir->track:%s track:%s ir->channel:%d channel:%d ir->channels:%d channels:%d\n",
    //       ir->type, ir->track->name().latin1(), track->name().latin1(), ir->channel, channel, ir->channels, channels);
    //if(ir->type == Route::TRACK_ROUTE && ir->track == route_track && ir->channel == channel && ir->remoteChannel == r.remoteChannel) 
    //if(*ir == r)
    //if(ir->type == Route::TRACK_ROUTE && ir->track == route_track && ir->channel == channel && ir->channels == channels && ir->remoteChannel == r.remoteChannel) 
    
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
      
      //if(ir->type == Route::TRACK_ROUTE && ir->track == route_track && ir->channel == r.channel && ir->channels == r.channels && ir->remoteChannel == r.remoteChannel) 
      if(compch == tcompch && compchs == tcompchs) 
      {
        //lb->setItemChecked(it, true);
        lb->setItemChecked(id, true);
        break;
      }
    }  
  }
  return ++id;      
}

//---------------------------------------------------------
//   addAuxPorts
//---------------------------------------------------------

//static void addAuxPorts(AudioTrack* t, QPopupMenu* lb, RouteList* r)
static int addAuxPorts(QButton* parent, AudioTrack* t, QPopupMenu* lb, int id, RouteMenuMap& mm, int channel, int channels, bool isOutput)
      {
      AuxList* al = song->auxs();
      for (iAudioAux i = al->begin(); i != al->end(); ++i) {
            Track* track = *i;
            if (t == track)
                  continue;
            id = addMenuItem(parent, t, track, lb, id, mm, channel, channels, isOutput);
            
            /*
            QString s(track->name());
            //int it = lb->insertItem(s);
            lb->insertItem(s, id);
            for (iRoute ir = r->begin(); ir != r->end(); ++ir) {
                  //if (ir->type == 0 && ir->track == track) {
                  if (ir->type == 0 && ir->track == track && ir->channels == channels) {
                        //lb->setItemChecked(it, true);
                        lb->setItemChecked(id, true);
                        break;
                        }
                  }
            ++id;      
            */      
            
            }
      return id;      
      }

//---------------------------------------------------------
//   addInPorts
//---------------------------------------------------------

//static void addInPorts(AudioTrack* t, QPopupMenu* lb, RouteList* r)
static int addInPorts(QButton* parent, AudioTrack* t, QPopupMenu* lb, int id, RouteMenuMap& mm, int channel, int channels, bool isOutput)
      {
      InputList* al = song->inputs();
      for (iAudioInput i = al->begin(); i != al->end(); ++i) {
            Track* track = *i;
            if (t == track)
                  continue;
            id = addMenuItem(parent, t, track, lb, id, mm, channel, channels, isOutput);
            
            /*
            QString s(track->name());
            //int it = lb->insertItem(s);
            lb->insertItem(s, id);
            for (iRoute ir = r->begin(); ir != r->end(); ++ir) {
                  //if (ir->type == 0 && ir->track == track) {
                  if (ir->type == 0 && ir->track == track && ir->channels == channels) {
                        //lb->setItemChecked(it, true);
                        lb->setItemChecked(id, true);
                        break;
                        }
                  }
            ++id;      
            */
            
            }
      return id;      
      }

//---------------------------------------------------------
//   addOutPorts
//---------------------------------------------------------

//static void addOutPorts(AudioTrack* t, QPopupMenu* lb, RouteList* r)
static int addOutPorts(QButton* parent, AudioTrack* t, QPopupMenu* lb, int id, RouteMenuMap& mm, int channel, int channels, bool isOutput)
      {
      OutputList* al = song->outputs();
      for (iAudioOutput i = al->begin(); i != al->end(); ++i) {
            Track* track = *i;
            if (t == track)
                  continue;
            id = addMenuItem(parent, t, track, lb, id, mm, channel, channels, isOutput);
            
            /*
            QString s(track->name());
            //int it = lb->insertItem(s);
            lb->insertItem(s, id);
            for (iRoute ir = r->begin(); ir != r->end(); ++ir) {
                  //if (ir->type == 0 && ir->track == track) {
                  if (ir->type == 0 && ir->track == track && ir->channels == channels) {
                        //lb->setItemChecked(it, true);
                        lb->setItemChecked(id, true);
                        break;
                        }
                  }
            ++id;      
            */
            
            }
      return id;      
      }

//---------------------------------------------------------
//   addGroupPorts
//---------------------------------------------------------

//static void addGroupPorts(AudioTrack* t, QPopupMenu* lb, RouteList* r)
static int addGroupPorts(QButton* parent, AudioTrack* t, QPopupMenu* lb, int id, RouteMenuMap& mm, int channel, int channels, bool isOutput)
      {
      GroupList* al = song->groups();
      for (iAudioGroup i = al->begin(); i != al->end(); ++i) {
            Track* track = *i;
            if (t == track)
                  continue;
            id = addMenuItem(parent, t, track, lb, id, mm, channel, channels, isOutput);
            
            /*
            QString s(track->name());
            //int it = lb->insertItem(s);
            lb->insertItem(s, id);
            for (iRoute ir = r->begin(); ir != r->end(); ++ir) {
                  //if (ir->type == 0 && ir->track == track) {
                  if (ir->type == 0 && ir->track == track && ir->channels == channels) {
                        //lb->setItemChecked(it, true);
                        lb->setItemChecked(id, true);
                        break;
                        }
                  }
            ++id;      
            */
            
            }
      return id;      
      }

//---------------------------------------------------------
//   addWavePorts
//---------------------------------------------------------

//static void addWavePorts(AudioTrack* t, QPopupMenu* lb, RouteList* r)
static int addWavePorts(QButton* parent, AudioTrack* t, QPopupMenu* lb, int id, RouteMenuMap& mm, int channel, int channels, bool isOutput)
      {
      WaveTrackList* al = song->waves();
      for (iWaveTrack i = al->begin(); i != al->end(); ++i) {
            Track* track = *i;
            if (t == track)
                  continue;
            id = addMenuItem(parent, t, track, lb, id, mm, channel, channels, isOutput);
            
            /*
            QString s(track->name());
            //int it = lb->insertItem(s);
            lb->insertItem(s, id);
            for (iRoute ir = r->begin(); ir != r->end(); ++ir) {
                  //if (ir->type == 0 && ir->track == track) {
                  if (ir->type == 0 && ir->track == track && ir->channels == channels) {
                        //lb->setItemChecked(it, true);
                        lb->setItemChecked(id, true);
                        break;
                        }
                  }
            ++id;      
            */
            
            }
      return id;      
      }

//---------------------------------------------------------
//   addSyntiPorts
//---------------------------------------------------------

//static void addSyntiPorts(AudioTrack* t, QPopupMenu* lb, RouteList* r)
static int addSyntiPorts(QButton* parent, AudioTrack* t, QPopupMenu* lb, int id, RouteMenuMap& mm, int channel, int channels, bool isOutput)
{
      RouteList* rl = isOutput ? t->outRoutes() : t->inRoutes();
      
      SynthIList* al = song->syntis();
      for (iSynthI i = al->begin(); i != al->end(); ++i) 
      {
            Track* track = *i;
            if (t == track)
                  continue;
            //id = addMenuItem(parent, track, lb, r, id, mm, channel, channels);
            
            /*
            QString s(track->name());
            //int it = lb->insertItem(s);
            lb->insertItem(s, id);
            for (iRoute ir = r->begin(); ir != r->end(); ++ir) {
                  //if (ir->type == 0 && ir->track == track) {
                  if (ir->type == 0 && ir->track == track && ir->channels == channels) {
                        //lb->setItemChecked(it, true);
                        lb->setItemChecked(id, true);
                        break;
                        }
                  }
            ++id;      
            */
            
            //SynthI* synti = (SynthI*)track;
            
            int toch = ((AudioTrack*)track)->totalOutChannels();
            // If track channels = 1, it must be a mono synth. And synti channels cannot be changed by user.
            if(track->channels() == 1)
              toch = 1;
            
            //int chans = synti->totalOutChannels();
            //int chans = (!isOutput || track->type() != Track::AUDIO_SOFTSYNTH) ? ((AudioTrack*)track)->totalOutChannels() : ((AudioTrack*)track)->totalInChannels();
            // totalInChannels is only used by syntis.
            int chans = (!isOutput || track->type() != Track::AUDIO_SOFTSYNTH) ? toch : ((AudioTrack*)track)->totalInChannels();
            
            //int schans = synti->channels();
            //if(schans < chans)
            //  chans = schans;
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
              QPopupMenu* chpup = new QPopupMenu(parent);
              chpup->setCheckable(true);
              for(int ch = 0; ch < chans; ++ch)
              {
                char buffer[128];
                if(tchans == 2)
                  snprintf(buffer, 128, "%s %d,%d", chpup->tr("Channel").latin1(), ch+1, ch+2);
                else  
                  snprintf(buffer, 128, "%s %d", chpup->tr("Channel").latin1(), ch+1);
                chpup->insertItem(QString(buffer), id);
                
                int ach = (channel == -1) ? ch : channel;
                int bch = (channel == -1) ? -1 : ch;
                
                Route rt(track, (t->type() != Track::AUDIO_SOFTSYNTH || isOutput) ? ach : bch, tchans);
                //Route rt(track, ch);
                //rt.remoteChannel = -1;
                rt.remoteChannel = (t->type() != Track::AUDIO_SOFTSYNTH || isOutput) ? bch : ach;
                
                mm.insert( pRouteMenuMap(id, rt) );
                
                for(iRoute ir = rl->begin(); ir != rl->end(); ++ir) 
                {
                  //if (ir->type == 0 && ir->track == track) {
                  //if(ir->type == 0 && ir->track == track && ir->channels == channels) 
                  //if(ir->type == Route::TRACK_ROUTE && ir->track == track && ir->channel == channel && 
                  //   ir->channels == channels && ir->remoteChannel == r.remoteChannel) 
                  //if(ir->type == Route::TRACK_ROUTE && ir->track == track && ir->channel == ch && 
                  //   ir->remoteChannel == rt.remoteChannel) 
                  //if(ir->type == Route::TRACK_ROUTE && ir->track == track && ir->channel == rt.channel && 
                  //   ir->channels == rt.channels && ir->remoteChannel == rt.remoteChannel) 
                  
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
                      chpup->setItemChecked(id, true);
                      break;
                    }
                  }
                }  
                ++id;
              }
            
              lb->insertItem(track->name(), chpup);
            }
      }
      return id;      
}

//---------------------------------------------------------
//   addMultiChannelOutPorts
//---------------------------------------------------------

static int addMultiChannelPorts(QButton* parent, AudioTrack* t, QPopupMenu* pup, int id, RouteMenuMap& mm, bool isOutput)
{
  int toch = t->totalOutChannels();
  // If track channels = 1, it must be a mono synth. And synti channels cannot be changed by user.
  if(t->channels() == 1)
    toch = 1;
  
  // Number of allocated buffers is always MAX_CHANNELS or more, even if _totalOutChannels is less. 
  //int chans = t->totalOutChannels();  
  // totalInChannels is only used by syntis.
  //int chans = isOutput ? t->totalOutChannels() : t->totalInChannels();
  //int chans = (isOutput || t->type() != Track::AUDIO_SOFTSYNTH) ? t->totalOutChannels() : t->totalInChannels();
  int chans = (isOutput || t->type() != Track::AUDIO_SOFTSYNTH) ? toch : t->totalInChannels();

  // If track channels = 1, it must be a mono synth. And synti channels cannot be changed by user.
  //if(t->channels() == 1)
  //  chans = 1;
  
  if(chans > 1)
  {
    pup->insertItem(new MenuTitleItem("<Mono>"));
    //pup->insertSeparator();
  }
  
  //
  // If it's more than one channel, create a sub-menu. If it's just one channel, don't bother with a sub-menu...
  //

  QPopupMenu* chpup = pup;
  
  for(int ch = 0; ch < chans; ++ch)
  {
    // If more than one channel, create the sub-menu.
    if(chans > 1)
    {
      chpup = new QPopupMenu(parent);
      chpup->setCheckable(true);
    }
    
    if(isOutput)
    {
      switch(t->type()) 
      {
        
        case Track::AUDIO_INPUT:
              id = addWavePorts(parent, t, chpup, id, mm, ch, 1, isOutput);
        case Track::WAVE:
        case Track::AUDIO_GROUP:
        case Track::AUDIO_SOFTSYNTH:
              id = addOutPorts(parent, t, chpup, id, mm, ch, 1, isOutput);
              id = addGroupPorts(parent, t, chpup, id, mm, ch, 1, isOutput);
              id = addSyntiPorts(parent, t, chpup, id, mm, ch, 1, isOutput);
              break;
        case Track::AUDIO_AUX:
              id = addOutPorts(parent, t, chpup, id, mm, ch, 1, isOutput);
              break;
        default:
              break;
        
        /*
        case Track::AUDIO_INPUT:
               id = addWavePorts(parent, t, chpup, id, mm, ch, isOutput);
        case Track::WAVE:
        case Track::AUDIO_GROUP:
        case Track::AUDIO_SOFTSYNTH:
              id = addOutPorts(parent, t, chpup, id, mm, ch, isOutput);
              id = addGroupPorts(parent, t, chpup, id, mm, ch, isOutput);
              id = addSyntiPorts(parent, t, chpup, id, mm, ch, isOutput);
              break;
        case Track::AUDIO_AUX:
              id = addOutPorts(parent, t, chpup, id, mm, ch, isOutput);
              break;
        default:
              break;
        */      
      }
    }
    else
    {
      switch(t->type()) 
      {
        
        case Track::AUDIO_OUTPUT:
              id = addWavePorts(parent, t, chpup, id, mm, ch, 1, isOutput);
              id = addInPorts(parent, t, chpup, id, mm, ch, 1, isOutput);
              id = addGroupPorts(parent, t, chpup, id, mm, ch, 1, isOutput);
              id = addAuxPorts(parent, t, chpup, id, mm, ch, 1, isOutput);
              id = addSyntiPorts(parent, t, chpup, id, mm, ch, 1, isOutput);
              break;
        case Track::WAVE:
              id = addInPorts(parent, t, chpup, id, mm, ch, 1, isOutput);
              break;
        case Track::AUDIO_SOFTSYNTH:
        case Track::AUDIO_GROUP:
              id = addWavePorts(parent, t, chpup, id, mm, ch, 1, isOutput);
              id = addInPorts(parent, t, chpup, id, mm, ch, 1, isOutput);
              id = addGroupPorts(parent, t, chpup, id, mm, ch, 1, isOutput);
              id = addSyntiPorts(parent, t, chpup, id, mm, ch, 1, isOutput);
              break;
        default:
              break;
        
        /*
        case Track::AUDIO_OUTPUT:
              id = addWavePorts(parent, t, chpup, id, mm, ch, isOutput);
              id = addInPorts(parent, t, chpup, id, mm, ch, isOutput);
              id = addGroupPorts(parent, t, chpup, id, mm, ch, isOutput);
              id = addAuxPorts(parent, t, chpup, id, mm, ch, isOutput);
              id = addSyntiPorts(parent, t, chpup, id, mm, ch, isOutput);
              break;
        case Track::WAVE:
              id = addInPorts(parent, t, chpup, id, mm, ch, isOutput);
              break;
        case Track::AUDIO_SOFTSYNTH:
        case Track::AUDIO_GROUP:
              id = addWavePorts(parent, t, chpup, id, mm, ch, isOutput);
              id = addInPorts(parent, t, chpup, id, mm, ch, isOutput);
              id = addGroupPorts(parent, t, chpup, id, mm, ch, isOutput);
              id = addSyntiPorts(parent, t, chpup, id, mm, ch, isOutput);
              break;
        default:
              break;
       */       
      }
    }
      
    // If more than one channel, add the created sub-menu.
    if(chans > 1)
    {
      char buffer[128];
      snprintf(buffer, 128, "%s %d", pup->tr("Channel").latin1(), ch+1);
      pup->insertItem(QString(buffer), chpup);
    }  
  } 
       
  // For stereo listing, ignore odd numbered left-over channels.
  chans -= 1;
  if(chans > 0)
  {
    // Ignore odd numbered left-over channels.
    //int schans = (chans & ~1) - 1;
    
    pup->insertSeparator();
    pup->insertItem(new MenuTitleItem("<Stereo>"));
    //pup->insertSeparator();
  
    //
    // If it's more than two channels, create a sub-menu. If it's just two channels, don't bother with a sub-menu...
    //
    
    //QPopupMenu* chpup = pup;
    chpup = pup;
    if(chans <= 2)
      // Just do one iteration.
      chans = 1;
    
    //for(int ch = 0; ch < schans; ++ch)
    for(int ch = 0; ch < chans; ++ch)
    {
      // If more than two channels, create the sub-menu.
      if(chans > 2)
      {
        chpup = new QPopupMenu(parent);
        chpup->setCheckable(true);
      }
      
      if(isOutput)
      {
        switch(t->type()) 
        {
          case Track::AUDIO_INPUT:
                id = addWavePorts(parent, t, chpup, id, mm, ch, 2, isOutput);
          case Track::WAVE:
          case Track::AUDIO_GROUP:
          case Track::AUDIO_SOFTSYNTH:
                id = addOutPorts(parent, t, chpup, id, mm, ch, 2, isOutput);
                id = addGroupPorts(parent, t, chpup, id, mm, ch, 2, isOutput);
                id = addSyntiPorts(parent, t, chpup, id, mm, ch, 2, isOutput);
                break;
          case Track::AUDIO_AUX:
                id = addOutPorts(parent, t, chpup, id, mm, ch, 2, isOutput);
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
                id = addWavePorts(parent, t, chpup, id, mm, ch, 2, isOutput);
                id = addInPorts(parent, t, chpup, id, mm, ch, 2, isOutput);
                id = addGroupPorts(parent, t, chpup, id, mm, ch, 2, isOutput);
                id = addAuxPorts(parent, t, chpup, id, mm, ch, 2, isOutput);
                id = addSyntiPorts(parent, t, chpup, id, mm, ch, 2, isOutput);
                break;
          case Track::WAVE:
                id = addInPorts(parent, t, chpup, id, mm, ch, 2, isOutput);
                break;
          case Track::AUDIO_SOFTSYNTH:
          case Track::AUDIO_GROUP:
                id = addWavePorts(parent, t, chpup, id, mm, ch, 2, isOutput);
                id = addInPorts(parent, t, chpup, id, mm, ch, 2, isOutput);
                id = addGroupPorts(parent, t, chpup, id, mm, ch, 2, isOutput);
                id = addSyntiPorts(parent, t, chpup, id, mm, ch, 2, isOutput);
                break;
          default:
                break;
        }
      }
      
      // If more than two channels, add the created sub-menu.
      if(chans > 2)
      {
        char buffer[128];
        snprintf(buffer, 128, "%s %d,%d", pup->tr("Channel").latin1(), ch+1, ch+2);
        pup->insertItem(QString(buffer), chpup);
      }  
    } 
  }
  
  return id;
}

//---------------------------------------------------------
//   nonSyntiTrackAddSyntis
//---------------------------------------------------------

//static int nonSyntiTrackAddSyntis(QButton* parent, AudioTrack* t, QPopupMenu* lb, int id, RouteMenuMap& mm, int channel, int channels, bool isOutput)
static int nonSyntiTrackAddSyntis(QButton* parent, AudioTrack* t, QPopupMenu* lb, int id, RouteMenuMap& mm, bool isOutput)
{
      RouteList* rl = isOutput ? t->outRoutes() : t->inRoutes();
      
      SynthIList* al = song->syntis();
      for (iSynthI i = al->begin(); i != al->end(); ++i) 
      {
            Track* track = *i;
            if (t == track)
                  continue;
            //id = addMenuItem(parent, track, lb, r, id, mm, channel, channels);
            
            /*
            QString s(track->name());
            //int it = lb->insertItem(s);
            lb->insertItem(s, id);
            for (iRoute ir = r->begin(); ir != r->end(); ++ir) {
                  //if (ir->type == 0 && ir->track == track) {
                  if (ir->type == 0 && ir->track == track && ir->channels == channels) {
                        //lb->setItemChecked(it, true);
                        lb->setItemChecked(id, true);
                        break;
                        }
                  }
            ++id;      
            */
            
            //SynthI* synti = (SynthI*)track;
            
            int toch = ((AudioTrack*)track)->totalOutChannels();
            // If track channels = 1, it must be a mono synth. And synti channels cannot be changed by user.
            if(track->channels() == 1)
              toch = 1;
            
            //int chans = synti->totalOutChannels();
            //int chans = (!isOutput || track->type() != Track::AUDIO_SOFTSYNTH) ? ((AudioTrack*)track)->totalOutChannels() : ((AudioTrack*)track)->totalInChannels();
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
              QPopupMenu* chpup = new QPopupMenu(parent);
              chpup->setCheckable(true);
              
              if(chans > 1)
              {
                chpup->insertItem(new MenuTitleItem("<Mono>"));
                //pup->insertSeparator();
              }
              
              for(int ch = 0; ch < chans; ++ch)
              {
                char buffer[128];
                //if(tchans == 2)
                //  snprintf(buffer, 128, "%s %d,%d", chpup->tr("Channel").latin1(), ch+1, ch+2);
                //else  
                  snprintf(buffer, 128, "%s %d", chpup->tr("Channel").latin1(), ch+1);
                chpup->insertItem(QString(buffer), id);
                
                //int ach = (channel == -1) ? ch : channel;
                //int bch = (channel == -1) ? -1 : ch;
                int ach = ch;
                int bch = -1;
                
                //Route rt(track, (t->type() != Track::AUDIO_SOFTSYNTH || isOutput) ? ach : bch, tchans);
                Route rt(track, isOutput ? bch : ach, 1);
                //Route rt(track, ch);
                
                //rt.remoteChannel = -1;
                //rt.remoteChannel = (t->type() != Track::AUDIO_SOFTSYNTH || isOutput) ? bch : ach;
                rt.remoteChannel = isOutput ? ach : bch;
                
                mm.insert( pRouteMenuMap(id, rt) );
                
                for(iRoute ir = rl->begin(); ir != rl->end(); ++ir) 
                {
                  //if (ir->type == 0 && ir->track == track) {
                  //if(ir->type == 0 && ir->track == track && ir->channels == channels) 
                  //if(ir->type == Route::TRACK_ROUTE && ir->track == track && ir->channel == channel && 
                  //   ir->channels == channels && ir->remoteChannel == r.remoteChannel) 
                  //if(ir->type == Route::TRACK_ROUTE && ir->track == track && ir->channel == ch && 
                  //   ir->remoteChannel == rt.remoteChannel) 
                  //if(ir->type == Route::TRACK_ROUTE && ir->track == track && ir->channel == rt.channel && 
                  //   ir->channels == rt.channels && ir->remoteChannel == rt.remoteChannel) 
                  
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
                      chpup->setItemChecked(id, true);
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
                
                chpup->insertSeparator();
                chpup->insertItem(new MenuTitleItem("<Stereo>"));
                //pup->insertSeparator();
              
                for(int ch = 0; ch < chans; ++ch)
                {
                  char buffer[128];
                  //if(tchans == 2)
                    snprintf(buffer, 128, "%s %d,%d", chpup->tr("Channel").latin1(), ch+1, ch+2);
                  //else  
                  //  snprintf(buffer, 128, "%s %d", chpup->tr("Channel").latin1(), ch+1);
                  chpup->insertItem(QString(buffer), id);
                  
                  //int ach = (channel == -1) ? ch : channel;
                  //int bch = (channel == -1) ? -1 : ch;
                  int ach = ch;
                  int bch = -1;
                  
                  //Route rt(track, (t->type() != Track::AUDIO_SOFTSYNTH || isOutput) ? ach : bch, tchans);
                  Route rt(track, isOutput ? bch : ach, 2);
                  //Route rt(track, ch);
                  
                  //rt.remoteChannel = -1;
                  //rt.remoteChannel = (t->type() != Track::AUDIO_SOFTSYNTH || isOutput) ? bch : ach;
                  rt.remoteChannel = isOutput ? ach : bch;
                  
                  mm.insert( pRouteMenuMap(id, rt) );
                  
                  for(iRoute ir = rl->begin(); ir != rl->end(); ++ir) 
                  {
                    //if (ir->type == 0 && ir->track == track) {
                    //if(ir->type == 0 && ir->track == track && ir->channels == channels) 
                    //if(ir->type == Route::TRACK_ROUTE && ir->track == track && ir->channel == channel && 
                    //   ir->channels == channels && ir->remoteChannel == r.remoteChannel) 
                    //if(ir->type == Route::TRACK_ROUTE && ir->track == track && ir->channel == ch && 
                    //   ir->remoteChannel == rt.remoteChannel) 
                    //if(ir->type == Route::TRACK_ROUTE && ir->track == track && ir->channel == rt.channel && 
                    //  ir->channels == rt.channels && ir->remoteChannel == rt.remoteChannel) 
                    
                    
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
                        chpup->setItemChecked(id, true);
                        break;
                      }
                    }  
                  }
                  ++id;
                }
              }
              
              lb->insertItem(track->name(), chpup);
            }
      }
      return id;      
}

//---------------------------------------------------------
//   iRoutePressed
//---------------------------------------------------------

void AudioStrip::iRoutePressed()
      {
      //if(track->isMidiTrack() || (track->type() == Track::AUDIO_AUX) || (track->type() == Track::AUDIO_SOFTSYNTH))
      if(track->isMidiTrack() || (track->type() == Track::AUDIO_AUX))
        return;
        
      QPopupMenu* pup = new QPopupMenu(iR);
      //pup->setCheckable(true);
      AudioTrack* t = (AudioTrack*)track;
      RouteList* irl = t->inRoutes();

      int gid = 0;
      RouteMenuMap mm;
      
      switch(track->type()) 
      {
        case Track::AUDIO_INPUT:
        {
          pup->setCheckable(true);
          //int gid = 0;
          for(int i = 0; i < channel; ++i) 
          {
            char buffer[128];
            snprintf(buffer, 128, "%s %d", tr("Channel").latin1(), i+1);
            MenuTitleItem* titel = new MenuTitleItem(QString(buffer));
            pup->insertItem(titel);
  
            if(!checkAudioDevice())
            { 
              delete pup;
              return;
            }
            std::list<QString> ol = audioDevice->outputPorts();
            for(std::list<QString>::iterator ip = ol.begin(); ip != ol.end(); ++ip) 
            {
              int id = pup->insertItem(*ip, (gid * 16) + i);
              //Route dst(*ip, true, i);
              Route dst(*ip, true, i, Route::JACK_ROUTE);
              ++gid;
              for(iRoute ir = irl->begin(); ir != irl->end(); ++ir) 
              {
                if(*ir == dst) 
                {
                  pup->setItemChecked(id, true);
                  break;
                }
              }
            }
            if(i+1 != channel)
              pup->insertSeparator();
          }
        }
        break;
        /*
        case Track::AUDIO_OUTPUT:
        case Track::WAVE:
        case Track::AUDIO_GROUP:
        */
        
        case Track::AUDIO_OUTPUT:
              gid = addWavePorts( iR, t, pup, gid, mm, -1, -1, false);
              gid = addInPorts(   iR, t, pup, gid, mm, -1, -1, false);
              gid = addGroupPorts(iR, t, pup, gid, mm, -1, -1, false);
              gid = addAuxPorts(  iR, t, pup, gid, mm, -1, -1, false);
              //gid = addSyntiPorts(iR, t, pup, gid, mm, -1, -1, false);
              gid = nonSyntiTrackAddSyntis(iR, t, pup, gid, mm, false);
              break;
        case Track::WAVE:
              gid = addInPorts(   iR, t, pup, gid, mm, -1, -1, false);
              break;
        case Track::AUDIO_GROUP:
              gid = addWavePorts( iR, t, pup, gid, mm, -1, -1, false);
              gid = addInPorts(   iR, t, pup, gid, mm, -1, -1, false);
              gid = addGroupPorts(iR, t, pup, gid, mm, -1, -1, false);
              //gid = addSyntiPorts(iR, t, pup, gid, mm, -1, -1, false);
              gid = nonSyntiTrackAddSyntis(iR, t, pup, gid, mm, false);
              break;
        
        case Track::AUDIO_SOFTSYNTH:
              gid = addMultiChannelPorts(iR, t, pup, gid, mm, false);
              break;
        default:
              delete pup;
              return;
      }  
      
      if(pup->count() == 0)
      {
        delete pup;
        return;
      }
      
      int n = pup->exec(QCursor::pos());
      if(n != -1) 
      {
            QString s(pup->text(n));
            
            if(track->type() == Track::AUDIO_INPUT)
            {
              delete pup;
              int chan = n & 0xf;
              
              Route srcRoute(s, false, -1, Route::JACK_ROUTE);
              Route dstRoute(t, chan);
              
              srcRoute.channel = chan;
              
              iRoute iir = irl->begin();
              for(; iir != irl->end(); ++iir) 
              {
                if(*iir == srcRoute)
                  break;
              }
              if(iir != irl->end()) 
                // disconnect
                audio->msgRemoveRoute(srcRoute, dstRoute);
              else 
                // connect
                audio->msgAddRoute(srcRoute, dstRoute);
              
              audio->msgUpdateSoloStates();
              song->update(SC_ROUTE);
              iR->setDown(false);     // pup->exec() catches mouse release event
              return;
            }
            
            iRouteMenuMap imm = mm.find(n);
            if(imm == mm.end())
            {  
              delete pup;
              iR->setDown(false);     // pup->exec() catches mouse release event
              return;
            }  
            
            //int chan = n >> 16;
            //int chans = (chan >> 15) + 1; // Bit 31 MSB: Mono or stereo.
            //chan &= 0xffff;
            //int chan = imm->second.channel;
            //int chans = imm->second.channels; 
            
            //Route srcRoute(s, false, -1);
            //Route srcRoute(s, false, -1, Route::TRACK_ROUTE);
            Route &srcRoute = imm->second;
            
            //Route dstRoute(t, -1);
            //Route dstRoute(t, chan, chans);
            Route dstRoute(t, imm->second.channel, imm->second.channels);
            //Route dstRoute(t, imm->second.channel);
            dstRoute.remoteChannel = imm->second.remoteChannel;

            iRoute iir = irl->begin();
            for (; iir != irl->end(); ++iir) {
                  if (*iir == srcRoute)
                        break;
                  }
            if (iir != irl->end()) {
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
      delete pup;
      iR->setDown(false);     // pup->exec() catches mouse release event
      }

//---------------------------------------------------------
//   oRoutePressed
//---------------------------------------------------------

void AudioStrip::oRoutePressed()
{
      if(track->isMidiTrack())
        return;
        
      QPopupMenu* pup = new QPopupMenu(oR);
      //pup->setCheckable(true);
      AudioTrack* t = (AudioTrack*)track;
      RouteList* orl = t->outRoutes();

      int gid = 0;
      RouteMenuMap mm;
      
      switch(track->type()) 
      {
        case Track::AUDIO_OUTPUT:
        {
          pup->setCheckable(true);
          //int gid = 0;
          for(int i = 0; i < channel; ++i) 
          {
            char buffer[128];
            snprintf(buffer, 128, "%s %d", tr("Channel").latin1(), i+1);
            MenuTitleItem* titel = new MenuTitleItem(QString(buffer));
            pup->insertItem(titel);
  
            if(!checkAudioDevice())
            { 
              delete pup;
              return;
            }
            std::list<QString> ol = audioDevice->inputPorts();
            for(std::list<QString>::iterator ip = ol.begin(); ip != ol.end(); ++ip) 
            {
              int id = pup->insertItem(*ip, (gid * 16) + i);
              //Route dst(*ip, true, i);
              Route dst(*ip, true, i, Route::JACK_ROUTE);
              ++gid;
              for(iRoute ir = orl->begin(); ir != orl->end(); ++ir) 
              {
                if(*ir == dst) 
                {
                  pup->setItemChecked(id, true);
                  break;
                }
              }
            }
            if(i+1 != channel)
              pup->insertSeparator();
          }      
        }
        break;
        /*
        case Track::AUDIO_INPUT:
        case Track::WAVE:
        case Track::AUDIO_GROUP:
        case Track::AUDIO_AUX:
        */
        
        case Track::AUDIO_SOFTSYNTH:
              //addOutPorts(t, pup, orl);
              //addGroupPorts(t, pup, orl);
              gid = addMultiChannelPorts(oR, t, pup, gid, mm, true);
        break;
        
        case Track::AUDIO_INPUT:
              gid = addWavePorts(        oR, t, pup, gid, mm, -1, -1, true);
        case Track::WAVE:
        case Track::AUDIO_GROUP:
        //case Track::AUDIO_SOFTSYNTH:
              gid = addOutPorts(         oR, t, pup, gid, mm, -1, -1, true);
              gid = addGroupPorts(       oR, t, pup, gid, mm, -1, -1, true);
              //gid = addSyntiPorts(       oR, t, pup, gid, mm, -1, -1, true);
              gid = nonSyntiTrackAddSyntis(oR, t, pup, gid, mm, true);
        break;
        case Track::AUDIO_AUX:
              gid = addOutPorts(         oR, t, pup, gid, mm, -1, -1, true);
        break;
        
        default:
              delete pup;
              return;
      }
      
      if(pup->count() == 0)
      {
        delete pup;
        return;
      }
      
      int n = pup->exec(QCursor::pos());
      if (n != -1) {
            QString s(pup->text(n));
            
            if(track->type() == Track::AUDIO_OUTPUT)
            {
              delete pup;
              int chan = n & 0xf;
              
              //Route srcRoute(t, -1);
              //Route srcRoute(t, chan, chans);
              //Route srcRoute(t, chan, 1);
              Route srcRoute(t, chan);
              
              //Route dstRoute(s, true, -1);
              Route dstRoute(s, true, -1, Route::JACK_ROUTE);
              //Route dstRoute(s, true, 0, Route::JACK_ROUTE);
  
              //srcRoute.channel = dstRoute.channel = chan;
              dstRoute.channel = chan;
              //dstRoute.channels = 1;
  
              // check if route src->dst exists:
              iRoute iorl = orl->begin();
              for (; iorl != orl->end(); ++iorl) {
                    if (*iorl == dstRoute)
                          break;
                    }
              if (iorl != orl->end()) {
                    // disconnect if route exists
                    audio->msgRemoveRoute(srcRoute, dstRoute);
                    }
              else {
                    // connect if route does not exist
                    audio->msgAddRoute(srcRoute, dstRoute);
                    }
              audio->msgUpdateSoloStates();
              song->update(SC_ROUTE);
              oR->setDown(false);     // pup->exec() catches mouse release event
              return;
            }
            
            iRouteMenuMap imm = mm.find(n);
            if(imm == mm.end())
            {  
              delete pup;
              oR->setDown(false);     // pup->exec() catches mouse release event
              return;
            }  
            
            //int chan = n >> 16;
            //int chans = (chan >> 15) + 1; // Bit 31 MSB: Mono or stereo.
            //chan &= 0xffff;
            //int chan = imm->second.channel;
            //int chans = imm->second.channels; 
            
            //Route srcRoute(t, -1);
            //srcRoute.remoteChannel = chan;
            //Route srcRoute(t, chan, chans);
            Route srcRoute(t, imm->second.channel, imm->second.channels);
            //Route srcRoute(t, imm->second.channel);
            srcRoute.remoteChannel = imm->second.remoteChannel;
            
            //Route dstRoute(s, true, -1);
            //Route dstRoute(s, true, -1, Route::TRACK_ROUTE);
            Route &dstRoute = imm->second;

            // check if route src->dst exists:
            iRoute iorl = orl->begin();
            for (; iorl != orl->end(); ++iorl) {
                  if (*iorl == dstRoute)
                        break;
                  }
            if (iorl != orl->end()) {
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
      delete pup;
      oR->setDown(false);     // pup->exec() catches mouse release event
}

/*
//---------------------------------------------------------
//   iRoutePressed
//---------------------------------------------------------

void AudioStrip::iRoutePressed()
      {
      if(track->isMidiTrack() || (track->type() == Track::AUDIO_AUX) || (track->type() == Track::AUDIO_SOFTSYNTH))
        return;
        
      QPopupMenu* pup = new QPopupMenu(iR);
      //pup->setCheckable(true);
      AudioTrack* t = (AudioTrack*)track;
      RouteList* irl = t->inRoutes();

      RouteMenuMap mm;
      
      if(track->type() == Track::AUDIO_INPUT)
      {
        pup->setCheckable(true);
        int gid = 0;
        for(int i = 0; i < channel; ++i) 
        {
          char buffer[128];
          snprintf(buffer, 128, "%s %d", tr("Channel").latin1(), i+1);
          MenuTitleItem* titel = new MenuTitleItem(QString(buffer));
          pup->insertItem(titel);

          if(!checkAudioDevice())
          { 
            delete pup;
            return;
          }
          std::list<QString> ol = audioDevice->outputPorts();
          for(std::list<QString>::iterator ip = ol.begin(); ip != ol.end(); ++ip) 
          {
            int id = pup->insertItem(*ip, (gid * 16) + i);
            //Route dst(*ip, true, i);
            Route dst(*ip, true, i, Route::JACK_ROUTE);
            ++gid;
            for(iRoute ir = irl->begin(); ir != irl->end(); ++ir) 
            {
              if(*ir == dst) 
              {
                pup->setItemChecked(id, true);
                break;
              }
            }
          }
          if(i+1 != channel)
            pup->insertSeparator();
        }
      }
      else
        addMultiChannelOutPorts(iR, t, pup, irl, mm, false);
      
      int n = pup->exec(QCursor::pos());
      if (n != -1) {
            QString s(pup->text(n));
            
            //Route srcRoute(s, false, -1);
            Route srcRoute(s, false, -1, (track->type() == Track::AUDIO_INPUT) ? Route::JACK_ROUTE : Route::TRACK_ROUTE);
            Route dstRoute(t, -1);

            if (track->type() == Track::AUDIO_INPUT)
                  srcRoute.channel = dstRoute.channel = n & 0xf;
            iRoute iir = irl->begin();
            for (; iir != irl->end(); ++iir) {
                  if (*iir == srcRoute)
                        break;
                  }
            if (iir != irl->end()) {
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
      delete pup;
      iR->setDown(false);     // pup->exec() catches mouse release event
      }
*/

/*
//---------------------------------------------------------
//   oRoutePressed
//---------------------------------------------------------

void AudioStrip::oRoutePressed()
{
      if(track->isMidiTrack())
        return;
        
      QPopupMenu* pup = new QPopupMenu(oR);
      //pup->setCheckable(true);
      AudioTrack* t = (AudioTrack*)track;
      RouteList* orl = t->outRoutes();

      RouteMenuMap mm;
      
      if(track->type() == Track::AUDIO_OUTPUT)
      {
        pup->setCheckable(true);
        int gid = 0;
        for(int i = 0; i < channel; ++i) 
        {
          char buffer[128];
          snprintf(buffer, 128, "%s %d", tr("Channel").latin1(), i+1);
          MenuTitleItem* titel = new MenuTitleItem(QString(buffer));
          pup->insertItem(titel);

          if(!checkAudioDevice())
          { 
            delete pup;
            return;
          }
          std::list<QString> ol = audioDevice->inputPorts();
          for(std::list<QString>::iterator ip = ol.begin(); ip != ol.end(); ++ip) 
          {
            int id = pup->insertItem(*ip, (gid * 16) + i);
            //Route dst(*ip, true, i);
            Route dst(*ip, true, i, Route::JACK_ROUTE);
            ++gid;
            for(iRoute ir = orl->begin(); ir != orl->end(); ++ir) 
            {
              if(*ir == dst) 
              {
                pup->setItemChecked(id, true);
                break;
              }
            }
          }
          if(i+1 != channel)
            pup->insertSeparator();
        }      
      }
      else
        addMultiChannelOutPorts(oR, t, pup, orl, mm, true);
      
      int n = pup->exec(QCursor::pos());
      if (n != -1) {
            QString s(pup->text(n));
            
            if(track->type() == Track::AUDIO_OUTPUT)
            {
              delete pup;
              int chan = n & 0xf;
              
              //Route srcRoute(t, -1);
              //Route srcRoute(t, chan, chans);
              Route srcRoute(t, chan, 1);
              
              //Route dstRoute(s, true, -1);
              Route dstRoute(s, true, -1, Route::JACK_ROUTE);
              //Route dstRoute(s, true, 0, Route::JACK_ROUTE);
  
              //srcRoute.channel = dstRoute.channel = chan;
              dstRoute.channel = chan;
              dstRoute.channels = 1;
  
              // check if route src->dst exists:
              iRoute iorl = orl->begin();
              for (; iorl != orl->end(); ++iorl) {
                    if (*iorl == dstRoute)
                          break;
                    }
              if (iorl != orl->end()) {
                    // disconnect if route exists
                    audio->msgRemoveRoute(srcRoute, dstRoute);
                    }
              else {
                    // connect if route does not exist
                    audio->msgAddRoute(srcRoute, dstRoute);
                    }
              audio->msgUpdateSoloStates();
              song->update(SC_ROUTE);
              oR->setDown(false);     // pup->exec() catches mouse release event
              return;
            }
            
            iRouteMenuMap imm = mm.find(n);
            if(imm == mm.end())
            {  
              delete pup;
              oR->setDown(false);     // pup->exec() catches mouse release event
              return;
            }  
            
            //int chan = n >> 16;
            //int chans = (chan >> 15) + 1; // Bit 31 MSB: Mono or stereo.
            //chan &= 0xffff;
            int chan = imm->second.channel;
            int chans = imm->second.channels; 
             
            //Route srcRoute(t, -1);
            Route srcRoute(t, chan, chans);
            
            //Route dstRoute(s, true, -1);
            Route dstRoute(s, true, -1, Route::TRACK_ROUTE);

            // check if route src->dst exists:
            iRoute iorl = orl->begin();
            for (; iorl != orl->end(); ++iorl) {
                  if (*iorl == dstRoute)
                        break;
                  }
            if (iorl != orl->end()) {
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
      delete pup;
      oR->setDown(false);     // pup->exec() catches mouse release event
}
*/

/*
//---------------------------------------------------------
//   iRoutePressed
//---------------------------------------------------------

void AudioStrip::iRoutePressed()
      {
      QPopupMenu* pup = new QPopupMenu(iR);
      pup->setCheckable(true);
      AudioTrack* t = (AudioTrack*)track;
      RouteList* irl = t->inRoutes();

      switch(track->type()) {
            case Track::MIDI:
            case Track::DRUM:
            case Track::AUDIO_AUX:
            case Track::AUDIO_SOFTSYNTH:
                  delete pup;
                  return;
            case Track::AUDIO_INPUT:
                  {
                  int gid = 0;
                  for (int i = 0; i < channel; ++i) {
                        char buffer[128];
                        snprintf(buffer, 128, "%s %d", tr("Channel").latin1(), i+1);
                        MenuTitleItem* titel = new MenuTitleItem(QString(buffer));
                        pup->insertItem(titel);

                        if (!checkAudioDevice()) return;
                        std::list<QString> ol = audioDevice->outputPorts();
                        for (std::list<QString>::iterator ip = ol.begin(); ip != ol.end(); ++ip) {
                              int id = pup->insertItem(*ip, (gid * 16) + i);
                              //Route dst(*ip, true, i);
                              Route dst(*ip, true, i, Route::JACK_ROUTE);
                              ++gid;
                              for (iRoute ir = irl->begin(); ir != irl->end(); ++ir) {
                                    if (*ir == dst) {
                                          pup->setItemChecked(id, true);
                                          break;
                                          }
                                    }
                              }
                        if (i+1 != channel)
                              pup->insertSeparator();
                        }
                  }
                  break;
            case Track::AUDIO_OUTPUT:
                  addWavePorts(t, pup, irl);
                  addInPorts(t, pup, irl);
                  addGroupPorts(t, pup, irl);
                  addAuxPorts(t, pup, irl);
                  addSyntiPorts(t, pup, irl);
                  break;
            case Track::WAVE:
                  addInPorts(t, pup, irl);
                  break;
            case Track::AUDIO_GROUP:
                  addWavePorts(t, pup, irl);
                  addInPorts(t, pup, irl);
                  addGroupPorts(t, pup, irl);
                  addSyntiPorts(t, pup, irl);
                  break;
            }
      int n = pup->exec(QCursor::pos());
      if (n != -1) {
            QString s(pup->text(n));
            
            //Route srcRoute(s, false, -1);
            Route srcRoute(s, false, -1, (track->type() == Track::AUDIO_INPUT) ? Route::JACK_ROUTE : Route::TRACK_ROUTE);
            Route dstRoute(t, -1);

            if (track->type() == Track::AUDIO_INPUT)
                  srcRoute.channel = dstRoute.channel = n & 0xf;
            iRoute iir = irl->begin();
            for (; iir != irl->end(); ++iir) {
                  if (*iir == srcRoute)
                        break;
                  }
            if (iir != irl->end()) {
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
      delete pup;
      iR->setDown(false);     // pup->exec() catches mouse release event
      }
*/

/*
//---------------------------------------------------------
//   oRoutePressed
//---------------------------------------------------------

void AudioStrip::oRoutePressed()
      {
      QPopupMenu* pup = new QPopupMenu(oR);
      pup->setCheckable(true);
      AudioTrack* t = (AudioTrack*)track;
      RouteList* orl = t->outRoutes();

      switch(track->type()) {
            case Track::MIDI:
            case Track::DRUM:
                  delete pup;
                  return;
            case Track::AUDIO_OUTPUT:
                  {
                  int gid = 0;
                  for (int i = 0; i < channel; ++i) {
                        char buffer[128];
                        snprintf(buffer, 128, "%s %d", tr("Channel").latin1(), i+1);
                        MenuTitleItem* titel = new MenuTitleItem(QString(buffer));
                        pup->insertItem(titel);

                        if (!checkAudioDevice()) return;
                        std::list<QString> ol = audioDevice->inputPorts();
                        for (std::list<QString>::iterator ip = ol.begin(); ip != ol.end(); ++ip) {
                              int id = pup->insertItem(*ip, (gid * 16) + i);
                              //Route dst(*ip, true, i);
                              Route dst(*ip, true, i, Route::JACK_ROUTE);
                              ++gid;
                              for (iRoute ir = orl->begin(); ir != orl->end(); ++ir) {
                                    if (*ir == dst) {
                                          pup->setItemChecked(id, true);
                                          break;
                                          }
                                    }
                              }
                        if (i+1 != channel)
                              pup->insertSeparator();
                        }
                  }
                  break;
            case Track::AUDIO_INPUT:
                  addWavePorts(t, pup, orl);
            case Track::WAVE:
            case Track::AUDIO_GROUP:
            case Track::AUDIO_SOFTSYNTH:
                  addOutPorts(t, pup, orl);
                  addGroupPorts(t, pup, orl);
                  break;
            case Track::AUDIO_AUX:
                  addOutPorts(t, pup, orl);
                  break;
            }
      int n = pup->exec(QCursor::pos());
      if (n != -1) {
            QString s(pup->text(n));
            Route srcRoute(t, -1);
            //Route dstRoute(s, true, -1);
            Route dstRoute(s, true, -1, (track->type() == Track::AUDIO_OUTPUT) ? Route::JACK_ROUTE : Route::TRACK_ROUTE);

            if (track->type() == Track::AUDIO_OUTPUT)
                  srcRoute.channel = dstRoute.channel = n & 0xf;

            // check if route src->dst exists:
            iRoute iorl = orl->begin();
            for (; iorl != orl->end(); ++iorl) {
                  if (*iorl == dstRoute)
                        break;
                  }
            if (iorl != orl->end()) {
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
      delete pup;
      oR->setDown(false);     // pup->exec() catches mouse release event
      }
*/
