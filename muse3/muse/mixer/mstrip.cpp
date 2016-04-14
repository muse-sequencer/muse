//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mstrip.cpp,v 1.9.2.13 2009/11/14 03:37:48 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 - 2016 Tim E. Real (terminator356 on sourceforge)
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
#include <QToolButton>
#include <QLabel>
#include <QComboBox>
#include <QToolTip>
#include <QTimer>
#include <QCursor>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QScrollArea>

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
#include "popupmenu.h"
#include "routepopup.h"

#include "minstrument.h"
#include "midievent.h"
#include "compact_slider.h"
#include "compact_patch_edit.h"
#include "scroll_area.h"
#include "elided_label.h"
#include "utils.h"
#include "muse_math.h"

#include "synth.h"
#ifdef LV2_SUPPORT
#include "lv2host.h"
#endif

// For debugging output: Uncomment the fprintf section.
#define DEBUG_MIDI_STRIP(dev, format, args...) // fprintf(dev, format, ##args);

namespace MusEGui {

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
  
  switch(desc->_componentType)
  {
    case controllerComponent:
    {
      const int midiCtrlNum = desc->_index;
      const int chan  = _track->outChannel();
      const int port  = _track->outPort();
      if(chan < 0 || chan >= MIDI_CHANNELS || port < 0 || port >= MIDI_PORTS)
        return;
      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
      MusECore::MidiController* mc = mp->midiController(midiCtrlNum);
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
        switch(midiCtrlNum)
        {
          case MusECore::CTRL_PROGRAM:
            desc->_label = tr("Pro");
          break;
          
          case MusECore::CTRL_VARIATION_SEND:
            desc->_label = tr("Var");
          break;

          case MusECore::CTRL_REVERB_SEND:
            desc->_label = tr("Rev");
          break;

          case MusECore::CTRL_CHORUS_SEND:
            desc->_label = tr("Cho");
          break;

          case MusECore::CTRL_PANPOT:
            desc->_label = tr("Pan");
          break;
          
          default:
            desc->_label = mc->name() + tr("\n(Ctrl-double-click on/off)");
          break;
        }
      }
      
      if(desc->_toolTipText.isEmpty())
      {
        switch(midiCtrlNum)
        {
          case MusECore::CTRL_PROGRAM:
            desc->_toolTipText = tr("Program");
          break;
          
          case MusECore::CTRL_VARIATION_SEND:
            desc->_toolTipText = tr("VariationSend\n(Ctrl-double-click on/off)");
          break;

          case MusECore::CTRL_REVERB_SEND:
            desc->_toolTipText = tr("ReverbSend\n(Ctrl-double-click on/off)");
          break;

          case MusECore::CTRL_CHORUS_SEND:
            desc->_toolTipText = tr("ChorusSend\n(Ctrl-double-click on/off)");
          break;

          case MusECore::CTRL_PANPOT:
            desc->_toolTipText = tr("Pan/Balance\n(Ctrl-double-click on/off)");
          break;
          
          default:
            desc->_toolTipText = mc->name() + tr("\n(Ctrl-double-click on/off)");
          break;
        }
      }
      
      if(!desc->_color.isValid())
      {
        switch(midiCtrlNum)
        {
          case MusECore::CTRL_PANPOT:
            desc->_color = MusEGlobal::config.panSliderColor;
          break;
          
          case MusECore::CTRL_PROGRAM:
            desc->_color = MusEGlobal::config.midiPatchSliderColor;
          break;
          
          default:
            desc->_color = MusEGlobal::config.midiControllerSliderDefaultColor;
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
          if(port >= 0 && port < MIDI_PORTS)
          {
            if(MusECore::MidiInstrument* minstr = MusEGlobal::midiPorts[_track->outPort()].instrument())
            {
              desc->_enabled = !minstr->isSynti();
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
            desc->_label = tr("Tran");
          if(desc->_toolTipText.isEmpty())
            desc->_toolTipText = tr("Transpose notes up or down");
          if(!desc->_color.isValid())
            desc->_color = MusEGlobal::config.midiPropertySliderDefaultColor;
        }
        break;
        
        case mStripDelayProperty:
        {
          val = _track->delay;
          min = -1000;
          max = 1000;
          if(desc->_label.isEmpty())
            desc->_label = tr("Dly");
          if(desc->_toolTipText.isEmpty())
            desc->_toolTipText = tr("Offset playback of notes before or after actual note");
          if(!desc->_color.isValid())
            desc->_color = MusEGlobal::config.midiPropertySliderDefaultColor;
        }
        break;
        
        case mStripLenProperty:
        {
          val = _track->len;
          min = 25;
          max = 200;
          if(desc->_label.isEmpty())
            desc->_label = tr("Len");
          if(desc->_toolTipText.isEmpty())
            desc->_toolTipText = tr("Change note length in percent of actual length");
          if(!desc->_color.isValid())
            desc->_color = MusEGlobal::config.midiPropertySliderDefaultColor;
        }
        break;
        
        case mStripVeloProperty:
        {
          val = _track->velocity;
          min = -127;
          max = 127;
          if(desc->_label.isEmpty())
            desc->_label = tr("Vel");
          if(desc->_toolTipText.isEmpty())
            desc->_toolTipText = tr("<html><head/><body><p>Add or substract velocity to notes"
                                     " on track.</p><p><span style="" font-style:italic;"">Since"
                                     " the midi note range is 0-127 this <br/>might mean that the"
                                     " notes do not reach <br/>the combined velocity, note + "
                                     " Velocity.</span></p></body></html>");
          if(!desc->_color.isValid())
            desc->_color = MusEGlobal::config.midiPropertySliderDefaultColor;
        }
        break;
        
        case mStripComprProperty:
        {
          val = _track->compression;
          min = 25;
          max = 200;
          if(desc->_label.isEmpty())
            desc->_label = tr("Cmp");
          if(desc->_toolTipText.isEmpty())
            desc->_toolTipText = tr("Compress the notes velocity range, in percent of actual velocity");
          if(!desc->_color.isValid())
            desc->_color = MusEGlobal::config.midiPropertySliderDefaultColor;
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
      if(!d->_color.isValid())
        d->_color = MusEGlobal::config.sliderDefaultColor;
      // Set the bar color the same.
      if(!d->_barColor.isValid())
        d->_barColor = d->_color;

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
        d->_color = MusEGlobal::config.midiPatchSliderColor;
      // Set the bar color the same.
      if(!d->_barColor.isValid())
        d->_barColor = d->_color;
      
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
        CompactPatchEdit* control = new CompactPatchEdit(0, d->_objName, Qt::Horizontal, CompactSlider::None);
        d->_compactPatchEdit = control;
        control->setId(d->_index);
        control->setValueState(d->_initVal, d->_isOff);
        // Don't allow anything here, it interferes with the CompactPatchEdit which sets it's own controls' tooltips.
        //control->setToolTip(d->_toolTipText);
        control->setEnabled(d->_enabled);
        control->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
        control->setContentsMargins(0, 0, 0, 0);

        if(d->_color.isValid())
          control->setBorderColor(d->_color);
        if(d->_barColor.isValid())
          control->setBarColor(d->_barColor);
        if(d->_slotColor.isValid())
          control->setSlotColor(d->_slotColor);
        if(d->_thumbColor.isValid())
          control->setThumbColor(d->_thumbColor);
        
        control->setMaxAliasedPointSize(MusEGlobal::config.maxAliasedPointSize);
        
        if(d->_patchEditChangedSlot)
          connect(d->_compactPatchEdit, SIGNAL(valueStateChanged(double,bool,int, int)), d->_patchEditChangedSlot);
        if(d->_patchEditSliderRightClickedSlot)
          connect(d->_compactPatchEdit, SIGNAL(sliderRightClicked(QPoint,int)), d->_patchEditSliderRightClickedSlot);
        if(d->_patchEditNameClickedSlot)
          connect(d->_compactPatchEdit, SIGNAL(patchNameClicked(QPoint,int)), d->_patchEditNameClickedSlot);
        if(d->_patchEditNameRightClickedSlot)
          connect(d->_compactPatchEdit, SIGNAL(patchNameRightClicked(QPoint,int)), d->_patchEditNameRightClickedSlot);
    
        if(!d->_patchEditChangedSlot)
          connect(d->_compactPatchEdit, SIGNAL(valueStateChanged(double,bool,int,int)), SLOT(controllerChanged(double,bool,int,int)));
        if(!d->_patchEditSliderRightClickedSlot)
          connect(d->_compactPatchEdit, SIGNAL(sliderRightClicked(QPoint,int)), SLOT(controllerRightClicked(QPoint,int)));
        if(!d->_patchEditNameClickedSlot)
          connect(d->_compactPatchEdit, SIGNAL(patchNameClicked(QPoint,int)), SLOT(patchEditNameClicked(QPoint,int)));
        if(!d->_patchEditNameRightClickedSlot)
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
  if(chan < 0 || chan >= MIDI_CHANNELS || port < 0 || port >= MIDI_PORTS)
    return;
  
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
        MusECore::MidiCtrlValListList* mcvll = MusEGlobal::midiPorts[port].controller();
        if(mcvll->find(chan, cw._index) == mcvll->end())
          to_be_erased.push_back(ic);
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
      delete cw._widget;
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
        if(channel < 0 || channel >= MIDI_CHANNELS || port < 0 || port >= MIDI_PORTS)
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
                  CompactPatchEdit* control = static_cast<CompactPatchEdit*>(cw._widget);
                  int hwVal = mcvl->hwVal();
                  if(hwVal == MusECore::CTRL_VAL_UNKNOWN)
                  {
                    hwVal = mcvl->lastValidHWVal();
                    if(hwVal == MusECore::CTRL_VAL_UNKNOWN)
                    {
                      if(!control->isOff())
                      {
                        control->blockSignals(true);
                        control->setOff(true);
                        control->blockSignals(false);
                      }
                    }
                    else
                    {
                      if(!control->isOff() || double(hwVal) != control->value())
                      {
                        control->blockSignals(true);
                        control->setValueState(double(hwVal), true);
                        control->blockSignals(false);
                      }  
                    }
                  }
                  else
                  {
                    if(control->isOff() || double(hwVal) != control->value()) 
                    {
                      control->blockSignals(true);
                      control->setValueState(double(hwVal), false);
                      control->blockSignals(false);
                    }  
                    
                  }  
                  
                  if(hwVal == MusECore::CTRL_VAL_UNKNOWN)
                  {
                    const QString patchName(tr("<unknown>"));
                    if(control->patchName() != patchName)
                      control->setPatchName(patchName);
                  }
                  else
                  {
                    // Try to avoid calling MidiInstrument::getPatchName too often.
//                     if(_heartBeatCounter == 0)
                    {
                      MusECore::MidiInstrument* instr = mp->instrument();
                      QString patchName(instr->getPatchName(channel, hwVal, _track->isDrumTrack()));
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
                case CompactSliderComponentWidget:
                {                  
                  CompactSlider* control = static_cast<CompactSlider*>(cw._widget);
                  
                  int hwVal = mcvl->hwVal();
                  int min = 0;
                  int max = 127;
                  int bias = 0;
                  MusECore::MidiController* mc = mp->midiController(cw._index);
                  if(mc)
                  {
                    bias = mc->bias();
                    min = mc->minVal();
                    max = mc->maxVal();
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
                      if(!control->isOff())
                      {
                        control->blockSignals(true);
                        control->setOff(true);
                        control->blockSignals(false);
                      }
                    }
                    else
                    {
                      hwVal -= bias;
                      if(!control->isOff() || double(hwVal) != control->value())
                      {
                        control->blockSignals(true);
                        control->setValueState(double(hwVal), true);
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
                      control->setValueState(double(hwVal), false);
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
            if(port >= 0 && port < MIDI_PORTS)
            {
              if(MusECore::MidiInstrument* minstr = MusEGlobal::midiPorts[_track->outPort()].instrument())
              {
                setComponentEnabled(cw, !minstr->isSynti());
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
  if(port < 0 || port >= MIDI_PORTS)
    return;

  MusECore::MidiInstrument* instr = MusEGlobal::midiPorts[port].instrument();
  if(!instr)
    return;
  
  PopupMenu* pup = new PopupMenu(false);
  
  MusECore::MidiInstrument::populateInstrPopup(pup, instr, false);

  if(pup->actions().count() == 0)
  {
    delete pup;
    return;
  }  
  
  QAction *act = pup->exec(p);
  if(act) 
  {
    QString s = act->text();
    for(MusECore::iMidiInstrument i = MusECore::midiInstruments.begin(); i != MusECore::midiInstruments.end(); ++i) 
    {
      if((*i)->iname() == s) 
      {
        MusEGlobal::audio->msgIdle(true); // Make it safe to edit structures
        MusEGlobal::midiPorts[port].setInstrument(*i);
        MusEGlobal::audio->msgIdle(false);
        // Make sure device initializations are sent if necessary.
        MusEGlobal::audio->msgInitMidiDevices(false);  // false = Don't force
        MusEGlobal::song->update(SC_MIDI_INSTRUMENT);
        break;
      }
    }
  }
  delete pup;      
}

//---------------------------------------------------------
//   patchPopup
//---------------------------------------------------------

void MidiComponentRack::patchPopup(QPoint p)
{
  const int channel = _track->outChannel();
  const int port    = _track->outPort();
  if(channel < 0 || channel >= MIDI_CHANNELS || port < 0 || port >= MIDI_PORTS)
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
  if(channel < 0 || channel >= MIDI_CHANNELS || port < 0 || port >= MIDI_PORTS)
    return;
    
  MusECore::MidiInstrument* instr = MusEGlobal::midiPorts[port].instrument();
  if(!instr)
    return;
  
  if(act->data().type() == QVariant::Int)
  {
    int rv = act->data().toInt();
    if(rv != -1)
    {
        //++_blockHeartbeatCount;
        MusECore::MidiPlayEvent ev(0, port, channel, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, rv);
        MusEGlobal::audio->msgPlayMidiEvent(&ev);
        //updateTrackInfo(-1);
        //--_blockHeartbeatCount;
    }
  }
  else if(instr->isSynti() && act->data().canConvert<void *>())
  {
    MusECore::SynthI *si = static_cast<MusECore::SynthI *>(instr);
    MusECore::Synth *s = si->synth();
#ifdef LV2_SUPPORT
    //only for lv2 synths call applyPreset function.
    if(s && s->synthType() == MusECore::Synth::LV2_SYNTH)
    {
        MusECore::LV2SynthIF *sif = static_cast<MusECore::LV2SynthIF *>(si->sif());
        //be pedantic about checks
        if(sif)
        {
          MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
          if(mp)
          {
              if(mp->hwCtrlState(channel, MusECore::CTRL_PROGRAM) != MusECore::CTRL_VAL_UNKNOWN)
                MusEGlobal::audio->msgSetHwCtrlState(mp, channel, MusECore::CTRL_PROGRAM, MusECore::CTRL_VAL_UNKNOWN);
              sif->applyPreset(act->data().value<void *>());
          }
        }
    }
#endif
  }
}


void MidiComponentRack::controllerChanged(double val, bool off, int id, int /*scrollMode*/)
{
  DEBUG_MIDI_STRIP(stderr, "MidiComponentRack::controllerChanged id:%d val:%.20f scrollMode:%d\n", id, val, scrollMode);
//   if (inHeartBeat)
//         return;
  int port     = _track->outPort();
  int channel  = _track->outChannel();
  if(channel < 0 || channel >= MIDI_CHANNELS || port < 0 || port >= MIDI_PORTS)
    return;
  int ival = lrint(val);

  MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
  
  MusECore::MidiCtrlValListList* mcvll = mp->controller();
  MusECore::ciMidiCtrlValList imcvl = mcvll->find(channel, id);
  if(imcvl == mcvll->end())
    return;
  
  MusECore::MidiController* mc = mp->midiController(id);
  MusECore::MidiCtrlValList* mcvl = imcvl->second;
  
  if(off || (ival < mc->minVal()) || (ival > mc->maxVal()))
  {
    if(mcvl->hwVal() != MusECore::CTRL_VAL_UNKNOWN)
      MusEGlobal::audio->msgSetHwCtrlState(mp, channel, id, MusECore::CTRL_VAL_UNKNOWN);
  }  
  else
  {
    ival += mc->bias();
    int tick     = MusEGlobal::song->cpos();
    MusECore::MidiPlayEvent ev(tick, port, channel, MusECore::ME_CONTROLLER, id, ival);
    MusEGlobal::audio->msgPlayMidiEvent(&ev);
  }  
}

void MidiComponentRack::controllerMoved(double /*val*/, int /*id*/, bool /*shift_pressed*/)
{
  DEBUG_MIDI_STRIP(stderr, "MidiComponentRack::controllerMoved id:%d val:%.20f\n", id, val);
}

void MidiComponentRack::controllerPressed(int /*id*/)
{
  DEBUG_MIDI_STRIP(stderr, "MidiComponentRack::controllerPressed id:%d\n", id);
}

void MidiComponentRack::controllerReleased(int /*id*/)
{
  DEBUG_MIDI_STRIP(stderr, "MidiComponentRack::controllerReleased id:%d\n", id);
}

void MidiComponentRack::controllerRightClicked(QPoint p, int id)
{
  DEBUG_MIDI_STRIP(stderr, "MidiComponentRack::controllerRightClicked id:%d\n", id);
  MusEGlobal::song->execMidiAutomationCtlPopup(_track, 0, p, id); // Do not give a parent here, otherwise accelerators are returned in text() !
}


void MidiComponentRack::propertyChanged(double val, bool, int id, int /*scrollMode*/)
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
}

void MidiComponentRack::propertyMoved(double /*val*/, int /*id*/, bool /*shift_pressed*/)
{
  DEBUG_MIDI_STRIP(stderr, "MidiComponentRack::propertyMoved id:%d val:%.20f\n", id, val);
}

void MidiComponentRack::propertyPressed(int)
{
  
}

void MidiComponentRack::propertyReleased(int)
{
  
}

void MidiComponentRack::propertyRightClicked(QPoint, int)
{
  
}

void MidiComponentRack::labelPropertyPressed(QPoint /*p*/, int id, Qt::MouseButtons /*buttons*/, Qt::KeyboardModifiers /*keys*/)
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

void MidiComponentRack::labelPropertyReleased(QPoint /*p*/, int /*id*/, Qt::MouseButtons /*buttons*/, Qt::KeyboardModifiers /*keys*/)
{
  
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

void MidiComponentRack::songChanged(MusECore::SongChangedFlags_t flags)
{
  // Scan controllers.
  if(flags & (SC_RACK | SC_MIDI_CONTROLLER_ADD))
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
    
    switch(cw._widgetType)
    {
      case mStripCompactPatchEditComponentWidget:
      {
        CompactPatchEdit* w = static_cast<CompactPatchEdit*>(cw._widget);
        w->setMaxAliasedPointSize(MusEGlobal::config.maxAliasedPointSize);
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
    
    QColor color = MusEGlobal::config.sliderDefaultColor;
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
            color = MusEGlobal::config.midiPatchSliderColor;
          break;
          
          default:
            color = MusEGlobal::config.midiControllerSliderDefaultColor;
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
            color = MusEGlobal::config.midiPropertySliderDefaultColor;
          break;
        }
      }
      break;
    }  

    switch(cw._widgetType)
    {
      case CompactSliderComponentWidget:
      {
        CompactSlider* w = static_cast<CompactSlider*>(cw._widget);
        w->setBorderColor(color);
        w->setBarColor(color);
      }
      break;
      
      case mStripCompactPatchEditComponentWidget:
      {
        CompactPatchEdit* w = static_cast<CompactPatchEdit*>(cw._widget);
        w->setBorderColor(color);
        w->setBarColor(color);
      }
      break;
    }  
  }
}

//---------------------------------------------------------
//   MidiStrip
//---------------------------------------------------------

MidiStrip::MidiStrip(QWidget* parent, MusECore::MidiTrack* t, bool hasHandle)
   : Strip(parent, t, hasHandle)
      {
      inHeartBeat = true;
      _heartBeatCounter = 0;

      // Start the layout in mode A (normal, racks on left).
      _isExpanded = false;
      
      // Set the whole strip's font, except for the label.
      setFont(MusEGlobal::config.fonts[1]); // For some reason must keep this, the upper rack is too tall at first.
      setStyleSheet(MusECore::font2StyleSheet(MusEGlobal::config.fonts[1]));
      
      // Clear so the meters don't start off by showing stale values.
      t->setActivity(0);
      t->setLastActivity(0);
      
      _preScrollAreaPos_A  = GridPosStruct(_curGridRow,     0, 1, 3);
      
      _preScrollAreaPos_B  = GridPosStruct(_curGridRow + 1, 2, 1, 1);
      _sliderPos           = GridPosStruct(_curGridRow + 1, 0, 4, 2);

      _infoSpacerTop       = GridPosStruct(_curGridRow + 2, 2, 1, 1);
      
      _propertyRackPos     = GridPosStruct(_curGridRow + 3, 2, 1, 1);
      
      _infoSpacerBottom    = GridPosStruct(_curGridRow + 4, 2, 1, 1);
      
      _sliderLabelPos      = GridPosStruct(_curGridRow + 5, 0, 1, 2);
      _postScrollAreaPos_B = GridPosStruct(_curGridRow + 5, 2, 1, 1);
      
      _postScrollAreaPos_A = GridPosStruct(_curGridRow + 6, 0, 1, 3);
      
      _offPos              = GridPosStruct(_curGridRow + 7, 0, 1, 1);
      _recPos              = GridPosStruct(_curGridRow + 7, 1, 1, 1);
      
      _mutePos             = GridPosStruct(_curGridRow + 8, 0, 1, 1);
      _soloPos             = GridPosStruct(_curGridRow + 8, 1, 1, 1);
      
      _routesPos        = GridPosStruct(_curGridRow + 9, 0, 1, 2);
      
      _automationPos       = GridPosStruct(_curGridRow + 10, 0, 1, 2);
      
      _rightSpacerPos      = GridPosStruct(_curGridRow + 10, 2, 1, 1);

      volume      = MusECore::CTRL_VAL_UNKNOWN;
      
      _infoRack = new MidiComponentRack(t, mStripInfoRack);
      
      //_infoRack->setVisible(false); // Not visible unless expanded.
      
      // FIXME For some reason StyledPanel has trouble, intermittent sometimes panel is drawn, sometimes not. 
//       _infoRack->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
      _infoRack->setFrameStyle(QFrame::Box | QFrame::Sunken);
      
      _infoRack->setLineWidth(rackFrameWidth);
      _infoRack->setMidLineWidth(0);
      _infoRack->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
      _infoRack->setContentsMargins(rackFrameWidth, rackFrameWidth, rackFrameWidth, rackFrameWidth);

      CompactSliderComponentDescriptor transpPropertyDesc(ComponentRack::propertyComponent, "MixerStripMidiTranspProperty", MidiComponentRack::mStripTranspProperty);
      CompactSliderComponentDescriptor delayPropertyDesc(ComponentRack::propertyComponent, "MixerStripMidiDelayProperty", MidiComponentRack::mStripDelayProperty);
      CompactSliderComponentDescriptor lenPropertyDesc(ComponentRack::propertyComponent, "MixerStripMidiLenProperty", MidiComponentRack::mStripLenProperty);
      CompactSliderComponentDescriptor veloPropertyDesc(ComponentRack::propertyComponent, "MixerStripMidiVeloProperty", MidiComponentRack::mStripVeloProperty);
      CompactSliderComponentDescriptor comprPropertyDesc(ComponentRack::propertyComponent, "MixerStripMidiComprProperty", MidiComponentRack::mStripComprProperty);
      
      _infoRack->newComponent(&transpPropertyDesc);
      _infoRack->newComponent(&delayPropertyDesc);
      _infoRack->newComponent(&lenPropertyDesc);
      _infoRack->newComponent(&veloPropertyDesc);
      _infoRack->newComponent(&comprPropertyDesc);
      
      _infoRack->addStretch();
      
      addGridWidget(_infoRack, _propertyRackPos);
                  
      grid->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::Expanding), 
                    _infoSpacerTop._row, _infoSpacerTop._col, _infoSpacerTop._rowSpan, _infoSpacerTop._colSpan);

      
      _upperRack = new MidiComponentRack(t, mStripUpperRack);
      
      // FIXME For some reason StyledPanel has trouble, intermittent sometimes panel is drawn, sometimes not. 
//       _upperRack->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
      _upperRack->setFrameStyle(QFrame::Box | QFrame::Sunken);
      
      _upperRack->setLineWidth(rackFrameWidth);
      _upperRack->setMidLineWidth(0);
      // We do set a minimum height on this widget. Tested: Must be on fixed. Thankfully, it'll expand if more controls are added.
      _upperRack->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
      _upperRack->setContentsMargins(rackFrameWidth, rackFrameWidth, rackFrameWidth, rackFrameWidth);

      ElidedLabelComponentDescriptor instrPropertyDesc(ComponentRack::propertyComponent, 
                                                "MixerStripInstrumentProperty", 
                                                MidiComponentRack::mStripInstrumentProperty);
      _upperRack->newComponent(&instrPropertyDesc);
      
      CompactPatchEditComponentDescriptor progControllerDesc(ComponentRack::controllerComponent, "MixerStripMidiProgramController", MusECore::CTRL_PROGRAM);
      CompactSliderComponentDescriptor varSendControllerDesc(ComponentRack::controllerComponent, "MixerStripMidiVarSendController", MusECore::CTRL_VARIATION_SEND);
      CompactSliderComponentDescriptor revSendControllerDesc(ComponentRack::controllerComponent, "MixerStripMidiRevSendController", MusECore::CTRL_REVERB_SEND);
      CompactSliderComponentDescriptor choSendControllerDesc(ComponentRack::controllerComponent, "MixerStripMidiChoSendController", MusECore::CTRL_CHORUS_SEND);
      
      _upperRack->newComponent(&progControllerDesc);
      _upperRack->newComponent(&varSendControllerDesc);
      _upperRack->newComponent(&revSendControllerDesc);
      _upperRack->newComponent(&choSendControllerDesc);
      
      
// Keep this if dynamic layout (flip to right side) is desired.
      _upperRack->addStretch();

      updateRackSizes(true, false);

      addGridWidget(_upperRack, _preScrollAreaPos_A);
      
      //---------------------------------------------------
      //    slider, label, meter
      //---------------------------------------------------

      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[t->outPort()];
      MusECore::MidiController* mc = mp->midiController(MusECore::CTRL_VOLUME);
      int chan  = t->outChannel();
      int mn = mc->minVal();
      int mx = mc->maxVal();
      
      slider = new Slider(0, "vol", Qt::Vertical, Slider::InsideVertical, 14, 
                          MusEGlobal::config.midiVolumeSliderColor, 
                          ScaleDraw::TextHighlightSplitAndShadow);
      slider->setContentsMargins(0, 0, 0, 0);
      slider->setCursorHoming(true);
      //slider->setThumbLength(1);
      slider->setRange(double(mn), double(mx), 1.0);
      //slider->setScaleMaxMinor(5);
      slider->setScale(double(mn), double(mx), 10.0, false);
      slider->setScaleBackBone(false);
      //slider->setFillThumb(false);
      slider->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
      slider->setId(MusECore::CTRL_VOLUME);

      meter[0] = new MusEGui::Meter(0, MusEGui::Meter::LinMeter);
      meter[0]->setContentsMargins(0, 0, 0, 0);
      meter[0]->setRange(0, 127.0);
      meter[0]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
      meter[0]->setFixedWidth(FIXED_METER_WIDTH);
      meter[0]->setPrimaryColor(MusEGlobal::config.midiMeterPrimaryColor);
      connect(meter[0], SIGNAL(mousePress()), this, SLOT(resetPeaks()));
      
      sliderGrid = new QGridLayout(); 
      sliderGrid->setSpacing(0);
      sliderGrid->setContentsMargins(0, 0, 0, 0);
      sliderGrid->addWidget(slider, 0, 0, Qt::AlignHCenter);
      sliderGrid->addWidget(meter[0], 0, 1, Qt::AlignHCenter);
      
      addGridLayout(sliderGrid, _sliderPos);

      sl = new MusEGui::DoubleLabel(0.0, -98.0, 0.0);
      sl->setContentsMargins(0, 0, 0, 0);
      sl->setBackgroundRole(QPalette::Mid);
      sl->setSpecialText(tr("off"));
      sl->setSuffix(tr("dB"));
      sl->setToolTip(tr("Volume/gain\n(Ctrl-double-click on/off)"));
      sl->setFrame(true);
      sl->setPrecision(0);
      sl->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
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
          dlv = -muse_val2dbr(double(127*127)/double(v*v));
          if(dlv > sl->maxValue())
            dlv = sl->maxValue();
        }    
        // Auto bias...
        v -= mc->bias();
      }      
      slider->setValue(double(v));
      sl->setValue(dlv);
        
      grid->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::Expanding), 
                    _infoSpacerBottom._row, _infoSpacerBottom._col, _infoSpacerBottom._rowSpan, _infoSpacerBottom._colSpan);

      connect(slider, SIGNAL(valueChanged(double,int)), SLOT(setVolume(double)));
      connect(slider, SIGNAL(sliderRightClicked(QPoint,int)), SLOT(controlRightClicked(QPoint,int)));
      connect(sl, SIGNAL(valueChanged(double, int)), SLOT(volLabelChanged(double)));
      connect(sl, SIGNAL(ctrlDoubleClicked(int)), SLOT(volLabelDoubleClicked()));
      
      addGridWidget(sl, _sliderLabelPos, Qt::AlignCenter);

      //---------------------------------------------------
      //    pan, balance
      //---------------------------------------------------

      _lowerRack = new MidiComponentRack(t, mStripLowerRack);
      
      // FIXME For some reason StyledPanel has trouble, intermittent sometimes panel is drawn, sometimes not. 
//       _lowerRack->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
      _lowerRack->setFrameStyle(QFrame::Box | QFrame::Sunken);
      
      _lowerRack->setLineWidth(rackFrameWidth);
      _lowerRack->setMidLineWidth(0);
      // We do set a minimum height on this widget. Tested: Must be on fixed. Thankfully, it'll expand if more controls are added.
      _lowerRack->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
      _lowerRack->setContentsMargins(rackFrameWidth, rackFrameWidth, rackFrameWidth, rackFrameWidth);

      CompactSliderComponentDescriptor panControllerDesc(ComponentRack::controllerComponent, "MixerStripMidiPanController", MusECore::CTRL_PANPOT);
      _lowerRack->newComponent(&panControllerDesc);
      
      
// Keep this if dynamic layout (flip to right side) is desired.
       _lowerRack->addStretch();
       
      updateRackSizes(false, true);
      
      addGridWidget(_lowerRack, _postScrollAreaPos_A);
      
      _upperRack->setEnabled(!t->off());
      _infoRack->setEnabled(!t->off());
      _lowerRack->setEnabled(!t->off());
      
      //---------------------------------------------------
      //    mute, solo
      //    or
      //    record, mixdownfile
      //---------------------------------------------------

      record  = new CompactToolButton();
      record->setFocusPolicy(Qt::NoFocus);
      record->setCheckable(true);
      record->setContentsMargins(0, 0, 0, 0);
      record->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
      record->setToolTip(tr("record"));
      QIcon rec_icon(*record_off_Icon);
      rec_icon.addPixmap(*record_on_Icon, QIcon::Normal, QIcon::On);
      record->setIcon(rec_icon);
      record->setIconSize(record_off_Icon->size());  
      record->setChecked(track->recordFlag());
      connect(record, SIGNAL(clicked(bool)), SLOT(recordToggled(bool)));

      mute  = new CompactToolButton();
      mute->setFocusPolicy(Qt::NoFocus);
      mute->setCheckable(true);
      mute->setContentsMargins(0, 0, 0, 0);
      mute->setToolTip(tr("mute"));
      QIcon mute_icon(*muteIconOn);
      mute_icon.addPixmap(*muteIconOff, QIcon::Normal, QIcon::On);
      mute->setIcon(mute_icon);
      mute->setIconSize(muteIconOn->size());  
      mute->setChecked(track->mute());
      mute->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
      connect(mute, SIGNAL(clicked(bool)), SLOT(muteToggled(bool)));

      solo  = new CompactToolButton();
      solo->setFocusPolicy(Qt::NoFocus);
      solo->setToolTip(tr("solo mode"));
      solo->setContentsMargins(0, 0, 0, 0);
      solo->setCheckable(true);
      QIcon solo_icon_A(*soloIconOff);
      solo_icon_A.addPixmap(*soloIconOn, QIcon::Normal, QIcon::On);
      solo->setIcon(solo_icon_A);
      QIcon solo_icon_B(*soloblksqIconOff);
      solo_icon_B.addPixmap(*soloblksqIconOn, QIcon::Normal, QIcon::On);
      solo->setIconSize(soloIconOff->size());  
      solo->setChecked(track->solo());
      solo->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
      connect(solo, SIGNAL(clicked(bool)), SLOT(soloToggled(bool)));
      
      off  = new CompactToolButton();
      off->setContentsMargins(0, 0, 0, 0);
      off->setFocusPolicy(Qt::NoFocus);
      off->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
      off->setCheckable(true);
      off->setToolTip(tr("off"));
      QIcon off_icon(*exitIcon);
      off_icon.addPixmap(*exit1Icon, QIcon::Normal, QIcon::On);
      off->setIcon(off_icon);
      off->setIconSize(exitIcon->size());  
      off->setChecked(track->off());
      connect(off, SIGNAL(clicked(bool)), SLOT(offToggled(bool)));

      addGridWidget(off, _offPos);
      addGridWidget(record, _recPos);
      addGridWidget(mute, _mutePos);
      addGridWidget(solo, _soloPos);
      
      //---------------------------------------------------
      //    routing
      //---------------------------------------------------

      iR = new CompactToolButton(0, *routesMidiInIcon);
      iR->setContentsMargins(0, 0, 0, 0);
      iR->setFocusPolicy(Qt::NoFocus);
      iR->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
      // Give it a wee bit more height.
      iR->setIconSize(QSize(routesMidiInIcon->width(), routesMidiInIcon->height() + 5));
      iR->setCheckable(false);
      iR->setToolTip(MusEGlobal::inputRoutingToolTipBase);
      connect(iR, SIGNAL(pressed()), SLOT(iRoutePressed()));
      
      oR = new CompactToolButton(0, *routesMidiOutIcon);
      oR->setContentsMargins(0, 0, 0, 0);
      oR->setFocusPolicy(Qt::NoFocus);
      oR->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
      // Give it a wee bit more height.
      oR->setIconSize(QSize(routesMidiOutIcon->width(), routesMidiOutIcon->height() + 5));
      oR->setCheckable(false);
      oR->setToolTip(MusEGlobal::outputRoutingToolTipBase);
      connect(oR, SIGNAL(pressed()), SLOT(oRoutePressed()));
   
      updateRouteButtons();
      
      _midiThru = new CompactToolButton();
      _midiThru->setFocusPolicy(Qt::NoFocus);
      _midiThru->setContentsMargins(0, 0, 0, 0);
      _midiThru->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
      _midiThru->setCheckable(true);
      _midiThru->setToolTip(tr("midi thru"));
      _midiThru->setWhatsThis(tr("Pass input events through ('thru') to output"));
      QIcon thruIcon(*midiThruOffIcon);
      thruIcon.addPixmap(*midiThruOnIcon, QIcon::Normal, QIcon::On);
      _midiThru->setIcon(thruIcon);
      _midiThru->setIconSize(midiThruOffIcon->size());  
      _midiThru->setChecked(t->recEcho());
      connect(_midiThru, SIGNAL(toggled(bool)), SLOT(midiThruToggled(bool)));
      QHBoxLayout* routesLayout = new QHBoxLayout();
      routesLayout->setContentsMargins(0, 0, 0, 0);
      routesLayout->setSpacing(0);
      routesLayout->addWidget(iR);
      routesLayout->addWidget(_midiThru);
      routesLayout->addWidget(oR);
      addGridLayout(routesLayout, _routesPos); 
      
      //---------------------------------------------------
      //    automation mode
      //---------------------------------------------------

      autoType = new CompactComboBox();
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
      autoType->addAction(" ", AUTO_OFF);  // Just a dummy text to fix sizing problems. REMOVE later if full automation added.
      autoType->setCurrentItem(AUTO_OFF);    //

      addGridWidget(autoType, _automationPos);

      grid->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Ignored), 
                    _rightSpacerPos._row, _rightSpacerPos._col, _rightSpacerPos._rowSpan, _rightSpacerPos._colSpan);

      
      // TODO: Activate this. But owners want to marshall this signal and send it themselves. Change that.
      //connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedFlags_t)), SLOT(songChanged(MusECore::SongChangedFlags_t)));

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
      
      _upperRack->setEnabled(val);
      _infoRack->setEnabled(val);
      _lowerRack->setEnabled(val);
      
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
  // Set the whole strip's font, except for the label.
  if(font() != MusEGlobal::config.fonts[1])
  {
    //DEBUG_MIDI_STRIP(stderr, "MidiStrip::configChanged changing font: current size:%d\n", font().pointSize());
    setFont(MusEGlobal::config.fonts[1]);
    setStyleSheet(MusECore::font2StyleSheet(MusEGlobal::config.fonts[1]));
    // Update in case font changed.
    updateRackSizes(true, true);
  }
  // Update always, in case style, stylesheet, or font changed.
  //updateRackSizes(true, true);
  
  // Set the strip label's font.
  setLabelFont();
  setLabelText();
  
  slider->setFillColor(MusEGlobal::config.midiVolumeSliderColor); 
  
  _upperRack->configChanged();
  _infoRack->configChanged();
  _lowerRack->configChanged();
  
  // Adjust meter and colour.
  meter[0]->setPrimaryColor(MusEGlobal::config.midiMeterPrimaryColor);
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MidiStrip::songChanged(MusECore::SongChangedFlags_t val)
      {
      if (mute && (val & SC_MUTE)) {      // mute && off
            mute->blockSignals(true);
            mute->setChecked(track->mute());
            mute->blockSignals(false);
            updateOffState();
            }
      if (solo && (val & (SC_SOLO | SC_ROUTE))) 
      {
            solo->blockSignals(true);
            solo->setChecked(track->solo());
            solo->blockSignals(false);
            if(track->internalSolo())
              solo->setIcon(track->solo() ? QIcon(*soloblksqIconOn) : QIcon(*soloblksqIconOff));
            else
              solo->setIcon(track->solo() ? QIcon(*soloIconOn) : QIcon(*soloIconOff));
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
      
      _upperRack->songChanged(val);
      _infoRack->songChanged(val);
      _lowerRack->songChanged(val);
      
      if (val & SC_ROUTE) {
        updateRouteButtons();
      }
      
      if(val & SC_MIDI_TRACK_PROP)
      {
        MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(track);
        
        // Set record echo.
        if(_midiThru->isChecked() != mt->recEcho())
        {
          _midiThru->blockSignals(true);
          _midiThru->setChecked(mt->recEcho());
          _midiThru->blockSignals(false);
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

//---------------------------------------------------------
//   midiThruToggled
//---------------------------------------------------------

void MidiStrip::midiThruToggled(bool v)
{
  if(MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(track))
  {
    mt->setRecEcho(v);
    MusEGlobal::song->update(SC_MIDI_TRACK_PROP);
  }
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
  MusECore::MidiController* mc = mp->midiController(num);
  
  const int lastv = mp->lastValidHWCtrlState(chan, num);
  const int curv = mp->hwCtrlState(chan, num);
  
  if(curv == MusECore::CTRL_VAL_UNKNOWN)
  {
    // If no value has ever been set yet, use the current knob value 
    //  (or the controller's initial value?) to 'turn on' the controller.
    if(lastv == MusECore::CTRL_VAL_UNKNOWN)
    {
      //int kiv = _ctrl->initVal());
      int kiv = lrint(slider->value());
      if(kiv < mc->minVal())
        kiv = mc->minVal();
      if(kiv > mc->maxVal())
        kiv = mc->maxVal();
      kiv += mc->bias();
      
      MusECore::MidiPlayEvent ev(0, outport, chan, MusECore::ME_CONTROLLER, num, kiv);
      MusEGlobal::audio->msgPlayMidiEvent(&ev);
    }
    else
    {
      MusECore::MidiPlayEvent ev(0, outport, chan, MusECore::ME_CONTROLLER, num, lastv);
      MusEGlobal::audio->msgPlayMidiEvent(&ev);
    }
  }  
  else
  {
    if(mp->hwCtrlState(chan, num) != MusECore::CTRL_VAL_UNKNOWN)
      MusEGlobal::audio->msgSetHwCtrlState(mp, chan, num, MusECore::CTRL_VAL_UNKNOWN);
  }
}

//---------------------------------------------------------
//   offToggled
//---------------------------------------------------------

void MidiStrip::offToggled(bool val)
      {
      MusEGlobal::audio->msgSetTrackOff(track, val);
      MusEGlobal::song->update(SC_MUTE);
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
      
      int act = track->activity();
      double dact = double(act) * (slider->value() / 127.0);
      
      if((int)dact > track->lastActivity())
        track->setLastActivity((int)dact);
      
      if(meter[0]) 
        meter[0]->setVal(dact, track->lastActivity(), false);  
      
      // Gives reasonable decay with gui update set to 20/sec.
      if(act)
        track->setActivity((int)((double)act * 0.8));
      
      updateControls();
      
      _upperRack->updateComponents();
      _infoRack->updateComponents();
      _lowerRack->updateComponents();
            
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
  if(channel < 0 || channel >= MIDI_CHANNELS || port < 0 || port >= MIDI_PORTS)
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
    int hwVal = mcvl->hwVal();
    int bias = 0;
    MusECore::MidiController* mc = mp->midiController(MusECore::CTRL_VOLUME);
    if(mc)
      bias = mc->bias();
    
    if(hwVal == MusECore::CTRL_VAL_UNKNOWN)
    {
      sl->setValue(sl->off() - 1.0);
      volume = MusECore::CTRL_VAL_UNKNOWN;
      
      hwVal = mcvl->lastValidHWVal();
      if(hwVal == MusECore::CTRL_VAL_UNKNOWN)
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
        hwVal -= bias;
//         if(!control->isOff() || double(hwVal) != control->value()) // TODO
        if(double(hwVal) != slider->value())
        {
          slider->blockSignals(true);
//           slider->setValueState(double(hwVal), true); // TODO
          slider->setValue(double(hwVal));
          slider->blockSignals(false);
        }  
      }
    }
    else
    {
      int ivol = hwVal;
      hwVal -= bias;
      if(hwVal != volume) 
      {
        //if(slider->isOff() || hwVal != slider->value())  // TODO
        if(hwVal != slider->value())
        {
          slider->blockSignals(true);
          //slider->setValueState(double(hwVal), false);  // TODO
          slider->setValue(double(hwVal));
          slider->blockSignals(false);
        }  
        
        if(ivol == 0)
          sl->setValue(sl->minValue() - 0.5 * (sl->minValue() - sl->off()));
        else
        {  
          double v = -muse_val2dbr(double(127*127)/double(ivol*ivol));
          if(v > sl->maxValue())
            sl->setValue(sl->maxValue());
          else  
            sl->setValue(v);
        }    
        volume = hwVal;
      }
    }  
  }
}

//---------------------------------------------------------
//   ctrlChanged
//---------------------------------------------------------

void MidiStrip::ctrlChanged(double v, bool off, int num)
    {
      if (inHeartBeat)
            return;
      int val = lrint(v);
      MusECore::MidiTrack* t = static_cast<MusECore::MidiTrack*>(track);
      int port     = t->outPort();
      int chan  = t->outChannel();
      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
      MusECore::MidiController* mctl = mp->midiController(num);
      if(off || (val < mctl->minVal()) || (val > mctl->maxVal()))
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
    }

//---------------------------------------------------------
//   volLabelChanged
//---------------------------------------------------------

void MidiStrip::volLabelChanged(double val)
{
  val = sqrt( double(127*127) / muse_db2val(-val) );
  ctrlChanged(val, false, MusECore::CTRL_VOLUME);
}
      
//---------------------------------------------------------
//   setVolume
//---------------------------------------------------------

void MidiStrip::setVolume(double val)
      {
      DEBUG_MIDI_STRIP("Vol %d\n", lrint(val));
      ctrlChanged(val, false, MusECore::CTRL_VOLUME);
      }
      
//---------------------------------------------------------
//   iRoutePressed
//---------------------------------------------------------

void MidiStrip::iRoutePressed()
{
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
  RoutePopupMenu* pup = new RoutePopupMenu();
  pup->exec(QCursor::pos(), track, true);
  delete pup;
  oR->setDown(false);     
}

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void MidiStrip::resizeEvent(QResizeEvent* ev)
{
  //fprintf(stderr, "MidiStrip::resizeEvent\n");  
  
  ev->ignore();
  Strip::resizeEvent(ev);
}  

} // namespace MusEGui
