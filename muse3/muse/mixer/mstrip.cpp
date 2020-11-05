//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mstrip.cpp,v 1.9.2.13 2009/11/14 03:37:48 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 - 2017 Tim E. Real (terminator356 on sourceforge)
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
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QToolTip>
#include <QTimer>
#include <QCursor>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QPushButton>

#include "app.h"
#include "midi_consts.h"
#include "midictrl.h"
#include "ctrl.h"
#include "mstrip.h"
#include "midiport.h"
#include "globals.h"
#include "audio.h"
#include "song.h"
#include "slider.h"
#include "knob.h"
#include "combobox.h"
#include "track.h"
#include "doublelabel.h"
#include "rack.h"
#include "amixer.h"
#include "icons.h"
#include "gconfig.h"
#include "pixmap_button.h"
#include "popupmenu.h"
#include "routepopup.h"

#include "minstrument.h"
#include "midievent.h"
#include "compact_knob.h"
#include "compact_slider.h"
#include "compact_patch_edit.h"
#include "lcd_widgets.h"
#include "elided_label.h"
#include "utils.h"
#include "muse_math.h"
#include "operations.h"

#include "synth.h"
#ifdef LV2_SUPPORT
#include "lv2host.h"
#endif

// For debugging output: Uncomment the fprintf section.
#define DEBUG_MIDI_STRIP(dev, format, args...) // fprintf(dev, format, ##args);

namespace MusEGui {

const double MidiStrip::volSliderStepLin =  1.0;

const double MidiStrip::volSliderStepDb =  0.5;
const double MidiStrip::volSliderMaxDb  =  0.0;
const int    MidiStrip::volSliderPrecDb =    1;

const int MidiStrip::xMarginHorSlider = 1;
const int MidiStrip::yMarginHorSlider = 1;
const int MidiStrip::upperRackSpacerHeight = 2;
const int MidiStrip::rackFrameWidth = 1;

//---------------------------------------------------------
//   MidiComponentRack
//---------------------------------------------------------

MidiComponentRack::MidiComponentRack(MusECore::MidiTrack* track, int id, QWidget* parent, Qt::WindowFlags f) 
  : ComponentRack(id, parent, f), _track(track)
{ 
  
}

void MidiComponentRack::newComponent( ComponentDescriptor* desc, const ComponentWidget& before )
{
  int min = 0;
  int max = 0;
  int val = 0;
  bool hasOffMode = false;
  bool off = false;
  bool showval = MusEGlobal::config.showControlValues;

  switch(desc->_componentType)
  {
    case controllerComponent:
    {
      const int midiCtrlNum = desc->_index;
      const int chan  = _track->outChannel();
      const int port  = _track->outPort();
      if(chan < 0 || chan >= MusECore::MUSE_MIDI_CHANNELS || port < 0 || port >= MusECore::MIDI_PORTS)
        return;
      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
      MusECore::MidiController* mc = mp->midiController(midiCtrlNum, chan); // Auto-create the controller if necessary.
      if(!mc)
        return;
      min = mc->minVal();
      max = mc->maxVal();

      if(midiCtrlNum == MusECore::CTRL_PROGRAM)
      {
        val = mp->hwCtrlState(chan, midiCtrlNum);
        if(val == MusECore::CTRL_VAL_UNKNOWN)
        {
          int lastv = mp->lastValidHWCtrlState(chan, midiCtrlNum);
          if(lastv == MusECore::CTRL_VAL_UNKNOWN)
          {
            if(mc->initVal() == MusECore::CTRL_VAL_UNKNOWN)
              val = 0;
            else  
              val = mc->initVal();
          }
          off = true;
        }  
      }
      else
      {
        hasOffMode = true;
        val = mp->hwCtrlState(chan, midiCtrlNum);
        if(val == MusECore::CTRL_VAL_UNKNOWN)
        {
          int lastv = mp->lastValidHWCtrlState(chan, midiCtrlNum);
          if(lastv == MusECore::CTRL_VAL_UNKNOWN)
          {
            if(mc->initVal() == MusECore::CTRL_VAL_UNKNOWN)
              val = 0;
            else  
              val = mc->initVal();
          }
          else  
            val = lastv - mc->bias();
          off = true;
        }  
        else
        {
          // Auto bias...
          val -= mc->bias();
        }
      }
      
      if(desc->_label.isEmpty())
      {
        QString ctlname = mc->name();
        if(ctlname.isEmpty())
        {
          switch(midiCtrlNum)
          {
            case MusECore::CTRL_PROGRAM:
              ctlname = tr("Pro");
            break;

            case MusECore::CTRL_VARIATION_SEND:
              ctlname = tr("Var");
            break;

            case MusECore::CTRL_REVERB_SEND:
              ctlname = tr("Rev");
            break;

            case MusECore::CTRL_CHORUS_SEND:
              ctlname = tr("Cho");
            break;

            case MusECore::CTRL_PANPOT:
              ctlname = tr("Pan");
            break;

            default:
              ctlname = QString("#%1").arg(midiCtrlNum);
            break;
          }
        }
        desc->_label = ctlname;
      }
      
      if(desc->_toolTipText.isEmpty())
      {
        QString ctlname = mc->name();
        if(ctlname.isEmpty())
        {
          switch(midiCtrlNum)
          {
            case MusECore::CTRL_PROGRAM:
              ctlname = tr("Program");
            break;

            case MusECore::CTRL_VARIATION_SEND:
              ctlname = tr("VariationSend");
            break;

            case MusECore::CTRL_REVERB_SEND:
              ctlname = tr("ReverbSend");
            break;

            case MusECore::CTRL_CHORUS_SEND:
              ctlname = tr("ChorusSend");
            break;

            case MusECore::CTRL_PANPOT:
              ctlname = tr("Pan/Balance");
            break;

            default:
              ctlname = tr("Controller");
            break;
          }
        }
        desc->_toolTipText = QString("%1 (# %2)\n%3").arg(ctlname).arg(midiCtrlNum).arg(tr("(Ctrl-double-click on/off)"));
      }
      
      if(!desc->_color.isValid())
      {
        switch(midiCtrlNum)
        {
          case MusECore::CTRL_PANPOT:
            desc->_color = MusEGlobal::config.panSliderColor;
          break;
          
          case MusECore::CTRL_PROGRAM:
            desc->_color = MusEGlobal::config.midiPatchReadoutColor;
          break;
          
          default:
            desc->_color = MusEGlobal::config.midiControllerSliderColor;
          break;
       }
      }
    }
    break;
    
    case propertyComponent:
    {
      switch(desc->_index)
      {
        case mStripInstrumentProperty:
        {
          const int port = _track->outPort();
          if(port >= 0 && port < MusECore::MIDI_PORTS)
          {
            if(MusECore::MidiInstrument* minstr = MusEGlobal::midiPorts[_track->outPort()].instrument())
            {
              //desc->_enabled = !minstr->isSynti();
              if(desc->_label.isEmpty())
                desc->_label = minstr->iname();
            }
            else
            {
              desc->_enabled = false;
              if(desc->_label.isEmpty())
                desc->_label = tr("<unknown>");
            }
          }
          if(desc->_toolTipText.isEmpty())
            desc->_toolTipText = tr("Instrument");
        }
        break;
        
        case mStripTranspProperty:
        {
          val = _track->transposition;
          min = -127;
          max = 127;
          if(desc->_label.isEmpty())
            desc->_label = tr("Transpose");
          if(desc->_toolTipText.isEmpty())
            desc->_toolTipText = tr("Transpose notes up or down");
          if(!desc->_color.isValid())
            desc->_color = MusEGlobal::config.midiPropertySliderColor;
        }
        break;
        
        case mStripDelayProperty:
        {
          val = _track->delay;
          min = -1000;
          max = 1000;
          if(desc->_label.isEmpty())
            desc->_label = tr("Delay");
          if(desc->_toolTipText.isEmpty())
            desc->_toolTipText = tr("Offset playback of notes before or after actual note");
          if(!desc->_color.isValid())
            desc->_color = MusEGlobal::config.midiPropertySliderColor;
        }
        break;
        
        case mStripLenProperty:
        {
          val = _track->len;
          min = 25;
          max = 200;
          if(desc->_label.isEmpty())
            desc->_label = tr("Length");
          if(desc->_toolTipText.isEmpty())
            desc->_toolTipText = tr("Change note length in percent of actual length");
          if(!desc->_color.isValid())
            desc->_color = MusEGlobal::config.midiPropertySliderColor;
        }
        break;
        
        case mStripVeloProperty:
        {
          val = _track->velocity;
          min = -127;
          max = 127;
          if(desc->_label.isEmpty())
            desc->_label = tr("Velocity");
          if(desc->_toolTipText.isEmpty())
            desc->_toolTipText = tr("<html><head/><body><p>Add or substract velocity to notes"
                                     " on track.</p><p><span style="" font-style:italic;"">Since"
                                     " the midi note range is 0-127 this <br/>might mean that the"
                                     " notes do not reach <br/>the combined velocity, note + "
                                     " Velocity.</span></p></body></html>");
          if(!desc->_color.isValid())
            desc->_color = MusEGlobal::config.midiPropertySliderColor;
        }
        break;
        
        case mStripComprProperty:
        {
          val = _track->compression;
          min = 25;
          max = 200;
          if(desc->_label.isEmpty())
            desc->_label = tr("Compress");
          if(desc->_toolTipText.isEmpty())
            desc->_toolTipText = tr("Compress the notes velocity range, in percent of actual velocity");
          if(!desc->_color.isValid())
            desc->_color = MusEGlobal::config.midiPropertySliderColor;
        }
        break;
        
      }
    }
    break;
  }  

  switch(desc->_widgetType)
  {
    case ElidedLabelComponentWidget:
    {
      ElidedLabelComponentDescriptor* d = static_cast<ElidedLabelComponentDescriptor*>(desc);
      
      d->_color = MusEGlobal::config.midiInstrumentBorderColor;
      d->_bgColor = MusEGlobal::config.midiInstrumentBackgroundColor;
      d->_bgActiveColor = MusEGlobal::config.midiInstrumentBgActiveColor;
      d->_fontColor = MusEGlobal::config.midiInstrumentFontColor;
      d->_fontActiveColor = MusEGlobal::config.midiInstrumentFontActiveColor;

      // Adds a component. Creates a new component using the given desc values if the desc widget is not given.
      // Connects known widget types' signals to slots.
      newComponentWidget(d, before);
    }
    break;
    
    case CompactKnobComponentWidget:
    {
      CompactKnobComponentDescriptor* d = static_cast<CompactKnobComponentDescriptor*>(desc);
      d->_min = min;
      d->_max = max;
      d->_precision = 0;
      d->_step = 1.0;
      d->_initVal = val;
      d->_hasOffMode = hasOffMode;
      d->_isOff = off;
      d->_showValue = showval;
      if(!d->_color.isValid())
        d->_color = MusEGlobal::config.sliderBackgroundColor;

      // Adds a component. Creates a new component using the given desc values if the desc widget is not given.
      // Connects known widget types' signals to slots.
      newComponentWidget(d, before);
    }
    break;

    case CompactSliderComponentWidget:
    {
      CompactSliderComponentDescriptor* d = static_cast<CompactSliderComponentDescriptor*>(desc);
      d->_min = min;
      d->_max = max;
      d->_precision = 0;
      d->_step = 1.0;
      d->_initVal = val;
      d->_hasOffMode = hasOffMode;
      d->_isOff = off;
      d->_showValue = showval;

      if(!d->_color.isValid()) // wrong color, but hopefully set before...
          d->_color = MusEGlobal::config.sliderBackgroundColor;
      if(!d->_barColor.isValid())
        d->_barColor = MusEGlobal::config.sliderBarColor;
      if(!d->_slotColor.isValid())
          d->_slotColor = MusEGlobal::config.sliderBackgroundColor;

      // Adds a component. Creates a new component using the given desc values if the desc widget is not given.
      // Connects known widget types' signals to slots.
      newComponentWidget(d, before);
    }
    break;
    
    case mStripCompactPatchEditComponentWidget:
    {
      CompactPatchEditComponentDescriptor* d = static_cast<CompactPatchEditComponentDescriptor*>(desc);
      d->_initVal = val;
      d->_isOff = off;
      if(!d->_color.isValid())
        d->_color = MusEGlobal::config.midiPatchReadoutColor;
      
      // Adds a component. Creates a new component using the given desc values if the desc widget is not given.
      // Connects known widget types' signals to slots.
      newComponentWidget(d, before);
    }
    break;
  }  
}

void MidiComponentRack::newComponentWidget( ComponentDescriptor* desc, const ComponentWidget& before )
{
  switch(desc->_widgetType)
  {
    case mStripCompactPatchEditComponentWidget:
    {
      CompactPatchEditComponentDescriptor* d = static_cast<CompactPatchEditComponentDescriptor*>(desc);
      if(!d->_compactPatchEdit)
      {
        CompactPatchEdit* control = new CompactPatchEdit(nullptr,
                                                         d->_objName,
                                                         CompactSlider::None);
        d->_compactPatchEdit = control;
        control->setId(d->_index);
        control->setValue(d->_initVal);
        // Don't allow anything here, it interferes with the CompactPatchEdit which sets it's own controls' tooltips.
        //control->setToolTip(d->_toolTipText);
        control->setEnabled(d->_enabled);
        control->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
        control->setContentsMargins(0, 0, 0, 0);

        if(d->_color.isValid())
          control->setReadoutColor(d->_color);

        control->setBgColor(MusEGlobal::config.midiInstrumentBackgroundColor);
        control->setBgActiveColor(MusEGlobal::config.midiInstrumentBgActiveColor);
        control->setBorderColor(MusEGlobal::config.midiInstrumentBorderColor);
        control->setFontColor(MusEGlobal::config.midiInstrumentFontColor);
        control->setFontActiveColor(MusEGlobal::config.midiInstrumentFontActiveColor);
        
        control->setMaxAliasedPointSize(MusEGlobal::config.maxAliasedPointSize);
        
        connect(d->_compactPatchEdit, SIGNAL(valueChanged(int,int)), SLOT(controllerChanged(int,int)));
        connect(d->_compactPatchEdit, SIGNAL(patchValueRightClicked(QPoint,int)), SLOT(controllerRightClicked(QPoint,int)));
        connect(d->_compactPatchEdit, SIGNAL(patchNameClicked(QPoint,int)), SLOT(patchEditNameClicked(QPoint,int)));
        connect(d->_compactPatchEdit, SIGNAL(patchNameRightClicked(QPoint,int)), SLOT(controllerRightClicked(QPoint,int)));
      }
      
      ComponentWidget cw = ComponentWidget(
                            d->_compactPatchEdit,
                            d->_widgetType, 
                            d->_componentType, 
                            d->_index 
                          );
      
      addComponentWidget(cw, before);
      return;
    }
    break;
  }
  
  // If not handled above, let ancestor handle it.
  ComponentRack::newComponentWidget(desc, before);
}

void MidiComponentRack::scanControllerComponents()
{
  const int chan  = _track->outChannel();
  const int port  = _track->outPort();
  if(chan < 0 || chan >= MusECore::MUSE_MIDI_CHANNELS || port < 0 || port >= MusECore::MIDI_PORTS)
    return;
  
  QString namestr;
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
        MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
        MusECore::MidiCtrlValListList* mcvll = mp->controller();
        MusECore::ciMidiCtrlValList imcvll = mcvll->find(chan, cw._index);
        if(imcvll == mcvll->end())
          to_be_erased.push_back(ic);
        else
        {
          // While we are here, let's update the name of the control, in case the instrument changed.
          switch(cw._widgetType)
          {
            case CompactKnobComponentWidget:
            case CompactSliderComponentWidget:
            {
              // false = do not create the controller if not found.
              MusECore::MidiController* mc = mp->midiController(cw._index, chan, false);
              if(mc)
                setComponentText(cw, mc->name());
            }
            break;
          }
        }
      }
      break;
    }
  }
  for(std::vector<iComponentWidget>::iterator i = to_be_erased.begin(); i != to_be_erased.end(); ++i)
  {
    iComponentWidget icw = *i;
    ComponentWidget& cw = *icw;
    DEBUG_MIDI_STRIP(stderr, "MidiComponentRack::scanControllerComponents: deleting controller component index:%d\n", cw._index);
    if(cw._widget)
      cw._widget->deleteLater();
    _components.erase(icw);
  }
}

