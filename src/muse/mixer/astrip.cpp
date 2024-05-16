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
#include <QPushButton>

#include "app.h"
#include "globals.h"
#include "audio.h"
#include "midi_consts.h"
#include "song.h"
#include "slider.h"
#include "compact_knob.h"
#include "combobox.h"
#include "astrip.h"
#include "synth.h"
#include "audio_fifo.h"
#include "amixer.h"
#include "icons.h"
#include "gconfig.h"
#include "menutitleitem.h"
#include "routepopup.h"
#include "ctrl.h"
#include "utils.h"
#include "muse_math.h"
#include "operations.h"

// Forwards from header:
#include <QHBoxLayout>
#include "track.h"
#include "doublelabel.h"
#include "rack.h"
#include "slider.h"
#include "compact_slider.h"
#include "pixmap_button.h"
#include "clipper_label.h"

// For debugging output: Uncomment the fprintf section.
//#include <stdio.h>
#define DEBUG_AUDIO_STRIP(dev, format, args...) // fprintf(dev, format, ##args);

namespace MusEGui {

const double AudioStrip::volSliderStep =  0.5;
const int     AudioStrip::volSliderPrec =    1;
const QString AudioStrip::volDBSymbol("dB");

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
  bool showval = MusEGlobal::config.showControlValues;

