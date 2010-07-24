//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mstrip.cpp,v 1.9.2.13 2009/11/14 03:37:48 terminator356 Exp $
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
//#include <qpopupmenu.h>
#include <qcursor.h>

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

enum { KNOB_PAN, KNOB_VAR_SEND, KNOB_REV_SEND, KNOB_CHO_SEND };

//---------------------------------------------------------
//   addKnob
//---------------------------------------------------------

void MidiStrip::addKnob(int idx, const QString& tt, const QString& label,
   const char* slot, bool enabled)
      {
      int ctl = CTRL_PANPOT, mn, mx, v;
      int chan  = ((MidiTrack*)track)->outChannel();
      switch(idx)
      {
        //case KNOB_PAN:
        //  ctl = CTRL_PANPOT;
        //break;
        case KNOB_VAR_SEND:
          ctl = CTRL_VARIATION_SEND;
        break;
        case KNOB_REV_SEND:
          ctl = CTRL_REVERB_SEND;
        break;
        case KNOB_CHO_SEND:
          ctl = CTRL_CHORUS_SEND;
        break;
      }
      MidiPort* mp = &midiPorts[((MidiTrack*)track)->outPort()];
      MidiController* mc = mp->midiController(ctl);
      mn = mc->minVal();
      mx = mc->maxVal();
      
      Knob* knob = new Knob(this);
      knob->setRange(double(mn), double(mx), 1.0);
      knob->setId(ctl);
      
      controller[idx].knob = knob;
      knob->setFixedWidth(STRIP_WIDTH/2-3);
      knob->setFixedHeight(30);
      knob->setBackgroundMode(PaletteMid);
      QToolTip::add(knob, tt);
      knob->setEnabled(enabled);

      DoubleLabel* dl = new DoubleLabel(0.0, double(mn), double(mx), this);
      dl->setId(idx);
      dl->setSpecialText(tr("off"));
      QToolTip::add(dl, tr("double click on/off"));
      controller[idx].dl = dl;
      dl->setFont(config.fonts[1]);
      dl->setBackgroundMode(PaletteMid);
      dl->setFrame(true);
      dl->setPrecision(0);
      dl->setFixedWidth(STRIP_WIDTH/2);
      dl->setFixedHeight(15);
      dl->setEnabled(enabled);

      double dlv;
      v = mp->hwCtrlState(chan, ctl);
      if(v == CTRL_VAL_UNKNOWN)
      {
        //v = mc->initVal();
        //if(v == CTRL_VAL_UNKNOWN)
        //  v = 0;
//        v = mn - 1;
        int lastv = mp->lastValidHWCtrlState(chan, ctl);
        if(lastv == CTRL_VAL_UNKNOWN)
        {
          if(mc->initVal() == CTRL_VAL_UNKNOWN)
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
      lb->setFont(config.fonts[1]);
      lb->setFixedWidth(STRIP_WIDTH/2-3);
      lb->setAlignment(AlignCenter);
      lb->setFixedHeight(15);
      lb->setEnabled(enabled);

      QGridLayout* grid = new QGridLayout(0, 2, 2, 0, 0, "grid");
      grid->setMargin(2);
      grid->addWidget(lb, 0, 0);
      grid->addWidget(dl, 1, 0);
      grid->addMultiCellWidget(knob, 0, 1, 1, 1);
      layout->addLayout(grid);

      connect(knob, SIGNAL(sliderMoved(double,int)), slot);
      connect(knob, SIGNAL(sliderRightClicked(const QPoint &, int)), SLOT(controlRightClicked(const QPoint &, int)));
      connect(dl, SIGNAL(valueChanged(double, int)), slot);
      connect(dl, SIGNAL(doubleClicked(int)), SLOT(labelDoubleClicked(int)));
      }

//---------------------------------------------------------
//   MidiStrip
//---------------------------------------------------------

MidiStrip::MidiStrip(QWidget* parent, MidiTrack* t)
   : Strip(parent, t)
      {
      inHeartBeat = true;

      // Clear so the meters don't start off by showing stale values.
      t->setActivity(0);
      t->setLastActivity(0);
      
      volume      = CTRL_VAL_UNKNOWN;
      pan         = CTRL_VAL_UNKNOWN;
      variSend    = CTRL_VAL_UNKNOWN;
      chorusSend  = CTRL_VAL_UNKNOWN;
      reverbSend  = CTRL_VAL_UNKNOWN;
      
      addKnob(KNOB_VAR_SEND, tr("VariationSend"), tr("Var"), SLOT(setVariSend(double)), false);
      addKnob(KNOB_REV_SEND, tr("ReverbSend"), tr("Rev"), SLOT(setReverbSend(double)), false);
      addKnob(KNOB_CHO_SEND, tr("ChorusSend"), tr("Cho"), SLOT(setChorusSend(double)), false);
      int auxsSize = song->auxs()->size();
      if (auxsSize)
            layout->addSpacing((STRIP_WIDTH/2 + 1) * auxsSize);

      //---------------------------------------------------
      //    slider, label, meter
      //---------------------------------------------------

      MidiPort* mp = &midiPorts[t->outPort()];
      MidiController* mc = mp->midiController(CTRL_VOLUME);
      int chan  = t->outChannel();
      int mn = mc->minVal();
      int mx = mc->maxVal();
      
      slider = new Slider(this, "vol", Slider::Vertical, Slider::None,
         Slider::BgTrough | Slider::BgSlot);
      slider->setCursorHoming(true);
      slider->setRange(double(mn), double(mx), 1.0);
      slider->setFixedWidth(20);
      slider->setFont(config.fonts[1]);
      slider->setId(CTRL_VOLUME);

      meter[0] = new Meter(this, Meter::LinMeter);
      meter[0]->setRange(0, 127.0);
      meter[0]->setFixedWidth(15);
      connect(meter[0], SIGNAL(mousePress()), this, SLOT(resetPeaks()));
      sliderGrid = new QGridLayout;
      sliderGrid->setRowStretch(0, 100);
      sliderGrid->addWidget(slider, 0, 0, AlignRight);
      sliderGrid->addWidget(meter[0], 0, 1, AlignLeft);
      layout->addLayout(sliderGrid);

      sl = new DoubleLabel(0.0, -98.0, 0.0, this);
      sl->setFont(config.fonts[1]);
      sl->setBackgroundMode(PaletteMid);
      sl->setSpecialText(tr("off"));
      sl->setSuffix(tr("dB"));
      QToolTip::add(sl, tr("double click on/off"));
      sl->setFrame(true);
      sl->setPrecision(0);
      sl->setFixedWidth(STRIP_WIDTH);
      // Set the label's slider 'buddy'.
      sl->setSlider(slider);
      
      double dlv;
      int v = mp->hwCtrlState(chan, CTRL_VOLUME);
      if(v == CTRL_VAL_UNKNOWN)
      {
        int lastv = mp->lastValidHWCtrlState(chan, CTRL_VOLUME);
        if(lastv == CTRL_VAL_UNKNOWN)
        {
          if(mc->initVal() == CTRL_VAL_UNKNOWN)
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
          dlv = -fast_log10(float(127*127)/float(v*v))*20.0;
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
      connect(sl, SIGNAL(doubleClicked(int)), SLOT(labelDoubleClicked(int)));
      
      layout->addWidget(sl);

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

      record  = new TransparentToolButton(this);
      record->setBackgroundMode(PaletteMid);
      record->setToggleButton(true);
      
      //record->setFixedWidth(STRIP_WIDTH/2);
      //record->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum));
      record->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
      
      QIconSet iconSet;
      iconSet.setPixmap(*record_on_Icon, QIconSet::Automatic, QIconSet::Normal, QIconSet::On);
      iconSet.setPixmap(*record_off_Icon, QIconSet::Automatic, QIconSet::Normal, QIconSet::Off);
      record->setIconSet(iconSet);
      QToolTip::add(record, tr("record"));
      record->setOn(track->recordFlag());
      connect(record, SIGNAL(toggled(bool)), SLOT(recordToggled(bool)));

      mute  = new QToolButton(this);
      QIconSet muteSet;
      muteSet.setPixmap(*muteIconOn,   QIconSet::Automatic, QIconSet::Normal, QIconSet::Off);
      muteSet.setPixmap(*muteIconOff, QIconSet::Automatic, QIconSet::Normal, QIconSet::On);
      mute->setIconSet(muteSet);
      mute->setToggleButton(true);
      QToolTip::add(mute, tr("mute"));
      mute->setOn(track->mute());
      mute->setFixedWidth(STRIP_WIDTH/2);
      connect(mute, SIGNAL(toggled(bool)), SLOT(muteToggled(bool)));

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
      
      //QToolTip::add(solo, tr("pre fader listening"));
      QToolTip::add(solo, tr("solo mode"));
      solo->setToggleButton(true);
      solo->setOn(t->solo());
      solo->setFixedWidth(STRIP_WIDTH/2);
      connect(solo, SIGNAL(toggled(bool)), SLOT(soloToggled(bool)));

      QHBoxLayout* smBox1 = new QHBoxLayout(0);
      QHBoxLayout* smBox2 = new QHBoxLayout(0);

      smBox2->addWidget(mute);
      smBox2->addWidget(solo);

      // Changed by Tim. p3.3.21
      //QToolTip::add(record, tr("record"));
      //smBox1->addStretch(100);
      //smBox1->addWidget(record);
      QLabel* dev_ch_label = new QLabel(this);
      dev_ch_label->setMinimumWidth(STRIP_WIDTH/2);
      //dev_ch_label->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum));
      dev_ch_label->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum));
      dev_ch_label->setAlignment(AlignCenter);
      int port = t->outPort();
      int channel = t->outChannel();
      QString dcs;
      dcs.sprintf("%d-%d", port + 1, channel + 1);
      dev_ch_label->setText(dcs);
      //dev_ch_label->setBackgroundColor(QColor(0, 160, 255)); // Med blue
      //dev_ch_label->setFont(config.fonts[6]);
      dev_ch_label->setFont(config.fonts[1]);
      // Dealing with a horizontally constrained label. Ignore vertical. Use a minimum readable point size.
      //autoAdjustFontSize(dev_ch_label, dev_ch_label->text(), false, true, config.fonts[6].pointSize(), 5);
      QToolTip::add(dev_ch_label, tr("output port and channel"));
      smBox1->addWidget(dev_ch_label);
      smBox1->addWidget(record);
      
      layout->addLayout(smBox1);
      layout->addLayout(smBox2);

      //---------------------------------------------------
      //    routing
      //---------------------------------------------------

      // p3.3.38
      //route = new QToolButton(this);
      //route->setFont(config.fonts[1]);
      //route->setFixedWidth(STRIP_WIDTH);
      //route->setText(tr("Route"));
      //QToolTip::add(route, tr("set routing"));
      //layout->addWidget(route);
      QHBoxLayout* rBox = new QHBoxLayout(0);
      iR = new QToolButton(this);
      iR->setFont(config.fonts[1]);
      iR->setFixedWidth((STRIP_WIDTH-4)/2);
      iR->setText(tr("iR"));
      iR->setToggleButton(false);
      QToolTip::add(iR, tr("input routing"));
      rBox->addWidget(iR);
      connect(iR, SIGNAL(pressed()), SLOT(iRoutePressed()));
      oR = new QToolButton(this);
      oR->setFont(config.fonts[1]);
      oR->setFixedWidth((STRIP_WIDTH-4)/2);
      oR->setText(tr("oR"));
      oR->setToggleButton(false);
      // TODO: Works OK, but disabled for now, until we figure out what to do about multiple out routes and display values...
      oR->setEnabled(false);
      QToolTip::add(oR, tr("output routing"));
      rBox->addWidget(oR);
      connect(oR, SIGNAL(pressed()), SLOT(oRoutePressed()));
      layout->addLayout(rBox);

      //---------------------------------------------------
      //    automation mode
      //---------------------------------------------------

      autoType = new ComboBox(this);
      autoType->setFont(config.fonts[1]);
      autoType->setFixedWidth(STRIP_WIDTH-4);
      autoType->setEnabled(false);
      // Removed by T356. 
      // Disabled for now. There is no midi automation mechanism yet...
      //autoType->insertItem(tr("Off"), AUTO_OFF);
      //autoType->insertItem(tr("Read"), AUTO_READ);
      //autoType->insertItem(tr("Touch"), AUTO_TOUCH);
      //autoType->insertItem(tr("Write"), AUTO_WRITE);
      //autoType->setCurrentItem(t->automationType());
      //QToolTip::add(autoType, tr("automation type"));
      //connect(autoType, SIGNAL(activated(int,int)), SLOT(setAutomationType(int,int)));
      layout->addWidget(autoType);
      connect(heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
      inHeartBeat = false;
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MidiStrip::songChanged(int val)
      {
      if (mute && (val & SC_MUTE)) {      // mute && off
            mute->blockSignals(true);
            mute->setOn(track->isMute());
            updateOffState();
            mute->blockSignals(false);
            }
      if (solo && (val & SC_SOLO)) 
      {
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
      // Added by Tim. p3.3.9
      
      // Catch when label font changes.
      if (val & SC_CONFIG)
      {
        // Set the strip label's font.
        //label->setFont(config.fonts[1]);
        setLabelFont();
      }  
      
      // p3.3.47 Update the routing popup menu if anything relevant changes.
      if(gRoutingPopupMenuMaster == this && track && (val & (SC_ROUTE | SC_CHANNELS | SC_CONFIG))) 
        // Use this handy shared routine.
        muse->updateRouteMenus(track);
    }

//---------------------------------------------------------
//   controlRightClicked
//---------------------------------------------------------

void MidiStrip::controlRightClicked(const QPoint &p, int id)
{
  song->execMidiAutomationCtlPopup((MidiTrack*)track, 0, p, id);
}

//---------------------------------------------------------
//   labelDoubleClicked
//---------------------------------------------------------

void MidiStrip::labelDoubleClicked(int idx)
{
  //int mn, mx, v;
  //int num = CTRL_VOLUME;
  int num;
  switch(idx)
  {
    case KNOB_PAN:
      num = CTRL_PANPOT;
    break;
    case KNOB_VAR_SEND:
      num = CTRL_VARIATION_SEND;
    break;
    case KNOB_REV_SEND:
      num = CTRL_REVERB_SEND;
    break;
    case KNOB_CHO_SEND:
      num = CTRL_CHORUS_SEND;
    break;
    //case -1:
    default:
      num = CTRL_VOLUME;
    break;  
  }
  int outport = ((MidiTrack*)track)->outPort();
  int chan = ((MidiTrack*)track)->outChannel();
  MidiPort* mp = &midiPorts[outport];
  MidiController* mc = mp->midiController(num);
  
  int lastv = mp->lastValidHWCtrlState(chan, num);
  int curv = mp->hwCtrlState(chan, num);
  
  if(curv == CTRL_VAL_UNKNOWN)
  {
    // If no value has ever been set yet, use the current knob value 
    //  (or the controller's initial value?) to 'turn on' the controller.
    if(lastv == CTRL_VAL_UNKNOWN)
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
      
      //MidiPlayEvent ev(song->cpos(), outport, chan, ME_CONTROLLER, num, kiv);
      MidiPlayEvent ev(0, outport, chan, ME_CONTROLLER, num, kiv);
      audio->msgPlayMidiEvent(&ev);
    }
    else
    {
      //MidiPlayEvent ev(song->cpos(), outport, chan, ME_CONTROLLER, num, lastv);
      MidiPlayEvent ev(0, outport, chan, ME_CONTROLLER, num, lastv);
      audio->msgPlayMidiEvent(&ev);
    }
  }  
  else
  {
    if(mp->hwCtrlState(chan, num) != CTRL_VAL_UNKNOWN)
      audio->msgSetHwCtrlState(mp, chan, num, CTRL_VAL_UNKNOWN);
  }
  song->update(SC_MIDI_CONTROLLER);
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
        int channel  = ((MidiTrack*)track)->outChannel();
        MidiPort* mp = &midiPorts[((MidiTrack*)track)->outPort()];
        MidiCtrlValListList* mc = mp->controller();
        ciMidiCtrlValList icl;
        
          MidiController* ctrl = mp->midiController(CTRL_VOLUME);
          int nvolume = mp->hwCtrlState(channel, CTRL_VOLUME);
          if(nvolume == CTRL_VAL_UNKNOWN)
          {
            //if(nvolume != volume) 
            //{
              // DoubleLabel ignores the value if already set...
              sl->setValue(sl->off() - 1.0);
              //volume = nvolume;
            //}  
            volume = CTRL_VAL_UNKNOWN;
            nvolume = mp->lastValidHWCtrlState(channel, CTRL_VOLUME);
            //if(nvolume != volume) 
            if(nvolume != CTRL_VAL_UNKNOWN)
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
                  double v = -fast_log10(float(127*127)/float(ivol*ivol))*20.0;
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
          ctrl = mp->midiController(CTRL_PANPOT);
          int npan = mp->hwCtrlState(channel, CTRL_PANPOT);
          if(npan == CTRL_VAL_UNKNOWN)
          {
            // DoubleLabel ignores the value if already set...
            //if(npan != pan) 
            //{
              gcon->dl->setValue(gcon->dl->off() - 1.0);
              //pan = npan;
            //}
            pan = CTRL_VAL_UNKNOWN;
            npan = mp->lastValidHWCtrlState(channel, CTRL_PANPOT);
            if(npan != CTRL_VAL_UNKNOWN)
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
              
              
        icl = mc->find(channel, CTRL_VARIATION_SEND);
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
          ctrl = mp->midiController(CTRL_VARIATION_SEND);
          int nvariSend = icl->second->hwVal();
          if(nvariSend == CTRL_VAL_UNKNOWN)
          {
            // DoubleLabel ignores the value if already set...
            //if(nvariSend != variSend) 
            //{
              gcon->dl->setValue(gcon->dl->off() - 1.0);
              //variSend = nvariSend;
            //}
            variSend = CTRL_VAL_UNKNOWN;
            nvariSend = mp->lastValidHWCtrlState(channel, CTRL_VARIATION_SEND);
            if(nvariSend != CTRL_VAL_UNKNOWN)
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
        
        icl = mc->find(channel, CTRL_REVERB_SEND);
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
          ctrl = mp->midiController(CTRL_REVERB_SEND);
          int nreverbSend = icl->second->hwVal();
          if(nreverbSend == CTRL_VAL_UNKNOWN)
          {
            // DoubleLabel ignores the value if already set...
            //if(nreverbSend != reverbSend) 
            //{
              gcon->dl->setValue(gcon->dl->off() - 1.0);
              //reverbSend = nreverbSend;
            //}
            reverbSend = CTRL_VAL_UNKNOWN;
            nreverbSend = mp->lastValidHWCtrlState(channel, CTRL_REVERB_SEND);
            if(nreverbSend != CTRL_VAL_UNKNOWN)
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
        
        icl = mc->find(channel, CTRL_CHORUS_SEND);
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
          ctrl = mp->midiController(CTRL_CHORUS_SEND);
          int nchorusSend = icl->second->hwVal();
          if(nchorusSend == CTRL_VAL_UNKNOWN)
          {
            // DoubleLabel ignores the value if already set...
            //if(nchorusSend != chorusSend) 
            //{
              gcon->dl->setValue(gcon->dl->off() - 1.0);
              //chorusSend = nchorusSend;
            //}
            chorusSend = CTRL_VAL_UNKNOWN;
            nchorusSend = mp->lastValidHWCtrlState(channel, CTRL_CHORUS_SEND);
            if(nchorusSend != CTRL_VAL_UNKNOWN)
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
      
      MidiTrack* t = (MidiTrack*) track;
      int port     = t->outPort();
      
      int chan  = t->outChannel();
      MidiPort* mp = &midiPorts[port];
      MidiController* mctl = mp->midiController(num);
      if((val < mctl->minVal()) || (val > mctl->maxVal()))
      {
        if(mp->hwCtrlState(chan, num) != CTRL_VAL_UNKNOWN)
          audio->msgSetHwCtrlState(mp, chan, num, CTRL_VAL_UNKNOWN);
      }  
      else
      {
        val += mctl->bias();
        
        int tick     = song->cpos();
        
        MidiPlayEvent ev(tick, port, chan, ME_CONTROLLER, num, val);
        
        audio->msgPlayMidiEvent(&ev);
      }  
      song->update(SC_MIDI_CONTROLLER);
    }

//---------------------------------------------------------
//   volLabelChanged
//---------------------------------------------------------

void MidiStrip::volLabelChanged(double val)
      {
      val = sqrt( float(127*127) / pow(10.0, -val/20.0) );
      
      ctrlChanged(CTRL_VOLUME, lrint(val));
      
      }
      
//---------------------------------------------------------
//   setVolume
//---------------------------------------------------------

void MidiStrip::setVolume(double val)
      {
      
// printf("Vol %d\n", lrint(val));
      ctrlChanged(CTRL_VOLUME, lrint(val));
      }
      
//---------------------------------------------------------
//   setPan
//---------------------------------------------------------

void MidiStrip::setPan(double val)
      {
      
      ctrlChanged(CTRL_PANPOT, lrint(val));
      }

//---------------------------------------------------------
//   setVariSend
//---------------------------------------------------------

void MidiStrip::setVariSend(double val)
      {
      ctrlChanged(CTRL_VARIATION_SEND, lrint(val));
      }
      
//---------------------------------------------------------
//   setChorusSend
//---------------------------------------------------------

void MidiStrip::setChorusSend(double val)
      {
      ctrlChanged(CTRL_CHORUS_SEND, lrint(val));
      }
      
//---------------------------------------------------------
//   setReverbSend
//---------------------------------------------------------

void MidiStrip::setReverbSend(double val)
      {
      ctrlChanged(CTRL_REVERB_SEND, lrint(val));
      }
      
//---------------------------------------------------------
//   updateOffState
//---------------------------------------------------------

void MidiStrip::updateOffState() // Ripped from AudioStrip, hehh(mg)
      {
      bool val = !track->off();
      slider->setEnabled(val);
      //KNOB* gcon = &controller[KNOB_PAN]; // TODO: Pan ctrl
      //gcon->setOn(val);
      label->setEnabled(val);
      if (record)
            record->setEnabled(val);
      if (solo)
            solo->setEnabled(val);
      if (mute)
            mute->setEnabled(val);
      }

//---------------------------------------------------------
//   routingPopupMenuActivated
//---------------------------------------------------------

void MidiStrip::routingPopupMenuActivated(int n)
{
  if(gRoutingPopupMenuMaster != this || !track || !track->isMidiTrack())
    return;
  muse->routingPopupMenuActivated(track, n);
}

//---------------------------------------------------------
//   iRoutePressed
//---------------------------------------------------------

void MidiStrip::iRoutePressed()
{
  if(!track || !track->isMidiTrack())
    return;
  
  //song->chooseMidiRoutes(iR, (MidiTrack*)track, false);
  PopupMenu* pup = muse->prepareRoutingPopupMenu(track, false);
  if(!pup)
    return;
  
  //pup->disconnect();
  gRoutingPopupMenuMaster = this;
  connect(pup, SIGNAL(activated(int)), SLOT(routingPopupMenuActivated(int)));
  connect(pup, SIGNAL(aboutToHide()), muse, SLOT(routingPopupMenuAboutToHide()));
  pup->popup(QCursor::pos(), 0);
  iR->setDown(false);     
  return;
  
  /*
  RouteList* irl = track->inRoutes();
  //Route dst(track, -1);

  QPopupMenu* pup = new QPopupMenu(iR);
  pup->setCheckable(true);
  
  int gid = 0;
  
  //MidiInPortList* tl = song->midiInPorts();
  //for(iMidiInPort i = tl->begin();i != tl->end(); ++i) 
  for(int i = 0; i < MIDI_PORTS; ++i)
  {
    //MidiInPort* track = *i;
    // NOTE: Could possibly list all devices, bypassing ports, but no, let's stick wth ports.
    MidiPort* mp = &midiPorts[i];
    MidiDevice* md = mp->device();
    if(!md)
      continue;
    
    if(!(md->rwFlags() & 2))
      continue;
      
    //printf("MidiStrip::iRoutePressed adding submenu portnum:%d\n", i);
    
    //QMenu* m = menu->addMenu(track->name());
    QPopupMenu* subp = new QPopupMenu(iR);
    
    for(int ch = 0; ch < MIDI_CHANNELS; ++ch) 
    {
      //QAction* a = m->addAction(QString("Channel %1").arg(ch+1));
      //subp->insertItem(QT_TR_NOOP(QString("Channel %1").arg(ch+1)), i * MIDI_CHANNELS + ch);
      gid = i * MIDI_CHANNELS + ch;
      
      //printf("MidiStrip::iRoutePressed inserting gid:%d\n", gid);
      
      subp->insertItem(QString("Channel %1").arg(ch+1), gid);
      //a->setCheckable(true);
      //Route src(track, ch, RouteNode::TRACK);
      //Route src(md, ch);
      //Route r = Route(src, dst);
      //a->setData(QVariant::fromValue(r));
      //a->setChecked(rl->indexOf(r) != -1);
      Route srcRoute(md, ch);
      for(iRoute ir = irl->begin(); ir != irl->end(); ++ir) 
      {
        //if(*ir == dst) 
        if(*ir == srcRoute) 
        {
          subp->setItemChecked(gid, true);
          break;
        }
      }
    }
    pup->insertItem(QT_TR_NOOP(md->name()), subp);
  }
      
      int n = pup->exec(QCursor::pos());
      delete pup;
      if (n != -1) 
      {
            int mdidx = n / MIDI_CHANNELS;
            int ch = n % MIDI_CHANNELS;
            
            //if(debugMsg)
              printf("MidiStrip::iRoutePressed mdidx:%d ch:%d\n", mdidx, ch);
              
            MidiPort* mp = &midiPorts[mdidx];
            MidiDevice* md = mp->device();
            if(!md)
              return;
            
            if(!(md->rwFlags() & 2))
              return;
              
            
            //QString s(pup->text(n));
            //QT_TR_NOOP(md->name())
            
            //Route srcRoute(s, false, -1);
            Route srcRoute(md, ch);
            //Route srcRoute(md, -1);
            //Route dstRoute(track, -1);
            Route dstRoute(track, ch);

            //if (track->type() == Track::AUDIO_INPUT)
            //      srcRoute.channel = dstRoute.channel = n & 0xf;
            iRoute iir = irl->begin();
            for (; iir != irl->end(); ++iir) {
                  if (*iir == srcRoute)
                        break;
                  }
            if (iir != irl->end()) {
                  // disconnect
                  printf("MidiStrip::iRoutePressed removing route src device name: %s dst track name: %s\n", md->name().latin1(), track->name().latin1());
                  audio->msgRemoveRoute(srcRoute, dstRoute);
                  }
            else {
                  // connect
                  printf("MidiStrip::iRoutePressed adding route src device name: %s dst track name: %s\n", md->name().latin1(), track->name().latin1());
                  audio->msgAddRoute(srcRoute, dstRoute);
                  }
            printf("MidiStrip::iRoutePressed calling msgUpdateSoloStates\n");
            audio->msgUpdateSoloStates();
            printf("MidiStrip::iRoutePressed calling song->update\n");
            song->update(SC_ROUTE);
      }
      //delete pup;
      iR->setDown(false);     // pup->exec() catches mouse release event
      printf("MidiStrip::iRoutePressed end\n");
      */
      
}

//---------------------------------------------------------
//   oRoutePressed
//---------------------------------------------------------

void MidiStrip::oRoutePressed()
      {
  if(!track || !track->isMidiTrack())
    return;
  
  //song->chooseMidiRoutes(oR, (MidiTrack*)track, true);
  PopupMenu* pup = muse->prepareRoutingPopupMenu(track, true);
  if(!pup)
    return;
  
  //pup->disconnect();
  gRoutingPopupMenuMaster = this;
  connect(pup, SIGNAL(activated(int)), SLOT(routingPopupMenuActivated(int)));
  connect(pup, SIGNAL(aboutToHide()), muse, SLOT(routingPopupMenuAboutToHide()));
  pup->popup(QCursor::pos(), 0);
  oR->setDown(false);     
  return;
      
      /*
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
                              Route dst(*ip, true, i);
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
            Route dstRoute(s, true, -1);

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
      */
      
      
      }