void MidiComponentRack::updateComponents()
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
        
        const int channel  = _track->outChannel();
        const int port  = _track->outPort();
        if(channel < 0 || channel >= MusECore::MUSE_MIDI_CHANNELS || port < 0 || port >= MusECore::MIDI_PORTS)
          continue;
        
        MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
        MusECore::MidiCtrlValListList* mcvll = mp->controller();

        MusECore::ciMidiCtrlValList imcvl = mcvll->find(channel, cw._index);
        const bool enable = imcvl != mcvll->end() && !_track->off();
        if(cw._widget->isEnabled() != enable)
          cw._widget->setEnabled(enable);
          
        if(enable)
        {
          MusECore::MidiCtrlValList* mcvl = imcvl->second;
          switch(cw._index)
          {
            case MusECore::CTRL_PROGRAM:
            {
              switch(cw._widgetType)
              {
                case mStripCompactPatchEditComponentWidget:
                {    
                  // Special for new LCD patch edit control: Need to give both current and last values.
                  // Keeping a local last value with the control won't work.
                  CompactPatchEdit* control = static_cast<CompactPatchEdit*>(cw._widget);
                  const int hwVal = mcvl->hwVal();
                  control->blockSignals(true);
                  control->setLastValidValue(mcvl->lastValidHWVal());
                  control->setLastValidBytes(mcvl->lastValidByte2(), mcvl->lastValidByte1(), mcvl->lastValidByte0());
                  control->setValue(hwVal);
                  control->blockSignals(false);

                  if(hwVal == MusECore::CTRL_VAL_UNKNOWN)
                  {
                    control->setPatchNameOff(true);
                    const QString patchName(tr("<unknown>"));
                    if(control->patchName() != patchName)
                      control->setPatchName(patchName);
                  }
                  else
                  {
                    // Try to avoid calling MidiInstrument::getPatchName too often.
//                     if(_heartBeatCounter == 0)
                    {
                      control->setPatchNameOff(false);
                      MusECore::MidiInstrument* instr = mp->instrument();
                      QString patchName(instr->getPatchName(channel, hwVal, _track->isDrumTrack(), true)); // Include default.
                      if(patchName.isEmpty())
                        patchName = QString("???");
                      if(control->patchName() != patchName)
                        control->setPatchName(patchName);
                    }
                  }
                }  
                break;
              }
            }
            break;
            
            default:
            {
              switch(cw._widgetType)
              {
                case CompactKnobComponentWidget:
                {
                  CompactKnob* control = static_cast<CompactKnob*>(cw._widget);

                  int hwVal = mcvl->hwVal();
                  int min = 0;
                  int max = 127;
                  int bias = 0;
                  int initval = 0;
                  MusECore::MidiController* mc = mp->midiController(cw._index, channel, false);
                  if(mc)
                  {
                    bias = mc->bias();
                    min = mc->minVal();
                    max = mc->maxVal();
                    initval = mc->initVal();
                    if(initval == MusECore::CTRL_VAL_UNKNOWN)
                      initval = 0;
                  }

                  const double dmin = (double)min;
                  const double dmax = (double)max;
                  const double c_dmin = control->minValue();
                  const double c_dmax = control->maxValue();
                  if(c_dmin != min && c_dmax != max)
                  {
                    control->blockSignals(true);
                    control->setRange(dmin, dmax, 1.0);
                    control->blockSignals(false);
                  }
                  else if(c_dmin != min)
                  {
                    control->blockSignals(true);
                    control->setMinValue(min);
                    control->blockSignals(false);
                  }
                  else if(c_dmax != max)
                  {
                    control->blockSignals(true);
                    control->setMaxValue(max);
                    control->blockSignals(false);
                  }

                  if(hwVal == MusECore::CTRL_VAL_UNKNOWN)
                  {
                    hwVal = mcvl->lastValidHWVal();
                    if(hwVal == MusECore::CTRL_VAL_UNKNOWN)
                    {
                      hwVal = initval;
                      if(!control->isOff() || hwVal != control->value())
                      {
                        control->blockSignals(true);
                        control->setValueState(hwVal, true);
                        control->blockSignals(false);
                      }
                    }
                    else
                    {
                      hwVal -= bias;
                      if(!control->isOff() || hwVal != control->value())
                      {
                        control->blockSignals(true);
                        control->setValueState(hwVal, true);
                        control->blockSignals(false);
                      }
                    }
                  }
                  else
                  {
                    hwVal -= bias;
                    if(control->isOff() || hwVal != control->value())
                    {
                      control->blockSignals(true);
                      control->setValueState(hwVal, false);
                      control->blockSignals(false);
                    }
                  }
                }
                break;

                case CompactSliderComponentWidget:
                {                  
                  CompactSlider* control = static_cast<CompactSlider*>(cw._widget);
                  
                  int hwVal = mcvl->hwVal();
                  int min = 0;
                  int max = 127;
                  int bias = 0;
                  int initval = 0;
                  MusECore::MidiController* mc = mp->midiController(cw._index, channel, false);
                  if(mc)
                  {
                    bias = mc->bias();
                    min = mc->minVal();
                    max = mc->maxVal();
                    initval = mc->initVal();
                    if(initval == MusECore::CTRL_VAL_UNKNOWN)
                      initval = 0;
                  }
                  
                  const double dmin = (double)min;
                  const double dmax = (double)max;
                  const double c_dmin = control->minValue();
                  const double c_dmax = control->maxValue();
                  if(c_dmin != min && c_dmax != max)
                  {
                    control->blockSignals(true);
                    control->setRange(dmin, dmax, 1.0);
                    control->blockSignals(false);
                  }
                  else if(c_dmin != min)
                  {
                    control->blockSignals(true);
                    control->setMinValue(min);
                    control->blockSignals(false);
                  }
                  else if(c_dmax != max)
                  {
                    control->blockSignals(true);
                    control->setMaxValue(max);
                    control->blockSignals(false);
                  }
                  
                  if(hwVal == MusECore::CTRL_VAL_UNKNOWN)
                  {
                    hwVal = mcvl->lastValidHWVal();
                    if(hwVal == MusECore::CTRL_VAL_UNKNOWN)
                    {
                      hwVal = initval;
                      if(!control->isOff() || hwVal != control->value())
                      {
                        control->blockSignals(true);
                        control->setValueState(hwVal, true);
                        control->blockSignals(false);
                      }
                    }
                    else
                    {
                      hwVal -= bias;
                      if(!control->isOff() || hwVal != control->value())
                      {
                        control->blockSignals(true);
                        control->setValueState(hwVal, true);
                        control->blockSignals(false);
                      }  
                    }
                  }
                  else
                  {
                    hwVal -= bias;
                    if(control->isOff() || hwVal != control->value())
                    {
                      control->blockSignals(true);
                      control->setValueState(hwVal, false);
                      control->blockSignals(false);
                    }  
                  }  
                } 
                break;
              }
            }
            break;
          }
        }
      }
      break;
      
      case propertyComponent:
      {
        switch(cw._index)
        {
          case mStripInstrumentProperty:
          {
            const int port = _track->outPort();
            if(port >= 0 && port < MusECore::MIDI_PORTS)
            {
              if(MusECore::MidiInstrument* minstr = MusEGlobal::midiPorts[_track->outPort()].instrument())
              {
                //setComponentEnabled(cw, !minstr->isSynti());
                setComponentText(cw, minstr->iname());
              }
              else 
              {
                setComponentEnabled(cw, false);
                setComponentText(cw, tr("<unknown>"));
              }
            }
          } 
          break;
          
          case mStripTranspProperty:
            setComponentValue(cw, _track->transposition);  // Signals blocked. Redundant ignored.
          break;
          
          case mStripDelayProperty:
            setComponentValue(cw, _track->delay);  // Signals blocked. Redundant ignored.
          break;
          
          case mStripLenProperty:
            setComponentValue(cw, _track->len);  // Signals blocked. Redundant ignored.
          break;
          
          case mStripVeloProperty:
            setComponentValue(cw, _track->velocity);  // Signals blocked. Redundant ignored.
          break;
          
          case mStripComprProperty:
            setComponentValue(cw, _track->compression);  // Signals blocked. Redundant ignored.
          break;
        }
      }
      break;
    }
  }
}