  switch(desc->_componentType)
  {
    case aStripAuxComponent:
    {
      val = _track->auxSend(desc->_index);
      //if(val <= 0.0)
      if(val < MusEGlobal::config.minSlider)
        val = MusEGlobal::config.minSlider;
      else
      {
        val = muse_val2db(val);
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
        desc->_label = ((MusECore::AudioAux*)(MusEGlobal::song->auxs()->at(desc->_index)))->name();
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
            desc->_color = MusEGlobal::config.audioControllerSliderColor;
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
            desc->_color = MusEGlobal::config.audioPropertySliderColor;
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
        d->_color = MusEGlobal::config.sliderBackgroundColor;
      // test kybos
//      if(!d->_faceColor.isValid())
//          d->_faceColor = MusEGlobal::config.sliderBackgroundColor;
//      if(!d->_shinyColor.isValid())
//          d->_shinyColor = MusEGlobal::config.sliderBackgroundColor;
//      if(!d->_rimColor.isValid())
//          d->_rimColor = MusEGlobal::config.sliderBackgroundColor;

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

      if(!d->_color.isValid()) // wrong color, but hopefully set before...
          d->_color = MusEGlobal::config.sliderBackgroundColor;
      if(!d->_barColor.isValid())
        d->_barColor = MusEGlobal::config.sliderBarColor;
      if(!d->_slotColor.isValid())
          d->_slotColor = MusEGlobal::config.sliderBackgroundColor;

//      if(!d->_color.isValid())
//        d->_color = MusEGlobal::config.sliderBackgroundColor;
//      // Set the bar color the same.
//      if(!d->_barColor.isValid())
//        //d->_barColor = d->_color;
//        d->_barColor = MusEGlobal::config.sliderBarColor;

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
      cw._widget->deleteLater();
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
      cw._widget->deleteLater();
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
        if(val <= 0.0)
          val = MusEGlobal::config.minSlider;
        else
        {
          val = muse_val2db(val);
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

  if(at == MusECore::AUTO_OFF || (at == MusECore::AUTO_READ && MusEGlobal::audio->isPlaying()) || at == MusECore::AUTO_TOUCH)
  {
    DEBUG_AUDIO_STRIP(stderr, "    calling enableController(true)\n");
    _track->enableController(id, true);
  }

  emit componentReleased(controllerComponent, v, id);
}

void AudioComponentRack::controllerRightClicked(QPoint p, int id)
{
  DEBUG_AUDIO_STRIP(stderr, "AudioComponentRack::controllerRightClicked id:%d\n", id);
  MusEGlobal::song->execAutomationCtlPopup(_track, p, MusECore::MidiAudioCtrlStruct::AudioControl, id);
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
  if(flags & (SC_RACK | SC_AUDIO_CONTROLLER_LIST))
  {
    scanControllerComponents();
  }

  // Take care of scanning aux before setting aux enabled below.
  if(flags & SC_AUX)
  {
    scanAuxComponents();
  }

  if(flags & SC_ROUTE) {
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

    QColor color = MusEGlobal::config.sliderBackgroundColor;
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
            color = MusEGlobal::config.audioControllerSliderColor;
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
            color = MusEGlobal::config.audioPropertySliderColor;
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
        w->setThumbColor(color);
        w->setBarColor(MusEGlobal::config.sliderBarColor);
        w->setSlotColor(MusEGlobal::config.sliderBackgroundColor);
      }
      break;
    }
  }
}


//---------------------------------------------------------
//   AudioStripProperties
//---------------------------------------------------------

AudioStripProperties::AudioStripProperties()
{
    _sliderRadius = 4;
    _sliderRadiusHandle = 2;
    _sliderHandleHeight = 16;
    _sliderHandleWidth = 16;
    _sliderFillOver = true;
    _sliderUseGradient = true;
    _sliderBackbone = false;
    _sliderFillHandle = true;
    _sliderGrooveWidth = 14;
    _sliderScalePos = Slider::ScaleInside;
    _sliderFrame = false;
    _sliderFrameColor = Qt::darkGray;
    _meterWidth = Strip::FIXED_METER_WIDTH;
    _meterWidthPerChannel = false;
    _meterSpacing = 4;
    _meterFrame = false;
    _meterFrameColor = Qt::darkGray;
    ensurePolished();
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
//   _infoRack->updateComponents();
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

  if(mute)
  {
    // TODO If switching to momentary, un-mute any already muted tracks!
    mute->blockSignals(true);
    mute->setCheckable(!MusEGlobal::config.momentaryMute);
    mute->blockSignals(false);
  }
  if(solo)
  {
    // TODO If switching to momentary, un-solo any already soloed tracks!
    solo->blockSignals(true);
    solo->setCheckable(!MusEGlobal::config.momentarySolo);
    solo->blockSignals(false);
  }

  // Set the whole strip's font, except for the label.
  if (font() != MusEGlobal::config.fonts[1])
  {
//    DEBUG_AUDIO_STRIP(stderr, "AudioStrip::configChanged changing font: current size:%d\n", font().pointSize());
      setStripStyle();
  }

  // Set the strip label's font.
  setLabelText();

  slider->setFillColor(MusEGlobal::config.audioVolumeSliderColor);
  slider->setHandleColor(MusEGlobal::config.audioVolumeHandleColor);

  // Adjust minimum volume slider and label values.
  double track_vol = 0.0;
  double vol_min = 0.0;
  double vol_max = 3.16227766017 /* roughly 10 db */;
  if(!track->isMidiTrack())
  {
    const MusECore::AudioTrack* at = static_cast<MusECore::AudioTrack*>(track);
    track_vol = at->volume();
    MusECore::ciCtrlList ic = at->controller()->find(MusECore::AC_VOLUME);
    if(ic != at->controller()->cend())
      ic->second->range(&vol_min, &vol_max);
  }

  setupControllerWidgets(
    slider, sl, nullptr, meter, channel,
    vol_min, vol_max,
    false,
    true,
    true,
    // If the slider and meter scales differ (minimum values differ), then we really ought to
    //  show a separate meter scale. Otherwise only the slider scale is shown, which gives the
    //  false impression that the meter scale is the same as the slider's scale, which it is not.
    MusEGlobal::config.minSlider != MusEGlobal::config.minMeter,
    volSliderStep, 0.01, 1.0, volSliderPrec, 2, 3, 20.0,
    MusEGlobal::config.minSlider, MusEGlobal::config.minMeter,
    volDBSymbol);

  slider->blockSignals(true);
  sl->blockSignals(true);

  // Reset the values to re-align.
  // FIXME TODO: We shouldn't have to do this here, the controls should do it automatically.
  slider->setValue(track_vol);
  sl->setValue(track_vol);

  slider->blockSignals(false);
  sl->blockSignals(false);


  // Enable special hack for line edits.
//  if(sl->enableStyleHack() != MusEGlobal::config.lineEditStyleHack)
//    sl->setEnableStyleHack(MusEGlobal::config.lineEditStyleHack);

  // Possible, but leave it to the background painter for now.
  //rack->setActiveColor(MusEGlobal::config.rackItemBackgroundColor);

  _upperRack->configChanged();
//  _infoRack->configChanged();
  _lowerRack->configChanged();

  // Ensure updateGeometry is called in case the number of rack items changed.
  // Not required for at least suse, but required for at least mint cinnamon.
  rack->updateGeometry();
  rack->update();

  // Adjust minimum meter values, and colours.
  for(int c = 0; c < channel; ++c)
  {
    meter[c]->setPrimaryColor(MusEGlobal::config.audioMeterPrimaryColor,
                              MusEGlobal::config.meterBackgroundColor);
    meter[c]->setRefreshRate(MusEGlobal::config.guiRefresh);
    // In case something in the slider changed, update the meter alignment margins.
    // TODO This is somewhat crude, might miss automatic changes.
    // Maybe later link up MeterLayout and Slider better?
    meter[c]->setAlignmentMargins(slider->scaleEndpointsMargins());
  }

}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void AudioStrip::songChanged(MusECore::SongChangedStruct_t val)
      {
      MusECore::AudioTrack* src = static_cast<MusECore::AudioTrack*>(track);

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
            mute->setDown(src->mute());
            mute->blockSignals(false);
            updateMuteIcon();
            updateOffState();
            }
      if (solo && (val & (SC_SOLO | SC_ROUTE))) {
            solo->blockSignals(true);
            solo->setDown(track->solo());
            solo->blockSignals(false);
            if (track->internalSolo()) {
                if (solo->isDown())
                    solo->setIcon(*soloAndProxyOnSVGIcon);
                else
                    solo->setIcon(*soloProxyOnAloneSVGIcon);
            } else {
                if(solo->isDown())
                  solo->setIcon(*soloOnSVGIcon);
                else
                  solo->setIcon(*soloOffSVGIcon);
            }
            updateMuteIcon();
      }
      if (val & SC_RECFLAG)
      {
            setRecordFlag(track->recordFlag());
      }
      if (val & SC_TRACK_MODIFIED)
      {
            setLabelText();
      }
      //if (val & SC_CHANNELS)
      //      updateChannels();
      if (val & SC_ROUTE) {
            updateRouteButtons();
            if (pre) {
                  pre->blockSignals(true);
                  pre->setChecked(src->prefader());
                  pre->blockSignals(false);
                  }
          }

      if(val & SC_TRACK_REC_MONITOR)
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
//      _infoRack->songChanged(val);
      _lowerRack->songChanged(val);

      if (autoType && (val & SC_AUTOMATION)) {
            autoType->blockSignals(true);
            autoType->setCurrentItem(track->automationType());
            colorAutoType();
            autoType->blockSignals(false);
            }
      }

//---------------------------------------------------------
//   updateVolume
//---------------------------------------------------------

void AudioStrip::updateVolume()
{
      double vol = static_cast<MusECore::AudioTrack*>(track)->volume();
      {
          double val;
          val = vol;
          slider->blockSignals(true);
          sl->blockSignals(true);
          // Slider::fitValue should not be required since the log function is accurate but rounded to the nearest .000001
          slider->setValue(val);
          sl->setValue(val);
          sl->blockSignals(false);
          slider->blockSignals(false);
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
//      _infoRack->setEnabled(val);
      _lowerRack->setEnabled(val);

      if (track->type() != MusECore::Track::AUDIO_SOFTSYNTH)
            stereo->setEnabled(val);

      // Are there any Aux Track routing paths to this track? Then we cannot process aux for this track!
      // Hate to do this, but as a quick visual reminder, seems most logical to disable Aux knobs and labels.
      const bool ae = track->auxRefCount() == 0 && val;
      _upperRack->setAuxEnabled(ae);
//      _infoRack->setAuxEnabled(ae);
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

  MusECore::AudioTrack* at = static_cast<MusECore::AudioTrack*>(track);
  // Hack: Be sure to ignore in ScrDirect mode since we get both pressed AND changed signals.
  // ScrDirect mode is one-time only on press with modifier.
  if(scrollMode != SliderBase::ScrDirect)
    at->recordAutomation(id, val);
  at->setParam(id, val);  // Schedules a timed control change.
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
      MusECore::AudioTrack* at = static_cast<MusECore::AudioTrack*>(track);
      at->startAutoRecord(id, val);
      at->setVolume(val);
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
      DEBUG_AUDIO_STRIP(stderr, "    val:%.20f\n", val);
      at->stopAutoRecord(id, val);
      if(atype == MusECore::AUTO_OFF || (atype == MusECore::AUTO_READ && MusEGlobal::audio->isPlaying()) || atype == MusECore::AUTO_TOUCH)
      {
        DEBUG_AUDIO_STRIP(stderr, "    calling enableController(true)\n");
        at->enableController(id, true);
      }
      componentReleased(ComponentRack::controllerComponent, val, id);
      }

//---------------------------------------------------------
//   volumeRightClicked
//---------------------------------------------------------
void AudioStrip::volumeRightClicked(QPoint p)
{
  MusEGlobal::song->execAutomationCtlPopup(static_cast<MusECore::AudioTrack*>(track), p,
                                           MusECore::MidiAudioCtrlStruct::AudioControl, MusECore::AC_VOLUME);
}

//---------------------------------------------------------
//   volLabelChanged
//---------------------------------------------------------

void AudioStrip::volLabelChanged(double val)
      {
      if(!track || track->isMidiTrack())
        return;
      MusECore::AudioTrack* t = static_cast<MusECore::AudioTrack*>(track);
      t->startAutoRecord(MusECore::AC_VOLUME, val);
      t->setParam(MusECore::AC_VOLUME, val);  // Schedules a timed control change.
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
    const MusECore::AudioTrack* t = static_cast<const MusECore::AudioTrack*>(track);
    int c = t->channels();
    DEBUG_AUDIO_STRIP(stderr, "AudioStrip::updateChannels track channels:%d current channels:%d\n", c, channel);

    double vol_min = 0.0;
    double vol_max = 3.16227766017 /* roughly 10 db */;
    MusECore::ciCtrlList ic = t->controller()->find(MusECore::AC_VOLUME);
    if(ic != t->controller()->cend())
      ic->second->range(&vol_min, &vol_max);

    const int maxc = qMax(c, channel);
    for(int cc = 0; cc < maxc; ++cc)
    {
      if(cc >= c)
      {
        if(_clipperLabel[cc])
            delete _clipperLabel[cc];
        _clipperLabel[cc] = nullptr;

        if(meter[cc])
            delete meter[cc];
        meter[cc] = nullptr;
      }
      else
      {
        if(cc >= channel)
        {
          _clipperLabel[cc] = new ClipperLabel();
          _clipperLabel[cc]->setContentsMargins(0, 0, 0, 0);
          _clipperLabel[cc]->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
          setClipperTooltip(cc);
          _clipperLayout->addWidget(_clipperLabel[cc]);
          connect(_clipperLabel[cc], SIGNAL(clicked()), SLOT(resetClipper()));

          meter[cc] = new Meter(this);
          meter[cc]->setOrientation(Qt::Vertical);
          meter[cc]->setRefreshRate(MusEGlobal::config.guiRefresh);
          meter[cc]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
          meter[cc]->setTextHighlightMode(ScaleDraw::TextHighlightNone);
          meter[cc]->setScaleBackBone(false);
          meter[cc]->setPrimaryColor(MusEGlobal::config.audioMeterPrimaryColor,
                                      MusEGlobal::config.meterBackgroundColor);
          meter[cc]->setFrame(props.meterFrame(), props.meterFrameColor());
        }

        const int chdiv = (!meter[cc]->vu3d() && !props.meterWidthPerChannel()) ? c : 1;
        meter[cc]->setVUSizeHint(QSize(props.meterWidth() / chdiv, 20));
      }
    }

    setupControllerWidgets(
      nullptr, nullptr, nullptr, meter, c,
      vol_min, vol_max,
      false,
      true,
      true,
      // If the slider and meter scales differ (minimum values differ), then we really ought to
      //  show a separate meter scale. Otherwise only the slider scale is shown, which gives the
      //  false impression that the meter scale is the same as the slider's scale, which it is not.
      MusEGlobal::config.minSlider != MusEGlobal::config.minMeter,
      volSliderStep, 0.01, 1.0, volSliderPrec, 2, 3, 20.0,
      MusEGlobal::config.minSlider, MusEGlobal::config.minMeter,
      volDBSymbol);

    for(int cc = channel; cc < c; ++cc)
    {
      // TODO This is somewhat crude, might miss automatic changes.
      // Maybe later link up MeterLayout and Slider better?
      meter[cc]->setAlignmentMargins(slider->scaleEndpointsMargins());
      _meterLayout->hlayout()->addWidget(meter[cc], Qt::AlignLeft);
      // meter[cc]->show();
      connect(meter[cc], SIGNAL(mousePress()), this, SLOT(resetClipper()));
    }

    channel = c;

    stereo->blockSignals(true);
    stereo->setChecked(channel == 2);
    stereo->blockSignals(false);
    update();
}

//---------------------------------------------------------
//   AudioStrip
//---------------------------------------------------------

AudioStrip::~AudioStrip()
      {
        DEBUG_AUDIO_STRIP(stderr, "~AudioStrip:%p\n", this);
      }

//---------------------------------------------------------
//   AudioStrip
//    create mixer strip
//---------------------------------------------------------

AudioStrip::AudioStrip(QWidget* parent, MusECore::AudioTrack* at, bool hasHandle, bool isEmbedded, bool isDocked)
   : Strip(parent, at, hasHandle, isEmbedded, isDocked)
      {
      DEBUG_AUDIO_STRIP(stderr, "AudioStrip::AudioStrip:%p\n", this);
      _preferKnobs = MusEGlobal::config.preferKnobsVsSliders;

      MusECore::Track::TrackType type = at->type();

      slider        = nullptr;
      sl            = nullptr;
      off           = nullptr;
      _recMonitor   = nullptr;

      // Start the layout in mode A (normal, racks on left).
      _isExpanded = false;

      setStripStyle();

      channel       = at->channels();

      _routePos            = GridPosStruct(_curGridRow,     0, 1, 3);
      _effectRackPos       = GridPosStruct(_curGridRow + 1, 0, 1, 3);
      _stereoPrePos        = GridPosStruct(_curGridRow + 2, 0, 1, 3);
      _upperRackPos        = GridPosStruct(_curGridRow + 3, 0, 1, 3);
      _sliderMeterPos      = GridPosStruct(_curGridRow + 4, 0, 1, 3);
      _lowerRackPos        = GridPosStruct(_curGridRow + 5, 0, 1, 3);
      _bottomPos           = GridPosStruct(_curGridRow + 6, 0, 1, 3);
//      _routePos            = GridPosStruct(_curGridRow,     0, 1, 2);
//      _effectRackPos       = GridPosStruct(_curGridRow + 1, 0, 1, 3);
//      _stereoPrePos        = GridPosStruct(_curGridRow + 2, 0, 1, 2);
//      _upperRackPos        = GridPosStruct(_curGridRow + 3, 0, 1, 3);
//      _sliderMeterPos      = GridPosStruct(_curGridRow + 4, 0, 1, 2);
//      _lowerRackPos        = GridPosStruct(_curGridRow + 5, 0, 1, 3);
//      _bottomPos           = GridPosStruct(_curGridRow + 6, 0, 1, 2);

      //---------------------------------------------------
      //    routing
      //---------------------------------------------------

      QHBoxLayout *routeLayout = new QHBoxLayout;
      routeLayout->setContentsMargins(1,3,1,2);
      routeLayout->setSpacing(1);

      if (type != MusECore::Track::AUDIO_AUX) {
          //            iR = new IconButton(routingInputSVGIcon, routingInputSVGIcon,
          //                                routingInputUnconnectedSVGIcon, routingInputUnconnectedSVGIcon, false, true);
          iR = new QPushButton(this);
          iR->setIcon(*routingInputSVGIcon);
          iR->setObjectName("InputRouteButton");
          iR->setStatusTip(tr("Input routing. Hold CTRL to keep menu open. Press F1 for help."));
          iR->setFocusPolicy(Qt::NoFocus);
          iR->setCheckable(false);
          iR->setToolTip(MusEGlobal::inputRoutingToolTipBase);
          connect(iR, SIGNAL(pressed()), SLOT(iRoutePressed()));
          routeLayout->addWidget(iR);
      } else {
          QPushButton *iRx = new QPushButton(this);
          iRx->setIcon(*routingInputSVGIcon);
          iRx->setObjectName("InputRouteButton");
          iRx->setEnabled(false);
          routeLayout->addWidget(iRx);
      }

//      oR = new IconButton(routingOutputSVGIcon, routingOutputSVGIcon,
//                          routingOutputUnconnectedSVGIcon, routingOutputUnconnectedSVGIcon, false, true);
      oR = new QPushButton(this);
      oR->setIcon(*routingOutputSVGIcon);
      oR->setObjectName("OutputRouteButton");
      oR->setStatusTip(tr("Output routing. Hold CTRL to keep menu open. Press F1 for help."));
      oR->setFocusPolicy(Qt::NoFocus);
      oR->setCheckable(false);
      oR->setToolTip(MusEGlobal::outputRoutingToolTipBase);
      connect(oR, SIGNAL(pressed()), SLOT(oRoutePressed()));
      routeLayout->addWidget(oR);

      updateRouteButtons();

      addGridLayout(routeLayout, _routePos);

//      _infoRack = new AudioComponentRack(at, aStripInfoRack, false);
//      //_infoRack->setVisible(false); // Not visible unless expanded.
//      _infoRack->setVisible(false); // Hide until we figure out what to put in there....
//      // FIXME For some reason StyledPanel has trouble, intermittent sometimes panel is drawn, sometimes not.
//      //_infoRack->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
//      _infoRack->setFrameStyle(QFrame::Box | QFrame::Sunken);
//      _infoRack->setLineWidth(rackFrameWidth);
//      _infoRack->setMidLineWidth(0);
//      _infoRack->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
//      _infoRack->setContentsMargins(rackFrameWidth, rackFrameWidth, rackFrameWidth, rackFrameWidth);
//      _infoRack->setFocusPolicy(Qt::NoFocus);
//      _infoRack->addStretch();
//       addGridWidget(_infoRack, _propertyRackPos);

//      grid->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::Expanding),
//                    _infoSpacerTop._row, _infoSpacerTop._col, _infoSpacerTop._rowSpan, _infoSpacerTop._colSpan);

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

      //---------------------------------------------------
      //    plugin rack
      //---------------------------------------------------

      rack = new EffectRack(this, at);
      rack->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);

      addGridWidget(rack, _effectRackPos);
      addGridWidget(_upperRack, _upperRackPos);

      //---------------------------------------------------
      //    mono/stereo  pre/post
      //---------------------------------------------------

      QHBoxLayout *stereoPreLayout = new QHBoxLayout;
      stereoPreLayout->setContentsMargins(1,2,1,2);
      stereoPreLayout->setSpacing(1);

//      stereo  = new IconButton(stereoOnSVGIcon, stereoOffSVGIcon, nullptr, nullptr, false, true);
      stereo = new QPushButton(this);
      stereo->setIcon(*stereoOnSVGIcon);
      stereo->setFocusPolicy(Qt::NoFocus);
      stereo->setCheckable(true);
      stereo->setToolTip(tr("1/2 channel"));
      stereo->setChecked(channel == 2);

      // disable mono/stereo for Synthesizer-Plugins
      if (type == MusECore::Track::AUDIO_SOFTSYNTH)
          stereo->setEnabled(false);

      connect(stereo, SIGNAL(toggled(bool)), SLOT(stereoToggled(bool)));
      stereoPreLayout->addWidget(stereo);


//      pre = new IconButton(preFaderOnSVGIcon, preFaderOffSVGIcon, nullptr, nullptr, false, true);
      pre = new QPushButton(this);
      pre->setIcon(*preFaderOnSVGIcon);
      pre->setFocusPolicy(Qt::NoFocus);
      pre->setCheckable(true);
      pre->setToolTip(tr("Pre Fader Listening (PFL)"));
      pre->setChecked(at->prefader());
      connect(pre, SIGNAL(toggled(bool)), SLOT(preToggled(bool)));
      stereoPreLayout->addWidget(pre);

      addGridLayout(stereoPreLayout, _stereoPrePos);

      //---------------------------------------------------
      //    slider, label, meter
      //---------------------------------------------------

      sliderGrid = new QGridLayout();
      sliderGrid->setContentsMargins(2, 0, 3, 2);
      sliderGrid->setSpacing(0);
      sliderGrid->setHorizontalSpacing(2);

      /*-------------- clipper label -------------------*/
      _clipperLayout = new QHBoxLayout();
      _clipperLayout->setSpacing(0);

      {
          int ch = 0;
          for (; ch < channel; ++ch)
          {
              _clipperLabel[ch] = new ClipperLabel(this);
              _clipperLabel[ch]->setContentsMargins(0, 0, 0, 0);
              _clipperLabel[ch]->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
              setClipperTooltip(ch);
              connect(_clipperLabel[ch], SIGNAL(clicked()), SLOT(resetClipper()));
              _clipperLayout->addWidget(_clipperLabel[ch]);
          }
          for (; ch < MusECore::MAX_CHANNELS; ++ch)
          {
              _clipperLabel[ch] = nullptr;
              meter[ch] = nullptr;
          }
      }

      sliderGrid->addLayout(_clipperLayout, 0, 0, 1, 2, Qt::AlignCenter);

      slider = new Slider(this, "vol", Qt::Vertical, MusEGui::Slider::ScaleInside, 14,
                          MusEGlobal::config.audioVolumeSliderColor,
                          ScaleDraw::TextHighlightSplitAndShadow,
                          MusEGlobal::config.audioVolumeHandleColor);
      slider->setId(MusECore::AC_VOLUME);
      slider->setFocusPolicy(Qt::NoFocus);
      slider->setContentsMargins(0, 0, 0, 0);
      slider->setCursorHoming(true);
      DEBUG_AUDIO_STRIP(stderr, "AudioStrip::AudioStrip new slider: step:%.20f\n", volSliderStep);

      sl = new DoubleLabel(this);

      double vol_min = 0.0;
      double vol_max = 3.16227766017 /* roughly 10 db */;
      MusECore::iCtrlList ic = at->controller()->find(MusECore::AC_VOLUME);
      if(ic != at->controller()->end())
        ic->second->range(&vol_min, &vol_max);

      slider->setScaleBackBone(props.sliderBackbone());
      slider->setRadius(props.sliderRadius());
      slider->setRadiusHandle(props.sliderRadiusHandle());
      slider->setHandleHeight(props.sliderHandleHeight());
      slider->setHandleWidth(props.sliderHandleWidth());
      slider->setFillThumb(props.sliderFillHandle());
      slider->setGrooveWidth(props.sliderGrooveWidth());
      slider->setFillEmptySide(props.sliderFillOver());
      slider->setUseGradient(props.sliderUseGradient());
      slider->setScalePos(static_cast<Slider::ScalePos>(props.sliderScalePos()));
      slider->setFrame(props.sliderFrame());
      slider->setFrameColor(props.sliderFrameColor());

      slider->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
      slider->setMinimumHeight(80);

      double track_vol = at->volume();

//       // Slider::fitValue() not required so far. The log function is accurate but rounded to the nearest .000001
//       slider->setValue(track_vol);

      _meterLayout = new MeterLayout();
      _meterLayout->setMargin(0);
      _meterLayout->setSpacing(props.meterSpacing());

      for (int i = 0; i < channel; ++i) {
          meter[i] = new Meter(this);
          meter[i]->setOrientation(Qt::Vertical);
          meter[i]->setRefreshRate(MusEGlobal::config.guiRefresh);
          if (meter[i]->vu3d() || props.meterWidthPerChannel()) {
              meter[i]->setVUSizeHint(QSize(props.meterWidth(), 20));
          }
          else {
              meter[i]->setVUSizeHint(QSize(props.meterWidth() / channel, 20));
          }
          meter[i]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
          meter[i]->setTextHighlightMode(ScaleDraw::TextHighlightNone);
          meter[i]->setScaleBackBone(false);
          meter[i]->setPrimaryColor(MusEGlobal::config.audioMeterPrimaryColor,
                                    MusEGlobal::config.meterBackgroundColor);
          meter[i]->setFrame(props.meterFrame(), props.meterFrameColor());
          meter[i]->setAlignmentMargins(slider->scaleEndpointsMargins());
          _meterLayout->hlayout()->addWidget(meter[i], Qt::AlignHCenter);
          connect(meter[i], SIGNAL(mousePress()), this, SLOT(resetClipper()));
      }

      sl->setObjectName("VolumeEditAudio");
      sl->setContentsMargins(0, 0, 0, 0);
      sl->setTextMargins(0, 0, 0, 0);
      sl->setFocusPolicy(Qt::WheelFocus);
      sl->setMouseTracking(true);
      sl->setFrame(true);
      sl->setAlignment(Qt::AlignCenter);
      //sl->setAutoFillBackground(true);

      //sl->setBackgroundRole(QPalette::Mid);
      sl->setToolTip(tr("Volume/Gain"));
      sl->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
//      sl->setEnableStyleHack(MusEGlobal::config.lineEditStyleHack);

      setupControllerWidgets(
        slider, sl, nullptr, meter, channel,
        vol_min, vol_max,
        false,
        true,
        true,
        // If the slider and meter scales differ (minimum values differ), then we really ought to
        //  show a separate meter scale. Otherwise only the slider scale is shown, which gives the
        //  false impression that the meter scale is the same as the slider's scale, which it is not.
        MusEGlobal::config.minSlider != MusEGlobal::config.minMeter,
        volSliderStep, 0.01, 1.0, volSliderPrec, 2, 3, 20.0,
        MusEGlobal::config.minSlider, MusEGlobal::config.minMeter,
        volDBSymbol);

      // Slider::fitValue() not required so far. The log function is accurate but rounded to the nearest .000001
      slider->setValue(track_vol);
      sl->setValue(track_vol);

      sliderGrid->addWidget(slider, 1, 0, Qt::AlignHCenter);
      sliderGrid->addLayout(_meterLayout, 1, 1, Qt::AlignHCenter);


      // If smart focus is on redirect strip focus to slider label.
      //if(MusEGlobal::config.smartFocus)
//         setFocusProxy(sl);

      connect(sl, SIGNAL(valueChanged(double,int)), SLOT(volLabelChanged(double)));
      connect(slider, SIGNAL(valueChanged(double,int,int)), SLOT(volumeChanged(double,int,int)));
      connect(slider, SIGNAL(sliderMoved(double,int,bool)), SLOT(volumeMoved(double,int,bool)));
      connect(slider, SIGNAL(sliderPressed(double, int)), SLOT(volumePressed(double,int)));
      connect(slider, SIGNAL(sliderReleased(double, int)), SLOT(volumeReleased(double,int)));
      connect(slider, SIGNAL(sliderRightClicked(QPoint,int)), SLOT(volumeRightClicked(QPoint)));

      sliderGrid->addWidget(sl, 2, 0, 1, 2, Qt::AlignHCenter);
//      sliderGrid->setColumnStretch(0, slider->sizeHint().width());
//      sliderGrid->setColumnStretch(1, _meterLayout->sizeHint().width());

      QHBoxLayout *sliderHLayout = new QHBoxLayout();
      sliderHLayout->setContentsMargins(0,0,0,0);
      sliderHLayout->setSpacing(0);
      sliderHLayout->addStretch();
      sliderHLayout->addLayout(sliderGrid);
      sliderHLayout->addStretch();
      sliderHLayout->setAlignment(Qt::AlignHCenter);

      QFrame *sliderMeterFrame = new QFrame;
      sliderMeterFrame->setObjectName("SliderMeterFrameAudio");
      sliderMeterFrame->setLayout(sliderHLayout);
      sliderMeterFrame->setMinimumWidth(cMinStripWidth);

      QHBoxLayout *sliderMeterLayout = new QHBoxLayout();
      sliderMeterLayout->setContentsMargins(1,2,1,2);
      sliderMeterLayout->addWidget(sliderMeterFrame);
      addGridLayout(sliderMeterLayout, _sliderMeterPos);

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

      addGridWidget(_lowerRack, _lowerRackPos);

      _upperRack->setEnabled(!at->off());
//      _infoRack->setEnabled(!at->off());
      _lowerRack->setEnabled(!at->off());

      //---------------------------------------------------
      //    mute, solo, record
      //---------------------------------------------------

      QGridLayout *bottomLayout = new QGridLayout;
      bottomLayout->setContentsMargins(1,1,1,2);
      bottomLayout->setSpacing(1);

      if (track && track->canRecordMonitor()) {
          //        _recMonitor = new IconButton(monitorOnSVGIcon, monitorOffSVGIcon, nullptr, nullptr, false, true);
          _recMonitor = new QPushButton;
          _recMonitor->setIcon(*monitorStateSVGIcon);
          _recMonitor->setFocusPolicy(Qt::NoFocus);
          _recMonitor->setCheckable(true);
          _recMonitor->setToolTip(tr("Input monitor"));
          _recMonitor->setWhatsThis(tr("Pass input through to output"));
          _recMonitor->setStatusTip(tr("Input monitor: Pass input through to output."));
          _recMonitor->setChecked(at->recMonitor());
          _recMonitor->setContextMenuPolicy(Qt::CustomContextMenu);
          connect(_recMonitor, SIGNAL(toggled(bool)), SLOT(recMonitorToggled(bool)));
          bottomLayout->addWidget(_recMonitor, 0, 0, 1, 1);
      } else {
          QPushButton *recMonitorx = new QPushButton(this);
          recMonitorx->setIcon(*monitorOnSVGIcon);
          recMonitorx->setEnabled(false);
          recMonitorx->setContextMenuPolicy(Qt::CustomContextMenu);
          bottomLayout->addWidget(recMonitorx, 0, 0, 1, 1);
      }

      if (track->canRecord()) {
          //            record  = new IconButton(recArmOnSVGIcon, recArmOffSVGIcon, nullptr, nullptr, false, true);
          record  = new QPushButton(this);
          if (type == MusECore::Track::AUDIO_OUTPUT)
              record->setIcon(*downmixStateSVGIcon);
          else
              record->setIcon(*recArmStateSVGIcon);
          record->setFocusPolicy(Qt::NoFocus);
          record->setCheckable(true);
          if (type == MusECore::Track::AUDIO_OUTPUT)
              record->setToolTip(tr("Record downmix to a file..."));
          else
              record->setToolTip(tr("Record arm"));
          record->setChecked(at->recordFlag());
          record->setContextMenuPolicy(Qt::CustomContextMenu);
          connect(record, SIGNAL(toggled(bool)), SLOT(recordToggled(bool)));
          bottomLayout->addWidget(record, 0, 1, 1, 1);
      } else {
          QPushButton *recordx = new QPushButton(this);
          recordx->setIcon(*recArmOnSVGIcon);
          recordx->setFocusPolicy(Qt::NoFocus);
          recordx->setEnabled(false);
          recordx->setContextMenuPolicy(Qt::CustomContextMenu);
          bottomLayout->addWidget(recordx, 0, 1, 1, 1);
      }

//      mute  = new IconButton(muteOnSVGIcon, muteOffSVGIcon, muteAndProxyOnSVGIcon, muteProxyOnSVGIcon, false, true);
      mute  = new QPushButton(this);
      mute->setIcon(*muteStateSVGIcon);
      mute->setFocusPolicy(Qt::NoFocus);
      mute->setCheckable(!MusEGlobal::config.momentaryMute);
      mute->setToolTip(tr("Mute or proxy mute"));
      mute->setStatusTip(tr("Mute or proxy mute. Connected tracks are 'phantom' muted."));
      mute->setDown(at->mute());
      updateMuteIcon();
      connect(mute, SIGNAL(toggled(bool)), SLOT(muteToggled(bool)));
      connect(mute, &QPushButton::pressed, [this]() { mutePressed(); } );
      connect(mute, &QPushButton::released, [this]() { muteReleased(); } );
      mute->setContextMenuPolicy(Qt::CustomContextMenu);
      connect(mute, &QPushButton::customContextMenuRequested, [this](const QPoint p) { muteContextMenuReq(p); } );
      bottomLayout->addWidget(mute, 1, 0, 1, 1);

//      solo  = new IconButton(soloOnSVGIcon, soloOffSVGIcon, soloAndProxyOnSVGIcon, soloProxyOnSVGIcon, false, true);
      solo  = new QPushButton(this);
      solo->setIcon(*soloStateSVGIcon);
      solo->setObjectName("SoloButton");
      solo->setToolTip(tr("Solo or proxy solo"));
      solo->setStatusTip(tr("Solo or proxy solo. Connected tracks are 'phantom' soloed. Press F1 for help."));
      solo->setFocusPolicy(Qt::NoFocus);
      solo->setCheckable(!MusEGlobal::config.momentarySolo);
      if (at->internalSolo())
        solo->setIcon(*soloAndProxyOnSVGIcon);
//      solo->setIconSetB(at->internalSolo());
      solo->setDown(at->solo());
      connect(solo, SIGNAL(toggled(bool)), SLOT(soloToggled(bool)));
      connect(solo, &QPushButton::pressed, [this]() { soloPressed(); } );
      connect(solo, &QPushButton::released, [this]() { soloReleased(); } );
      solo->setContextMenuPolicy(Qt::CustomContextMenu);
      connect(solo, &QPushButton::customContextMenuRequested, [this](const QPoint p) { soloContextMenuReq(p); } );
      bottomLayout->addWidget(solo, 1, 1, 1, 1);

//      off  = new IconButton(trackOffSVGIcon, trackOnSVGIcon, nullptr, nullptr, false, true);
      off = new QPushButton(this);
      off->setObjectName("TrackOffButton");
      off->setIcon(*trackOnSVGIcon);
      off->setFocusPolicy(Qt::NoFocus);
      off->setCheckable(true);
      off->setToolTip(tr("Track off"));
      off->setChecked(at->off());
      off->setContextMenuPolicy(Qt::CustomContextMenu);
      connect(off, SIGNAL(toggled(bool)), SLOT(offToggled(bool)));
      bottomLayout->addWidget(off, 3, 0, 1, 2);


      //---------------------------------------------------
      //    automation type
      //---------------------------------------------------

      autoType = new CompactComboBox();
      autoType->setObjectName("AudioAutoType");
      autoType->setContentsMargins(0, 0, 0, 0);
      autoType->setFocusPolicy(Qt::NoFocus);
      autoType->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
      //autoType->setAutoFillBackground(true);

      autoType->addAction(tr("Auto off"), MusECore::AUTO_OFF);
      autoType->addAction(tr("Read"), MusECore::AUTO_READ);
      autoType->addAction(tr("Touch"), MusECore::AUTO_TOUCH);
      autoType->addAction(tr("Latch"), MusECore::AUTO_LATCH);
      autoType->addAction(tr("Write"), MusECore::AUTO_WRITE);
      autoType->setCurrentItem(at->automationType());

      autoType->ensurePolished();
      colorNameButton = autoType->palette().color(QPalette::Button).name();
      colorAutoType();

      autoType->setToolTip(tr("Automation type"));
      autoType->setStatusTip(tr("Automation type: Off, Read, Touch, Latch or Write. Press F1 for help."));
      connect(autoType, SIGNAL(activated(int)), SLOT(setAutomationType(int)));
      bottomLayout->addWidget(autoType, 2, 0, 1, 2);

      addGridLayout(bottomLayout, _bottomPos);

      grid->setColumnStretch(2, 10);

      off->blockSignals(true);
      updateOffState();   // init state
      off->blockSignals(false);


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

//      connect(_infoRack, SIGNAL(componentChanged(int,double,bool,int,int)), SLOT(componentChanged(int,double,bool,int,int)));
//      connect(_infoRack, SIGNAL(componentMoved(int,double,int,bool)), SLOT(componentMoved(int,double,int,bool)));
//      connect(_infoRack, SIGNAL(componentPressed(int,double,int)), SLOT(componentPressed(int,double,int)));
//      connect(_infoRack, SIGNAL(componentReleased(int,double,int)), SLOT(componentReleased(int,double,int)));

      connect(_lowerRack, SIGNAL(componentChanged(int,double,bool,int,int)), SLOT(componentChanged(int,double,bool,int,int)));
      connect(_lowerRack, SIGNAL(componentMoved(int,double,int,bool)), SLOT(componentMoved(int,double,int,bool)));
      connect(_lowerRack, SIGNAL(componentPressed(int,double,int)), SLOT(componentPressed(int,double,int)));
      connect(_lowerRack, SIGNAL(componentReleased(int,double,int)), SLOT(componentReleased(int,double,int)));
      // When closing/(re)loading, we must wait for this to delete to avoid crashes
      //  due to still active external connections like heartBeatTimer.
      MusEGlobal::muse->addPendingObjectDestruction(this);
}


void AudioStrip::setStripStyle() {
    // Set the whole strip's font, except for the label.
    // May be good to keep this. In the midi strip without it the upper rack is too tall at first. So avoid trouble.
    setFont(MusEGlobal::config.fonts[1]);
    int iconSize = MusEGlobal::config.fonts[1].pointSize() * 2;
    setStyleSheet(MusECore::font2StyleSheetFull(MusEGlobal::config.fonts[1])
            + "#Strip > QAbstractButton { padding: 0px; qproperty-iconSize:" +
                  QString::number(iconSize) + "px; }"
            + "#Strip #TrackOffButton { qproperty-iconSize:" + QString::number(iconSize - 2) + "px; }");
}

void AudioStrip::colorAutoType() {

      if (track->automationType() == MusECore::AUTO_TOUCH ||
          track->automationType() == MusECore::AUTO_LATCH ||
          track->automationType() == MusECore::AUTO_WRITE)
      {
          autoType->setStyleSheet("QToolButton { background: rgb(150, 0, 0); }");
      }
      else if (track->automationType() == MusECore::AUTO_READ)
      {
          autoType->setStyleSheet("QToolButton { background: rgb(0, 100, 50); }");
      }
      else
      {
          autoType->setStyleSheet("QToolButton { background:" + colorNameButton + "; }");
      }
}

//---------------------------------------------------
//  buildStrip
//    Destroy and rebuild strip components.
//---------------------------------------------------

void AudioStrip::buildStrip()
{
  // Destroys all components and clears the component list.
//  _infoRack->clearDelete();
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

  // noop
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

  // noop
  updateRackSizes(false, true);
}

QWidget* AudioStrip::setupComponentTabbing(QWidget* previousWidget)
{
  QWidget* prev = previousWidget;
  prev = _upperRack->setupComponentTabbing(prev);
//  prev = _infoRack->setupComponentTabbing(prev);
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
      RoutePopupMenu* pup = new RoutePopupMenu(nullptr, false, _broadcastChanges);
      pup->exec(QCursor::pos(), track, false);
      delete pup;
      iR->setDown(false);
      }

//---------------------------------------------------------
//   oRoutePressed
//---------------------------------------------------------

void AudioStrip::oRoutePressed()
{
      RoutePopupMenu* pup = new RoutePopupMenu(nullptr, true, _broadcastChanges);
      pup->exec(QCursor::pos(), track, true);
      delete pup;
      oR->setDown(false);
}

void AudioStrip::incVolume(int increaseValue)
{
  if(!track || track->isMidiTrack())
    return;

  const int id = MusECore::AC_VOLUME;
  MusECore::AudioTrack* at = static_cast<MusECore::AudioTrack*>(track);

  // Get the slider's current value.
  const double prev_val = slider->value();
  // Increment the slider. Do not allow signalling.
  slider->blockSignals(true);
  slider->incValue(increaseValue);
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

void AudioStrip::incPan(int increaseValue)
{
  if(!track || track->isMidiTrack())
    return;

  const int id = MusECore::AC_PAN;
  MusECore::AudioTrack* at = static_cast<MusECore::AudioTrack*>(track);

  ComponentRack* rack = nullptr;
  ComponentWidget* cw = nullptr;
  // Be sure to search all racks. Even if pan is in multiple racks, only one hit is
  //  needed since after the value is set, the other pan controls will be updated too.
  if((cw = _upperRack->findComponent(ComponentRack::controllerComponent, -1, id)))
    rack = _upperRack;
//  else if((cw = _infoRack->findComponent(ComponentRack::controllerComponent, -1, id)))
//    rack = _infoRack;
  else if((cw = _lowerRack->findComponent(ComponentRack::controllerComponent, -1, id)))
    rack = _lowerRack;

  if(!cw || !rack)
    return;

  // Get the component's current value.
  const double prev_val = rack->componentValue(*cw);
  // Now increment the component. Do not allow signalling.
  rack->incComponentValue(*cw, increaseValue, true);
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
