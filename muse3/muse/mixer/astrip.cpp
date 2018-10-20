//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: astrip.cpp,v 1.23.2.17 2009/11/16 01:55:55 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011-2016 Tim E. Real (terminator356 on sourceforge)
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

#include <stdio.h>
#include <stdlib.h>

#include <QLayout>
#include <QVBoxLayout>
#include <QApplication>
#include <QToolButton>
#include <QComboBox>
#include <QToolTip>
#include <QTimer>
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
#include "midi.h"
#include "song.h"
#include "slider.h"
#include "compact_knob.h"
#include "compact_slider.h"
#include "combobox.h"
#include "meter.h"
#include "astrip.h"
#include "track.h"
#include "synth.h"
#include "doublelabel.h"
#include "rack.h"
#include "node.h"
#include "amixer.h"
#include "icons.h"
#include "gconfig.h"
#include "pixmap_button.h"
#include "menutitleitem.h"
#include "routepopup.h"
#include "ctrl.h"
#include "utils.h"
#include "muse_math.h"
#include "operations.h"

// For debugging output: Uncomment the fprintf section.
#define DEBUG_AUDIO_STRIP(dev, format, args...)  //fprintf(dev, format, ##args);


namespace MusEGui {
  
const double AudioStrip::volSliderStep =  0.5;
const double AudioStrip::volSliderMax  = 10.0;
const int    AudioStrip::volSliderPrec =    1;

const double AudioStrip::auxSliderStep =  1.0;
const double AudioStrip::auxSliderMax  = 10.0;
const int    AudioStrip::auxSliderPrec =    0;

const double AudioStrip::gainSliderStep = 0.1;
const double AudioStrip::gainSliderMin  = 0.5;
const double AudioStrip::gainSliderMax = 20.0;
const int    AudioStrip::gainSliderPrec =   1;

const int AudioStrip::xMarginHorSlider = 1;
const int AudioStrip::yMarginHorSlider = 1;
const int AudioStrip::upperRackSpacerHeight = 2;
const int AudioStrip::rackFrameWidth = 1;

//---------------------------------------------------------
//   AudioComponentRack
//---------------------------------------------------------

AudioComponentRack::AudioComponentRack(MusECore::AudioTrack* track, int id, bool manageAuxs, QWidget* parent, Qt::WindowFlags f) 
  : ComponentRack(id, parent, f), _track(track), _manageAuxs(manageAuxs)
{ 
  
}

void AudioComponentRack::newComponent( ComponentDescriptor* desc, const ComponentWidget& before )
{
  double min = 0.0;
  double max = 0.0;
  double val = 0.0;
  int prec = 0.0;
  double step = 0.0;
  bool showval = MusEGlobal::config.showControlValues;;
  
  switch(desc->_componentType)
  {
    case aStripAuxComponent:
    {
      val = _track->auxSend(desc->_index);
      //if(val == 0.0)
      if(val < MusEGlobal::config.minSlider)
        val = MusEGlobal::config.minSlider;
      else
      {
        val = muse_val2dbr(val);
        if(val < MusEGlobal::config.minSlider)
          val = MusEGlobal::config.minSlider;
      }
      min = MusEGlobal::config.minSlider;
      max = AudioStrip::auxSliderMax;
      prec = AudioStrip::auxSliderPrec;
      step = AudioStrip::auxSliderStep;
      
      // Are there any Aux Track routing paths to this track? Then we cannot process aux for this track! 
      // Hate to do this, but as a quick visual reminder, seems most logical to disable Aux knobs and labels. 
      desc->_enabled = _track->auxRefCount() == 0;
      
      if(!desc->_color.isValid())
        desc->_color = MusEGlobal::config.auxSliderColor;
      
      if(desc->_label.isEmpty())
      {
        // the thought was to acquire the correct Aux name for each Aux
        // now they are only called Aux1, Aux2, which isn't too usable.
        desc->_label = ((MusECore::AudioAux*)(MusEGlobal::song->auxs()->at(desc->_index)))->auxName();
        if (desc->_label.length() > 8) { // shorten name
            desc->_label = desc->_label.mid(0,8) + ".";
        }
      }
      if(desc->_toolTipText.isEmpty())
        desc->_toolTipText = tr("Aux send level (dB)");
    }
    break;
    
    case controllerComponent:
    {
      MusECore::iCtrlList ic = _track->controller()->find(desc->_index);
      if(ic == _track->controller()->end())
        return;
      MusECore::CtrlList* cl = ic->second;
      val = _track->pluginCtrlVal(desc->_index);
      cl->range(&min, &max);
      prec = 2;
      step = 0.01;
      
      if(desc->_label.isEmpty())
      {
        switch(desc->_index)
        {
          case MusECore::AC_VOLUME:
            desc->_label = tr("Vol");
          break;
          
          case MusECore::AC_PAN:
            desc->_label = tr("Pan");
          break;

          case MusECore::AC_MUTE:
            desc->_label = tr("Mute");
          break;
          
          default:
            desc->_label = cl->name();
          break;
        }
      }
      
      if(desc->_toolTipText.isEmpty())
      {
        switch(desc->_index)
        {
          case MusECore::AC_VOLUME:
            desc->_toolTipText = tr("Volume/gain");
          break;
          
          case MusECore::AC_PAN:
            desc->_toolTipText = tr("Panorama/Balance");
          break;

          case MusECore::AC_MUTE:
            desc->_toolTipText = tr("Mute");
          break;

          default:
            desc->_toolTipText = cl->name();
          break;
        }
      }
      
      if(!desc->_color.isValid())
      {
        switch(desc->_index)
        {
          case MusECore::AC_PAN:
            desc->_color = MusEGlobal::config.panSliderColor;
          break;
          
          default:
            desc->_color = MusEGlobal::config.audioControllerSliderDefaultColor;
          break;
        }
      }
    }
    break;
    
    case propertyComponent:
    {
      switch(desc->_index)
      {
        case aStripGainProperty:
        {
          val = _track->gain();
          min = AudioStrip::gainSliderMin;
          max = AudioStrip::gainSliderMax;
          prec = AudioStrip::gainSliderPrec;
          step = AudioStrip::gainSliderStep;
          if(desc->_label.isEmpty())
            desc->_label = tr("Gain");
          if(desc->_toolTipText.isEmpty())
            desc->_toolTipText = tr("Calibration gain");
          if(!desc->_color.isValid())
            desc->_color = MusEGlobal::config.gainSliderColor;
        }
        break;
        
        default:
          if(!desc->_color.isValid())
            desc->_color = MusEGlobal::config.audioPropertySliderDefaultColor;
        break;
      }
    }
    break;
  }  

  switch(desc->_widgetType)
  {
    case CompactKnobComponentWidget:
    {
      CompactKnobComponentDescriptor* d = static_cast<CompactKnobComponentDescriptor*>(desc);
      d->_min = min;
      d->_max = max;
      d->_precision = prec;
      d->_step = step;
      d->_initVal = val;
      d->_showValue = showval;
      if(!d->_color.isValid())
        d->_color = MusEGlobal::config.sliderDefaultColor;

      // Adds a component. Creates a new component using the given desc values if the desc widget is not given.
      // Connects known widget types' signals to slots.
      newComponentWidget(d, before);

      // Handle special slots for audio strip.
      switch(desc->_componentType)
      {
        case aStripAuxComponent:
        {
          if(d->_compactKnob->specialValueText().isEmpty())
          {
            d->_compactKnob->setSpecialValueText(QString('-') + QString(QChar(0x221e))); // The infinity character
          }

          connect(d->_compactKnob, SIGNAL(valueStateChanged(double,bool,int,int)), SLOT(auxChanged(double,bool,int,int)));
          connect(d->_compactKnob, SIGNAL(sliderMoved(double,int,bool)), SLOT(auxMoved(double,int,bool)));
          connect(d->_compactKnob, SIGNAL(sliderPressed(double, int)), SLOT(auxPressed(double, int)));
          connect(d->_compactKnob, SIGNAL(sliderReleased(double, int)), SLOT(auxReleased(double, int)));
          connect(d->_compactKnob, SIGNAL(sliderRightClicked(QPoint,int)), SLOT(auxRightClicked(QPoint,int)));
        }
        break;
      }
    }
    break;

    case CompactSliderComponentWidget:
    {
      CompactSliderComponentDescriptor* d = static_cast<CompactSliderComponentDescriptor*>(desc);
      d->_min = min;
      d->_max = max;
      d->_precision = prec;
      d->_step = step;
      d->_initVal = val;
      d->_showValue = showval;
      if(!d->_color.isValid())
        d->_color = MusEGlobal::config.sliderDefaultColor;
      // Set the bar color the same.
      if(!d->_barColor.isValid())
        //d->_barColor = d->_color;
        d->_barColor = MusEGlobal::config.sliderBarDefaultColor;

      // Adds a component. Creates a new component using the given desc values if the desc widget is not given.
      // Connects known widget types' signals to slots.
      newComponentWidget(d, before);
      
      // Handle special slots for audio strip.
      switch(desc->_componentType)
      {
        case aStripAuxComponent:
        {
          if(d->_compactSlider->specialValueText().isEmpty())
          {
            d->_compactSlider->setSpecialValueText(QString('-') + QString(QChar(0x221e))); // The infinity character
          }
      
          connect(d->_compactSlider, SIGNAL(valueStateChanged(double,bool,int,int)), SLOT(auxChanged(double,bool,int,int)));
          connect(d->_compactSlider, SIGNAL(sliderMoved(double,int,bool)), SLOT(auxMoved(double,int,bool)));
          connect(d->_compactSlider, SIGNAL(sliderPressed(double, int)), SLOT(auxPressed(double, int)));
          connect(d->_compactSlider, SIGNAL(sliderReleased(double, int)), SLOT(auxReleased(double, int)));
          connect(d->_compactSlider, SIGNAL(sliderRightClicked(QPoint,int)), SLOT(auxRightClicked(QPoint,int)));
        }
        break;
      }  
    }
    break;
  }
}

void AudioComponentRack::scanControllerComponents()
{
  std::vector<iComponentWidget> to_be_erased;
  for(iComponentWidget ic = _components.begin(); ic != _components.end(); ++ic)
  {
    ComponentWidget& cw = *ic;
    if(!cw._widget)
      continue;
    
    switch(cw._componentType)
    {
      case controllerComponent:
      {
        MusECore::iCtrlList ictrl = _track->controller()->find(cw._index);
        if(ictrl == _track->controller()->end())
          to_be_erased.push_back(ic);
      }
      break;
    }
  }
  for(std::vector<iComponentWidget>::iterator i = to_be_erased.begin(); i != to_be_erased.end(); ++i)
  {
    iComponentWidget icw = *i;
    ComponentWidget& cw = *icw;
    DEBUG_AUDIO_STRIP(stderr, "AudioComponentRack::scanControllerComponents: deleting controller component index:%d\n", cw._index);
    if(cw._widget)
      delete cw._widget;
    _components.erase(icw);
  }
}

void AudioComponentRack::scanAuxComponents()
{
  std::vector<iComponentWidget> to_be_erased;
  for(iComponentWidget ic = _components.begin(); ic != _components.end(); ++ic)
  {
    ComponentWidget& cw = *ic;
    if(!cw._widget)
      continue;
    
    switch(cw._componentType)
    {
      case aStripAuxComponent:
      {
        // TODO: This is just brute-force deletion and recreation of all the auxs. 
        //       Make this more efficient by only removing what's necessary and updating/re-using the rest.
        to_be_erased.push_back(ic);
      }
      break;
    }
  }
  for(std::vector<iComponentWidget>::iterator i = to_be_erased.begin(); i != to_be_erased.end(); ++i)
  {
    iComponentWidget icw = *i;
    ComponentWidget& cw = *icw;
    DEBUG_AUDIO_STRIP(stderr, "AudioComponentRack::scanAuxComponents: deleting aux component index:%d\n", cw._index);
    if(cw._widget)
      delete cw._widget;
    _components.erase(icw);
  }
  
  // Add auxs, only if we want this rack to manage auxs.
  if(_manageAuxs)
  {
    int auxsSize = MusEGlobal::song->auxs()->size();
    if(_track->hasAuxSend()) 
    {
      for (int idx = 0; idx < auxsSize; ++idx) 
      {
        // the thought was to acquire the correct Aux name for each Aux
        // now they are only called Aux1, Aux2, which isn't too usable.
//         QString title = ((MusECore::AudioAux*)(MusEGlobal::song->auxs()->at(idx)))->auxName();
//         if (title.length() > 8) { // shorten name
//             title = title.mid(0,8) + ".";
//         }
        
        // Are there any Aux Track routing paths to this track? Then we cannot process aux for this track! 
        // Hate to do this, but as a quick visual reminder, seems most logical to disable Aux knobs and labels. 
//         const bool enable = _track->auxRefCount() == 0;
        
        if(MusEGlobal::config.preferKnobsVsSliders)
        {
          CompactKnobComponentDescriptor aux_desc
          (
            aStripAuxComponent,
            "MixerStripAudioAux",
            idx
          );
          DEBUG_AUDIO_STRIP(stderr, "AudioComponentRack::scanAuxComponents: adding aux component index:%d\n", idx);
          newComponent(&aux_desc);
        }
        else
        {
          CompactSliderComponentDescriptor aux_desc
          (
            aStripAuxComponent,
            "MixerStripAudioAux",
            idx
          );
          DEBUG_AUDIO_STRIP(stderr, "AudioComponentRack::scanAuxComponents: adding aux component index:%d\n", idx);
          newComponent(&aux_desc);
        }
      }
    }
  }
}

void AudioComponentRack::updateComponents()
{
  for(iComponentWidget ic = _components.begin(); ic != _components.end(); ++ic)
  {
    ComponentWidget& cw = *ic;
    if(!cw._widget)
      continue;
    
    switch(cw._componentType)
    {
      case controllerComponent:
      {
        // Inhibit the controller stream if control is currently pressed.
        // Note _pressed operates differently than simply checking if the control is pressed!
        if(cw._pressed) 
          continue;
        const double val = _track->pluginCtrlVal(cw._index);
        setComponentValue(cw, val); // Signals blocked. Redundant ignored.
      }
      break;
      
      case propertyComponent:
      {
        switch(cw._index)
        {
          case aStripGainProperty:
          {
            const double val = _track->gain();
            setComponentValue(cw, val);  // Signals blocked. Redundant ignored.
          }
          break;
        }
      }
      break;
      
      case aStripAuxComponent:
      {
        double val = _track->auxSend(cw._index);
        if(val == 0.0)
          val = MusEGlobal::config.minSlider;
        else
        {
          val = muse_val2dbr(val);
          if(val < MusEGlobal::config.minSlider)
            val = MusEGlobal::config.minSlider;
        }
        setComponentValue(cw, val);  // Signals blocked. Redundant ignored.
      }
      break;
    }
  }
}

void AudioComponentRack::setAuxEnabled(bool enable)
{
  for(iComponentWidget ic = _components.begin(); ic != _components.end(); ++ic)
  {
    ComponentWidget& cw = *ic;
    switch(cw._componentType)
    {
      case aStripAuxComponent:
        setComponentEnabled(cw, enable);
      break;
    }
  }
}


void AudioComponentRack::controllerChanged(double val, bool off, int id, int scrollMode)
{
  DEBUG_AUDIO_STRIP(stderr, "AudioComponentRack::controllerChanged id:%d val:%.20f scrollMode:%d\n", id, val, scrollMode);
  // Hack: Be sure to ignore in ScrDirect mode since we get both pressed AND changed signals. 
  // ScrDirect mode is one-time only on press with modifier.
  if(scrollMode != SliderBase::ScrDirect) 
    _track->recordAutomation(id, val);
  _track->setParam(id, val);            // Schedules a timed control change.
  //_track->setPluginCtrlVal(id, val);  // TODO Try this instead. setParam gives a slight jump at release, in tracking off temp mode.
  _track->enableController(id, false);

  emit componentChanged(controllerComponent, val, off, id, scrollMode);
}

void AudioComponentRack::controllerMoved(double val, int id, bool shift_pressed)
{
  DEBUG_AUDIO_STRIP(stderr, "AudioComponentRack::controllerMoved id:%d val:%.20f\n", id, val);
  emit componentMoved(controllerComponent, val, id, shift_pressed);
}

void AudioComponentRack::controllerPressed(double v, int id)
{
  DEBUG_AUDIO_STRIP(stderr, "AudioComponentRack::controllerPressed id:%d\n", id);
  double val = 0.0;
  iComponentWidget ic = _components.find(controllerComponent, -1, id);
  if(ic != _components.end())
  {
    ComponentWidget& cw = *ic;
    cw._pressed = true;
    val = componentValue(cw);
    DEBUG_AUDIO_STRIP(stderr, "    val:%.20f\n", val);
  }
  _track->startAutoRecord(id, val);
  _track->setPluginCtrlVal(id, val);
  //_track->setParam(id, val);   // Schedules a timed control change. // TODO Try this instead
  DEBUG_AUDIO_STRIP(stderr, "    calling enableController(false)\n");
  _track->enableController(id, false);

  emit componentPressed(controllerComponent, v, id);
}

void AudioComponentRack::controllerReleased(double v, int id)
{
  DEBUG_AUDIO_STRIP(stderr, "AudioComponentRack::controllerReleased id:%d\n", id);
  MusECore::AutomationType at = _track->automationType();
  double val = 0.0;
  iComponentWidget ic = _components.find(controllerComponent, -1, id);
  if(ic != _components.end())
  {
    ComponentWidget& cw = *ic;
    val = componentValue(cw);
    DEBUG_AUDIO_STRIP(stderr, "    val:%.20f\n", val);
    cw._pressed = false;
  }
  _track->stopAutoRecord(id, val);
  if(at == MusECore::AUTO_OFF || at == MusECore::AUTO_TOUCH)
  {
    DEBUG_AUDIO_STRIP(stderr, "    calling enableController(true)\n");
    _track->enableController(id, true);
  }

  emit componentReleased(controllerComponent, v, id);
}

void AudioComponentRack::controllerRightClicked(QPoint p, int id)
{
  DEBUG_AUDIO_STRIP(stderr, "AudioComponentRack::controllerRightClicked id:%d\n", id);
  MusEGlobal::song->execAutomationCtlPopup(_track, p, id);
}


void AudioComponentRack::propertyChanged(double val, bool off, int id, int scrollMode)
{
  DEBUG_AUDIO_STRIP(stderr, "AudioComponentRack::propertyChanged id:%d val:%.20f\n", id, val);
  switch(id)
  {
    case aStripGainProperty:
      if(_track->gain() != val)
        _track->setGain(val); // FIXME: Realtime safe?
    break;
  }

  emit componentChanged(propertyComponent, val, off, id, scrollMode);
}

void AudioComponentRack::propertyMoved(double val, int id, bool shift_pressed)
{
  DEBUG_AUDIO_STRIP(stderr, "AudioComponentRack::propertyMoved id:%d val:%.20f\n", id, val);
  emit componentMoved(propertyComponent, val, id, shift_pressed);
}

void AudioComponentRack::propertyPressed(double val, int id)
{
  emit componentPressed(propertyComponent, val, id);
}

void AudioComponentRack::propertyReleased(double val, int id)
{
  emit componentReleased(propertyComponent, val, id);
}

void AudioComponentRack::propertyRightClicked(QPoint, int)
{
  
}

void AudioComponentRack::auxChanged(double val, bool off, int id, int scrollMode)
{
  DEBUG_AUDIO_STRIP(stderr, "AudioComponentRack::auxChanged id:%d val:%.20f\n", id, val);
  double vol;
  if (val <= MusEGlobal::config.minSlider)
    vol = 0.0;
  else
    vol = muse_db2val(val);
  MusEGlobal::audio->msgSetAux(_track, id, vol);

  emit componentChanged(aStripAuxComponent, val, off, id, scrollMode);
}

void AudioComponentRack::auxMoved(double val, int id, bool shift_pressed)
{
  DEBUG_AUDIO_STRIP(stderr, "AudioComponentRack::auxMoved id:%d val:%.20f\n", id, val);
  emit componentMoved(aStripAuxComponent, val, id, shift_pressed);
}

void AudioComponentRack::auxPressed(double val, int id)
{
  emit componentPressed(aStripAuxComponent, val, id);
}

void AudioComponentRack::auxReleased(double val, int id)
{
  emit componentReleased(aStripAuxComponent, val, id);
}

void AudioComponentRack::auxRightClicked(QPoint, int)
{
  
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void AudioComponentRack::songChanged(MusECore::SongChangedStruct_t flags)
{
  // Scan controllers.
  if(flags._flags & (SC_RACK | SC_AUDIO_CONTROLLER_LIST))
  {
    scanControllerComponents();
  }
  
  // Take care of scanning aux before setting aux enabled below.
  if(flags._flags & SC_AUX) 
  {
    scanAuxComponents();
  }
  
  if(flags._flags & SC_ROUTE) {
        // Are there any Aux Track routing paths to this track? Then we cannot process aux for this track! 
        // Hate to do this, but as a quick visual reminder, seems most logical to disable Aux knobs and labels. 
        setAuxEnabled(_track->auxRefCount() == 0);
      }
}

//---------------------------------------------------------
//   configChanged
//   Catch when label font, or configuration min slider and meter values change, or viewable tracks etc.
//---------------------------------------------------------

void AudioComponentRack::configChanged()    
{ 
  // Handle font changes etc.
  ComponentRack::configChanged();
  
  for(iComponentWidget ic = _components.begin(); ic != _components.end(); ++ic)
  {
    ComponentWidget& cw = *ic;

    // Whether to show values along with labels for certain controls.
    setComponentShowValue(cw, MusEGlobal::config.showControlValues);

    switch(cw._componentType)
    {
      // Special for Aux controls.
      case aStripAuxComponent:
        // Adjust aux minimum value.
        setComponentRange(cw, MusEGlobal::config.minSlider, AudioStrip::auxSliderMax, true, AudioStrip::auxSliderStep);
      break;
    }
  }
  setComponentColors();
}

//---------------------------------------------------------
//   setComponentColors
//---------------------------------------------------------

void AudioComponentRack::setComponentColors()
{ 
  for(ciComponentWidget ic = _components.begin(); ic != _components.end(); ++ic)
  {
    const ComponentWidget& cw = *ic;
    if(!cw._widget)
      continue;
    
    QColor color = MusEGlobal::config.sliderDefaultColor;
    switch(cw._componentType)
    {
      case aStripAuxComponent:
        color = MusEGlobal::config.auxSliderColor;
      break;
      
      case controllerComponent:
      {
        switch(cw._index)
        {
          case MusECore::AC_PAN:
            color = MusEGlobal::config.panSliderColor;
          break;
          
          default:
            color = MusEGlobal::config.audioControllerSliderDefaultColor;
          break;
        }
      }
      break;
      
      case propertyComponent:
      {
        switch(cw._index)
        {
          case aStripGainProperty:
            color = MusEGlobal::config.gainSliderColor;
          break;
          
          default:
            color = MusEGlobal::config.audioPropertySliderDefaultColor;
          break;
        }
      }
      break;
    }  

    switch(cw._widgetType)
    {
      case CompactKnobComponentWidget:
      {
        CompactKnob* w = static_cast<CompactKnob*>(cw._widget);
        w->setFaceColor(color);
      }
      break;

      case CompactSliderComponentWidget:
      {
        CompactSlider* w = static_cast<CompactSlider*>(cw._widget);
        w->setBorderColor(color);
        //w->setBarColor(color);
        w->setBarColor(MusEGlobal::config.sliderBarDefaultColor);
      }
      break;
    }  
  }
}

//---------------------------------------------------------
//   AudioStrip
//---------------------------------------------------------
  
//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void AudioStrip::heartBeat()
{
   const int tch = track->channels();
   for (int ch = 0; ch < tch; ++ch) {
      if (meter[ch]) {
         meter[ch]->setVal(track->meter(ch), track->peak(ch), false);
      }
      if(_clipperLabel[ch])
      {
        _clipperLabel[ch]->setVal(track->peak(ch));
        _clipperLabel[ch]->setClipped(track->isClipped(ch));
      }
   }
   updateVolume();
   _upperRack->updateComponents();
   _infoRack->updateComponents();
   _lowerRack->updateComponents();

//    if(_recMonitor && _recMonitor->isChecked() && MusEGlobal::blinkTimerPhase != _recMonitor->blinkPhase())
//      _recMonitor->setBlinkPhase(MusEGlobal::blinkTimerPhase);

   Strip::heartBeat();
}

void AudioStrip::updateRackSizes(bool upper, bool lower)
{
//   const QFontMetrics fm = fontMetrics();
  if(upper)
  {
    // Make room for 3 CompactSliders and one CompactPatchEdit.
    // TODO: Add the instrument select label height!

// //     const int csh = CompactSlider::getMinimumSizeHint(fm,
// //                                             Qt::Horizontal, 
// //                                             CompactSlider::None, 
// //                                             xMarginHorSlider, yMarginHorSlider).height();
// //     const int cpeh = CompactPatchEdit::getMinimumSizeHint(fm, 
// //                                             Qt::Horizontal, 
// //                                             CompactSlider::None, 
// //                                             xMarginHorSlider, yMarginHorSlider).height();
// //     const int ilh = _instrLabel->sizeHint().height();
//     
// //     DEBUG_AUDIO_STRIP(stderr, "MidiStrip::updateRackSizes: CompactSlider h:%d CompactPatchEdit h:%d instrLabel h:%d upper frame w:%d \n", 
// //                      csh, cpeh, ilh, _upperRack->frameWidth());
    
//     _upperRack->setMinimumHeight(
//       3 * CompactSlider::getMinimumSizeHint(fm,
//                                             Qt::Horizontal, 
//                                             CompactSlider::None, 
//                                             xMarginHorSlider, yMarginHorSlider).height() + 
//       1 * CompactPatchEdit::getMinimumSizeHint(fm, 
//                                             Qt::Horizontal, 
//                                             CompactSlider::None, 
//                                             xMarginHorSlider, yMarginHorSlider).height() +
//       upperRackSpacerHeight +
//       
//       _instrLabel->sizeHint().height() +
//       
//       2 * rackFrameWidth);
  }
  if(lower)
  {
    // Make room for 1 CompactSlider (Pan, so far).
    
    //DEBUG_AUDIO_STRIP(stderr, "MidiStrip::updateRackSizes: lower frame w:%d \n", _lowerRack->frameWidth());
    
//     _lowerRack->setMinimumHeight(
//       1 * CompactSlider::getMinimumSizeHint(fm, 
//                                             Qt::Horizontal, 
//                                             CompactSlider::None, 
//                                             xMarginHorSlider, yMarginHorSlider).height() + 
//       2 * rackFrameWidth);
  }
}

//---------------------------------------------------------
//   configChanged
//   Catch when label font, or configuration min slider and meter values change, or viewable tracks etc.
//---------------------------------------------------------

void AudioStrip::configChanged()    
{ 
  // Detect when knobs are preferred and rebuild.
  if(_preferKnobs != MusEGlobal::config.preferKnobsVsSliders)
  {
    _preferKnobs = MusEGlobal::config.preferKnobsVsSliders;
    // Rebuild the strip components.
    buildStrip();
    // Now set up all tabbing on the strip.
    // Don't bother if the strip is part of the mixer (not embedded), 
    //  the non-embedding parent (mixer) should set up all the tabs and make this call.
    if(isEmbedded())
      setupComponentTabbing();
  }

  // Set the whole strip's font, except for the label.
  if(font() != MusEGlobal::config.fonts[1])
  {
    setFont(MusEGlobal::config.fonts[1]);
    DEBUG_AUDIO_STRIP(stderr, "AudioStrip::configChanged changing font: current size:%d\n", font().pointSize());
    setStyleSheet(MusECore::font2StyleSheet(MusEGlobal::config.fonts[1]));
  }
  
  // Set the strip label's font.
  setLabelText();        

  slider->setFillColor(MusEGlobal::config.audioVolumeSliderColor);
  // Adjust minimum volume slider and label values.
  slider->setRange(MusEGlobal::config.minSlider, volSliderMax, volSliderStep);
  slider->setScale(MusEGlobal::config.minSlider, volSliderMax, 6.0, false);

  sl->setRange(MusEGlobal::config.minSlider, volSliderMax);
  sl->setOff(MusEGlobal::config.minSlider);
  // Enable special hack for line edits.
  if(sl->enableStyleHack() != MusEGlobal::config.lineEditStyleHack)
    sl->setEnableStyleHack(MusEGlobal::config.lineEditStyleHack);

  _upperRack->configChanged();
  _infoRack->configChanged();
  _lowerRack->configChanged();
  
  // Adjust minimum meter values, and colours.
  for(int c = 0; c < channel; ++c) 
  {
    meter[c]->setRange(MusEGlobal::config.minMeter, volSliderMax);
    meter[c]->setPrimaryColor(MusEGlobal::config.audioMeterPrimaryColor);
    meter[c]->setRefreshRate(MusEGlobal::config.guiRefresh);
  }

  // If smart focus is on redirect strip focus to slider label.
//   if(MusEGlobal::config.smartFocus)
//     setFocusProxy(sl);
//   else
//     setFocusProxy(0);
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void AudioStrip::songChanged(MusECore::SongChangedStruct_t val)
      {
      MusECore::AudioTrack* src = static_cast<MusECore::AudioTrack*>(track);

      // Do channels before MusEGlobal::config...
      if (val._flags & SC_CHANNELS)
        updateChannels();
      
      // Catch when label font, or configuration min slider and meter values change.
      if (val._flags & SC_CONFIG)
      {
        // So far only 1 instance of sending SC_CONFIG in the entire app, in instrument editor when a new instrument is saved.
      }
      
      if (mute && (val._flags & SC_MUTE)) {      // mute && off
            mute->blockSignals(true);
            mute->setChecked(src->mute());
            mute->blockSignals(false);
            updateMuteIcon();
            updateOffState();
            }
      if (solo && (val._flags & (SC_SOLO | SC_ROUTE))) {
            solo->blockSignals(true);
            solo->setChecked(track->solo());
            solo->blockSignals(false);
            solo->setIconSetB(track->internalSolo());
            updateMuteIcon();
            }
      if (val._flags & SC_RECFLAG)
      {
            setRecordFlag(track->recordFlag());
      }
      if (val._flags & SC_TRACK_MODIFIED)
      {
            setLabelText();
      }      
      //if (val & SC_CHANNELS)
      //      updateChannels();
      if (val._flags & SC_ROUTE) {
            updateRouteButtons();
            if (pre) {
                  pre->blockSignals(true);
                  pre->setChecked(src->prefader());
                  pre->blockSignals(false);
                  }
          }

      if(val._flags & SC_TRACK_REC_MONITOR)
      {
        // Set record monitor.
        if(_recMonitor) // && (_recMonitor->isChecked() != track->recMonitor()))
        {
          _recMonitor->blockSignals(true);
          _recMonitor->setChecked(track->recMonitor());
          _recMonitor->blockSignals(false);
        }
      }
      // Are there any Aux Track routing paths to this track? Then we cannot process aux for this track!
      // Hate to do this, but as a quick visual reminder, seems most logical to disable Aux knobs and labels. 
      _upperRack->songChanged(val);
      _infoRack->songChanged(val);
      _lowerRack->songChanged(val);

      if (autoType && (val._flags & SC_AUTOMATION)) {
            autoType->blockSignals(true);
            autoType->setCurrentItem(track->automationType());
            QPalette palette;
            //QLinearGradient gradient(autoType->geometry().topLeft(), autoType->geometry().bottomLeft());
            if(track->automationType() == MusECore::AUTO_TOUCH || track->automationType() == MusECore::AUTO_WRITE)
                  {
                  palette.setColor(QPalette::Button, QColor(215, 76, 39)); // red
                  autoType->setPalette(palette);
                  }
            else if(track->automationType() == MusECore::AUTO_READ)
                  {
                  palette.setColor(QPalette::Button, QColor(100, 172, 49)); // green
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
      if(_volPressed) // Inhibit the controller stream if control is currently pressed.
        return;
      double vol = static_cast<MusECore::AudioTrack*>(track)->volume();
      if (vol != volume)
      {
          double val;
          if(vol == 0.0)
            val = MusEGlobal::config.minSlider;
          else
          {  
            val = muse_val2dbr(vol);
            if(val < MusEGlobal::config.minSlider)
              val = MusEGlobal::config.minSlider;
          }
          
          slider->blockSignals(true);
          sl->blockSignals(true);
          // Slider::fitValue should not be required since the log function is accurate but rounded to the nearest .000001
          slider->setValue(val);
          sl->setValue(val);
          sl->blockSignals(false);
          slider->blockSignals(false);
          volume = vol;
          }
}

//---------------------------------------------------------
//   offToggled
//---------------------------------------------------------

void AudioStrip::offToggled(bool val)
      {
      if(!track)
        return;
      // This is a minor operation easily manually undoable. Let's not clog the undo list with it.
      MusECore::PendingOperationList operations;
      operations.add(MusECore::PendingOperationItem(track, val, MusECore::PendingOperationItem::SetTrackOff));
      MusEGlobal::audio->msgExecutePendingOperations(operations, true);
      }

//---------------------------------------------------------
//   updateOffState
//---------------------------------------------------------

void AudioStrip::updateOffState()
      {
      bool val = !track->off();
      slider->setEnabled(val);
      sl->setEnabled(val);
      
      _upperRack->setEnabled(val);
      _infoRack->setEnabled(val);
      _lowerRack->setEnabled(val);
      
      if (track->type() != MusECore::Track::AUDIO_SOFTSYNTH)
            stereo->setEnabled(val);
      label->setEnabled(val);
      
      // Are there any Aux Track routing paths to this track? Then we cannot process aux for this track! 
      // Hate to do this, but as a quick visual reminder, seems most logical to disable Aux knobs and labels. 
      const bool ae = track->auxRefCount() == 0 && val;
      _upperRack->setAuxEnabled(ae);
      _infoRack->setAuxEnabled(ae);
      _lowerRack->setAuxEnabled(ae);
            
      if (_recMonitor)
            _recMonitor->setEnabled(val);
      if (pre)
            pre->setEnabled(val);
      if (record)
            record->setEnabled(val);
      if (solo)
            solo->setEnabled(val);
      if (mute)
            mute->setEnabled(val);
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
      if(!track)
        return;
      MusEGlobal::audio->msgSetPrefader(static_cast<MusECore::AudioTrack*>(track), val);
      resetPeaks();
      MusEGlobal::song->update(SC_ROUTE);
      }

//---------------------------------------------------------
//   stereoToggled
//---------------------------------------------------------

void AudioStrip::stereoToggled(bool val)
      {
      if(!track)
        return;
      const int nc = val ? 2 : 1;
      const int oc = track->channels();
      if (oc == nc)
            return;
      MusEGlobal::audio->msgSetChannels(static_cast<MusECore::AudioTrack*>(track), nc);
      MusEGlobal::song->update(SC_CHANNELS);
      }

//---------------------------------------------------------
//   recMonitorToggled
//---------------------------------------------------------

void AudioStrip::recMonitorToggled(bool v)
{
  if(!track)
    return;
  // This is a minor operation easily manually undoable. Let's not clog the undo list with it.
  MusECore::PendingOperationList operations;
  operations.add(MusECore::PendingOperationItem(track, v, MusECore::PendingOperationItem::SetTrackRecMonitor));
  MusEGlobal::audio->msgExecutePendingOperations(operations, true);
}

//---------------------------------------------------------
//   volumeMoved
//---------------------------------------------------------

void AudioStrip::volumeMoved(double val, int id, bool shift_pressed)
      {
      DEBUG_AUDIO_STRIP(stderr, "AudioStrip::volumeMoved id:%d val:%.20f\n", id, val);
      componentMoved(ComponentRack::controllerComponent, val, id, shift_pressed);
      }

//---------------------------------------------------------
//   volumeChanged
//---------------------------------------------------------

void AudioStrip::volumeChanged(double val, int id, int scrollMode)
{
  DEBUG_AUDIO_STRIP(stderr, "AudioStrip::volumeChanged id:%d val:%.20f scrollMode:%d\n", id, val, scrollMode);

  if(!track || track->isMidiTrack())
    return;

  double vol;
  if (val <= MusEGlobal::config.minSlider)
    vol = 0.0;
  else
    vol = muse_db2val(val);
  volume = vol;

  MusECore::AudioTrack* at = static_cast<MusECore::AudioTrack*>(track);
  // Hack: Be sure to ignore in ScrDirect mode since we get both pressed AND changed signals.
  // ScrDirect mode is one-time only on press with modifier.
  if(scrollMode != SliderBase::ScrDirect)
    at->recordAutomation(id, vol);
  at->setParam(id, vol);  // Schedules a timed control change.
  at->enableController(id, false);

  componentChanged(ComponentRack::controllerComponent, val, false, id, scrollMode);
}

//---------------------------------------------------------
//   volumePressed
//---------------------------------------------------------

void AudioStrip::volumePressed(double val, int id)
      {
      DEBUG_AUDIO_STRIP(stderr, "AudioStrip::volumePressed\n");
      if(!track || track->isMidiTrack())
        return;
      _volPressed = true;
      double vol;
      if (val <= MusEGlobal::config.minSlider)
        vol = 0.0;
      else
        vol = muse_db2val(val);
      volume = vol;
      DEBUG_AUDIO_STRIP(stderr, "    val:%.20f\n", volume);

      MusECore::AudioTrack* at = static_cast<MusECore::AudioTrack*>(track);
      at->startAutoRecord(id, vol);
      at->setVolume(vol);
      //at->setParam(MusECore::AC_VOLUME, val);   // Schedules a timed control change. // TODO Try this instead
      DEBUG_AUDIO_STRIP(stderr, "    calling enableController(false)\n");
      at->enableController(id, false);

      componentPressed(ComponentRack::controllerComponent, val, id);
      }

//---------------------------------------------------------
//   volumeReleased
//---------------------------------------------------------

void AudioStrip::volumeReleased(double val, int id)
      {
      DEBUG_AUDIO_STRIP(stderr, "AudioStrip::volumeReleased\n");
      if(!track || track->isMidiTrack())
        return;

      MusECore::AudioTrack* at = static_cast<MusECore::AudioTrack*>(track);
      MusECore::AutomationType atype = at->automationType();
      DEBUG_AUDIO_STRIP(stderr, "    val:%.20f\n", volume);
      at->stopAutoRecord(id, volume);
      if(atype == MusECore::AUTO_OFF || atype == MusECore::AUTO_TOUCH)
      {
        DEBUG_AUDIO_STRIP(stderr, "    calling enableController(true)\n");
        at->enableController(id, true);
      }

      componentReleased(ComponentRack::controllerComponent, val, id);
      _volPressed = false;
      }

//---------------------------------------------------------
//   volumeRightClicked
//---------------------------------------------------------
void AudioStrip::volumeRightClicked(QPoint p)
{
  MusEGlobal::song->execAutomationCtlPopup(static_cast<MusECore::AudioTrack*>(track), p, MusECore::AC_VOLUME);
}

//---------------------------------------------------------
//   volLabelChanged
//---------------------------------------------------------

void AudioStrip::volLabelChanged(double val)
      {
      if(!track || track->isMidiTrack())
        return;
      MusECore::AudioTrack* t = static_cast<MusECore::AudioTrack*>(track);
      double v = val;
      double vol;
      if (v <= MusEGlobal::config.minSlider) {
            vol = 0.0;
            v = MusEGlobal::config.minSlider;
            }
      else
            vol = muse_db2val(v);
      volume = vol;
      slider->blockSignals(true);
      slider->setValue(v);
      slider->blockSignals(false);
      t->startAutoRecord(MusECore::AC_VOLUME, vol);
      t->setParam(MusECore::AC_VOLUME, vol);  // Schedules a timed control change.
      t->enableController(MusECore::AC_VOLUME, false);

      componentChanged(ComponentRack::controllerComponent, val, false, MusECore::AC_VOLUME, SliderBase::ScrNone);
      }

void AudioStrip::resetClipper()
{
   if(track)
   {
      track->resetClipper();
      resetPeaks();
   }
}

//---------------------------------------------------------
//   updateChannels
//---------------------------------------------------------
                                       
void AudioStrip::updateChannels()
      {
      MusECore::AudioTrack* t = static_cast<MusECore::AudioTrack*>(track);
      int c = t->channels();
      DEBUG_AUDIO_STRIP(stderr, "AudioStrip::updateChannels track channels:%d current channels:%d\n", c, channel);
      
      if (c > channel) {
            for (int cc = channel; cc < c; ++cc) {
                  _clipperLabel[cc] = new ClipperLabel();
                  _clipperLabel[cc]->setContentsMargins(0, 0, 0, 0);
                  _clipperLabel[cc]->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
                  setClipperTooltip(cc);
                  _clipperLayout->addWidget(_clipperLabel[cc]);
                  connect(_clipperLabel[cc], SIGNAL(clicked()), SLOT(resetClipper()));

                  meter[cc] = new Meter(this, Meter::DBMeter, Qt::Vertical, MusEGlobal::config.minMeter, volSliderMax);
                  meter[cc]->setRefreshRate(MusEGlobal::config.guiRefresh);
                  meter[cc]->setFixedWidth(FIXED_METER_WIDTH);
                  meter[cc]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
                  meter[cc]->setPrimaryColor(MusEGlobal::config.audioMeterPrimaryColor);
                  connect(meter[cc], SIGNAL(mousePress()), this, SLOT(resetClipper()));
                  sliderGrid->addWidget(meter[cc], 2, cc+1, Qt::AlignLeft);
                  meter[cc]->show();
                  }
            }
      else if (c < channel) {
            for (int cc = channel-1; cc >= c; --cc) {
                  if(_clipperLabel[cc])
                    delete _clipperLabel[cc];
                  _clipperLabel[cc] = 0;
                  
                  if(meter[cc])
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
//   AudioStrip
//---------------------------------------------------------

AudioStrip::~AudioStrip()
      {
      }

//---------------------------------------------------------
//   AudioStrip
//    create mixer strip
//---------------------------------------------------------

AudioStrip::AudioStrip(QWidget* parent, MusECore::AudioTrack* at, bool hasHandle, bool isEmbedded)
   : Strip(parent, at, hasHandle, isEmbedded)
      {
      _preferKnobs = MusEGlobal::config.preferKnobsVsSliders;

      MusECore::Track::TrackType type = at->type();

      volume        = -1.0;
      _volPressed   = false;
      
      slider        = 0;
      sl            = 0;
      off           = 0;
      _recMonitor   = 0;

      // Start the layout in mode A (normal, racks on left).
      _isExpanded = false;
      
      // Set the whole strip's font, except for the label.
      // May be good to keep this. In the midi strip without it the upper rack is too tall at first. So avoid trouble.
      setFont(MusEGlobal::config.fonts[1]);  
      setStyleSheet(MusECore::font2StyleSheet(MusEGlobal::config.fonts[1]));

      channel       = at->channels();

      _inRoutesPos         = GridPosStruct(_curGridRow,     0, 1, 1);
      _outRoutesPos        = GridPosStruct(_curGridRow,     1, 1, 1);
      _routesPos           = GridPosStruct(_curGridRow,     0, 1, 2);

      _effectRackPos       = GridPosStruct(_curGridRow + 1, 0, 1, 3);


      _stereoToolPos       = GridPosStruct(_curGridRow + 2, 0, 1, 1);
      _preToolPos          = GridPosStruct(_curGridRow + 2, 1, 1, 1);

      _preScrollAreaPos_A  = GridPosStruct(_curGridRow + 3, 0, 1, 3);


      _preScrollAreaPos_B  = GridPosStruct(_curGridRow + 4, 2, 1, 1);
      _sliderPos           = GridPosStruct(_curGridRow + 4, 0, 4, 2);


      _infoSpacerTop       = GridPosStruct(_curGridRow + 5, 2, 1, 1);

      _propertyRackPos     = GridPosStruct(_curGridRow + 6, 2, 1, 1);

      _infoSpacerBottom    = GridPosStruct(_curGridRow + 7, 2, 1, 1);

      _sliderLabelPos      = GridPosStruct(_curGridRow + 8, 0, 1, 2);
      _postScrollAreaPos_B = GridPosStruct(_curGridRow + 8, 2, 1, 1);

      _postScrollAreaPos_A = GridPosStruct(_curGridRow + 9, 0, 1, 3);

      _offPos              = GridPosStruct(_curGridRow + 10, 0, 1, 1);
      _recPos              = GridPosStruct(_curGridRow + 10, 1, 1, 1);
      _offMonRecPos        = GridPosStruct(_curGridRow + 10, 0, 1, 2);


      _mutePos             = GridPosStruct(_curGridRow + 11, 0, 1, 1);
      _soloPos             = GridPosStruct(_curGridRow + 11, 1, 1, 1);

      _automationPos       = GridPosStruct(_curGridRow + 12, 0, 1, 2);

      _rightSpacerPos      = GridPosStruct(_curGridRow + 13, 2, 1, 1);


      _infoRack = new AudioComponentRack(at, aStripInfoRack, false);
      //_infoRack->setVisible(false); // Not visible unless expanded.
      // FIXME For some reason StyledPanel has trouble, intermittent sometimes panel is drawn, sometimes not. 
      //_infoRack->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
      _infoRack->setFrameStyle(QFrame::Box | QFrame::Sunken);
      _infoRack->setLineWidth(rackFrameWidth);
      _infoRack->setMidLineWidth(0);
      _infoRack->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
      _infoRack->setContentsMargins(rackFrameWidth, rackFrameWidth, rackFrameWidth, rackFrameWidth);
      _infoRack->setFocusPolicy(Qt::NoFocus);
      _infoRack->addStretch();
       addGridWidget(_infoRack, _propertyRackPos);
                  
      grid->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::Expanding),
                    _infoSpacerTop._row, _infoSpacerTop._col, _infoSpacerTop._rowSpan, _infoSpacerTop._colSpan);

      _upperRack = new AudioComponentRack(at, aStripUpperRack, true); // True = manage auxs.
      // FIXME For some reason StyledPanel has trouble, intermittent sometimes panel is drawn, sometimes not.
      //_upperRack->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
      _upperRack->setFrameStyle(QFrame::Box | QFrame::Sunken);
      _upperRack->setLineWidth(rackFrameWidth);
      _upperRack->setMidLineWidth(0);
      // We do set a minimum height on this widget. Tested: Must be on fixed. Thankfully, it'll expand if more controls are added.
      _upperRack->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
      _upperRack->setContentsMargins(rackFrameWidth, rackFrameWidth, rackFrameWidth, rackFrameWidth);
      _upperRack->setFocusPolicy(Qt::NoFocus);

      int ch = 0;
      for (; ch < channel; ++ch)
      {
            meter[ch] = new Meter(this, Meter::DBMeter, Qt::Vertical, MusEGlobal::config.minMeter, volSliderMax);
            meter[ch]->setRefreshRate(MusEGlobal::config.guiRefresh);
            meter[ch]->setFixedWidth(FIXED_METER_WIDTH);
            meter[ch]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
            _clipperLabel[ch] = new ClipperLabel(this);
            _clipperLabel[ch]->setContentsMargins(0, 0, 0, 0);
            _clipperLabel[ch]->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
            setClipperTooltip(ch);
            connect(_clipperLabel[ch], SIGNAL(clicked()), SLOT(resetClipper()));
            
      }
      for (; ch < MusECore::MAX_CHANNELS; ++ch)
      {
            meter[ch] = 0;
            _clipperLabel[ch] = 0;
      }

      //---------------------------------------------------
      //    plugin rack
      //---------------------------------------------------

      rack = new EffectRack(this, at);
      rack->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);

      addGridWidget(rack, _effectRackPos);
      addGridWidget(_upperRack, _preScrollAreaPos_A);
      
      //---------------------------------------------------
      //    mono/stereo  pre/post
      //---------------------------------------------------

      stereo  = new IconButton(stereoOnSVGIcon, stereoOffSVGIcon, 0, 0, false, true);
      stereo->setContentsMargins(0, 0, 0, 0);
      stereo->setFocusPolicy(Qt::NoFocus);
      stereo->setCheckable(true);
      stereo->setToolTip(tr("1/2 channel"));
      stereo->setChecked(channel == 2);
      stereo->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
      connect(stereo, SIGNAL(toggled(bool)), SLOT(stereoToggled(bool)));

      // disable mono/stereo for Synthesizer-Plugins
      if (type == MusECore::Track::AUDIO_SOFTSYNTH)
            stereo->setEnabled(false);

      pre = new IconButton(preFaderOnSVGIcon, preFaderOffSVGIcon, 0, 0, false, true);
      pre->setContentsMargins(0, 0, 0, 0);
      pre->setFocusPolicy(Qt::NoFocus);
      pre->setCheckable(true);
      pre->setToolTip(tr("Pre Fader Listening (PFL)"));
      pre->setChecked(at->prefader());
      pre->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
      connect(pre, SIGNAL(toggled(bool)), SLOT(preToggled(bool)));

      addGridWidget(stereo, _stereoToolPos);
      addGridWidget(pre, _preToolPos);

      //---------------------------------------------------
      //    slider, label, meter
      //---------------------------------------------------

      sliderGrid = new QGridLayout(); 
      sliderGrid->setContentsMargins(0, 0, 0, 0);
      sliderGrid->setSpacing(0);

      /*-------------- clipper label -------------------*/      
      _clipperLayout = new QHBoxLayout();
      _clipperLayout->setSpacing(0);
      for(int ch = 0; ch < channel; ++ch)
        _clipperLayout->addWidget(_clipperLabel[ch]);
      sliderGrid->addLayout(_clipperLayout, 0, 0, 1, -1, Qt::AlignCenter);
      sliderGrid->addItem(new QSpacerItem(0, 1), 1, 0, 1, -1);
      
      slider = new Slider(this, "vol", Qt::Vertical, MusEGui::Slider::InsideVertical, 14, 
                          MusEGlobal::config.audioVolumeSliderColor, 
                          ScaleDraw::TextHighlightSplitAndShadow);
      slider->setId(MusECore::AC_VOLUME);
      slider->setFocusPolicy(Qt::NoFocus);
      slider->setContentsMargins(0, 0, 0, 0);
      slider->setCursorHoming(true);
      //slider->setThumbLength(1);
      DEBUG_AUDIO_STRIP(stderr, "AudioStrip::AudioStrip new slider: step:%.20f\n", volSliderStep);
      slider->setRange(MusEGlobal::config.minSlider, volSliderMax, volSliderStep);
      //slider->setScaleMaxMinor(5);
      slider->setScale(MusEGlobal::config.minSlider, volSliderMax, 6.0, false);
      slider->setSpecialText(QString('-') + QChar(0x221e)); // The infinity character.
      slider->setScaleBackBone(false);
      //slider->setFillThumb(false);
      
      slider->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
      
      double track_vol = at->volume();
      if(track_vol == 0.0)
        track_vol = MusEGlobal::config.minSlider;
      else
      {
        track_vol = muse_val2dbr(track_vol);
        if(track_vol < MusEGlobal::config.minSlider)
          track_vol = MusEGlobal::config.minSlider;
      }
      // Slider::fitValue() not required so far. The log function is accurate but rounded to the nearest .000001
      slider->setValue(track_vol);

      sliderGrid->addWidget(slider, 2, 0, Qt::AlignHCenter);

      for (int i = 0; i < channel; ++i) {
            //meter[i]->setRange(MusEGlobal::config.minSlider, 10.0);
            meter[i]->setRange(MusEGlobal::config.minMeter, volSliderMax);
            meter[i]->setRefreshRate(MusEGlobal::config.guiRefresh);
            meter[i]->setFixedWidth(Strip::FIXED_METER_WIDTH);
            meter[i]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
            meter[i]->setPrimaryColor(MusEGlobal::config.audioMeterPrimaryColor);
            connect(meter[i], SIGNAL(mousePress()), this, SLOT(resetClipper()));
            sliderGrid->addWidget(meter[i], 2, i+1, Qt::AlignHCenter);
            meter[i]->show();
            }
            
      addGridLayout(sliderGrid, _sliderPos);

      sl = new DoubleLabel(0.0, MusEGlobal::config.minSlider, volSliderMax, this);
      sl->setContentsMargins(0, 0, 0, 0);
      sl->setTextMargins(0, 0, 0, 0);
      sl->setFocusPolicy(Qt::WheelFocus);
      sl->setMouseTracking(true);
      sl->setFrame(true);
      sl->setAlignment(Qt::AlignCenter);
      //sl->setAutoFillBackground(true);

      sl->setSlider(slider);
      //sl->setBackgroundRole(QPalette::Mid);
      sl->setToolTip(tr("Volume/gain"));
      sl->setSuffix(tr("dB"));
      sl->setSpecialText(QString('-') + QChar(0x221e) + QChar(' ') + tr("dB"));
      sl->setOff(MusEGlobal::config.minSlider);
      sl->setPrecision(volSliderPrec);
      sl->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
      sl->setValue(track_vol);
      sl->setEnableStyleHack(MusEGlobal::config.lineEditStyleHack);

      // If smart focus is on redirect strip focus to slider label.
      //if(MusEGlobal::config.smartFocus)
        setFocusProxy(sl);

      connect(sl, SIGNAL(valueChanged(double,int)), SLOT(volLabelChanged(double)));
      connect(slider, SIGNAL(valueChanged(double,int)), sl, SLOT(setValue(double)));
      connect(slider, SIGNAL(valueChanged(double,int,int)), SLOT(volumeChanged(double,int,int)));
      connect(slider, SIGNAL(sliderMoved(double,int,bool)), SLOT(volumeMoved(double,int,bool)));
      connect(slider, SIGNAL(sliderPressed(double, int)), SLOT(volumePressed(double,int)));
      connect(slider, SIGNAL(sliderReleased(double, int)), SLOT(volumeReleased(double,int)));
      connect(slider, SIGNAL(sliderRightClicked(QPoint,int)), SLOT(volumeRightClicked(QPoint)));
   
      grid->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::Expanding),
                    _infoSpacerBottom._row, _infoSpacerBottom._col, _infoSpacerBottom._rowSpan, _infoSpacerBottom._colSpan);
      
      addGridWidget(sl, _sliderLabelPos, Qt::AlignCenter);

      //---------------------------------------------------
      //    pan, balance
      //---------------------------------------------------

      _lowerRack = new AudioComponentRack(at, aStripLowerRack, false);
      // FIXME For some reason StyledPanel has trouble, intermittent sometimes panel is drawn, sometimes not.
      //_lowerRack->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
      _lowerRack->setFrameStyle(QFrame::Box | QFrame::Sunken);
      _lowerRack->setLineWidth(rackFrameWidth);
      _lowerRack->setMidLineWidth(0);
      // We do set a minimum height on this widget. Tested: Must be on fixed. Thankfully, it'll expand if more controls are added.
      _lowerRack->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
      _lowerRack->setContentsMargins(rackFrameWidth, rackFrameWidth, rackFrameWidth, rackFrameWidth);
      _lowerRack->setFocusPolicy(Qt::NoFocus);

      addGridWidget(_lowerRack, _postScrollAreaPos_A);
      
      _upperRack->setEnabled(!at->off());
      _infoRack->setEnabled(!at->off());
      _lowerRack->setEnabled(!at->off());

      //---------------------------------------------------
      //    mute, solo, record
      //---------------------------------------------------

      if (track->canRecord()) {
            record  = new IconButton(recArmOnSVGIcon, recArmOffSVGIcon, 0, 0, false, true);
            record->setFocusPolicy(Qt::NoFocus);
            record->setCheckable(true);
            record->setContentsMargins(0, 0, 0, 0);
            record->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
            if(type == MusECore::Track::AUDIO_OUTPUT)
              record->setToolTip(tr("Record downmix"));
            else
              record->setToolTip(tr("Record arm"));
            record->setChecked(at->recordFlag());
            connect(record, SIGNAL(toggled(bool)), SLOT(recordToggled(bool)));
            }

      mute  = new IconButton(muteOnSVGIcon, muteOffSVGIcon, muteAndProxyOnSVGIcon, muteProxyOnSVGIcon, false, true);
      mute->setFocusPolicy(Qt::NoFocus);
      mute->setCheckable(true);
      mute->setContentsMargins(0, 0, 0, 0);
      mute->setToolTip(tr("Mute or proxy mute"));
      mute->setChecked(at->mute());
      updateMuteIcon();
      mute->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
      connect(mute, SIGNAL(toggled(bool)), SLOT(muteToggled(bool)));

      solo  = new IconButton(soloOnSVGIcon, soloOffSVGIcon, soloAndProxyOnSVGIcon, soloProxyOnSVGIcon, false, true);
      solo->setFocusPolicy(Qt::NoFocus);
      solo->setContentsMargins(0, 0, 0, 0);
      solo->setToolTip(tr("Solo or proxy solo"));
      solo->setCheckable(true);
      solo->setIconSetB(at->internalSolo());
      solo->setChecked(at->solo());
      solo->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
      connect(solo, SIGNAL(toggled(bool)), SLOT(soloToggled(bool)));

      off  = new IconButton(trackOffSVGIcon, trackOnSVGIcon, 0, 0, false, true);
      off->setContentsMargins(0, 0, 0, 0);
      off->setFocusPolicy(Qt::NoFocus);
      off->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
      off->setCheckable(true);
      off->setToolTip(tr("Track off"));
      off->setChecked(at->off());
      connect(off, SIGNAL(toggled(bool)), SLOT(offToggled(bool)));

      //---------------------------------------------------
      //    routing
      //---------------------------------------------------

      if (type != MusECore::Track::AUDIO_AUX) {
            iR = new IconButton(routingInputSVGIcon, routingInputSVGIcon,
                                routingInputUnconnectedSVGIcon, routingInputUnconnectedSVGIcon, false, true);
            iR->setContentsMargins(0, 0, 0, 0);
            iR->setFocusPolicy(Qt::NoFocus);
            iR->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
            iR->setCheckable(false);
            iR->setToolTip(MusEGlobal::inputRoutingToolTipBase);
            connect(iR, SIGNAL(pressed()), SLOT(iRoutePressed()));
            }
            
      oR = new IconButton(routingOutputSVGIcon, routingOutputSVGIcon,
                          routingOutputUnconnectedSVGIcon, routingOutputUnconnectedSVGIcon, false, true);
      oR->setContentsMargins(0, 0, 0, 0);
      oR->setFocusPolicy(Qt::NoFocus);
      oR->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
      oR->setCheckable(false);
      oR->setToolTip(MusEGlobal::outputRoutingToolTipBase);
      connect(oR, SIGNAL(pressed()), SLOT(oRoutePressed()));

      updateRouteButtons();

      if (track && track->canRecordMonitor())
      {
        _recMonitor = new IconButton(monitorOnSVGIcon, monitorOffSVGIcon, 0, 0, false, true);
        _recMonitor->setFocusPolicy(Qt::NoFocus);
        _recMonitor->setContentsMargins(0, 0, 0, 0);
        _recMonitor->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        _recMonitor->setCheckable(true);
        _recMonitor->setToolTip(tr("Input monitor"));
        _recMonitor->setWhatsThis(tr("Pass input through to output"));
        _recMonitor->setChecked(at->recMonitor());
        connect(_recMonitor, SIGNAL(toggled(bool)), SLOT(recMonitorToggled(bool)));
      }

      if(off && record && _recMonitor)
      {
        QHBoxLayout* offRecMonLayout = new QHBoxLayout();
        offRecMonLayout = new QHBoxLayout();
        offRecMonLayout->setContentsMargins(0, 0, 0, 0);
        offRecMonLayout->setSpacing(0);
        offRecMonLayout->addWidget(off);
        offRecMonLayout->addWidget(_recMonitor);
        offRecMonLayout->addWidget(record);
        addGridLayout(offRecMonLayout, _offMonRecPos);
      }
      else
      {
        if(off)
          addGridWidget(off, _offPos);
        if(_recMonitor)
          addGridWidget(_recMonitor, _recPos);
        else if(record)
          addGridWidget(record, _recPos);
      }
      addGridWidget(mute, _mutePos);
      addGridWidget(solo, _soloPos);

      if(iR)
        addGridWidget(iR, _inRoutesPos);
      if(oR)
        addGridWidget(oR, _outRoutesPos);

      //---------------------------------------------------
      //    automation type
      //---------------------------------------------------

      autoType = new CompactComboBox();
      autoType->setContentsMargins(0, 0, 0, 0);
      autoType->setFocusPolicy(Qt::NoFocus);
      autoType->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
      //autoType->setAutoFillBackground(true);
      
      autoType->addAction(tr("Off"), MusECore::AUTO_OFF);
      autoType->addAction(tr("Read"), MusECore::AUTO_READ);
      autoType->addAction(tr("Touch"), MusECore::AUTO_TOUCH);
      autoType->addAction(tr("Write"), MusECore::AUTO_WRITE);
      autoType->setCurrentItem(at->automationType());

      QPalette palette;
      if(at->automationType() == MusECore::AUTO_TOUCH || at->automationType() == MusECore::AUTO_WRITE)
            {
            palette.setColor(QPalette::Button, QColor(215, 76, 39));  // red
            autoType->setPalette(palette);
            }
      else if(at->automationType() == MusECore::AUTO_READ)
            {
            palette.setColor(QPalette::Button, QColor(100, 172, 49));  // green
            autoType->setPalette(palette);
            }
      else  
            {
            palette.setColor(QPalette::Button, qApp->palette().color(QPalette::Active, QPalette::Background));
            autoType->setPalette(palette);
            }

      autoType->setToolTip(tr("automation type"));
      connect(autoType, SIGNAL(activated(int)), SLOT(setAutomationType(int)));
      addGridWidget(autoType, _automationPos);

      grid->setColumnStretch(2, 10);

      if (off) {
            off->blockSignals(true);
            updateOffState();   // init state
            off->blockSignals(false);
            }

      // Now build the strip components.
      buildStrip();
      // Now set up all tabbing on the strip.
      // Don't bother if the strip is part of the mixer (not embedded), 
      //  the non-embedding parent (mixer) should set up all the tabs and make this call.
      if(isEmbedded)
        setupComponentTabbing();

      connect(MusEGlobal::heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));

      updateRouteButtons();

      connect(_upperRack, SIGNAL(componentChanged(int,double,bool,int,int)), SLOT(componentChanged(int,double,bool,int,int)));
      connect(_upperRack, SIGNAL(componentMoved(int,double,int,bool)), SLOT(componentMoved(int,double,int,bool)));
      connect(_upperRack, SIGNAL(componentPressed(int,double,int)), SLOT(componentPressed(int,double,int)));
      connect(_upperRack, SIGNAL(componentReleased(int,double,int)), SLOT(componentReleased(int,double,int)));

      connect(_infoRack, SIGNAL(componentChanged(int,double,bool,int,int)), SLOT(componentChanged(int,double,bool,int,int)));
      connect(_infoRack, SIGNAL(componentMoved(int,double,int,bool)), SLOT(componentMoved(int,double,int,bool)));
      connect(_infoRack, SIGNAL(componentPressed(int,double,int)), SLOT(componentPressed(int,double,int)));
      connect(_infoRack, SIGNAL(componentReleased(int,double,int)), SLOT(componentReleased(int,double,int)));

      connect(_lowerRack, SIGNAL(componentChanged(int,double,bool,int,int)), SLOT(componentChanged(int,double,bool,int,int)));
      connect(_lowerRack, SIGNAL(componentMoved(int,double,int,bool)), SLOT(componentMoved(int,double,int,bool)));
      connect(_lowerRack, SIGNAL(componentPressed(int,double,int)), SLOT(componentPressed(int,double,int)));
      connect(_lowerRack, SIGNAL(componentReleased(int,double,int)), SLOT(componentReleased(int,double,int)));
}

//---------------------------------------------------
//  buildStrip
//    Destroy and rebuild strip components.
//---------------------------------------------------

void AudioStrip::buildStrip()
{
  // Destroys all components and clears the component list.
  _infoRack->clearDelete();
  _upperRack->clearDelete();
  _lowerRack->clearDelete();

  MusECore::AudioTrack* at = static_cast<MusECore::AudioTrack*>(track);

  //---------------------------------------------------
  //    Upper rack
  //---------------------------------------------------

  // Gain...
  if(_preferKnobs)
  {
    CompactKnobComponentDescriptor gain_desc
    (
      ComponentRack::propertyComponent,
      "MixerStripAudioGain",
      AudioComponentRack::aStripGainProperty
    );
    _upperRack->newComponent(&gain_desc);
  }
  else
  {
    CompactSliderComponentDescriptor gain_desc
    (
      ComponentRack::propertyComponent,
      "MixerStripAudioGain",
      AudioComponentRack::aStripGainProperty
    );
    _upperRack->newComponent(&gain_desc);
  }

  // Aux sends...
  int auxsSize = MusEGlobal::song->auxs()->size();
  if(at->hasAuxSend())
  {
    for (int idx = 0; idx < auxsSize; ++idx)
    {
      if(_preferKnobs)
      {
        CompactKnobComponentDescriptor aux_desc
        (
          AudioComponentRack::aStripAuxComponent,
          "MixerStripAudioAux",
          idx
        );
        _upperRack->newComponent(&aux_desc);
      }
      else
      {
        CompactSliderComponentDescriptor aux_desc
        (
          AudioComponentRack::aStripAuxComponent,
          "MixerStripAudioAux",
          idx
        );
        _upperRack->newComponent(&aux_desc);
      }
    }
  }
  else
  {
    ///if (auxsSize)
          //layout->addSpacing((STRIP_WIDTH/2 + 2) * auxsSize);
          ///grid->addSpacing((STRIP_WIDTH/2 + 2) * auxsSize);  // ???
  }

  // Keep this if dynamic layout (flip to right side) is desired.
  _upperRack->addStretch();

  updateRackSizes(true, false);

  //---------------------------------------------------
  //    Lower rack
  //---------------------------------------------------

  // Pan...
  if(_preferKnobs)
  {
    CompactKnobComponentDescriptor pan_desc
    (
      ComponentRack::controllerComponent,
      "MixerStripAudioPan",
      MusECore::AC_PAN
    );
    _lowerRack->newComponent(&pan_desc);
  }
  else
  {
    CompactSliderComponentDescriptor pan_desc
    (
      ComponentRack::controllerComponent,
      "MixerStripAudioPan",
      MusECore::AC_PAN
    );
    _lowerRack->newComponent(&pan_desc);
  }

  // Keep this if dynamic layout (flip to right side) is desired.
  _lowerRack->addStretch();

  updateRackSizes(false, true);
}

QWidget* AudioStrip::setupComponentTabbing(QWidget* previousWidget)
{
  QWidget* prev = previousWidget;
  prev = _upperRack->setupComponentTabbing(prev);
  prev = _infoRack->setupComponentTabbing(prev);
  if(sl)
  {
    if(prev)
      QWidget::setTabOrder(prev, sl);
    prev = sl;
  }
  prev = _lowerRack->setupComponentTabbing(prev);
  return prev;
}

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
      RoutePopupMenu* pup = new RoutePopupMenu(0, false, _broadcastChanges);
      pup->exec(QCursor::pos(), track, false);
      delete pup;
      iR->setDown(false);
      }
      
//---------------------------------------------------------
//   oRoutePressed
//---------------------------------------------------------

void AudioStrip::oRoutePressed()
{
      RoutePopupMenu* pup = new RoutePopupMenu(0, true, _broadcastChanges);
      pup->exec(QCursor::pos(), track, true);
      delete pup;
      oR->setDown(false);     
}

void AudioStrip::incVolume(int v)
{
  if(!track || track->isMidiTrack())
    return;

  const int id = MusECore::AC_VOLUME;
  MusECore::AudioTrack* at = static_cast<MusECore::AudioTrack*>(track);

  // Get the slider's current value.
  const double prev_val = slider->value();
  // Increment the slider. Do not allow signalling.
  slider->blockSignals(true);
  slider->incValue(v);
  slider->blockSignals(false);
  // Now grab the control's new value.
  const double new_val = slider->value();
  
  sl->blockSignals(true);
  sl->setValue(new_val);
  sl->blockSignals(false);

  double d_new_val = new_val;
  if (d_new_val <= MusEGlobal::config.minSlider)
    d_new_val = 0.0;
  else
    d_new_val = muse_db2val(d_new_val);
  volume = d_new_val;

  // Hack: Be sure to ignore in ScrDirect mode since we get both pressed AND changed signals.
  // ScrDirect mode is one-time only on press with modifier.
//   if(scrollMode != SliderBase::ScrDirect)
    at->recordAutomation(id, d_new_val);
  at->setParam(id, d_new_val);  // Schedules a timed control change.
  at->enableController(id, false);

  componentIncremented(ComponentRack::controllerComponent,
                      prev_val, new_val,
                      false, id, Slider::ScrNone);
}

void AudioStrip::incPan(int v)
{
  if(!track || track->isMidiTrack())
    return;

  const int id = MusECore::AC_PAN;
  MusECore::AudioTrack* at = static_cast<MusECore::AudioTrack*>(track);

  ComponentRack* rack = 0;
  ComponentWidget* cw = 0;
  // Be sure to search all racks. Even if pan is in multiple racks, only one hit is
  //  needed since after the value is set, the other pan controls will be updated too.
  if((cw = _upperRack->findComponent(ComponentRack::controllerComponent, -1, id)))
    rack = _upperRack;
  else if((cw = _infoRack->findComponent(ComponentRack::controllerComponent, -1, id)))
    rack = _infoRack;
  else if((cw = _lowerRack->findComponent(ComponentRack::controllerComponent, -1, id)))
    rack = _lowerRack;

  if(!cw || !rack)
    return;

  // Get the component's current value.
  const double prev_val = rack->componentValue(*cw);
  // Now increment the component. Do not allow signalling.
  rack->incComponentValue(*cw, v, true);
  // Now grab its value.
  const double d_new_val = rack->componentValue(*cw);

  // Hack: Be sure to ignore in ScrDirect mode since we get both pressed AND changed signals.
  // ScrDirect mode is one-time only on press with modifier.
//   if(scrollMode != SliderBase::ScrDirect)
    at->recordAutomation(id, d_new_val);
  at->setParam(id, d_new_val);  // Schedules a timed control change.
  at->enableController(id, false);

  componentIncremented(ComponentRack::controllerComponent,
                        prev_val, d_new_val,
                        false, id, Slider::ScrNone);
}

} // namespace MusEGui