//---------------------------------------------------------
//   instrPopup
//---------------------------------------------------------

void MidiComponentRack::instrPopup(QPoint p)
{
  const int port = _track->outPort();
  if(port < 0 || port >= MusECore::MIDI_PORTS)
    return;

  MusECore::MidiInstrument* instr = MusEGlobal::midiPorts[port].instrument();
  //if(!instr)
  //  return;
  
  PopupMenu* pup = new PopupMenu(false);
  
  //MusECore::MidiInstrument::populateInstrPopup(pup, port, false);
  MusECore::MidiInstrument::populateInstrPopup(pup, port, true);
  
  if(pup->actions().count() == 0)
  {
    delete pup;
    return;
  }  
  
  QAction *act = pup->exec(p);
  if(!act)
  {
    delete pup;
    return;
  }
  
  const QString s = act->text();
  const int actid = act->data().toInt();
  delete pup;
    
  // Edit instrument
  if(actid == 100)
  {
    MusEGlobal::muse->startEditInstrument(instr && !instr->isSynti() ? instr->iname() : QString());
  }
  else
  {
    for(MusECore::iMidiInstrument i = MusECore::midiInstruments.begin(); i != MusECore::midiInstruments.end(); ++i) 
    {
      if((*i)->iname() == s) 
      {
        MusEGlobal::audio->msgIdle(true); // Make it safe to edit structures
        MusEGlobal::midiPorts[port].changeInstrument(*i);
        MusEGlobal::audio->msgIdle(false);
        // Make sure device initializations are sent if necessary.
        MusEGlobal::audio->msgInitMidiDevices(false);  // false = Don't force
        MusEGlobal::song->update(SC_MIDI_INSTRUMENT);
        break;
      }
    }
  }
}

//---------------------------------------------------------
//   patchPopup
//---------------------------------------------------------

void MidiComponentRack::patchPopup(QPoint p)
{
  const int channel = _track->outChannel();
  const int port    = _track->outPort();
  if(channel < 0 || channel >= MusECore::MUSE_MIDI_CHANNELS || port < 0 || port >= MusECore::MIDI_PORTS)
    return;

  MusECore::MidiInstrument* instr = MusEGlobal::midiPorts[port].instrument();
  PopupMenu* pup = new PopupMenu(true);
  
  instr->populatePatchPopup(pup, channel, _track->isDrumTrack());

  if(pup->actions().count() == 0)
  {
    delete pup;
    return;
  }  
  
  connect(pup, SIGNAL(triggered(QAction*)), SLOT(patchPopupActivated(QAction*)));

  pup->exec(p);
  delete pup;      
}   

//---------------------------------------------------------
//   patchPopupActivated
//---------------------------------------------------------

void MidiComponentRack::patchPopupActivated(QAction* act)
{
  if(!act)
    return;
  
  const int channel = _track->outChannel();
  const int port    = _track->outPort();
  if(channel < 0 || channel >= MusECore::MUSE_MIDI_CHANNELS || port < 0 || port >= MusECore::MIDI_PORTS)
    return;

  MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
  MusECore::MidiInstrument* instr = mp->instrument();
  if(!instr)
    return;

  if(act->data().type() == QVariant::Int || act->data().type() == QVariant::UInt)
  {
    bool ok;
    int rv = act->data().toInt(&ok);
    if(ok && rv != -1)
    {
        // If the chosen patch's number is don't care (0xffffff),
        //  then by golly since we "don't care" let's just set it to '-/-/1'.
        // 0xffffff cannot be a valid patch number... yet...
        if(rv == MusECore::CTRL_PROGRAM_VAL_DONT_CARE)
          rv = 0xffff00;
        MusECore::MidiPlayEvent ev(MusEGlobal::audio->curFrame(), port, channel, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, rv);
        mp->putEvent(ev);
    }
  }
  else if(instr->isSynti() && act->data().canConvert<void *>())
  {
#ifdef LV2_SUPPORT
    MusECore::SynthI *si = static_cast<MusECore::SynthI *>(instr);
    MusECore::Synth *s = si->synth();
    //only for lv2 synths call applyPreset function.
    if(s && s->synthType() == MusECore::Synth::LV2_SYNTH)
    {
        MusECore::LV2SynthIF *sif = static_cast<MusECore::LV2SynthIF *>(si->sif());
        //be pedantic about checks
        if(sif)
        {
          if(mp)
          {
              if(mp->hwCtrlState(channel, MusECore::CTRL_PROGRAM) != MusECore::CTRL_VAL_UNKNOWN)
                mp->putHwCtrlEvent(MusECore::MidiPlayEvent(MusEGlobal::audio->curFrame(), port, channel,
                                                           MusECore::ME_CONTROLLER,
                                                           MusECore::CTRL_PROGRAM,
                                                           MusECore::CTRL_VAL_UNKNOWN));
              sif->applyPreset(act->data().value<void *>());
          }
        }
    }
#endif
  }
}

void MidiComponentRack::controllerChanged(int v, int id)
{
  DEBUG_MIDI_STRIP(stderr, "MidiComponentRack::controllerChanged id:%d val:%d\n", id, val);

//   if (inHeartBeat)
//         return;
  int val = v;
  int port     = _track->outPort();
  int channel  = _track->outChannel();
  if(channel < 0 || channel >= MusECore::MUSE_MIDI_CHANNELS || port < 0 || port >= MusECore::MIDI_PORTS)
  {
    emit componentChanged(controllerComponent, val, false, id, 0);
    return;
  }

  MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];

  MusECore::MidiCtrlValListList* mcvll = mp->controller();
  MusECore::ciMidiCtrlValList imcvl = mcvll->find(channel, id);
  if(imcvl == mcvll->end())
  {
    emit componentChanged(controllerComponent, val, false, id, 0);
    return;
  }

  MusECore::MidiController* mc = mp->midiController(id, channel, false);
  if(mc)
  {
    int ival = val;
    //if(off || ival < mc->minVal() || ival > mc->maxVal())
    if(ival < mc->minVal() || ival > mc->maxVal())
      ival = MusECore::CTRL_VAL_UNKNOWN;
    
    if(ival != MusECore::CTRL_VAL_UNKNOWN)
      // Auto bias...
      ival += mc->bias();

    MusECore::MidiPlayEvent ev(MusEGlobal::audio->curFrame(), port, channel, MusECore::ME_CONTROLLER, id, ival);
    mp->putEvent(ev);
  }

  emit componentChanged(controllerComponent, v, false, id, 0);
}

void MidiComponentRack::controllerChanged(double val, int id)
{
  DEBUG_MIDI_STRIP(stderr, "MidiComponentRack::controllerChanged id:%d val:%.20f\n", id, val);
  controllerChanged(int(lrint(val)), id);
}

void MidiComponentRack::controllerChanged(double val, bool off, int id, int scrollMode)
{
  DEBUG_MIDI_STRIP(stderr, "MidiComponentRack::controllerChanged id:%d val:%.20f scrollMode:%d\n", id, val, scrollMode);

  int port     = _track->outPort();
  int channel  = _track->outChannel();
  if(channel < 0 || channel >= MusECore::MUSE_MIDI_CHANNELS || port < 0 || port >= MusECore::MIDI_PORTS)
  {
    emit componentChanged(controllerComponent, val, off, id, scrollMode);
    return;
  }

  MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
  
  MusECore::MidiCtrlValListList* mcvll = mp->controller();
  MusECore::ciMidiCtrlValList imcvl = mcvll->find(channel, id);
  if(imcvl == mcvll->end())
  {
    emit componentChanged(controllerComponent, val, off, id, scrollMode);
    return;
  }
  
  MusECore::MidiController* mc = mp->midiController(id, channel, false);
  if(mc)
  {
    int ival = lrint(val);
    if(off || ival < mc->minVal() || ival > mc->maxVal())
      ival = MusECore::CTRL_VAL_UNKNOWN;
    
    if(ival != MusECore::CTRL_VAL_UNKNOWN)
      // Auto bias...
      ival += mc->bias();

    MusECore::MidiPlayEvent ev(MusEGlobal::audio->curFrame(), port, channel, MusECore::ME_CONTROLLER, id, ival);
    mp->putEvent(ev);
  }

  emit componentChanged(controllerComponent, val, off, id, scrollMode);
}

void MidiComponentRack::controllerMoved(double val, int id, bool shift_pressed)
{
  DEBUG_MIDI_STRIP(stderr, "MidiComponentRack::controllerMoved id:%d val:%.20f\n", id, val);
  emit componentMoved(controllerComponent, val, id, shift_pressed);
}

void MidiComponentRack::controllerPressed(double val, int id)
{
  DEBUG_MIDI_STRIP(stderr, "MidiComponentRack::controllerPressed id:%d\n", id);
  emit componentPressed(controllerComponent, val, id);
}

void MidiComponentRack::controllerReleased(double val, int id)
{
  DEBUG_MIDI_STRIP(stderr, "MidiComponentRack::controllerReleased id:%d\n", id);
  emit componentReleased(controllerComponent, val, id);
}

void MidiComponentRack::controllerRightClicked(QPoint p, int id)
{
  DEBUG_MIDI_STRIP(stderr, "MidiComponentRack::controllerRightClicked id:%d\n", id);
  MusEGlobal::song->execMidiAutomationCtlPopup(_track, 0, p, id); // Do not give a parent here, otherwise accelerators are returned in text() !
}


void MidiComponentRack::propertyChanged(double val, bool off, int id, int scrollMode)
{
  DEBUG_MIDI_STRIP(stderr, "MidiComponentRack::propertyChanged id:%d val:%.20f\n", id, val);
  
  const int ival = lrint(val);
  
  // FIXME ! This direct setting is probably not safe. Use a FIFO buffer or something. 
  //         A problem with using MidiPort::putEvent is that it does not appear to be safe
  //          when directly setting the hwValues.
  switch(id)
  {
    case mStripTranspProperty:
      _track->transposition = ival;
    break;
    
    case mStripDelayProperty:
      _track->delay = ival;
    break;
    
    case mStripLenProperty:
      _track->len = ival;
    break;
    
    case mStripVeloProperty:
      _track->velocity = ival;
    break;
    
    case mStripComprProperty:
      _track->compression = ival;
    break;
  }

  emit componentChanged(propertyComponent, val, off, id, scrollMode);
}

void MidiComponentRack::propertyMoved(double val, int id, bool shift_pressed)
{
  DEBUG_MIDI_STRIP(stderr, "MidiComponentRack::propertyMoved id:%d val:%.20f\n", id, val);
  emit componentMoved(propertyComponent, val, id, shift_pressed);
}

void MidiComponentRack::propertyPressed(double val, int id)
{
  emit componentPressed(propertyComponent, val, id);
}

void MidiComponentRack::propertyReleased(double val, int id)
{
  emit componentReleased(propertyComponent, val, id);
}

void MidiComponentRack::propertyRightClicked(QPoint, int)
{
  
}

void MidiComponentRack::labelPropertyPressed(QPoint p, int id, Qt::MouseButtons /*buttons*/, Qt::KeyboardModifiers keys)
{
  labelPropertyPressHandler(p, id, keys);
}

void MidiComponentRack::labelPropertyReleased(QPoint /*p*/, int /*id*/, Qt::MouseButtons /*buttons*/, Qt::KeyboardModifiers /*keys*/)
{
  
}

void MidiComponentRack::labelPropertyReturnPressed(QPoint p, int id, Qt::KeyboardModifiers keys)
{
  labelPropertyPressHandler(p, id, keys);
}

void MidiComponentRack::labelPropertyPressHandler(QPoint /*p*/, int id, Qt::KeyboardModifiers /*keys*/)
{
  switch(id)
  {
    case mStripInstrumentProperty:
    {
      ciComponentWidget icw = _components.find(propertyComponent, -1, id);
      if(icw == _components.end())
        return;

      const ComponentWidget& cw = *icw;
      if(!cw._widget)
        return;

      instrPopup(cw._widget->mapToGlobal(QPoint(10,5)));
    }
    break;
  }
}

void MidiComponentRack::patchEditNameClicked(QPoint /*p*/, int id)
{
  ciComponentWidget icw = _components.find(controllerComponent, -1, id);
  if(icw == _components.end())
    return;
  
  const ComponentWidget& cw = *icw;
  if(!cw._widget)
    return;
  
  patchPopup(cw._widget->mapToGlobal(QPoint(10,5)));
}


//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MidiComponentRack::songChanged(MusECore::SongChangedStruct_t flags)
{
  // Scan controllers.
  if(flags & (SC_RACK | SC_MIDI_CONTROLLER_ADD | SC_MIDI_INSTRUMENT))
  {
    scanControllerComponents();
  }
}

//---------------------------------------------------------
//   configChanged
//   Catch when label font, or configuration min slider and meter values change, or viewable tracks etc.
//---------------------------------------------------------

void MidiComponentRack::configChanged()    
{ 
  // Handle font changes etc.
  ComponentRack::configChanged();

  for(ciComponentWidget ic = _components.begin(); ic != _components.end(); ++ic)
  {
    const ComponentWidget& cw = *ic;
    if(!cw._widget)
      continue;
    
    // Whether to show values along with labels for certain controls.
    setComponentShowValue(cw, MusEGlobal::config.showControlValues);

    switch(cw._widgetType)
    {
      case mStripCompactPatchEditComponentWidget:
      {
        //CompactPatchEdit* w = static_cast<CompactPatchEdit*>(cw._widget);
        //w->setMaxAliasedPointSize(MusEGlobal::config.maxAliasedPointSize);
      }
      break;

      case CompactKnobComponentWidget:
      {
        //CompactKnob* w = static_cast<CompactKnob*>(cw._widget);
        //w->setMaxAliasedPointSize(MusEGlobal::config.maxAliasedPointSize);
      }
      break;
    }
  }
  setComponentColors();
}

//---------------------------------------------------------
//   setComponentColors
//---------------------------------------------------------

void MidiComponentRack::setComponentColors()
{ 
  for(ciComponentWidget ic = _components.begin(); ic != _components.end(); ++ic)
  {
    const ComponentWidget& cw = *ic;
    if(!cw._widget)
      continue;
    
    QColor color = MusEGlobal::config.sliderBackgroundColor;
    switch(cw._componentType)
    {
      case controllerComponent:
      {
        switch(cw._index)
        {
          case MusECore::CTRL_PANPOT:
            color = MusEGlobal::config.panSliderColor;
          break;
          
          case MusECore::CTRL_PROGRAM:
            color = MusEGlobal::config.midiPatchReadoutColor;
          break;
          
          default:
            color = MusEGlobal::config.midiControllerSliderColor;
          break;
        }
      }
      break;
      
      case propertyComponent:
      {
        switch(cw._index)
        {
          case mStripInstrumentProperty:
          break;
          
          case mStripTranspProperty:
          case mStripDelayProperty:
          case mStripLenProperty:
          case mStripVeloProperty:
          case mStripComprProperty:
            color = MusEGlobal::config.midiPropertySliderColor;
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
        w->setBarColor(MusEGlobal::config.sliderBarColor);
        w->setSlotColor(MusEGlobal::config.sliderBackgroundColor);
      }
      break;
      
      case mStripCompactPatchEditComponentWidget:
      {
        CompactPatchEdit* w = static_cast<CompactPatchEdit*>(cw._widget);
        w->setReadoutColor(color);
        w->setBgColor(MusEGlobal::config.midiInstrumentBackgroundColor);
        w->setBgActiveColor(MusEGlobal::config.midiInstrumentBgActiveColor);
        w->setBorderColor(MusEGlobal::config.midiInstrumentBorderColor);
        w->setFontColor(MusEGlobal::config.midiInstrumentFontColor);
        w->setFontActiveColor(MusEGlobal::config.midiInstrumentFontActiveColor);
      }
      break;

    case ElidedLabelComponentWidget:
    {
        ElidedLabel* w = static_cast<ElidedLabel*>(cw._widget);

        w->setBgColor(MusEGlobal::config.midiInstrumentBackgroundColor);
        w->setBgActiveColor(MusEGlobal::config.midiInstrumentBgActiveColor);
        w->setBorderColor(MusEGlobal::config.midiInstrumentBorderColor);
        w->setFontColor(MusEGlobal::config.midiInstrumentFontColor);
        w->setFontActiveColor(MusEGlobal::config.midiInstrumentFontActiveColor);
    }
        break;
    }
  }
}

QWidget* MidiComponentRack::setupComponentTabbing(QWidget* previousWidget)
{
  QWidget* prev = previousWidget;
  for(ciComponentWidget ic = _components.begin(); ic != _components.end(); ++ic)
  {
    const ComponentWidget& cw = *ic;
    if(cw._widget)
    {
      switch(cw._widgetType)
      {
        case mStripCompactPatchEditComponentWidget:
        {
          CompactPatchEdit* w = static_cast<CompactPatchEdit*>(cw._widget);
          prev = w->setupComponentTabbing(prev);
        }
        break;
        
        default:
          if(prev)
            QWidget::setTabOrder(prev, cw._widget);
          prev = cw._widget;
        break;
      }  
    }
  }
  return prev;
}


//---------------------------------------------------------
//   MidiStripProperties
//---------------------------------------------------------

MidiStripProperties::MidiStripProperties()
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
    _sliderScalePos = Slider::InsideVertical;
    _sliderFrame = false;
    _sliderFrameColor = Qt::darkGray;
    _meterWidth = Strip::FIXED_METER_WIDTH;
    _meterSpacing = 2;
    _meterFrame = false;
    _meterFrameColor = Qt::darkGray;
    ensurePolished();
}


//---------------------------------------------------------
//   MidiStrip
//---------------------------------------------------------

MidiStrip::MidiStrip(QWidget* parent, MusECore::MidiTrack* t, bool hasHandle, bool isEmbedded)
   : Strip(parent, t, hasHandle, isEmbedded)
      {
      inHeartBeat = true;
      _heartBeatCounter = 0;
      volume = MusECore::CTRL_VAL_UNKNOWN;

      _preferKnobs = MusEGlobal::config.preferKnobsVsSliders;
      _preferMidiVolumeDb = MusEGlobal::config.preferMidiVolumeDb;

      slider        = nullptr;
      sl            = nullptr;
      off           = nullptr;
      _recMonitor   = nullptr;

      // Start the layout in mode A (normal, racks on left).
      _isExpanded = false;

      // Set the whole strip's font, except for the label.
      setFont(MusEGlobal::config.fonts[1]); // For some reason must keep this, the upper rack is too tall at first.
      setStyleSheet(MusECore::font2StyleSheetFull(MusEGlobal::config.fonts[1])
//              + "QAbstractButton { margin: 2px 1px 2px 1px; padding: 1px; }"
              + "QAbstractButton { padding: 1px; }"
              + "#TrackOffButton { padding: 0px; }"
              + "QTabBar::tab { margin: 0px; padding: 2px 4px 2px 4px; }");

      // Clear so the meters don't start off by showing stale values.
      t->setActivity(0);
      t->setLastActivity(0);

      _routePos            = GridPosStruct(_curGridRow,     0, 1, 2);
      _upperStackTabPos    = GridPosStruct(_curGridRow + 1, 0, 1, 3);
      _sliderPos           = GridPosStruct(_curGridRow + 2, 0, 1, 2);
      _sliderLabelPos      = GridPosStruct(_curGridRow + 3, 0, 1, 2);
      _lowerRackPos        = GridPosStruct(_curGridRow + 4, 0, 1, 3);
      _bottomPos           = GridPosStruct(_curGridRow + 5, 0, 1, 2);

      //---------------------------------------------------
      //    routing
      //---------------------------------------------------

      QHBoxLayout *routeLayout = new QHBoxLayout;
      routeLayout->setContentsMargins(1,2,1,2);
      routeLayout->setSpacing(1);
//      iR = new IconButton(routingInputSVGIcon, routingInputSVGIcon,
//                          routingInputUnconnectedSVGIcon, routingInputUnconnectedSVGIcon, false, true);
      iR = new QPushButton(this);
      iR->setIcon(*routingInputSVGIcon);
      iR->setObjectName("InputRouteButton");
      iR->setStatusTip(tr("Intput routing. Press F1 for help."));
      iR->setFocusPolicy(Qt::NoFocus);
      iR->setToolTip(MusEGlobal::inputRoutingToolTipBase);
      connect(iR, SIGNAL(pressed()), SLOT(iRoutePressed()));
      routeLayout->addWidget(iR);
//      addGridWidget(iR, _inRoutesPos);

//      oR = new IconButton(routingOutputSVGIcon, routingOutputSVGIcon,
//                          routingOutputUnconnectedSVGIcon, routingOutputUnconnectedSVGIcon, false, true);
      oR = new QPushButton(this);
      oR->setIcon(*routingOutputSVGIcon);
      oR->setObjectName("OutputRouteButton");
      oR->setStatusTip(tr("Output routing. Press F1 for help."));
      oR->setFocusPolicy(Qt::NoFocus);
      oR->setToolTip(MusEGlobal::outputRoutingToolTipBase);
      connect(oR, SIGNAL(pressed()), SLOT(oRoutePressed()));
      routeLayout->addWidget(oR);
//      addGridWidget(oR, _outRoutesPos);

      updateRouteButtons();

      addGridLayout(routeLayout, _routePos);


      tabwidget = new QTabWidget(this);
      tabwidget->setObjectName("MidiStripTabWidget");
      tabwidget->setContentsMargins(0,4,0,0);
      tabwidget->setUsesScrollButtons(false);

      _infoRack = new MidiComponentRack(t, mStripInfoRack);
      _infoRack->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
      _infoRack->setContentsMargins(0,0,0,0);
      _infoRack->setFocusPolicy(Qt::NoFocus);

      _upperRack = new MidiComponentRack(t, mStripUpperRack);
      _upperRack->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
      _upperRack->setContentsMargins(0,0,0,0);
      _upperRack->setFocusPolicy(Qt::NoFocus);

      tabwidget->addTab(_upperRack, tr("Prop"));
      tabwidget->addTab(_infoRack, tr("Ctrl"));
      tabwidget->setTabToolTip(0, tr("Midi instruments and properties"));
      tabwidget->setTabToolTip(1, tr("Midi controllers"));
      tabwidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
      addGridWidget(tabwidget, _upperStackTabPos);

      //---------------------------------------------------
      //    slider, label, meter
      //---------------------------------------------------

      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[t->outPort()];
      const int chan  = t->outChannel();
      MusECore::MidiController* mc = mp->midiController(MusECore::CTRL_VOLUME, chan); // Auto-create the controller if necessary.

      slider = new Slider(nullptr, "vol", Qt::Vertical, Slider::InsideVertical, 14,
                          MusEGlobal::config.midiVolumeSliderColor, 
                          ScaleDraw::TextHighlightSplitAndShadow,
                          MusEGlobal::config.midiVolumeHandleColor);
      slider->setId(MusECore::CTRL_VOLUME);
      slider->setFocusPolicy(Qt::NoFocus);
      slider->setContentsMargins(0, 0, 0, 0);
      slider->setCursorHoming(true);
      slider->setSpecialText(tr("off"));
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

      _meterLayout = new MeterLayout(slider->scaleEndpointsMargin());
      _meterLayout->setMargin(0);
      _meterLayout->setSpacing(props.meterSpacing());

      meter[0] = new Meter(nullptr, Meter::LinMeter, Qt::Vertical, 0.0, 127.0);
      meter[0]->setRefreshRate(MusEGlobal::config.guiRefresh);
      meter[0]->setContentsMargins(0, 0, 0, 0);
      meter[0]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
      meter[0]->setFixedWidth(props.meterWidth());
      meter[0]->setPrimaryColor(MusEGlobal::config.midiMeterPrimaryColor,
                                MusEGlobal::config.meterBackgroundColor);
      meter[0]->setFrame(props.meterFrame(), props.meterFrameColor());
      connect(meter[0], SIGNAL(mousePress()), this, SLOT(resetPeaks()));
      _meterLayout->hlayout()->addWidget(meter[0], Qt::AlignHCenter);
      
      sliderGrid = new QGridLayout(); 
      sliderGrid->setSpacing(0);
      sliderGrid->setHorizontalSpacing(2);
      sliderGrid->setContentsMargins(2, 0, 4, 0);
//      sliderGrid->setContentsMargins(2, 2, 4, 2);
      sliderGrid->addWidget(slider, 0, 0, Qt::AlignHCenter);
      sliderGrid->addLayout(_meterLayout, 0, 1, Qt::AlignHCenter);
      
      addGridLayout(sliderGrid, _sliderPos);

      sl = new MusEGui::DoubleLabel(0.0, -98.0, 0.0);
      sl->setContentsMargins(0, 0, 0, 0);
      sl->setTextMargins(0, 0, 0, 0);
      sl->setFocusPolicy(Qt::WheelFocus);
      sl->setMouseTracking(true);
      sl->setFrame(true);
      sl->setAlignment(Qt::AlignCenter);
      //sl->setAutoFillBackground(true);

      //sl->setBackgroundRole(QPalette::Mid);
      sl->setSpecialText(tr("off"));
      sl->setToolTip(tr("Volume/gain\n(Ctrl-double-click on/off)"));
      sl->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
      // Set the label's slider 'buddy'.
      sl->setSlider(slider);
      sl->setEnableStyleHack(MusEGlobal::config.lineEditStyleHack);

      // Special for midi volume slider and label: Setup midi volume as decibel preference.
      setupMidiVolume();

      // If smart focus is on redirect strip focus to slider label.
      //if(MusEGlobal::config.smartFocus)
        setFocusProxy(sl);

      if(mc)
      {
        double dlv;
        double v = mp->hwDCtrlState(chan, MusECore::CTRL_VOLUME);
        if(MusECore::MidiController::dValIsUnknown(v))
        {
          double lastv = mp->lastValidHWDCtrlState(chan, MusECore::CTRL_VOLUME);
          if(MusECore::MidiController::dValIsUnknown(lastv))
          {
            if(mc->initValIsUnknown())
              v = 0.0;
            else
              v = double(mc->initVal());
          }
          else
            v = lastv - double(mc->bias());
          dlv = sl->off() - 1.0;
        }
        else
        {
          if(v <= 0.0)
            dlv = sl->minValue() - 0.5 * (sl->minValue() - sl->off());
          else
          {
            dlv = _preferMidiVolumeDb ? (muse_val2dbr(v / double(mc->maxVal())) * 2.0) : v;
            if(dlv > sl->maxValue())
              dlv = sl->maxValue();
          }
          // Auto bias...
          v -= double(mc->bias());
        }
        double slv;
        if(v <= 0.0)
        {
          if(_preferMidiVolumeDb)
            slv = MusEGlobal::config.minSlider;
          else
            slv = 0.0;
        }
        else
          slv = _preferMidiVolumeDb ? (muse_val2dbr(v / double(mc->maxVal())) * 2.0) : v;

        slider->setValue(slv);
        sl->setValue(dlv);
      }
        
      connect(slider, SIGNAL(valueChanged(double,int,int)), SLOT(setVolume(double,int,int)));
      connect(slider, SIGNAL(sliderRightClicked(QPoint,int)), SLOT(controlRightClicked(QPoint,int)));
      connect(slider, SIGNAL(sliderPressed(double, int)), SLOT(volumePressed(double, int)));
      connect(slider, SIGNAL(sliderReleased(double, int)), SLOT(volumeReleased(double, int)));
      connect(sl, SIGNAL(valueChanged(double, int)), SLOT(volLabelChanged(double)));
      connect(sl, SIGNAL(ctrlDoubleClicked(int)), SLOT(volLabelDoubleClicked()));
      
      addGridWidget(sl, _sliderLabelPos, Qt::AlignCenter);

      //---------------------------------------------------
      //    pan, balance
      //---------------------------------------------------

      _lowerRack = new MidiComponentRack(t, mStripLowerRack);
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
      
      _upperRack->setEnabled(!t->off());
      _infoRack->setEnabled(!t->off());
      _lowerRack->setEnabled(!t->off());
      
      //---------------------------------------------------
      //    mute, solo
      //    or
      //    record, mixdownfile
      //---------------------------------------------------

      QGridLayout *lowLayout = new QGridLayout;
      lowLayout->setContentsMargins(1,2,1,2);
      lowLayout->setSpacing(1);

      if (track && track->canRecordMonitor()) {
          //        _recMonitor = new IconButton(monitorOnSVGIcon, monitorOffSVGIcon, nullptr, nullptr, false, true);
          _recMonitor = new QPushButton;
          _recMonitor->setIcon(*monitorOnSVGIcon);
          _recMonitor->setFocusPolicy(Qt::NoFocus);
          _recMonitor->setCheckable(true);
          _recMonitor->setToolTip(tr("Input monitor"));
          _recMonitor->setWhatsThis(tr("Pass input through to output"));
          _recMonitor->setStatusTip(tr("Input monitor: Pass input through to output."));
          _recMonitor->setChecked(t->recMonitor());
          connect(_recMonitor, SIGNAL(toggled(bool)), SLOT(recMonitorToggled(bool)));
//          addGridWidget(_recMonitor, _offMonRecPos);
          lowLayout->addWidget(_recMonitor, 0, 0, 1, 1);
      } else {
          QPushButton *recMonitorx = new QPushButton(this);
          recMonitorx->setIcon(*monitorOnSVGIcon);
          recMonitorx->setEnabled(false);
//          addGridWidget(recMonitorx, _offMonRecPos);
          lowLayout->addWidget(recMonitorx, 0, 0, 1, 1);
      }

//      record  = new IconButton(recArmOnSVGIcon, recArmOffSVGIcon, 0, 0, false, true);
      record  = new QPushButton(this);
      record->setIcon(*recArmOnSVGIcon);
      record->setFocusPolicy(Qt::NoFocus);
      record->setCheckable(true);
      record->setToolTip(tr("Record arm"));
      record->setChecked(track->recordFlag());
      connect(record, SIGNAL(toggled(bool)), SLOT(recordToggled(bool)));
//      addGridWidget(record, _recPos);
      lowLayout->addWidget(record, 0, 1, 1, 1);

//      mute  = new IconButton(muteOnSVGIcon, muteOffSVGIcon, muteAndProxyOnSVGIcon, muteProxyOnSVGIcon, false, true);
      mute  = new QPushButton(this);
      mute->setIcon(*muteOnSVGIcon);
      mute->setFocusPolicy(Qt::NoFocus);
      mute->setCheckable(true);
      mute->setToolTip(tr("Mute or proxy mute"));
      mute->setChecked(track->mute());
      updateMuteIcon();
      connect(mute, SIGNAL(toggled(bool)), SLOT(muteToggled(bool)));
//      addGridWidget(mute, _mutePos);
      lowLayout->addWidget(mute, 1, 0, 1, 1);

//      solo  = new IconButton(soloOnSVGIcon, soloOffSVGIcon, soloAndProxyOnSVGIcon, soloProxyOnSVGIcon, false, true);
      solo  = new QPushButton(this);
      solo->setIcon(*soloOnSVGIcon);
      solo->setObjectName("SoloButton");
      solo->setStatusTip(tr("Solo or proxy solo. Press F1 for help."));
      solo->setFocusPolicy(Qt::NoFocus);
      solo->setToolTip(tr("Solo or proxy solo"));
      solo->setCheckable(true);
      if (track->internalSolo())
        solo->setIcon(*soloAndProxyOnSVGIcon);
      //      solo->setIconSetB(track->internalSolo());
      solo->setChecked(track->solo());
      connect(solo, SIGNAL(toggled(bool)), SLOT(soloToggled(bool)));
//      addGridWidget(solo, _soloPos);
      lowLayout->addWidget(solo, 1, 1, 1, 1);

//      off  = new IconButton(trackOffSVGIcon, trackOnSVGIcon, 0, 0, false, true);
      off = new QPushButton(this);
      off->setObjectName("TrackOffButton");
      off->setIcon(*trackOffSVGIcon);
      off->setFocusPolicy(Qt::NoFocus);
      off->setCheckable(true);
      off->setToolTip(tr("Track off"));
      off->setChecked(track->off());
      connect(off, SIGNAL(toggled(bool)), SLOT(offToggled(bool)));
//      addGridWidget(off, _offPos);
      lowLayout->addWidget(off, 3, 0, 1, 2);


      //---------------------------------------------------
      //    automation mode
      //---------------------------------------------------

      autoType = new CompactComboBox();
      autoType->setObjectName("MidiAutoType");
      autoType->setContentsMargins(0, 0, 0, 0);
      autoType->setFocusPolicy(Qt::NoFocus);
      autoType->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
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
//      autoType->addAction("", MusECore::AUTO_OFF);  // Just a dummy text to fix sizing problems. REMOVE later if full automation added.
      autoType->addAction("n/a", MusECore::AUTO_OFF);
      autoType->setCurrentItem(MusECore::AUTO_OFF);

//      addGridWidget(autoType, _automationPos);
      lowLayout->addWidget(autoType, 2, 0, 1, 2);

      addGridLayout(lowLayout, _bottomPos);

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

      // TODO: Activate this. But owners want to marshall this signal and send it themselves. Change that.
      //connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), SLOT(songChanged(MusECore::SongChangedStruct_t)));

      connect(MusEGlobal::heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));

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

      inHeartBeat = false;
      }

//---------------------------------------------------
//  buildStrip
//    Destroy and rebuild strip components.
//---------------------------------------------------

void MidiStrip::buildStrip()
{
  // Destroys all components and clears the component list.
  _infoRack->clearDelete();
  _upperRack->clearDelete();
  _lowerRack->clearDelete();

  //---------------------------------------------------
  //    Upper rack
  //---------------------------------------------------

  ElidedLabelComponentDescriptor instrPropertyDesc(ComponentRack::propertyComponent,
                                            "MixerStripInstrumentProperty",
                                            MidiComponentRack::mStripInstrumentProperty,
                                            Qt::ElideNone);
  _upperRack->newComponent(&instrPropertyDesc);

  CompactPatchEditComponentDescriptor progControllerDesc(ComponentRack::controllerComponent, "MixerStripMidiProgramController", MusECore::CTRL_PROGRAM);
  _upperRack->newComponent(&progControllerDesc);

  if(_preferKnobs)
  {
    CompactKnobComponentDescriptor varSendControllerDesc(ComponentRack::controllerComponent, "MixerStripMidiVarSendController", MusECore::CTRL_VARIATION_SEND);
    CompactKnobComponentDescriptor revSendControllerDesc(ComponentRack::controllerComponent, "MixerStripMidiRevSendController", MusECore::CTRL_REVERB_SEND);
    CompactKnobComponentDescriptor choSendControllerDesc(ComponentRack::controllerComponent, "MixerStripMidiChoSendController", MusECore::CTRL_CHORUS_SEND);
    _upperRack->newComponent(&varSendControllerDesc);
    _upperRack->newComponent(&revSendControllerDesc);
    _upperRack->newComponent(&choSendControllerDesc);
  }
  else
  {
    // To avoid too bright or annoying joined borders which are twice the normal width,
    //  show no bottom borders except for last one!
    CompactSliderComponentDescriptor varSendControllerDesc(
      ComponentRack::controllerComponent,
      "MixerStripMidiVarSendController",
      MusECore::CTRL_VARIATION_SEND,
      CompactSlider::AllBordersExceptBottom);
    CompactSliderComponentDescriptor revSendControllerDesc(
      ComponentRack::controllerComponent,
      "MixerStripMidiRevSendController",
      MusECore::CTRL_REVERB_SEND,
      CompactSlider::AllBordersExceptBottom);
    CompactSliderComponentDescriptor choSendControllerDesc(
      ComponentRack::controllerComponent,
      "MixerStripMidiChoSendController",
      MusECore::CTRL_CHORUS_SEND,
      CompactSlider::AllBorders);
    _upperRack->newComponent(&varSendControllerDesc);
    _upperRack->newComponent(&revSendControllerDesc);
    _upperRack->newComponent(&choSendControllerDesc);
  }

  // Keep this if dynamic layout (flip to right side) is desired.
  _upperRack->addStretch();

  updateRackSizes(true, false);


  //---------------------------------------------------
  //    Track properties rack
  //---------------------------------------------------

  if(_preferKnobs)
  {
    CompactKnobComponentDescriptor transpPropertyDesc(ComponentRack::propertyComponent, "MixerStripMidiTranspProperty", MidiComponentRack::mStripTranspProperty);
    CompactKnobComponentDescriptor delayPropertyDesc(ComponentRack::propertyComponent, "MixerStripMidiDelayProperty", MidiComponentRack::mStripDelayProperty);
    CompactKnobComponentDescriptor lenPropertyDesc(ComponentRack::propertyComponent, "MixerStripMidiLenProperty", MidiComponentRack::mStripLenProperty);
    CompactKnobComponentDescriptor veloPropertyDesc(ComponentRack::propertyComponent, "MixerStripMidiVeloProperty", MidiComponentRack::mStripVeloProperty);
    CompactKnobComponentDescriptor comprPropertyDesc(ComponentRack::propertyComponent, "MixerStripMidiComprProperty", MidiComponentRack::mStripComprProperty);

    _infoRack->newComponent(&transpPropertyDesc);
    _infoRack->newComponent(&delayPropertyDesc);
    _infoRack->newComponent(&lenPropertyDesc);
    _infoRack->newComponent(&veloPropertyDesc);
    _infoRack->newComponent(&comprPropertyDesc);
  }
  else
  {
    // To avoid too bright or annoying joined borders which are twice the normal width,
    //  show no bottom borders except for last one!
    CompactSliderComponentDescriptor transpPropertyDesc(
      ComponentRack::propertyComponent,
      "MixerStripMidiTranspProperty",
      MidiComponentRack::mStripTranspProperty,
      CompactSlider::AllBordersExceptBottom);
    CompactSliderComponentDescriptor delayPropertyDesc(
      ComponentRack::propertyComponent,
      "MixerStripMidiDelayProperty",
      MidiComponentRack::mStripDelayProperty,
      CompactSlider::AllBordersExceptBottom);
    CompactSliderComponentDescriptor lenPropertyDesc(
      ComponentRack::propertyComponent,
      "MixerStripMidiLenProperty",
      MidiComponentRack::mStripLenProperty,
      CompactSlider::AllBordersExceptBottom);
    CompactSliderComponentDescriptor veloPropertyDesc(
      ComponentRack::propertyComponent,
      "MixerStripMidiVeloProperty",
      MidiComponentRack::mStripVeloProperty,
      CompactSlider::AllBordersExceptBottom);
    CompactSliderComponentDescriptor comprPropertyDesc(
      ComponentRack::propertyComponent,
      "MixerStripMidiComprProperty",
      MidiComponentRack::mStripComprProperty,
      CompactSlider::AllBorders);

    _infoRack->newComponent(&transpPropertyDesc);
    _infoRack->newComponent(&delayPropertyDesc);
    _infoRack->newComponent(&lenPropertyDesc);
    _infoRack->newComponent(&veloPropertyDesc);
    _infoRack->newComponent(&comprPropertyDesc);
  }
  _infoRack->addStretch();


  //---------------------------------------------------
  //    Lower rack
  //---------------------------------------------------

  // Pan...
  if(_preferKnobs)
  {
    CompactKnobComponentDescriptor panControllerDesc
    (
      ComponentRack::controllerComponent,
      "MixerStripMidiPanController",
      MusECore::CTRL_PANPOT
    );
    _lowerRack->newComponent(&panControllerDesc);
  }
  else
  {
    CompactSliderComponentDescriptor panControllerDesc
    (
      ComponentRack::controllerComponent,
      "MixerStripMidiPanController",
      MusECore::CTRL_PANPOT
    );
    _lowerRack->newComponent(&panControllerDesc);
  }

  // Keep this if dynamic layout (flip to right side) is desired.
  _lowerRack->addStretch();

  updateRackSizes(false, true);
}

QWidget* MidiStrip::setupComponentTabbing(QWidget* previousWidget)
{
  QWidget* prev = previousWidget;
//  if(_upperStackTabButtonA)
//  {
//    if(prev)
//      QWidget::setTabOrder(prev, _upperStackTabButtonA);
//    prev = _upperStackTabButtonA;
//  }
//  if(_upperStackTabButtonB)
//  {
//    if(prev)
//      QWidget::setTabOrder(prev, _upperStackTabButtonB);
//    prev = _upperStackTabButtonB;
//  }
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

void MidiStrip::setupMidiVolume()
{
  const bool show_db = MusEGlobal::config.preferMidiVolumeDb;

  if(track && track->isMidiTrack())
  {
    const int num = MusECore::CTRL_VOLUME;
    MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(track);
    MusECore::MidiPort* mp = &MusEGlobal::midiPorts[mt->outPort()];
    const int chan = mt->outChannel();
    MusECore::MidiController* mc = mp->midiController(num, chan, false);
    if(!mc)
      return;
    const int mn = mc->minVal();
    const int mx = mc->maxVal();

    if(show_db)
    {
      slider->setRange(MusEGlobal::config.minSlider, volSliderMaxDb, volSliderStepDb);
      //slider->setScaleMaxMinor(5);
      slider->setScale(MusEGlobal::config.minSlider, volSliderMaxDb, 6.0, false);
      //slider->setSpecialText(tr("off"));
      //slider->setSpecialText(QString('-') + QChar(0x221e)); // The infinity character.

      sl->setPrecision(volSliderPrecDb);
      sl->setRange(MusEGlobal::config.minSlider, volSliderMaxDb);
      sl->setOff(MusEGlobal::config.minSlider);
      //sl->setSpecialText(tr("off"));
      //sl->setSpecialText(QString('-') + QChar(0x221e) + QChar(' ') + "dB");  // The infinity character.
      //sl->setToolTip(tr("Volume/gain"));
      sl->setSuffix("dB");
    }
    else
    {
      slider->setRange(double(mn), double(mx), volSliderStepLin);
      //slider->setScaleMaxMinor(5);
      slider->setScale(double(mn), double(mx), 10.0, false);
      //slider->setSpecialText(tr("off"));
      //slider->setSpecialText(QString('-') + QChar(0x221e)); // The infinity character.

      sl->setPrecision(0);
      sl->setRange(double(mn), double(mx));
      sl->setOff(double(mn) - 1.0); // Reset to default.
      //sl->setSpecialText(tr("off"));
      //sl->setSpecialText(QString('-') + QChar(0x221e)); // The infinity character.
      //sl->setToolTip(tr("Volume/gain\n(Ctrl-double-click on/off)"));
      sl->setSuffix(QString());
    }

    // Invalidate the cached volume so that the next heartbeat updates with a new value.
    volume = MusECore::CTRL_VAL_UNKNOWN;

    if(_preferMidiVolumeDb != show_db)
    {
      const int chan = mt->outChannel();
      const double d_lastv = mp->lastValidHWDCtrlState(chan, num);
      const double d_curv = mp->hwDCtrlState(chan, num);

      if(MusECore::MidiController::dValIsUnknown(d_curv))
      {
        // If no value has ever been set yet, use the current knob value
        //  (or the controller's initial value?) to 'turn on' the controller.
        if(MusECore::MidiController::dValIsUnknown(d_lastv))
        {
          double slider_v = slider->value();
          if(slider_v == 0.0)
          {
            if(show_db)
              slider_v = MusEGlobal::config.minSlider;
          }
          else
          {
            if(show_db)
              slider_v = muse_val2dbr(slider_v / double(mx)) * 2.0;
            else
              slider_v = double(mx) * muse_db2val(slider_v / 2.0);
          }

          slider->blockSignals(true);
          slider->setValue(slider_v);
          slider->blockSignals(false);
        }
      }
    }
  }

  _preferMidiVolumeDb = show_db;
}

//---------------------------------------------------------
//   updateOffState
//---------------------------------------------------------

void MidiStrip::updateOffState()
      {
      if(!track)
        return;

      bool val = !track->off();
      slider->setEnabled(val);
      sl->setEnabled(val);
      
      _upperRack->setEnabled(val);
      _infoRack->setEnabled(val);
      _lowerRack->setEnabled(val);
      
      label->setEnabled(val);

      if (_recMonitor)
            _recMonitor->setEnabled(val);
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
            }
      }

void MidiStrip::updateRackSizes(bool upper, bool lower)
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
// //     DEBUG_MIDI_STRIP(stderr, "MidiStrip::updateRackSizes: CompactSlider h:%d CompactPatchEdit h:%d instrLabel h:%d upper frame w:%d \n", 
// //                      csh, cpeh, ilh, _upperRack->frameWidth());
//     
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
    
    //DEBUG_MIDI_STRIP(stderr, "MidiStrip::updateRackSizes: lower frame w:%d \n", _lowerRack->frameWidth());
    
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
//   Catch when config label font changes, viewable tracks etc.
//---------------------------------------------------------

void MidiStrip::configChanged()
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
    //DEBUG_MIDI_STRIP(stderr, "MidiStrip::configChanged changing font: current size:%d\n", font().pointSize());
    setFont(MusEGlobal::config.fonts[1]);
    setStyleSheet(MusECore::font2StyleSheetFull(MusEGlobal::config.fonts[1]));
    // Update in case font changed.
    updateRackSizes(true, true);
  }
  // Update always, in case style, stylesheet, or font changed.
  //updateRackSizes(true, true);
  
  // Set the strip label's font.
  setLabelText();
  
  slider->setFillColor(MusEGlobal::config.midiVolumeSliderColor);
  slider->setHandleColor(MusEGlobal::config.midiVolumeHandleColor);

//  _upperStackTabButtonA->setBgColor(MusEGlobal::config.palSwitchBackgroundColor);
//  _upperStackTabButtonB->setBgColor(MusEGlobal::config.palSwitchBackgroundColor);
//  _upperStackTabButtonA->setBgActiveColor(MusEGlobal::config.palSwitchBgActiveColor);
//  _upperStackTabButtonB->setBgActiveColor(MusEGlobal::config.palSwitchBgActiveColor);
//  _upperStackTabButtonA->setBorderColor(MusEGlobal::config.palSwitchBorderColor);
//  _upperStackTabButtonB->setBorderColor(MusEGlobal::config.palSwitchBorderColor);
//  _upperStackTabButtonA->setFontColor(MusEGlobal::config.palSwitchFontColor);
//  _upperStackTabButtonB->setFontColor(MusEGlobal::config.palSwitchFontColor);
//  _upperStackTabButtonA->setFontActiveColor(MusEGlobal::config.palSwitchFontActiveColor);
//  _upperStackTabButtonB->setFontActiveColor(MusEGlobal::config.palSwitchFontActiveColor);

  // Enable special hack for line edits.
  if(sl->enableStyleHack() != MusEGlobal::config.lineEditStyleHack)
    sl->setEnableStyleHack(MusEGlobal::config.lineEditStyleHack);

  // Special for midi volume slider and label: Setup midi volume as decibel preference.
  setupMidiVolume();

  // REMOVE Tim. mixer. Added.
  // In case something in the slider changed, update the meter layout.
  // TODO This is somewhat crude, might miss automatic changes.
  // Later link up MeterLayout and Slider better.
  _meterLayout->setMeterEndsMargin(slider->scaleEndpointsMargin());
  
  _upperRack->configChanged();
  _infoRack->configChanged();
  _lowerRack->configChanged();
  
  // Adjust meter and colour.
  meter[0]->setPrimaryColor(MusEGlobal::config.midiMeterPrimaryColor,
                            MusEGlobal::config.meterBackgroundColor);
  meter[0]->setRefreshRate(MusEGlobal::config.guiRefresh);

  // If smart focus is on redirect strip focus to slider label.
//   if(MusEGlobal::config.smartFocus)
//     setFocusProxy(sl);
//   else
//     setFocusProxy(0);
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MidiStrip::songChanged(MusECore::SongChangedStruct_t val)
      {
      if (mute && (val & SC_MUTE)) {      // mute && off
            mute->blockSignals(true);
            mute->setChecked(track->mute());
            mute->blockSignals(false);
            updateMuteIcon();
            updateOffState();
            }
      if (solo && (val & (SC_SOLO | SC_ROUTE))) 
      {
            solo->blockSignals(true);
            solo->setChecked(track->solo());
            solo->blockSignals(false);
//            solo->setIconSetB(track->internalSolo());
            if (track->internalSolo()) {
                if (solo->isChecked())
                    solo->setIcon(*soloAndProxyOnSVGIcon);
                else
                    solo->setIcon(*soloProxyOnSVGIcon);
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
      
      // Catch when label font changes. 
      if (val & SC_CONFIG)
      {
        // So far only 1 instance of sending SC_CONFIG in the entire app, in instrument editor when a new instrument is saved. 
      }  
      
      _upperRack->songChanged(val);
      _infoRack->songChanged(val);
      _lowerRack->songChanged(val);
      
      if (val & SC_ROUTE) {
        updateRouteButtons();
      }
      
      if(val & SC_TRACK_REC_MONITOR)
      {
        // Set record monitor.
        if(_recMonitor)
        {
          _recMonitor->blockSignals(true);
          _recMonitor->setChecked(track->recMonitor());
          _recMonitor->blockSignals(false);
        }
      }
    }

//---------------------------------------------------------
//   controlRightClicked
//---------------------------------------------------------

void MidiStrip::controlRightClicked(QPoint p, int id)
{
  MusEGlobal::song->execMidiAutomationCtlPopup(static_cast<MusECore::MidiTrack*>(track), 0, p, id);
}

//void MidiStrip::upperStackTabButtonAPressed()
//{
//  _infoRack->hide();
//  _upperRack->show();
//  _upperStackTabButtonA->setOff(false);
//  _upperStackTabButtonB->setOff(true);
//}

//void MidiStrip::upperStackTabButtonBPressed()
//{
//  _upperRack->hide();
//  _infoRack->show();
//  _upperStackTabButtonA->setOff(true);
//  _upperStackTabButtonB->setOff(false);
//}

//---------------------------------------------------------
//   recMonitorToggled
//---------------------------------------------------------

void MidiStrip::recMonitorToggled(bool v)
{
  if(!track)
    return;
  // This is a minor operation easily manually undoable. Let's not clog the undo list with it.
  MusECore::PendingOperationList operations;
  operations.add(MusECore::PendingOperationItem(track, v, MusECore::PendingOperationItem::SetTrackRecMonitor));
  MusEGlobal::audio->msgExecutePendingOperations(operations, true);
}

//---------------------------------------------------------
//   volLabelDoubleClicked
//---------------------------------------------------------

void MidiStrip::volLabelDoubleClicked()
{
  const int num = MusECore::CTRL_VOLUME;
  const int outport = static_cast<MusECore::MidiTrack*>(track)->outPort();
  const int chan = static_cast<MusECore::MidiTrack*>(track)->outChannel();
  MusECore::MidiPort* mp = &MusEGlobal::midiPorts[outport];
  MusECore::MidiController* mc = mp->midiController(num, chan, false);
  if(!mc)
    return;
  
  const double lastv = mp->lastValidHWDCtrlState(chan, num);
  const double curv = mp->hwDCtrlState(chan, num);

  if(MusECore::MidiController::dValIsUnknown(curv))
  {
    // If no value has ever been set yet, use the current knob value 
    //  (or the controller's initial value?) to 'turn on' the controller.
    if(MusECore::MidiController::dValIsUnknown(lastv))
    {
      double slv = slider->value();
      if(_preferMidiVolumeDb)
        slv = double(mc->maxVal()) * muse_db2val(slv / 2.0);
      if(slv < double(mc->minVal()))
        slv = mc->minVal();
      if(slv > double(mc->maxVal()))
        slv = mc->maxVal();
      slv += double(mc->bias());

      mp->putControllerValue(outport, chan, num, slv, false);
    }
    else
    {
      mp->putControllerValue(outport, chan, num, lastv, false);
    }
  }  
  else
  {
    if(mp->hwCtrlState(chan, num) != MusECore::CTRL_VAL_UNKNOWN)
      mp->putHwCtrlEvent(MusECore::MidiPlayEvent(MusEGlobal::audio->curFrame(), outport, chan,
                                                  MusECore::ME_CONTROLLER,
                                                  num,
                                                  MusECore::CTRL_VAL_UNKNOWN));
  }
}

//---------------------------------------------------------
//   offToggled
//---------------------------------------------------------

void MidiStrip::offToggled(bool val)
      {
      if(!track)
        return;
      // This is a minor operation easily manually undoable. Let's not clog the undo list with it.
      MusECore::PendingOperationList operations;
      operations.add(MusECore::PendingOperationItem(track, val, MusECore::PendingOperationItem::SetTrackOff));
      MusEGlobal::audio->msgExecutePendingOperations(operations, true);
      }

//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void MidiStrip::heartBeat()
      {
      inHeartBeat = true;
      
      // Try to avoid calling MidiInstrument::getPatchName too often.
      if(++_heartBeatCounter >= 10)
        _heartBeatCounter = 0;
      
      if(track && track->isMidiTrack())
      {
        int act = track->activity();
        double m_val = slider->value();

        if(_preferMidiVolumeDb)
        {
          MusECore::MidiTrack* t = static_cast<MusECore::MidiTrack*>(track);
          const int port = t->outPort();
          MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
          const int chan = t->outChannel();
          MusECore::MidiController* mctl = mp->midiController(MusECore::CTRL_VOLUME, chan, false);
          if(mctl)
            m_val = double(mctl->maxVal()) * muse_db2val(m_val / 2.0);
          
          m_val += double(mctl->bias());
          
          if(m_val < double(mctl->minVal()))
            m_val = double(mctl->minVal());
          if(m_val > double(mctl->maxVal()))
            m_val = double(mctl->maxVal());
        }
        
        double dact = double(act) * (m_val / 127.0);
          
        if((int)dact > track->lastActivity())
          track->setLastActivity((int)dact);
        
        if(meter[0]) 
          meter[0]->setVal(dact, track->lastActivity(), false);  
        
        // Gives reasonable decay with gui update set to 20/sec.
        if(act)
          track->setActivity((int)((double)act * 0.8));
      }
      
      updateControls();
      
      _upperRack->updateComponents();
      _infoRack->updateComponents();
      _lowerRack->updateComponents();

      //if(_recMonitor && _recMonitor->isChecked() && MusEGlobal::blinkTimerPhase != _recMonitor->blinkPhase())
      //  _recMonitor->setBlinkPhase(MusEGlobal::blinkTimerPhase);

      Strip::heartBeat();
      inHeartBeat = false;
      }

//---------------------------------------------------------
//   updateControls
//---------------------------------------------------------

void MidiStrip::updateControls()
{
  MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(track);
  const int channel  = mt->outChannel();
  const int port  = mt->outPort();
  if(channel < 0 || channel >= MusECore::MUSE_MIDI_CHANNELS || port < 0 || port >= MusECore::MIDI_PORTS)
    return;

  MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
  MusECore::MidiCtrlValListList* mcvll = mp->controller();

  MusECore::ciMidiCtrlValList imcvl = mcvll->find(channel, MusECore::CTRL_VOLUME);
  const bool enable = imcvl != mcvll->end() && !mt->off();
  if(slider->isEnabled() != enable)
    slider->setEnabled(enable);
  if(sl->isEnabled() != enable)
    sl->setEnabled(enable);

  if(enable)
  {
    MusECore::MidiCtrlValList* mcvl = imcvl->second;
    double d_hwVal = mcvl->hwDVal();
    int max = 127;
    int bias = 0;
    MusECore::MidiController* mc = mp->midiController(MusECore::CTRL_VOLUME, channel, false);
    if(mc)
    {
      max = mc->maxVal();
      bias = mc->bias();
    }

    if(mcvl->hwValIsUnknown())
    {
      sl->setValue(sl->off() - 1.0);
      volume = MusECore::CTRL_VAL_UNKNOWN;

      d_hwVal = mcvl->lastValidHWDVal();
      if(mcvl->lastHwValIsUnknown())
      {
// TODO
//         if(!control->isOff())
//         {
//           control->blockSignals(true);
//           control->setOff(true);
//           control->blockSignals(false);
//         }
      }
      else
      {
        d_hwVal -= double(bias);

        double slider_v;
        if(d_hwVal <= 0.0)
        {
          if(_preferMidiVolumeDb)
            slider_v = MusEGlobal::config.minSlider;
          else
            slider_v = 0.0;
        }
        else
        {
          if(_preferMidiVolumeDb)
          {
            slider_v = muse_val2dbr(d_hwVal / double(max)) * 2.0;
            if(slider_v < MusEGlobal::config.minSlider)
              slider_v = MusEGlobal::config.minSlider;
          }
          else
            slider_v = d_hwVal;
        }

        if(slider_v != slider->value())
        {
          slider->blockSignals(true);
          slider->setValue(slider_v);
          slider->blockSignals(false);
        }
      }
    }
    else
    {
      double d_vol = d_hwVal;
      d_hwVal -= double(bias);
      if(d_hwVal != volume)
      {
        double slider_v;
        if(d_hwVal <= 0.0)
        {
          if(_preferMidiVolumeDb)
            slider_v = MusEGlobal::config.minSlider;
          else
            slider_v = 0.0;
        }
        else
        {
          if(_preferMidiVolumeDb)
          {
            slider_v = muse_val2dbr(d_hwVal / double(max)) * 2.0;
            if(slider_v < MusEGlobal::config.minSlider)
              slider_v = MusEGlobal::config.minSlider;
          }
          else
            slider_v = d_hwVal;
        }

        if(slider_v != slider->value())
        {
          slider->blockSignals(true);
          slider->setValue(slider_v);
          slider->blockSignals(false);
        }

        if(d_vol <= 0.0)
          sl->setValue(sl->minValue() - 0.5 * (sl->minValue() - sl->off()));
        else
        {
          double sl_v = _preferMidiVolumeDb ? (muse_val2dbr(d_vol / double(max)) * 2.0) : d_vol;

          if(sl_v > sl->maxValue())
            sl->setValue(sl->maxValue());
          else
            sl->setValue(sl_v);
        }

        volume = d_hwVal;
      }
    }
  }
}

//---------------------------------------------------------
//   ctrlChanged
//---------------------------------------------------------

void MidiStrip::ctrlChanged(double v, bool off, int num, int scrollMode)
    {
      if (inHeartBeat)
            return;
      if(!track || !track->isMidiTrack())
        return;

      MusECore::MidiTrack* t = static_cast<MusECore::MidiTrack*>(track);
      int port     = t->outPort();
      int chan  = t->outChannel();
      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
      MusECore::MidiController* mctl = mp->midiController(num, chan, false);
      if(mctl)
      {
        double m_val = v;
        if(_preferMidiVolumeDb)
          m_val = double(mctl->maxVal()) * muse_db2val(m_val / 2.0);

        if(off || (m_val < double(mctl->minVal())) || (m_val > double(mctl->maxVal())))
        {
          if(mp->hwCtrlState(chan, num) != MusECore::CTRL_VAL_UNKNOWN)
            mp->putHwCtrlEvent(MusECore::MidiPlayEvent(MusEGlobal::audio->curFrame(), port, chan,
                                                      MusECore::ME_CONTROLLER,
                                                      num,
                                                      MusECore::CTRL_VAL_UNKNOWN));
        }
        else
        {
          m_val += double(mctl->bias());
          mp->putControllerValue(port, chan, num, m_val, false);
        }
      }

      componentChanged(ComponentRack::controllerComponent, v, off, num, scrollMode);
    }

//---------------------------------------------------------
//   volLabelChanged
//---------------------------------------------------------

void MidiStrip::volLabelChanged(double val)
{
  ctrlChanged(val, false, MusECore::CTRL_VOLUME, SliderBase::ScrNone);
}
      
//---------------------------------------------------------
//   setVolume
//---------------------------------------------------------

void MidiStrip::setVolume(double val, int id, int scrollMode)
{
      DEBUG_MIDI_STRIP("Vol %d\n", lrint(val));
      ctrlChanged(val, false, id, scrollMode);
}

//---------------------------------------------------------
//   volumePressed
//---------------------------------------------------------

void MidiStrip::volumePressed(double val, int id)
      {
      DEBUG_MIDI_STRIP(stderr, "MidiStrip::volumePressed\n");
      if(!track || !track->isMidiTrack())
        return;
      componentPressed(ComponentRack::controllerComponent, val, id);
      }

//---------------------------------------------------------
//   volumeReleased
//---------------------------------------------------------

void MidiStrip::volumeReleased(double val, int id)
      {
      DEBUG_MIDI_STRIP(stderr, "MidiStrip::volumeReleased\n");
      if(!track || !track->isMidiTrack())
        return;
      componentReleased(ComponentRack::controllerComponent, val, id);
      }


//---------------------------------------------------------
//   iRoutePressed
//---------------------------------------------------------

void MidiStrip::iRoutePressed()
{
  RoutePopupMenu* pup = new RoutePopupMenu(0, false, _broadcastChanges);
  pup->exec(QCursor::pos(), track, false);
  delete pup;
  iR->setDown(false);     
}

//---------------------------------------------------------
//   oRoutePressed
//---------------------------------------------------------

void MidiStrip::oRoutePressed()
{
  RoutePopupMenu* pup = new RoutePopupMenu(0, true, _broadcastChanges);
  pup->exec(QCursor::pos(), track, true);
  delete pup;
  oR->setDown(false);     
}

void MidiStrip::incVolume(int incrementValue)
{
  if(!track || !track->isMidiTrack())
    return;

  const int id = MusECore::CTRL_VOLUME;

  MusECore::MidiTrack* t = static_cast<MusECore::MidiTrack*>(track);
  const int port     = t->outPort();
  const int chan  = t->outChannel();
  MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
  MusECore::MidiController* mctl = mp->midiController(id, chan, false);

  if(mctl)
  {
    // Get the slider's current value.
    const double prev_val = slider->value();
    double d_prev_val = prev_val;
    if(_preferMidiVolumeDb)
      d_prev_val = double(mctl->maxVal()) * muse_db2val(d_prev_val / 2.0);

    // Increment the slider. Do not allow signalling.
    slider->blockSignals(true);
    slider->incValue(incrementValue * 2);
    slider->blockSignals(false);
    // Now grab the control's new value.
    const double new_val = slider->value();

    double d_new_val = new_val;
    if(_preferMidiVolumeDb)
      d_new_val = double(mctl->maxVal()) * muse_db2val(d_new_val / 2.0);

    if((d_new_val < double(mctl->minVal())) || (d_new_val > double(mctl->maxVal())))
    {
      if(mp->hwCtrlState(chan, id) != MusECore::CTRL_VAL_UNKNOWN)
        mp->putHwCtrlEvent(MusECore::MidiPlayEvent(MusEGlobal::audio->curFrame(), port, chan,
                                                    MusECore::ME_CONTROLLER,
                                                    id,
                                                    MusECore::CTRL_VAL_UNKNOWN));
    }
    else
    {
      d_new_val += double(mctl->bias());
      mp->putControllerValue(port, chan, id, d_new_val, false);
    }

    componentIncremented(ComponentRack::controllerComponent,
                        prev_val, new_val,
                        false, id, Slider::ScrNone);
  }
}

void MidiStrip::incPan(int v)
{
  if(!track || !track->isMidiTrack())
    return;

  const int id = MusECore::CTRL_PANPOT;

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

  MusECore::MidiTrack* t = static_cast<MusECore::MidiTrack*>(track);
  const int port     = t->outPort();
  const int chan  = t->outChannel();
  MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
  MusECore::MidiController* mctl = mp->midiController(id, chan, false);
  if(mctl)
  {
    // Get the component's current value.
    double prev_val = rack->componentValue(*cw);
    // Now increment the component. Do not allow signalling.
    rack->incComponentValue(*cw, v, true);
    // Now grab its value.
    const double d_new_val = rack->componentValue(*cw);

    double d_fin_val = d_new_val;

    if((d_fin_val < double(mctl->minVal())) || (d_fin_val > double(mctl->maxVal())))
    {
      if(mp->hwCtrlState(chan, MusECore::CTRL_PANPOT) != MusECore::CTRL_VAL_UNKNOWN)
        mp->putHwCtrlEvent(MusECore::MidiPlayEvent(MusEGlobal::audio->curFrame(), port, chan,
                                                    MusECore::ME_CONTROLLER,
                                                    id,
                                                    MusECore::CTRL_VAL_UNKNOWN));
    }
    else
    {
      d_fin_val += double(mctl->bias());
      mp->putControllerValue(port, chan, id, d_fin_val, false);
    }

    componentIncremented(ComponentRack::controllerComponent,
                         prev_val, d_new_val,
                         false, id, Slider::ScrNone);
  }
}

} // namespace MusEGui
