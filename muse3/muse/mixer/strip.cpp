//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: strip.cpp,v 1.6.2.5 2009/11/14 03:37:48 terminator356 Exp $
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

#include "muse_math.h"

#include <QLayout>
#include <QPalette>
#include <QColor>
#include <QFrame>
#include <QMouseEvent>
#include <QMenu>
#include <QString>
#include <QPainter>
#include <QList>
#include <QInputDialog>
#include <QMessageBox>

#include "globals.h"
#include "gconfig.h"
#include "app.h"
#include "audio.h"
#include "song.h"
#include "track.h"
#include "strip.h"
#include "meter.h"
#include "utils.h"
#include "muse_math.h"
#include "ctrl.h"
#include "midi.h"
#include "midictrl.h"
#include "icons.h"
#include "undo.h"
#include "operations.h"
#include "amixer.h"
#include "compact_knob.h"
#include "compact_slider.h"
#include "menutitleitem.h"
#include "pixmap_button.h"

// For debugging output: Uncomment the fprintf section.
#define DEBUG_STRIP(dev, format, args...) // fprintf(dev, format, ##args);

using MusECore::UndoOp;

namespace MusEGui {

  
//---------------------------------------------------------
//   ComponentRack
//---------------------------------------------------------

ComponentRack::ComponentRack(int id, QWidget* parent, Qt::WindowFlags f) 
  : QFrame(parent, f), _id(id)
{ 
  _layout = new ComponentRackLayout(this); // Install a layout manager.
  _layout->setSpacing(0);
  _layout->setContentsMargins(0, 0, 0, 0);
}

ComponentWidget* ComponentRack::findComponent(
      ComponentWidget::ComponentType componentType,
      int componentWidgetType,
      int index,
      QWidget* widget)
{
  iComponentWidget icw = _components.find(componentType, componentWidgetType, index, widget);
  if(icw != _components.end())
    return &(*icw);
  return 0;
}

void ComponentRack::clearDelete()
{
  for(iComponentWidget ic = _components.begin(); ic != _components.end(); ++ic)
  {
    ComponentWidget& cw = *ic;
    if(cw._widget)
      cw._widget->deleteLater();
  }
  _components.clear();
}

void ComponentRack::addComponentWidget( const ComponentWidget& cw, const ComponentWidget& before )
{
  if(cw._widget)
  {
    int idx = -1;
    if(before.isValid())
    {
      iComponentWidget ibcw = _components.find(before);
      if(ibcw == _components.end())
      {
        DEBUG_STRIP(stderr, "ComponentRack::addComponent: 'before' item not found. Pushing back.\n");
        _components.push_back(cw);
      }
      else
      {
        idx = _layout->indexOf(before._widget);
        if(idx == -1)
        {
          DEBUG_STRIP(stderr, "ComponentRack::addComponent: 'before' widget not found. Pushing back.\n");
          _components.push_back(cw);
        }
        else
        {
          DEBUG_STRIP(stderr, "ComponentRack::addComponent: 'before' widget found. Inserting. Layout idx:%d.\n", idx);
          _components.insert(ibcw, cw);
        }
      }
    }
    else
    {
      DEBUG_STRIP(stderr, "ComponentRack::addComponent: 'before' item not valid. Pushing back.\n");
      _components.push_back(cw);
    }
    
    if(idx == -1)
      _layout->addWidget(cw._widget);
    else
      _layout->insertWidget(idx, cw._widget);
  }
}

void ComponentRack::newComponentWidget( ComponentDescriptor* desc, const ComponentWidget& before )
{
  QPalette pal(palette());
  ComponentWidget cw;
  switch(desc->_widgetType)
  {
    case CompactKnobComponentWidget:
    {
      CompactKnobComponentDescriptor* d = static_cast<CompactKnobComponentDescriptor*>(desc);
      if(!d->_compactKnob)
      {
        CompactKnob* control = new CompactKnob(0,
                                               d->_objName,
                                               CompactKnob::Right,
                                               d->_label);
        d->_compactKnob = control;
        control->setId(d->_index);
        control->setRange(d->_min, d->_max, d->_step);
        control->setValueDecimals(d->_precision);
        control->setSpecialValueText(d->_specialValueText);
        control->setHasOffMode(d->_hasOffMode);
        control->setValueState(d->_initVal, d->_isOff);
        control->setValPrefix(d->_prefix);
        control->setValSuffix(d->_suffix);
        control->setShowValue(d->_showValue);
        // Do not set. Compact knob needs to manage it's own tooltips.
        //control->setToolTip(d->_toolTipText);
        control->setEnabled(d->_enabled);
        control->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
        control->setContentsMargins(0, 0, 0, 0);
        if(d->_color.isValid())
          control->setFaceColor(d->_color);
        //if(d->_rimColor.isValid())
        //  control->setRimColor(d->_rimColor);
        if(d->_faceColor.isValid())
          control->setFaceColor(d->_faceColor);
        if(d->_shinyColor.isValid())
          control->setShinyColor(d->_shinyColor);

        //control->setMaxAliasedPointSize(MusEGlobal::config.maxAliasedPointSize);

        switch(d->_componentType)
        {
          case controllerComponent:
              connect(control, SIGNAL(valueStateChanged(double,bool,int, int)), SLOT(controllerChanged(double,bool,int,int)));
              connect(control, SIGNAL(sliderMoved(double,int,bool)), SLOT(controllerMoved(double,int,bool)));
              connect(control, SIGNAL(sliderPressed(double, int)), SLOT(controllerPressed(double, int)));
              connect(control, SIGNAL(sliderReleased(double, int)), SLOT(controllerReleased(double, int)));
              connect(control, SIGNAL(sliderRightClicked(QPoint,int)), SLOT(controllerRightClicked(QPoint,int)));
          break;

          case propertyComponent:
              connect(control, SIGNAL(valueStateChanged(double,bool,int, int)), SLOT(propertyChanged(double,bool,int,int)));
              connect(control, SIGNAL(sliderMoved(double,int,bool)), SLOT(propertyMoved(double,int,bool)));
              connect(control, SIGNAL(sliderPressed(double, int)), SLOT(propertyPressed(double, int)));
              connect(control, SIGNAL(sliderReleased(double, int)), SLOT(propertyReleased(double, int)));
              connect(control, SIGNAL(sliderRightClicked(QPoint,int)), SLOT(propertyRightClicked(QPoint,int)));
          break;
        }
      }

      cw = ComponentWidget(
                            d->_compactKnob,
                            d->_widgetType,
                            d->_componentType,
                            d->_index
                          );

    }
    break;

    case CompactSliderComponentWidget:
    {
      CompactSliderComponentDescriptor* d = static_cast<CompactSliderComponentDescriptor*>(desc);
      if(!d->_compactSlider)
      {
        CompactSlider* control = new CompactSlider(0, d->_objName, Qt::Horizontal, CompactSlider::None, d->_label);
        d->_compactSlider = control;
        control->setId(d->_index);
        control->setRange(d->_min, d->_max, d->_step);
        control->setValueDecimals(d->_precision);
        control->setSpecialValueText(d->_specialValueText);
        control->setHasOffMode(d->_hasOffMode);
        control->setValueState(d->_initVal, d->_isOff);
        control->setValPrefix(d->_prefix);
        control->setValSuffix(d->_suffix);
        control->setShowValue(d->_showValue);
        control->setActiveBorders(d->_activeBorders);
        control->setToolTip(d->_toolTipText);
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
        
        switch(d->_componentType)
        {
          case controllerComponent:
              connect(control, SIGNAL(valueStateChanged(double,bool,int, int)), SLOT(controllerChanged(double,bool,int,int)));
              connect(control, SIGNAL(sliderMoved(double,int,bool)), SLOT(controllerMoved(double,int,bool)));
              connect(control, SIGNAL(sliderPressed(double, int)), SLOT(controllerPressed(double, int)));
              connect(control, SIGNAL(sliderReleased(double, int)), SLOT(controllerReleased(double, int)));
              connect(control, SIGNAL(sliderRightClicked(QPoint,int)), SLOT(controllerRightClicked(QPoint,int)));
          break;
          
          case propertyComponent:
              connect(control, SIGNAL(valueStateChanged(double,bool,int, int)), SLOT(propertyChanged(double,bool,int,int)));
              connect(control, SIGNAL(sliderMoved(double,int,bool)), SLOT(propertyMoved(double,int,bool)));
              connect(control, SIGNAL(sliderPressed(double, int)), SLOT(propertyPressed(double, int)));
              connect(control, SIGNAL(sliderReleased(double, int)), SLOT(propertyReleased(double, int)));
              connect(control, SIGNAL(sliderRightClicked(QPoint,int)), SLOT(propertyRightClicked(QPoint,int)));
          break;
        }
      }
      
      cw = ComponentWidget(
                            d->_compactSlider,
                            d->_widgetType, 
                            d->_componentType, 
                            d->_index 
                          );
      
    }
    break;
    
    case ElidedLabelComponentWidget:
    {
      ElidedLabelComponentDescriptor* d = static_cast<ElidedLabelComponentDescriptor*>(desc);
      if(!d->_elidedLabel)
      {
        ElidedLabel* control = new ElidedLabel(0, d->_elideMode);
        d->_elidedLabel = control;
        control->setObjectName(d->_objName);
        
        control->setId(d->_index);
        control->setToolTip(d->_toolTipText);
        control->setEnabled(d->_enabled);
        control->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
        control->setContentsMargins(0, 0, 0, 0);

        if(d->_color.isValid())
        {
          pal.setColor(QPalette::Active, QPalette::Button, d->_color); // Border
          pal.setColor(QPalette::Inactive, QPalette::Button, d->_color); // Border
          control->setPalette(pal);
        }

        switch(d->_componentType)
        {
          case propertyComponent:
              connect(control, SIGNAL(pressed(QPoint,int,Qt::MouseButtons,Qt::KeyboardModifiers)), 
                      SLOT(labelPropertyPressed(QPoint,int,Qt::MouseButtons,Qt::KeyboardModifiers)));
              connect(control, SIGNAL(released(QPoint,int,Qt::MouseButtons,Qt::KeyboardModifiers)),
                      SLOT(labelPropertyReleased(QPoint,int,Qt::MouseButtons,Qt::KeyboardModifiers)));
              connect(control, SIGNAL(returnPressed(QPoint,int,Qt::KeyboardModifiers)),
                      SLOT(labelPropertyReturnPressed(QPoint,int,Qt::KeyboardModifiers)));
          break;
        }
      }
      
      cw = ComponentWidget(
                            d->_elidedLabel,
                            d->_widgetType, 
                            d->_componentType, 
                            d->_index 
                          );
      
    }
    break;
    
    case ExternalComponentWidget:
    {
      WidgetComponentDescriptor* d = static_cast<WidgetComponentDescriptor*>(desc);
      
      QWidget* widget = d->_widget;
      if(widget)
      {
        widget->setToolTip(d->_toolTipText);
        widget->setEnabled(d->_enabled);
        widget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

        if(d->_color.isValid())
        {
          pal.setColor(QPalette::Active, QPalette::Button, d->_color); // Border
          pal.setColor(QPalette::Inactive, QPalette::Button, d->_color); // Border
          widget->setPalette(pal);
        }
        
        cw = ComponentWidget(
                              d->_widget, 
                              d->_widgetType, 
                              d->_componentType,
                              d->_index
                            );
      }
    }
    break;
  }
  
  if(cw._widget)
    addComponentWidget(cw, before);
}
  
void ComponentRack::setComponentMinValue(const ComponentWidget& cw, double min, bool updateOnly)
{
  if(!cw._widget)
    return;
  
  switch(cw._widgetType)
  {
    case CompactSliderComponentWidget:
    {
      CompactSlider* w = static_cast<CompactSlider*>(cw._widget);
      if(min != w->minValue())
      {
        if(updateOnly)
          w->blockSignals(true);
        w->setMinValue(min);
        if(updateOnly)
          w->blockSignals(false);
      }
    }
    break;

    case CompactKnobComponentWidget:
    {
      CompactKnob* w = static_cast<CompactKnob*>(cw._widget);
      if(min != w->minValue())
      {
        if(updateOnly)
          w->blockSignals(true);
        w->setMinValue(min);
        if(updateOnly)
          w->blockSignals(false);
      }
    }
    break;
  }
}

void ComponentRack::setComponentMaxValue(const ComponentWidget& cw, double max, bool updateOnly)
{
  if(!cw._widget)
    return;
  
  switch(cw._widgetType)
  {
    case CompactSliderComponentWidget:
    {
      CompactSlider* w = static_cast<CompactSlider*>(cw._widget);
      if(max != w->maxValue())
      {
        if(updateOnly)
          w->blockSignals(true);
        w->setMaxValue(max);
        if(updateOnly)
          w->blockSignals(false);
      }
    }
    break;

    case CompactKnobComponentWidget:
    {
      CompactKnob* w = static_cast<CompactKnob*>(cw._widget);
      if(max != w->maxValue())
      {
        if(updateOnly)
          w->blockSignals(true);
        w->setMaxValue(max);
        if(updateOnly)
          w->blockSignals(false);
      }
    }
    break;
  }
}

double ComponentRack::componentMinValue(const ComponentWidget& cw) const
{
  if(cw._widget)
  {
    switch(cw._widgetType)
    {
      case CompactSliderComponentWidget:
        return static_cast<CompactSlider*>(cw._widget)->minValue();
      break;

      case CompactKnobComponentWidget:
        return static_cast<CompactKnob*>(cw._widget)->minValue();
      break;
    }
  }

  return 0.0;
}

double ComponentRack::componentMaxValue(const ComponentWidget& cw) const
{
  if(cw._widget)
  {
    switch(cw._widgetType)
    {
      case CompactSliderComponentWidget:
        return static_cast<CompactSlider*>(cw._widget)->maxValue();
      break;

      case CompactKnobComponentWidget:
        return static_cast<CompactKnob*>(cw._widget)->maxValue();
      break;
    }
  }

  return 0.0;
}

void ComponentRack::setComponentRange(const ComponentWidget& cw, double min, double max, bool updateOnly,
                                      double step, int pageSize, 
                                      DoubleRange::ConversionMode mode)
{
  if(!cw._widget)
    return;
  
  switch(cw._widgetType)
  {
    case CompactSliderComponentWidget:
    {
      CompactSlider* w = static_cast<CompactSlider*>(cw._widget);
      if(min != w->minValue() || max != w->maxValue())
      {
        if(updateOnly)
          w->blockSignals(true);
        if(min != w->minValue() && max != w->maxValue())
          w->setRange(min, max, step, pageSize, mode);
        else if(min != w->minValue())
          w->setMinValue(max);
        else
          w->setMaxValue(max);
        if(updateOnly)
          w->blockSignals(false);
      }
    }
    break;

    case CompactKnobComponentWidget:
    {
      CompactKnob* w = static_cast<CompactKnob*>(cw._widget);
      if(min != w->minValue() || max != w->maxValue())
      {
        if(updateOnly)
          w->blockSignals(true);
        if(min != w->minValue() && max != w->maxValue())
          w->setRange(min, max, step, pageSize, mode);
        else if(min != w->minValue())
          w->setMinValue(max);
        else
          w->setMaxValue(max);
        if(updateOnly)
          w->blockSignals(false);
      }
    }
    break;
  }
}

double ComponentRack::componentValue(const ComponentWidget& cw) const
{
  if(cw._widget)
  {
    switch(cw._widgetType)
    {
      case CompactSliderComponentWidget:
        return static_cast<CompactSlider*>(cw._widget)->value();
      break;

      case CompactKnobComponentWidget:
        return static_cast<CompactKnob*>(cw._widget)->value();
      break;
    }
  }
  
  return 0.0;
}

void ComponentRack::setComponentValue(const ComponentWidget& cw, double val, bool updateOnly)
{
  if(!cw._widget)
    return;
  
  switch(cw._widgetType)
  {
    case CompactSliderComponentWidget:
    {
      CompactSlider* w = static_cast<CompactSlider*>(cw._widget);
      //if(val != cw->_currentValue) // TODO ?
      if(val != w->value())
      {
        if(updateOnly)
          w->blockSignals(true);
        w->setValue(val);
        if(updateOnly)
          w->blockSignals(false);
        //cw->_currentValue = val;  // TODO ?
      }
    }
    break;

    case CompactKnobComponentWidget:
    {
      CompactKnob* w = static_cast<CompactKnob*>(cw._widget);
      //if(val != cw->_currentValue) // TODO ?
      if(val != w->value())
      {
        if(updateOnly)
          w->blockSignals(true);
        w->setValue(val);
        if(updateOnly)
          w->blockSignals(false);
        //cw->_currentValue = val;  // TODO ?
      }
    }
    break;
  }
}

void ComponentRack::fitComponentValue(const ComponentWidget& cw, double val, bool updateOnly)
{
  if(!cw._widget)
    return;

  switch(cw._widgetType)
  {
    case CompactSliderComponentWidget:
    {
      CompactSlider* w = static_cast<CompactSlider*>(cw._widget);
      //if(val != cw->_currentValue) // TODO ?
      if(val != w->value())
      {
        if(updateOnly)
          w->blockSignals(true);
        w->fitValue(val);
        if(updateOnly)
          w->blockSignals(false);
        //cw->_currentValue = val;  // TODO ?
      }
    }
    break;

    case CompactKnobComponentWidget:
    {
      CompactKnob* w = static_cast<CompactKnob*>(cw._widget);
      //if(val != cw->_currentValue) // TODO ?
      if(val != w->value())
      {
        if(updateOnly)
          w->blockSignals(true);
        w->fitValue(val);
        if(updateOnly)
          w->blockSignals(false);
        //cw->_currentValue = val;  // TODO ?
      }
    }
    break;
  }
}

void ComponentRack::incComponentValue(const ComponentWidget& cw, int steps, bool updateOnly)
{
  if(!cw._widget)
    return;

  switch(cw._widgetType)
  {
    case CompactSliderComponentWidget:
    {
      CompactSlider* w = static_cast<CompactSlider*>(cw._widget);
      if(updateOnly)
        w->blockSignals(true);
      w->incValue(steps);
      if(updateOnly)
        w->blockSignals(false);
    }
    break;

    case CompactKnobComponentWidget:
    {
      CompactKnob* w = static_cast<CompactKnob*>(cw._widget);
      if(updateOnly)
        w->blockSignals(true);
      w->incValue(steps);
      if(updateOnly)
        w->blockSignals(false);
    }
    break;
  }
}

void ComponentRack::setComponentText(const ComponentWidget& cw, const QString& text, bool updateOnly)
{
  if(!cw._widget)
    return;
  
  switch(cw._widgetType)
  {
    case ElidedLabelComponentWidget:
    {
      ElidedLabel* w = static_cast<ElidedLabel*>(cw._widget);
      if(text != w->text())
      {
        if(updateOnly)
          w->blockSignals(true);
        w->setText(text);
        if(updateOnly)
          w->blockSignals(false);
      }
    }
    break;

    case CompactKnobComponentWidget:
    {
      CompactKnob* w = static_cast<CompactKnob*>(cw._widget);
      if(text != w->labelText())
      {
        if(updateOnly)
          w->blockSignals(true);
        w->setLabelText(text);
        if(updateOnly)
          w->blockSignals(false);
      }
    }
    break;

    case CompactSliderComponentWidget:
    {
      CompactSlider* w = static_cast<CompactSlider*>(cw._widget);
      if(text != w->labelText())
      {
        if(updateOnly)
          w->blockSignals(true);
        w->setLabelText(text);
        if(updateOnly)
          w->blockSignals(false);
      }
    }
    break;
  }
}

void ComponentRack::setComponentEnabled(const ComponentWidget& cw, bool enable, bool /*updateOnly*/)
{
  if(!cw._widget)
    return;

  // Nothing special for now. Just operate on the widget itself.
  cw._widget->setEnabled(enable);
}

void ComponentRack::setComponentShowValue(const ComponentWidget& cw, bool show, bool updateOnly)
{
  if(!cw._widget)
    return;

  switch(cw._widgetType)
  {
    case CompactKnobComponentWidget:
    {
      CompactKnob* w = static_cast<CompactKnob*>(cw._widget);
      if(show != w->showValue())
      {
        if(updateOnly)
          w->blockSignals(true);
        w->setShowValue(show);
        if(updateOnly)
          w->blockSignals(false);
      }
    }
    break;

    case CompactSliderComponentWidget:
    {
      CompactSlider* w = static_cast<CompactSlider*>(cw._widget);
      if(show != w->showValue())
      {
        if(updateOnly)
          w->blockSignals(true);
        w->setShowValue(show);
        if(updateOnly)
          w->blockSignals(false);
      }
    }
    break;
  }
}

QWidget* ComponentRack::setupComponentTabbing(QWidget* previousWidget)
{
  QWidget* prev = previousWidget;
  for(ciComponentWidget ic = _components.begin(); ic != _components.end(); ++ic)
  {
    const ComponentWidget& cw = *ic;
    if(cw._widget)
    {
      if(prev)
        QWidget::setTabOrder(prev, cw._widget);
      prev = cw._widget;
    }
  }
  return prev;
}

//---------------------------------------------------------
//   configChanged
//   Catch when label font, or configuration min slider and meter values change, or viewable tracks etc.
//---------------------------------------------------------

void ComponentRack::configChanged() 
{ 
  // FIXME For some reason we have to set the font and stylesheet here as well as the strip.
  // Strip font and stylesheet changes don't seem to be propagated to these racks.
  // FIXME They also don't seem to be propagated to our CompactPatchEdit component on the midi strip.
  if(font() != MusEGlobal::config.fonts[1])
  {
    setFont(MusEGlobal::config.fonts[1]); // should be redundant, overridden by style sheet
    setStyleSheet(MusECore::font2StyleSheetFull(MusEGlobal::config.fonts[1]));
  }

  for(ciComponentWidget ic = _components.begin(); ic != _components.end(); ++ic)
  {
    const ComponentWidget& cw = *ic;
    if(!cw._widget)
      continue;
    
    switch(cw._widgetType)
    {
      case CompactKnobComponentWidget:
      {
        //CompactKnob* w = static_cast<CompactKnob*>(cw._widget);
        //w->setMaxAliasedPointSize(MusEGlobal::config.maxAliasedPointSize);
      }
      break;

      case CompactSliderComponentWidget:
      {
        CompactSlider* w = static_cast<CompactSlider*>(cw._widget);
        w->setMaxAliasedPointSize(MusEGlobal::config.maxAliasedPointSize);
      }
      break;
      
//       case ElidedLabelComponentWidget:
//       {
//         ElidedLabel* w = static_cast<ElidedLabel*>(cw._widget);
//         //w->setMaxAliasedPointSize(MusEGlobal::config.maxAliasedPointSize);
//       }
//       break;

      default:
      break;
    }
  }
}


//---------------------------------------------------------
//   TrackNameLabel
//---------------------------------------------------------

TrackNameLabel::TrackNameLabel(QWidget* parent, const char* name, Qt::WindowFlags f)
 : ElidedTextLabel(parent, name, f)
{
}

TrackNameLabel::TrackNameLabel(const QString& text, QWidget* parent, const char* name, Qt::WindowFlags f)
 : ElidedTextLabel(text, parent, name, f)
{
}

void TrackNameLabel::mouseDoubleClickEvent(QMouseEvent* ev)
{
  ev->accept();
  emit doubleClicked();
}

//---------------------------------------------------------
//   Strip
//---------------------------------------------------------

const int Strip::FIXED_METER_WIDTH = 7;

//---------------------------------------------------------
//   setRecordFlag
//---------------------------------------------------------

void Strip::setRecordFlag(bool flag)
      {
      if (record) {
            record->blockSignals(true);
            record->setChecked(flag);
            record->blockSignals(false);
            }
      }

//---------------------------------------------------------
//   resetPeaks
//---------------------------------------------------------

void Strip::resetPeaks()
      {
      track->resetPeaks();
      }

//---------------------------------------------------------
//   recordToggled
//---------------------------------------------------------

void Strip::recordToggled(bool val)
{
  if (track->type() == MusECore::Track::AUDIO_OUTPUT)
  {
    if (val && !track->recordFlag())
    {
      MusEGlobal::muse->bounceToFile((MusECore::AudioOutput*)track);

      if (!((MusECore::AudioOutput*)track)->recFile())
      {
        if(record)
        {
          record->blockSignals(true);
          record->setChecked(false);
          record->blockSignals(false);
        }
      }
      return;
    }
  }

  MusEGlobal::song->setRecordFlag(track, val);
}

//---------------------------------------------------------
//   returnPressed
//---------------------------------------------------------

void Strip::changeTrackName()
{
  if(!track)
    return;

  const QString oldname = track->name();

  QInputDialog dlg(this);
  dlg.setWindowTitle(tr("Name"));
  dlg.setLabelText(tr("Enter track name:"));
  dlg.setTextValue(oldname);
  dlg.setStyleSheet("font-size:" + QString::number(qApp->font().pointSize()) + "pt");

  const int res = dlg.exec();
  if(res == QDialog::Rejected)
    return;

  const QString newname = dlg.textValue();

  if(newname == oldname)
    return;

  MusECore::TrackList* tl = MusEGlobal::song->tracks();
  for (MusECore::iTrack i = tl->begin(); i != tl->end(); ++i)
  {
    if ((*i)->name() == newname)
    {
      QMessageBox::critical(this,
        tr("MusE: bad trackname"),
        tr("Please choose a unique track name"),
        QMessageBox::Ok,
        Qt::NoButton,
        Qt::NoButton);
      return;
    }
  }

  MusEGlobal::song->applyOperation(
    MusECore::UndoOp(MusECore::UndoOp::ModifyTrackName, track, oldname, newname));
}

//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void Strip::heartBeat()
      {
      }

void Strip::paintEvent(QPaintEvent * ev)
{
  QFrame::paintEvent(ev);
  QPainter p(this);
  if (_highlight) {
    QPen pen(Qt::yellow);
    pen.setWidth(1);
    p.setPen(pen);
    p.drawRect(0,0,width()-1,height()-1);
  }
  ev->accept();
}

//---------------------------------------------------------
//   updateStyleSheet
//---------------------------------------------------------

void Strip::updateStyleSheet()
{
  if(!track)
    return;

  QFont fnt(MusEGlobal::config.fonts[6]);
//   const bool need_word_wrap =
//     !MusECore::autoAdjustFontSize(label, label->text(), fnt, false, true,
//                                 fnt.pointSize(), 7);
  MusECore::autoAdjustFontSize(label, label->text(), fnt, false, true,
                                fnt.pointSize(), 7);

  // Set the label's font.
  // Note that this is effectively useless if a stylesheet font is set.
  // When reading font(), it returns what the stylesheet says.
  // But we set it here anyway, in case stylesheets are not used.
  label->setFont(fnt);

//   if(need_word_wrap)
//     label->setWordWrap(true);
// //     label->setWordWrapMode(QTextOption::WrapAnywhere);
//   else
// //     label->setWordWrapMode(QTextOption::NoWrap);
//     label->setWordWrap(false);

  QColor c(track->labelColor());
  QColor c2(c.lighter());
  c.setAlpha(190);
  c2.setAlpha(190);

  QString stxt = QString("* { background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:1,"
      "stop:0.263158 rgba(%1, %2, %3, %4), stop:0.7547368 rgba(%5, %6, %7, %8));")
      .arg(c2.red()).arg(c2.green()).arg(c2.blue()).arg(c2.alpha()).arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alpha());
  //stxt += QString("color: rgb(0, 0, 0);");
  stxt += MusECore::font2StyleSheet(fnt) + "}";
  stxt += "QToolTip {font-size:" + QString::number(qApp->font().pointSize()) + "pt}";

  label->setStyleSheet(stxt);
}

//---------------------------------------------------------
//   setLabelText
//---------------------------------------------------------

void Strip::setLabelText()
{
      if(!track)
        return;
      label->setText(track->name());
      updateStyleSheet();
}

//---------------------------------------------------------
//   muteToggled
//---------------------------------------------------------

void Strip::muteToggled(bool val)
      {
      if(track)
      {
        // This is a minor operation easily manually undoable. Let's not clog the undo list with it.
        MusECore::PendingOperationList operations;
        operations.add(MusECore::PendingOperationItem(track, val, MusECore::PendingOperationItem::SetTrackMute));
        MusEGlobal::audio->msgExecutePendingOperations(operations, true);
      }
      updateMuteIcon();
      }

//---------------------------------------------------------
//   soloToggled
//---------------------------------------------------------

void Strip::soloToggled(bool val)
      {
      solo->setIconSetB(track && track->internalSolo());
      if(!track)
        return;
      // This is a minor operation easily manually undoable. Let's not clog the undo list with it.
      MusECore::PendingOperationList operations;
      operations.add(MusECore::PendingOperationItem(track, val, MusECore::PendingOperationItem::SetTrackSolo));
      MusEGlobal::audio->msgExecutePendingOperations(operations, true);
      }

//---------------------------------------------------------
//   Strip
//    create mixer strip
//---------------------------------------------------------

Strip::Strip(QWidget* parent, MusECore::Track* t, bool hasHandle, bool isEmbedded)
   : QFrame(parent)
      {
      setMouseTracking(true);
      setAttribute(Qt::WA_DeleteOnClose);
      setFrameStyle(Panel | Raised);
      setLineWidth(1);

      // Set so that strip can redirect focus proxy to volume label in descendants.
      // Nope. Seemed like a good idea but no. Do not allow the strip to gain focus.
      // Just in case it does, which is possible, keep descendants' focus proxy code
      //  so that the slider label will be a sensible place for focus to land.
      setFocusPolicy(Qt::NoFocus);

      _focusYieldWidget = 0;
      _isEmbedded = isEmbedded;
      _broadcastChanges = false;
      _selected = false;
      _highlight = false;

      _curGridRow = 0;
      _userWidth = 0;
      _visible = true;
      dragOn=false;

      sliderGrid    = 0;
      record        = 0;
      solo          = 0;
      mute          = 0;
      iR            = 0;
      oR            = 0;
      autoType      = 0;

      track    = t;
      meter[0] = 0;
      meter[1] = 0;
      setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding));
      
      grid = new QGridLayout();
      grid->setContentsMargins(0, 0, 0, 0);
      grid->setSpacing(0);
      
      _handle = 0;
      if(hasHandle)
      {
        _handle = new ExpanderHandle();
        connect(_handle, SIGNAL(moved(int)), SLOT(changeUserWidth(int)));
        QHBoxLayout* hlayout = new QHBoxLayout(this);
        hlayout->setContentsMargins(0, 0, 0, 0);
        hlayout->setSpacing(0);
        hlayout->addLayout(grid);
        hlayout->addWidget(_handle);
      }
      else
      {
        setLayout(grid);
      }
        
      //---------------------------------------------
      //    label
      //---------------------------------------------

//       label = new TextEdit(this);
//       label->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//       label->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//       label->viewport()->setContentsMargins(0, 0, 0, 0);
//       label->setMaximumBlockCount(1);
//       label->setBackgroundVisible(false);
//       label->setTextFormat(Qt::PlainText);
//       label->setLineWrapMode(QPlainTextEdit::WidgetWidth);

      label = new TrackNameLabel(this);
      label->setElideMode(Qt::ElideMiddle);
      label->setFocusPolicy(Qt::NoFocus);
      label->setObjectName(track->cname());
      label->setContentsMargins(0, 0, 0, 0);
      label->setAlignment(Qt::AlignCenter);
      label->setAutoFillBackground(true);
      label->setLineWidth(2);
      label->setFrameStyle(Sunken | StyledPanel);
      label->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum));
      //label->setWordWrap(true); // This is set dynamically.

      setLabelText();
      
      grid->addWidget(label, _curGridRow++, 0, 1, 3);

      connect(label, SIGNAL(doubleClicked()), SLOT(changeTrackName()));
      }

//---------------------------------------------------------
//   Strip
//---------------------------------------------------------

Strip::~Strip()
      {
      }

void Strip::setFocusYieldWidget(QWidget* w)
{
  if(_focusYieldWidget == w)
    return;
  if(_focusYieldWidget)
    disconnect(_focusYieldWidget, SIGNAL(destroyed(QObject*)), this, SLOT(focusYieldWidgetDestroyed(QObject*)));
  _focusYieldWidget = w;
  if(_focusYieldWidget)
    connect(_focusYieldWidget, SIGNAL(destroyed(QObject*)), this, SLOT(focusYieldWidgetDestroyed(QObject*)));
}

void Strip::focusYieldWidgetDestroyed(QObject* obj)
{
  if(obj != _focusYieldWidget)
    return;
  //disconnect(_focusYieldWidget, SIGNAL(destroyed(QObject*)), this, SLOT(focusYieldWidgetDestroyed(QObject*)));
  _focusYieldWidget = 0;
}

void Strip::addGridWidget(QWidget* w, const GridPosStruct& pos, Qt::Alignment alignment)
{
  grid->addWidget(w, pos._row, pos._col, pos._rowSpan, pos._colSpan, alignment);
}

void Strip::addGridLayout(QLayout* l, const GridPosStruct& pos, Qt::Alignment alignment)
{
  grid->addLayout(l, pos._row, pos._col, pos._rowSpan, pos._colSpan, alignment);
}

//---------------------------------------------------------
//   setAutomationType
//---------------------------------------------------------

void Strip::setAutomationType(int t)
{
// REMOVE Tim. mixer. Removed. TESTING...
//   // If going to OFF mode, need to update current 'manual' values from the automation values at this time...
//   if(t == AUTO_OFF && track->automationType() != AUTO_OFF) // && track->automationType() != AUTO_WRITE)
//   {
//     // May have a lot to do in updateCurValues, so try using idle.
//     MusEGlobal::audio->msgIdle(true);
//     track->setAutomationType(AutomationType(t));
//     if(!track->isMidiTrack())
//       (static_cast<MusECore::AudioTrack*>(track))->controller()->updateCurValues(MusEGlobal::audio->curFramePos());
//     MusEGlobal::audio->msgIdle(false);
//   }
//   else
    // Try it within one message.
    MusEGlobal::audio->msgSetTrackAutomationType(track, t);   
  
  MusEGlobal::song->update(SC_AUTOMATION);
}
      
void Strip::resizeEvent(QResizeEvent* ev)
{
  DEBUG_STRIP(stderr, "Strip::resizeEvent\n");  
  QFrame::resizeEvent(ev);
  setLabelText();
}  

void Strip::updateRouteButtons()
{
  if (iR)
  {
      iR->setIconSetB(track->noInRoute());
      if (track->noInRoute())
        iR->setToolTip(MusEGlobal::noInputRoutingToolTipWarn);
      else
        iR->setToolTip(MusEGlobal::inputRoutingToolTipBase);
  }

  if (oR)
  {
    oR->setIconSetB(track->noOutRoute());
    if (track->noOutRoute())
      oR->setToolTip(MusEGlobal::noOutputRoutingToolTipWarn);
    else
      oR->setToolTip(MusEGlobal::outputRoutingToolTipBase);
  }
}

void Strip::mousePressEvent(QMouseEvent* ev)
{
  ev->accept();

  // Only one button at a time.
  if(ev->buttons() ^ ev->button())
    return;

  QPoint mousePos = QCursor::pos();
  mouseWidgetOffset = pos() - mousePos;

  if (ev->button() == Qt::RightButton) {
    QMenu* menu = new QMenu;

    menu->addAction(new MenuTitleItem(tr("Configuration:"), menu));

    QAction* act = menu->addAction(tr("Prefer knobs, not sliders"));
    act->setData(int(2));
    act->setCheckable(true);
    act->setChecked(MusEGlobal::config.preferKnobsVsSliders);

    act = menu->addAction(tr("Show values in controls"));
    act->setData(int(3));
    act->setCheckable(true);
    act->setChecked(MusEGlobal::config.showControlValues);

    act = menu->addAction(tr("Prefer midi volume as decibels"));
    act->setData(int(4));
    act->setCheckable(true);
    act->setChecked(MusEGlobal::config.preferMidiVolumeDb);

    menu->addSeparator();

    act = menu->addAction(tr("Monitor on record-arm automatically"));
    act->setData(int(5));
    act->setCheckable(true);
    act->setChecked(MusEGlobal::config.monitorOnRecord);

    QMenu* audioEffectsRackVisibleItemsMenu = new QMenu(tr("Visible audio effects"));
    for(int i = 0; i <= MusECore::PipelineDepth; ++i)
    {
      act = audioEffectsRackVisibleItemsMenu->addAction(QString::number(i));
      act->setData(int(5000 + i));
      act->setCheckable(true);
      if(i == MusEGlobal::config.audioEffectsRackVisibleItems)
        act->setChecked(MusEGlobal::config.monitorOnRecord);
    }
    menu->addMenu(audioEffectsRackVisibleItemsMenu);
    
    menu->addAction(new MenuTitleItem(tr("Actions:"), menu));

    act = menu->addAction(tr("Change track name"));
    act->setData(int(1001));

    if(!_isEmbedded)
    {

//      act = menu->addAction(tr("Remove track"));
//      act->setData(int(0));
//      menu->addSeparator();
      act = menu->addAction(tr("Hide strip"));
      act->setData(int(1));
    }

    QPoint pt = QCursor::pos();
    act = menu->exec(pt, 0);
    if (!act)
    {
      delete menu;
      return;
    }

    DEBUG_STRIP("Menu finished, data returned %d\n", act->data().toInt());

    const int sel = act->data().toInt();
    const bool checked = act->isChecked();
    delete menu;

    switch(sel)
    {
      case 0:
        //  DEBUG_STRIP(stderr, "Strip:: delete track\n");
        //  MusEGlobal::song->applyOperation(UndoOp(UndoOp::DeleteTrack, MusEGlobal::song->tracks()->index(track), track));
      break;

      case 1:
        DEBUG_STRIP(stderr, "Strip:: setStripVisible false \n");
        setStripVisible(false);
        setVisible(false);
        emit visibleChanged(this, false);
      break;

      case 2:
        if(MusEGlobal::config.preferKnobsVsSliders != checked)
        {
          MusEGlobal::config.preferKnobsVsSliders = checked;
          MusEGlobal::muse->changeConfig(true); // Save settings immediately, and use simple version.
        }
      break;

      case 3:
        if(MusEGlobal::config.showControlValues != checked)
        {
          MusEGlobal::config.showControlValues = checked;
          MusEGlobal::muse->changeConfig(true); // Save settings immediately, and use simple version.
        }
      break;

      case 4:
        if(MusEGlobal::config.preferMidiVolumeDb != checked)
        {
          MusEGlobal::config.preferMidiVolumeDb = checked;
          MusEGlobal::muse->changeConfig(true); // Save settings immediately, and use simple version.
        }
      break;

      case 5:
        if(MusEGlobal::config.monitorOnRecord != checked)
        {
          MusEGlobal::config.monitorOnRecord = checked;
          MusEGlobal::muse->changeConfig(true); // Save settings immediately, and use simple version.
        }
      break;

      case 1001:
        changeTrackName();
      break;
      
      default:
        if(sel >= 5000 && sel <= 5000 + MusECore::PipelineDepth)
        {
          MusEGlobal::config.audioEffectsRackVisibleItems = sel - 5000;
          MusEGlobal::muse->changeConfig(true); // Save settings immediately, and use simple version.
        }
      break;
    }

    ev->accept();
    return;
  }
  else if (ev->button() == Qt::LeftButton)
  {
    if(!_isEmbedded)
    {
      if (ev->modifiers() & Qt::ControlModifier)
      {
        setSelected(!isSelected());
        track->setSelected(isSelected());
        MusEGlobal::song->update(SC_TRACK_SELECTION);
      }
      else
      {
        emit clearStripSelection();
        MusEGlobal::song->selectAllTracks(false);
        setSelected(true);
        track->setSelected(true);
        MusEGlobal::song->update(SC_TRACK_SELECTION);
      }
    }
  }
}
 
QSize Strip::sizeHint() const
{
  const QSize sz = QFrame::sizeHint();
  return QSize(sz.width() + _userWidth, sz.height());
//   return QSize(_userWidth, sz.height());
}

void Strip::setUserWidth(int w)
{
  _userWidth = w;
  if(_userWidth < 0)
    _userWidth = 0;
  
//   grid->invalidate();
//   grid->activate();
//   grid->update();
//   adjustSize();
  updateGeometry();
}

void Strip::changeUserWidth(int delta)
{
  _userWidth += delta;
  if(_userWidth < 0)
    _userWidth = 0;
  updateGeometry();
  emit userWidthChanged(this, _userWidth);
}


//============================================================


//---------------------------------------------------------
//   ExpanderHandle
//---------------------------------------------------------

ExpanderHandle::ExpanderHandle(QWidget* parent, int handleWidth, Qt::WindowFlags f) 
              : QFrame(parent, f), _handleWidth(handleWidth)
{
  setObjectName("ExpanderHandle");
  setCursor(Qt::SplitHCursor);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
  setFixedWidth(_handleWidth);
  setContentsMargins(0, 0, 0, 0);
 _resizeMode = ResizeModeNone;
}

void ExpanderHandle::paintEvent(QPaintEvent * ev)
{
  QPainter p(this);

  if(const QStyle* st = style())
  {
    st = st->proxy();

    QStyleOption o;
    o.initFrom(this);
    o.rect = rect();
    o.state = QStyle::State_Active | QStyle::State_Enabled;
    st->drawControl(QStyle::CE_Splitter, &o, &p);
  }

  ev->accept();
}

void ExpanderHandle::mousePressEvent(QMouseEvent* e)
{
  // Only one button at a time.
//   if(e->buttons() ^ e->button())
//   {
//     //_resizeMode = ResizeModeNone;
//     //unsetCursor();
//     e->accept();
//     return;
//   }
  
  //if(_resizeMode == ResizeModeHovering)
  //{
    // Switch to dragging mode.
    //_resizeMode = ResizeModeDragging;
    //ev->ignore();
    //return;
  //}

  switch(_resizeMode)
  {
    case ResizeModeNone:
    case ResizeModeHovering:
      _dragLastGlobPos = e->globalPos();
      _resizeMode = ResizeModeDragging;
      e->accept();
      return;
    break;
    
    case ResizeModeDragging:
      e->accept();
      return;
    break;
  }
  
  e->ignore();
  QFrame::mousePressEvent(e);
}

void ExpanderHandle::mouseMoveEvent(QMouseEvent* e)
{
//   const QPoint p = e->pos();
  switch(_resizeMode)
  {
    case ResizeModeNone:
    {
//       if(p.x() >= (width() - frameWidth()) && p.x() < width() && p.y() >= 0 && p.y() < height())
//       {
//         _resizeMode = ResizeModeHovering;
//         setCursor(Qt::SizeHorCursor);
//       }
//       e->accept();
//       return;
    }
    break;

    case ResizeModeHovering:
    {
//       if(p.x() < (width() - frameWidth()) || p.x() >= width() || p.y() < 0 || p.y() >= height())
//       {
//         _resizeMode = ResizeModeNone;
//         unsetCursor();
//       }
//       e->accept();
//       return;
    }
    break;
    
    case ResizeModeDragging:
    {
      const QPoint gp = e->globalPos();
      const QPoint delta = gp -_dragLastGlobPos;
      _dragLastGlobPos = gp;
      emit moved(delta.x());
      e->accept();
      return;
    }
    break;
  }

  e->ignore();
  QFrame::mouseMoveEvent(e);
}

void ExpanderHandle::mouseReleaseEvent(QMouseEvent* e)
{
//   switch(_resizeMode)
//   {
//     case ResizeModeNone:
//     case ResizeModeHovering:
//     break;
//     
//     case ResizeModeDragging:
//     {
//       const QPoint p = ev->pos();
//       if(p.x() >= (width() - frameWidth()) && p.x() < width() && p.y() >= 0 && p.y() < height())
//       {
//         _resizeMode = ResizeModeHovering;
//         setCursor(Qt::SizeHorCursor);
//       }
//       else
//       {
//         _resizeMode = ResizeModeNone;
//         unsetCursor();
//       }
//       ev->ignore();
//       return;
//     }
//     break;
//   }
  _resizeMode = ResizeModeNone;
  e->ignore();
  QFrame::mouseReleaseEvent(e);
}

// void ExpanderHandle::leaveEvent(QEvent* e)
// {
//   e->ignore();
//   QFrame::leaveEvent(e);
//   switch(_resizeMode)
//   {
//     case ResizeModeDragging:
//       return;
//     break;
//     
//     case ResizeModeHovering:
//       _resizeMode = ResizeModeNone;
//       Fall through...
//     case ResizeModeNone:
//       unsetCursor();
//       return;
//     break;
//   }
// }

QSize ExpanderHandle::sizeHint() const
{
  QSize sz = QFrame::sizeHint();
  sz.setWidth(_handleWidth);
  return sz;
}


void Strip::mouseReleaseEvent(QMouseEvent* ev)
{
  ev->accept();
  if (!_isEmbedded && dragOn) {
    emit moveStrip(this);
  }
  dragOn=false;
}

void Strip::mouseMoveEvent(QMouseEvent* e)
{
  e->accept();
  if(e->buttons() == Qt::LeftButton)
  {
    if(!_isEmbedded)
    {
      if(dragOn)
      {
        QPoint mousePos = QCursor::pos();
        move(mousePos + mouseWidgetOffset);
      }
      else
      {
        raise();
        dragOn = true;
      }
    }
  }
  //QFrame::mouseMoveEvent(ev);
}

void Strip::keyPressEvent(QKeyEvent* ev)
{
  const bool shift = ev->modifiers() & Qt::ShiftModifier;
  const bool alt = ev->modifiers() & Qt::AltModifier;
  const bool ctl = ev->modifiers() & Qt::ControlModifier;
  const bool meta = ev->modifiers() & Qt::MetaModifier;
  const int val = shift ? 5 : 1;

  switch (ev->key())
  {
    case Qt::Key_Escape:
      //if(hasFocus() && _focusYieldWidget)
      if(_focusYieldWidget)
      {
        ev->accept();
        // Yield the focus to the given widget.
        _focusYieldWidget->setFocus();
        // Activate the window.
        if(!_focusYieldWidget->isActiveWindow())
          _focusYieldWidget->activateWindow();
        return;
      }
    break;

    case Qt::Key_Up:
    if(alt && !ctl && !meta)
    {
      incVolume(val);
      ev->accept();
      return;
    }
    break;

    case Qt::Key_Down:
    if(alt && !ctl && !meta)
    {
      incVolume(-val);
      ev->accept();
      return;
    }
    break;

    case Qt::Key_Left:
    if(alt && !ctl && !meta)
    {
      incPan(-val);
      ev->accept();
      return;
    }
    break;

    case Qt::Key_Right:
    if(alt && !ctl && !meta)
    {
      incPan(val);
      ev->accept();
      return;
    }
    break;

    default:
    break;
  }

  // Let mixer window or other higher up handle it.
  ev->ignore();
  QFrame::keyPressEvent(ev);
}

void Strip::setSelected(bool v)
{
  if(_selected == v)
    return;
  
  if(_isEmbedded)
  {
    _selected = false;
    return;
  }
  if (v) {
    label->setFrameStyle(Raised | StyledPanel);
    setHighLight(true);
    // First time selected? Set the focus.
    setFocus();
  }
  else {
    label->setFrameStyle(Sunken | StyledPanel);
    setHighLight(false);
  }
  _selected=v;
}

void Strip::setHighLight(bool highlight)
{
  _highlight = highlight;
  update();
}

QString Strip::getLabelText()
{
  return label->text();
}

void Strip::updateMuteIcon()
{
  if(!track)
    return;

  bool found = false;
  MusECore::TrackList* tl = MusEGlobal::song->tracks();
  for(MusECore::ciTrack it = tl->begin(); it != tl->end(); ++it)
  {
    MusECore::Track* t = *it;
    // Ignore this track.
    if(t != track && (t->internalSolo() || t->solo()))
    {
      found = true;
      break;
    }
  }
  mute->setIconSetB(found && !track->internalSolo() && !track->solo());
}



//--------------------------------
//    Control ganging support.
//--------------------------------

struct MidiIncListStruct
{
  int _port;
  int _chan;
  MidiIncListStruct(int port, int chan) : _port(port), _chan(chan) { }
  bool operator==(const MidiIncListStruct& other) { return other._port == _port && other._chan == _chan; }
};

void Strip::componentChanged(int type, double val, bool off, int id, int scrollMode)
{
  // The track has already taken care of its own controllers.
  // Don't bother other tracks if the track is not selected. ie part of a selected group.
  // Don't bother if broadcasting changes is disabled.
  if(!track || !track->selected() || !_broadcastChanges)
    return;

  // TODO: Only controller components handled for now.
  if(type != ComponentRack::controllerComponent)
    return;

  QList<MidiIncListStruct> doneMidiTracks;
  QList<MusECore::Track*> doneAudioTracks;

  if(track->isMidiTrack())
  {
    // TODO: Only volume and pan handled for now.
    int a_ctlnum;
    switch(id)
    {
      case MusECore::CTRL_VOLUME:
        a_ctlnum = MusECore::AC_VOLUME;
      break;
      case MusECore::CTRL_PANPOT:
        a_ctlnum = MusECore::AC_PAN;
      break;
      default:
        return;
      break;
    }

    MusECore::MidiTrack* m_track = static_cast<MusECore::MidiTrack*>(track);
    const int m_port  = m_track->outPort();
    const int m_chan  = m_track->outChannel();
    MusECore::MidiPort* m_mp = &MusEGlobal::midiPorts[m_port];
    MusECore::MidiController* m_mctl = m_mp->midiController(id, false);
    if(!m_mctl)
      return;

    int i_m_min = m_mctl->minVal();
    const int i_m_max = m_mctl->maxVal();
    const int i_m_bias = m_mctl->bias();

    //----------------------------------------------------------
    // For midi volume, the formula given by MMA is:
    //  volume dB = 40 * log(midivol / 127)  and the inverse is:
    //  midi volume = 127 * 10 ^ (volume dB / 40)
    // Unusual, it is a factor of 40 not 20. Since muse_db2val()
    //  does 10 ^ (volume dB / 20), just divide volume dB by 2.
    //----------------------------------------------------------
    double m_val = val;
    double ma_val = val;
    if(id == MusECore::CTRL_VOLUME)
    {
      if(MusEGlobal::config.preferMidiVolumeDb)
      {
        if(ma_val <= MusEGlobal::config.minSlider)
          m_val = ma_val = 0.0;
        else
        {
          m_val = double(i_m_max) * muse_db2val(m_val);
          ma_val = double(i_m_max) * muse_db2val(ma_val / 2.0);
        }
      }
      else
      {
        // It's a linear scale. We need to convert to Db and
        //  scale the linear value by 2 since the midi scale is
        //  twice the usual Db per step. Then convert back to linear.
        m_val = muse_val2dbr(m_val / double(i_m_max)) * 2.0;
        m_val *= 2.0;
        m_val = double(i_m_max) * muse_db2val(m_val / 2.0);
      }
    }

    //--------------------------------------------------------------
    //   NOTE: Midi int to audio float conversion:
    //   If the control has a bias at all, it is supposed
    //    to define the 'middle', like pan.
    //   But if the control's range is odd (127), that
    //    makes an uneven, uncentered float conversion.
    //   So the policy here is to force an even range for
    //    symmetrical +/- float conversion. (0.0 in the middle.)
    //   Treat value '0' (-64 pan) and '1' (-63 pan) the same.
    //--------------------------------------------------------------
    if(i_m_bias != 0 && ((i_m_max - i_m_min) & 0x1))
      ++i_m_min;
    const int i_m_range = i_m_max - i_m_min;
    if(i_m_range == 0) // Avoid divide by zero.
      return;

    if(m_val < i_m_min)
     m_val = i_m_min;
    if(m_val > i_m_max)
     m_val = i_m_max;
    const double m_fact = (m_val - (double)i_m_min) / (double)i_m_range;

    // Make sure to include this track as already done.
    doneMidiTracks.append(MidiIncListStruct(m_port, m_chan));

    MusECore::TrackList* tracks = MusEGlobal::song->tracks();
    for(MusECore::iTrack it = tracks->begin(); it != tracks->end(); ++it)
    {
      MusECore::Track* t = *it;
      // Do selected tracks. Ignore this track, it has already taken care of its own controllers.
      if(t == track || !t->selected())
        continue;

      if(t->isMidiTrack())
      {
        MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(t);
        const int port     = mt->outPort();
        const int chan  = mt->outChannel();
        // Make sure midi tracks on same port and channel are only done once.
        const MidiIncListStruct mils(port, chan);
        if(doneMidiTracks.contains(mils))
          continue;
        doneMidiTracks.append(mils);

        double d_val = ma_val;
        MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
        MusECore::MidiController* mctl = mp->midiController(id, false);
        if(!mctl)
          continue;
        if(off || (d_val < double(mctl->minVal())) || (d_val > double(mctl->maxVal())))
        {
          if(mp->hwCtrlState(chan, id) != MusECore::CTRL_VAL_UNKNOWN)
            mp->putHwCtrlEvent(MusECore::MidiPlayEvent(0, port, chan,
                                                      MusECore::ME_CONTROLLER,
                                                      id,
                                                      MusECore::CTRL_VAL_UNKNOWN));
        }
        else
        {
          d_val += double(mctl->bias());
          mp->putControllerValue(port, chan, id, d_val, false);
        }
      }
      else
      {
        // Make sure we're not doing the same track more than once.
        if(doneAudioTracks.contains(t))
          continue;
        doneAudioTracks.append(t);

        MusECore::AudioTrack* at = static_cast<MusECore::AudioTrack*>(t);
        MusECore::iCtrlList icl = at->controller()->find(a_ctlnum);
        if(icl == at->controller()->end())
          continue;
        MusECore::CtrlList* cl = icl->second;

        // The audio volume can go above 0dB (amplification) while
        //  the top midi value of 127 represents 0dB. Cut it off at 0dB.
        const double a_min = cl->minVal();
        const double a_max = (a_ctlnum == MusECore::AC_VOLUME) ? 1.0 : cl->maxVal();
        const double a_range = a_max - a_min;
        const double a_val = (m_fact * a_range) + a_min;

        // Hack: Be sure to ignore in ScrDirect mode since we get both pressed AND changed signals.
        // ScrDirect mode is one-time only on press with modifier.
        if(scrollMode != SliderBase::ScrDirect)
          at->recordAutomation(a_ctlnum, a_val);
        at->setParam(a_ctlnum, a_val);  // Schedules a timed control change.
        at->enableController(a_ctlnum, false);
      }
    }
  }
  else
  {
    // TODO: Only volume and pan handled for now.
    int m_ctlnum;
    switch(id)
    {
      case MusECore::AC_VOLUME:
        m_ctlnum = MusECore::CTRL_VOLUME;
      break;
      case MusECore::AC_PAN:
        m_ctlnum = MusECore::CTRL_PANPOT;
      break;
      default:
        return;
      break;
    }

    MusECore::AudioTrack* a_track = static_cast<MusECore::AudioTrack*>(track);
    MusECore::iCtrlList icl = a_track->controller()->find(id);
    if(icl == a_track->controller()->end())
      return;
    MusECore::CtrlList* cl = icl->second;

    //----------------------------------------------------------
    // For midi volume, the formula given by MMA is:
    //  volume dB = 40 * log(midivol / 127)  and the inverse is:
    //  midi volume = 127 * 10 ^ (volume dB / 40)
    // Unusual, it is a factor of 40 not 20. Since muse_db2val()
    //  does 10 ^ (volume dB / 20), just divide volume dB by 2.
    //----------------------------------------------------------
    double a_val = val;
    double ma_val = val;
    if(id == MusECore::AC_VOLUME)
    {
      if(ma_val <= MusEGlobal::config.minSlider)
        a_val = ma_val = 0.0;
      else
      {
        a_val = muse_db2val(a_val);
        ma_val = muse_db2val(ma_val / 2.0);
      }
    }

    // The audio volume can go above 0dB (amplification) while
    //  the top midi value of 127 represents 0dB. Cut it off at 0dB.
    const double a_min = cl->minVal();
    const double a_max = (id == MusECore::AC_VOLUME) ? 1.0 : cl->maxVal();
    const double a_range = a_max - a_min;
    if(a_range < 0.0001) // Avoid divide by zero.
      return;
    const double a_fact = (ma_val - a_min) / a_range;

    MusECore::TrackList* tracks = MusEGlobal::song->tracks();
    for(MusECore::iTrack it = tracks->begin(); it != tracks->end(); ++it)
    {
      MusECore::Track* t = *it;
      // Do selected tracks. Ignore this track, it has already taken care of its own controllers.
      if(t == track || !t->selected())
        continue;
      if(t->isMidiTrack())
      {
        MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(t);
        const int port     = mt->outPort();
        const int chan  = mt->outChannel();
        // Make sure midi tracks on same port and channel are only done once.
        const MidiIncListStruct mils(port, chan);
        if(doneMidiTracks.contains(mils))
          continue;
        doneMidiTracks.append(mils);

        MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
        MusECore::MidiController* mctl = mp->midiController(m_ctlnum, false);
        if(mctl)
        {
          int min = mctl->minVal();
          const int max = mctl->maxVal();
          const int bias = mctl->bias();

          //--------------------------------------------------------------
          //   NOTE: Midi int to audio float conversion:
          //   If the control has a bias at all, it is supposed
          //    to define the 'middle', like pan.
          //   But if the control's range is odd (127), that
          //    makes an uneven, uncentered float conversion.
          //   So the policy here is to force an even range for
          //    symmetrical +/- float conversion. (0.0 in the middle.)
          //   Treat value '0' (-64 pan) and '1' (-63 pan) the same.
          //--------------------------------------------------------------
          if(bias != 0 && ((max - min) & 0x1))
            ++min;

          const double d_min = (double)min;
          const double d_range = double(max - min);
          double d_val = a_fact * d_range + d_min;

          if(d_val < double(mctl->minVal()))
            d_val = mctl->minVal();
          if(d_val > double(mctl->maxVal()))
            d_val = mctl->maxVal();
          d_val += double(mctl->bias());
          mp->putControllerValue(port, chan, m_ctlnum, d_val, false);
        }
      }
      else
      {
        // Make sure we're not doing the same track more than once.
        if(doneAudioTracks.contains(t))
          continue;
        doneAudioTracks.append(t);

        MusECore::AudioTrack* at = static_cast<MusECore::AudioTrack*>(t);
        // Hack: Be sure to ignore in ScrDirect mode since we get both pressed AND changed signals.
        // ScrDirect mode is one-time only on press with modifier.
        if(scrollMode != SliderBase::ScrDirect)
          at->recordAutomation(id, a_val);
        at->setParam(id, a_val);  // Schedules a timed control change.
        at->enableController(id, false);
      }
    }
  }
}

void Strip::componentMoved(int /*type*/, double /*val*/, int /*id*/, bool /*shift_pressed*/)
{
  //emit componentMoved(this, type, val, id, shift_pressed);
}

void Strip::componentPressed(int type, double val, int id)
{
  // The track has already taken care of its own controllers.
  // Don't bother other tracks if the track is not selected. ie part of a selected group.
  // Don't bother if broadcasting changes is disabled.
  if(!track || !track->selected() || !_broadcastChanges)
    return;

  // TODO: Only controller components handled for now.
  if(type != ComponentRack::controllerComponent)
    return;

  QList<MidiIncListStruct> doneMidiTracks;
  QList<MusECore::Track*> doneAudioTracks;

  if(track->isMidiTrack())
  {
    // TODO: Only volume and pan handled for now.
    int a_ctlnum;
    switch(id)
    {
      case MusECore::CTRL_VOLUME:
        a_ctlnum = MusECore::AC_VOLUME;
      break;
      case MusECore::CTRL_PANPOT:
        a_ctlnum = MusECore::AC_PAN;
      break;
      default:
        return;
      break;
    }

    MusECore::MidiTrack* m_track = static_cast<MusECore::MidiTrack*>(track);
    const int m_port  = m_track->outPort();
    const int m_chan  = m_track->outChannel();
    MusECore::MidiPort* m_mp = &MusEGlobal::midiPorts[m_port];
    MusECore::MidiController* m_mctl = m_mp->midiController(id, false);
    if(!m_mctl)
      return;

    int i_m_min = m_mctl->minVal();
    const int i_m_max = m_mctl->maxVal();
    const int i_m_bias = m_mctl->bias();

    //----------------------------------------------------------
    // For midi volume, the formula given by MMA is:
    //  volume dB = 40 * log(midivol / 127)  and the inverse is:
    //  midi volume = 127 * 10 ^ (volume dB / 40)
    // Unusual, it is a factor of 40 not 20. Since muse_db2val()
    //  does 10 ^ (volume dB / 20), just divide volume dB by 2.
    //----------------------------------------------------------
    double m_val = val;
    double ma_val = val;
    if(id == MusECore::CTRL_VOLUME)
    {
      if(MusEGlobal::config.preferMidiVolumeDb)
      {
        if(ma_val <= MusEGlobal::config.minSlider)
          m_val = ma_val = 0.0;
        else
        {
          m_val = double(i_m_max) * muse_db2val(m_val);
          ma_val = double(i_m_max) * muse_db2val(ma_val / 2.0);
        }
      }
      else
      {
        // It's a linear scale. We need to convert to Db and
        //  scale the linear value by 2 since the midi scale is
        //  twice the usual Db per step. Then convert back to linear.
        m_val = muse_val2dbr(m_val / double(i_m_max)) * 2.0;
        m_val *= 2.0;
        m_val = double(i_m_max) * muse_db2val(m_val / 2.0);
      }
    }

    //--------------------------------------------------------------
    //   NOTE: Midi int to audio float conversion:
    //   If the control has a bias at all, it is supposed
    //    to define the 'middle', like pan.
    //   But if the control's range is odd (127), that
    //    makes an uneven, uncentered float conversion.
    //   So the policy here is to force an even range for
    //    symmetrical +/- float conversion. (0.0 in the middle.)
    //   Treat value '0' (-64 pan) and '1' (-63 pan) the same.
    //--------------------------------------------------------------
    if(i_m_bias != 0 && ((i_m_max - i_m_min) & 0x1))
      ++i_m_min;
    const int i_m_range = i_m_max - i_m_min;
    if(i_m_range == 0) // Avoid divide by zero.
      return;

    if(m_val < i_m_min)
     m_val = i_m_min;
    if(m_val > i_m_max)
     m_val = i_m_max;
    const double m_fact = (m_val - (double)i_m_min) / (double)i_m_range;

    // Make sure to include this track as already done.
    doneMidiTracks.append(MidiIncListStruct(m_port, m_chan));

    MusECore::TrackList* tracks = MusEGlobal::song->tracks();
    for(MusECore::iTrack it = tracks->begin(); it != tracks->end(); ++it)
    {
      MusECore::Track* t = *it;
      // Do selected tracks. Ignore this track, it has already taken care of its own controllers.
      if(t == track || !t->selected())
        continue;

      if(t->isMidiTrack())
      {
      }
      else
      {
        // Make sure we're not doing the same track more than once.
        if(doneAudioTracks.contains(t))
          continue;
        doneAudioTracks.append(t);

        MusECore::AudioTrack* at = static_cast<MusECore::AudioTrack*>(t);
        MusECore::iCtrlList icl = at->controller()->find(a_ctlnum);
        if(icl == at->controller()->end())
          continue;
        MusECore::CtrlList* cl = icl->second;

        // The audio volume can go above 0dB (amplification) while
        //  the top midi value of 127 represents 0dB. Cut it off at 0dB.
        const double a_min = cl->minVal();
        const double a_max = (a_ctlnum == MusECore::AC_VOLUME) ? 1.0 : cl->maxVal();
        const double a_range = a_max - a_min;
        const double a_val = (m_fact * a_range) + a_min;

        at->startAutoRecord(a_ctlnum, a_val);
        at->setPluginCtrlVal(a_ctlnum, a_val);
        //at->setParam(a_ctlnum, val);   // Schedules a timed control change. // TODO Try this instead
        at->enableController(a_ctlnum, false);
      }
    }
  }
  else
  {
    // TODO: Only volume and pan handled for now.
    switch(id)
    {
      case MusECore::AC_VOLUME:
      break;
      case MusECore::AC_PAN:
      break;

      default:
        return;
      break;
    }

    double a_val = val;
    if(id == MusECore::AC_VOLUME)
    {
      if(a_val <= MusEGlobal::config.minSlider)
        a_val = 0.0;
      else
        a_val = muse_db2val(a_val);
    }

    MusECore::TrackList* tracks = MusEGlobal::song->tracks();
    for(MusECore::iTrack it = tracks->begin(); it != tracks->end(); ++it)
    {
      MusECore::Track* t = *it;
      // Do selected tracks. Ignore this track, it has already taken care of its own controllers.
      if(t == track || !t->selected())
        continue;
      if(t->isMidiTrack())
      {
      }
      else
      {
        // Make sure we're not doing the same track more than once.
        if(doneAudioTracks.contains(t))
          continue;
        doneAudioTracks.append(t);

        MusECore::AudioTrack* at = static_cast<MusECore::AudioTrack*>(t);
        at->startAutoRecord(id, a_val);
        at->setPluginCtrlVal(id, a_val);
        //at->setParam(id, val);   // Schedules a timed control change. // TODO Try this instead
        at->enableController(id, false);
      }
    }
  }
}

void Strip::componentReleased(int type, double val, int id)
{
  // The track has already taken care of its own controllers.
  // Don't bother other tracks if the track is not selected. ie part of a selected group.
  // Don't bother if broadcasting changes is disabled.
  if(!track || !track->selected() || !_broadcastChanges)
    return;

  // TODO: Only controller components handled for now.
  if(type != ComponentRack::controllerComponent)
    return;

  QList<MidiIncListStruct> doneMidiTracks;
  QList<MusECore::Track*> doneAudioTracks;

  if(track->isMidiTrack())
  {
    // TODO: Only volume and pan handled for now.
    int a_ctlnum;
    switch(id)
    {
      case MusECore::CTRL_VOLUME:
        a_ctlnum = MusECore::AC_VOLUME;
      break;
      case MusECore::CTRL_PANPOT:
        a_ctlnum = MusECore::AC_PAN;
      break;
      default:
        return;
      break;
    }

    MusECore::MidiTrack* m_track = static_cast<MusECore::MidiTrack*>(track);
    const int m_port  = m_track->outPort();
    const int m_chan  = m_track->outChannel();
    MusECore::MidiPort* m_mp = &MusEGlobal::midiPorts[m_port];
    MusECore::MidiController* m_mctl = m_mp->midiController(id, false);
    if(!m_mctl)
      return;

    int i_m_min = m_mctl->minVal();
    const int i_m_max = m_mctl->maxVal();
    const int i_m_bias = m_mctl->bias();

    //----------------------------------------------------------
    // For midi volume, the formula given by MMA is:
    //  volume dB = 40 * log(midivol / 127)  and the inverse is:
    //  midi volume = 127 * 10 ^ (volume dB / 40)
    // Unusual, it is a factor of 40 not 20. Since muse_db2val()
    //  does 10 ^ (volume dB / 20), just divide volume dB by 2.
    //----------------------------------------------------------
    double m_val = val;
    double ma_val = val;
    if(id == MusECore::CTRL_VOLUME)
    {
      if(MusEGlobal::config.preferMidiVolumeDb)
      {
        if(ma_val <= MusEGlobal::config.minSlider)
          m_val = ma_val = 0.0;
        else
        {
          m_val = double(i_m_max) * muse_db2val(m_val);
          ma_val = double(i_m_max) * muse_db2val(ma_val / 2.0);
        }
      }
      else
      {
        // It's a linear scale. We need to convert to Db and
        //  scale the linear value by 2 since the midi scale is
        //  twice the usual Db per step. Then convert back to linear.
        m_val = muse_val2dbr(m_val / double(i_m_max)) * 2.0;
        m_val *= 2.0;
        m_val = double(i_m_max) * muse_db2val(m_val / 2.0);
      }
    }

    //--------------------------------------------------------------
    //   NOTE: Midi int to audio float conversion:
    //   If the control has a bias at all, it is supposed
    //    to define the 'middle', like pan.
    //   But if the control's range is odd (127), that
    //    makes an uneven, uncentered float conversion.
    //   So the policy here is to force an even range for
    //    symmetrical +/- float conversion. (0.0 in the middle.)
    //   Treat value '0' (-64 pan) and '1' (-63 pan) the same.
    //--------------------------------------------------------------
    if(i_m_bias != 0 && ((i_m_max - i_m_min) & 0x1))
      ++i_m_min;
    const int i_m_range = i_m_max - i_m_min;
    if(i_m_range == 0) // Avoid divide by zero.
      return;

    if(m_val < i_m_min)
     m_val = i_m_min;
    if(m_val > i_m_max)
     m_val = i_m_max;
    const double m_fact = (m_val - (double)i_m_min) / (double)i_m_range;

    // Make sure to include this track as already done.
    doneMidiTracks.append(MidiIncListStruct(m_port, m_chan));

    MusECore::TrackList* tracks = MusEGlobal::song->tracks();
    for(MusECore::iTrack it = tracks->begin(); it != tracks->end(); ++it)
    {
      MusECore::Track* t = *it;
      // Do selected tracks. Ignore this track, it has already taken care of its own controllers.
      if(t == track || !t->selected())
        continue;

      if(t->isMidiTrack())
      {
      }
      else
      {
        // Make sure we're not doing the same track more than once.
        if(doneAudioTracks.contains(t))
          continue;
        doneAudioTracks.append(t);

        MusECore::AudioTrack* at = static_cast<MusECore::AudioTrack*>(t);
        MusECore::iCtrlList icl = at->controller()->find(a_ctlnum);
        if(icl == at->controller()->end())
          continue;
        MusECore::CtrlList* cl = icl->second;

        // The audio volume can go above 0dB (amplification) while
        //  the top midi value of 127 represents 0dB. Cut it off at 0dB.
        const double a_min = cl->minVal();
        const double a_max = (a_ctlnum == MusECore::AC_VOLUME) ? 1.0 : cl->maxVal();
        const double a_range = a_max - a_min;
        const double a_val = (m_fact * a_range) + a_min;

        MusECore::AutomationType atype = at->automationType();
        at->stopAutoRecord(a_ctlnum, a_val);
        if(atype == MusECore::AUTO_OFF || atype == MusECore::AUTO_TOUCH)
          at->enableController(a_ctlnum, true);
      }
    }
  }
  else
  {
    // TODO: Only volume and pan handled for now.
    switch(id)
    {
      case MusECore::AC_VOLUME:
      break;
      case MusECore::AC_PAN:
      break;

      default:
        return;
      break;
    }

    //----------------------------------------------------------
    // For midi volume, the formula given by MMA is:
    //  volume dB = 40 * log(midivol / 127)  and the inverse is:
    //  midi volume = 127 * 10 ^ (volume dB / 40)
    // Unusual, it is a factor of 40 not 20. Since muse_db2val()
    //  does 10 ^ (volume dB / 20), just divide volume dB by 2.
    //----------------------------------------------------------
    double a_val = val;
    if(id == MusECore::AC_VOLUME)
    {
      if(a_val <= MusEGlobal::config.minSlider)
        a_val = 0.0;
      else
        a_val = muse_db2val(a_val);
    }

    MusECore::TrackList* tracks = MusEGlobal::song->tracks();
    for(MusECore::iTrack it = tracks->begin(); it != tracks->end(); ++it)
    {
      MusECore::Track* t = *it;
      // Do selected tracks. Ignore this track, it has already taken care of its own controllers.
      if(t == track || !t->selected())
        continue;
      if(t->isMidiTrack())
      {
      }
      else
      {
        // Make sure we're not doing the same track more than once.
        if(doneAudioTracks.contains(t))
          continue;
        doneAudioTracks.append(t);

        MusECore::AudioTrack* at = static_cast<MusECore::AudioTrack*>(t);
        MusECore::AutomationType atype = at->automationType();
        at->stopAutoRecord(id, a_val);
        if(atype == MusECore::AUTO_OFF || atype == MusECore::AUTO_TOUCH)
          at->enableController(id, true);
      }
    }
  }
}

void Strip::componentIncremented(int type, double oldCompVal, double newCompVal,
                                 bool off, int id, int /*scrollMode*/)
{
  // The track has already taken care of its own controllers.
  // Don't bother other tracks if the track is not selected. ie part of a selected group.
  // Don't bother if broadcasting changes is disabled.
  if(!track || !track->selected() || !_broadcastChanges)
    return;

  // TODO: Only controller components handled for now.
  if(type != ComponentRack::controllerComponent)
    return;

  bool wait_required = false;
  QList<MidiIncListStruct> doneMidiTracks;
  QList<MusECore::Track*> doneAudioTracks;

  const double d_comp_val_delta = newCompVal - oldCompVal;

  if(track->isMidiTrack())
  {
    // TODO: Only volume and pan handled for now.
    int a_ctlnum;
    switch(id)
    {
      case MusECore::CTRL_VOLUME:
        a_ctlnum = MusECore::AC_VOLUME;
      break;
      case MusECore::CTRL_PANPOT:
        a_ctlnum = MusECore::AC_PAN;
      break;
      default:
        return;
      break;
    }

    MusECore::MidiTrack* m_track = static_cast<MusECore::MidiTrack*>(track);
    const int m_port  = m_track->outPort();
    const int m_chan  = m_track->outChannel();
    MusECore::MidiPort* m_mp = &MusEGlobal::midiPorts[m_port];
    MusECore::MidiController* m_mctl = m_mp->midiController(id, false);
    if(!m_mctl)
      return;

    const int i_m_min = m_mctl->minVal();
    const int i_m_max = m_mctl->maxVal();
    const int i_m_bias = m_mctl->bias();
    int i_ma_min = i_m_min;
    const int i_ma_max = i_m_max;

    //----------------------------------------------------------
    // For midi volume, the formula given by MMA is:
    //  volume dB = 40 * log(midivol / 127)  and the inverse is:
    //  midi volume = 127 * 10 ^ (volume dB / 40)
    // Unusual, it is a factor of 40 not 20. Since muse_db2val()
    //  does 10 ^ (volume dB / 20), just divide volume dB by 2.
    //----------------------------------------------------------
    
    double m_old_val = oldCompVal;
    double ma_old_val = oldCompVal;
    double m_new_val = newCompVal;
    double ma_new_val = newCompVal;

    if(id == MusECore::CTRL_VOLUME)
    {
      if(MusEGlobal::config.preferMidiVolumeDb)
      {
        if(ma_old_val <= MusEGlobal::config.minSlider)
          m_old_val = ma_old_val = 0.0;
        else
        {
          m_old_val = double(i_m_max) * muse_db2val(m_old_val / 2.0);
          ma_old_val = double(i_ma_max) * muse_db2val(ma_old_val);
        }

        if(ma_new_val <= MusEGlobal::config.minSlider)
          m_new_val = ma_new_val = 0.0;
        else
        {
          m_new_val = double(i_m_max) * muse_db2val(m_new_val / 2.0);
          ma_new_val = double(i_ma_max) * muse_db2val(ma_new_val);
        }
      }
      else
      {
        // It's a linear scale. We need to convert to Db and
        //  scale the linear value by 2 since the midi scale is
        //  twice the usual Db per step. Then convert back to linear.
        ma_old_val = muse_val2dbr(ma_old_val / double(i_ma_max)) * 2.0;
        ma_old_val *= 2.0;
        ma_old_val = double(i_ma_max) * muse_db2val(ma_old_val / 2.0);

        ma_new_val = muse_val2dbr(ma_new_val / double(i_ma_max)) * 2.0;
        ma_new_val *= 2.0;
        ma_new_val = double(i_ma_max) * muse_db2val(ma_new_val / 2.0);
      }
    }

    //--------------------------------------------------------------
    //   NOTE: Midi int to audio float conversion:
    //   If the control has a bias at all, it is supposed
    //    to define the 'middle', like pan.
    //   But if the control's range is odd (127), that
    //    makes an uneven, uncentered float conversion.
    //   So the policy here is to force an even range for
    //    symmetrical +/- float conversion. (0.0 in the middle.)
    //   Treat value '0' (-64 pan) and '1' (-63 pan) the same.
    //--------------------------------------------------------------
    if(i_m_bias != 0 && ((i_ma_max - i_ma_min) & 0x1))
      ++i_ma_min;
    const int i_m_range = i_m_max - i_m_min;
    const int i_ma_range = i_ma_max - i_ma_min;

    if(m_old_val < i_m_min)
     m_old_val = i_m_min;
    if(m_old_val > i_m_max)
     m_old_val = i_m_max;

    if(m_new_val < i_m_min)
     m_new_val = i_m_min;
    if(m_new_val > i_m_max)
     m_new_val = i_m_max;

    if(ma_old_val < i_ma_min)
     ma_old_val = i_ma_min;
    if(ma_old_val > i_ma_max)
     ma_old_val = i_ma_max;

    if(ma_new_val < i_ma_min)
     ma_new_val = i_ma_min;
    if(ma_new_val > i_ma_max)
     ma_new_val = i_ma_max;

    if(i_m_range == 0 || i_ma_range == 0) // Avoid divide by zero.
      return;

    // Get the current or last valid actual controller value.
    const double m_val_delta = m_new_val - m_old_val;
    const double ma_val_delta = ma_new_val - ma_old_val;

    const double ma_new_delta_fact = ma_val_delta / (double)i_ma_range;

    // Make sure to include this track as already done.
    doneMidiTracks.append(MidiIncListStruct(m_port, m_chan));

    MusECore::TrackList* tracks = MusEGlobal::song->tracks();
    for(MusECore::iTrack it = tracks->begin(); it != tracks->end(); ++it)
    {
      MusECore::Track* t = *it;
      // Do selected tracks. Ignore this track, it has already taken care of its own controllers.
      if(t == track || !t->selected())
        continue;

      if(t->isMidiTrack())
      {
        MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(t);
        const int port     = mt->outPort();
        const int chan  = mt->outChannel();
        // Make sure midi tracks on same port and channel are only done once.
        const MidiIncListStruct mils(port, chan);
        if(doneMidiTracks.contains(mils))
          continue;
        doneMidiTracks.append(mils);

        MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
        MusECore::MidiController* mctl = mp->midiController(id, false);
        if(!mctl)
          continue;
        int min = mctl->minVal();
        const int max = mctl->maxVal();

        if(off)
        {
          if(mp->hwCtrlState(chan, id) != MusECore::CTRL_VAL_UNKNOWN)
            mp->putHwCtrlEvent(MusECore::MidiPlayEvent(0, port, chan,
                                                      MusECore::ME_CONTROLLER,
                                                      id,
                                                      MusECore::CTRL_VAL_UNKNOWN));
        }
        else
        {
          double d_fin_val = m_new_val;
          // Get the current or last valid value.
          double d_cur_val =  mp->hwDCtrlState(chan, id);
          if(MusECore::MidiController::dValIsUnknown(d_cur_val))
            d_cur_val = mp->lastValidHWDCtrlState(chan, id);
          if(MusECore::MidiController::dValIsUnknown(d_cur_val))
            d_fin_val = m_new_val;
          else
          {
            d_cur_val = d_cur_val - double(mctl->bias());
            if(id == MusECore::CTRL_VOLUME && MusEGlobal::config.preferMidiVolumeDb)
            {
              d_fin_val = muse_val2dbr(d_cur_val / double(max)) * 2.0;
              d_fin_val += d_comp_val_delta;
              d_fin_val = double(max) * muse_db2val(d_fin_val / 2.0);
            }
            else
              d_fin_val = d_cur_val + m_val_delta;
          }

          if(d_fin_val < double(min))
            d_fin_val = min;
          if(d_fin_val > double(max))
            d_fin_val = max;

          d_fin_val += double(mctl->bias());

          // False = linear not dB because we are doing the conversion here.
          mp->putControllerValue(port, chan, id, d_fin_val, false);

          // Trip the wait flag.
          wait_required = true;
        }
      }
      else
      {
        // Make sure we're not doing the same track more than once.
        if(doneAudioTracks.contains(t))
          continue;
        doneAudioTracks.append(t);

        MusECore::AudioTrack* at = static_cast<MusECore::AudioTrack*>(t);
        MusECore::iCtrlList icl = at->controller()->find(a_ctlnum);
        if(icl == at->controller()->end())
          continue;
        MusECore::CtrlList* cl = icl->second;

        double d_cur_val = cl->curVal();
        // The audio volume can go above 0dB (amplification) while
        //  the top midi value of 127 represents 0dB. Cut it off at 0dB.
        const double a_min = cl->minVal();
        const double a_max = (a_ctlnum == MusECore::AC_VOLUME) ? 1.0 : cl->maxVal();
        const double a_range = a_max - a_min;
        const double a_val_delta = ma_new_delta_fact * a_range;

        if(id == MusECore::CTRL_VOLUME && MusEGlobal::config.preferMidiVolumeDb)
        {
          if(d_cur_val <= 0.0)
            d_cur_val = MusEGlobal::config.minSlider;
          else
            d_cur_val = muse_val2dbr(d_cur_val);

          d_cur_val += d_comp_val_delta;
          if(d_cur_val < MusEGlobal::config.minSlider)
            d_cur_val = MusEGlobal::config.minSlider;
          if(d_cur_val > 10.0)
            d_cur_val = 10.0;

          d_cur_val = muse_db2val(d_cur_val);
        }
        else
          d_cur_val += a_val_delta;

        if(d_cur_val < a_min)
          d_cur_val = a_min;
        if(d_cur_val > a_max)
          d_cur_val = a_max;

        at->recordAutomation(a_ctlnum, d_cur_val);
        at->setParam(a_ctlnum, d_cur_val);  // Schedules a timed control change.
        at->enableController(a_ctlnum, false);

        // Trip the wait flag.
        wait_required = true;
      }
    }
  }
  else
  {
    // TODO: Only volume and pan handled for now.
    int m_ctlnum;
    switch(id)
    {
      case MusECore::AC_VOLUME:
        m_ctlnum = MusECore::CTRL_VOLUME;
      break;
      case MusECore::AC_PAN:
        m_ctlnum = MusECore::CTRL_PANPOT;
      break;
      default:
        return;
      break;
    }

    MusECore::AudioTrack* a_track = static_cast<MusECore::AudioTrack*>(track);
    MusECore::iCtrlList icl = a_track->controller()->find(id);
    if(icl == a_track->controller()->end())
      return;
    MusECore::CtrlList* cl = icl->second;

    //----------------------------------------------------------
    // For midi volume, the formula given by MMA is:
    //  volume dB = 40 * log(midivol / 127)  and the inverse is:
    //  midi volume = 127 * 10 ^ (volume dB / 40)
    // Unusual, it is a factor of 40 not 20. Since muse_db2val()
    //  does 10 ^ (volume dB / 20), just divide volume dB by 2.
    //----------------------------------------------------------
    double a_val = newCompVal;
    double ma_val = newCompVal;
    if(id == MusECore::AC_VOLUME)
    {
      if(ma_val <= MusEGlobal::config.minSlider)
        a_val = ma_val = 0.0;
      else
      {
        a_val = muse_db2val(a_val);
        ma_val = muse_db2val(ma_val / 2.0);
      }
    }

    // The audio volume can go above 0dB (amplification) while
    //  the top midi value of 127 represents 0dB. Cut it off at 0dB.
    const double a_min = cl->minVal();
    const double a_max = cl->maxVal();
    const double a_range = a_max - a_min;
    if(a_range < 0.0001) // Avoid divide by zero.
      return;
    const double a_fact_delta = muse_round2micro((newCompVal - oldCompVal) / a_range);

    MusECore::TrackList* tracks = MusEGlobal::song->tracks();
    for(MusECore::iTrack it = tracks->begin(); it != tracks->end(); ++it)
    {
      MusECore::Track* t = *it;
      // Do selected tracks. Ignore this track, it has already taken care of its own controllers.
      if(t == track || !t->selected())
        continue;
      if(t->isMidiTrack())
      {
        MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(t);
        const int port     = mt->outPort();
        const int chan  = mt->outChannel();
        // Make sure midi tracks on same port and channel are only done once.
        const MidiIncListStruct mils(port, chan);
        if(doneMidiTracks.contains(mils))
          continue;
        doneMidiTracks.append(mils);

        MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
        MusECore::MidiController* mctl = mp->midiController(m_ctlnum, false);
        if(mctl)
        {
          int min = mctl->minVal();
          const int max = mctl->maxVal();
          const int bias = mctl->bias();

          //--------------------------------------------------------------
          //   NOTE: Midi int to audio float conversion:
          //   If the control has a bias at all, it is supposed
          //    to define the 'middle', like pan.
          //   But if the control's range is odd (127), that
          //    makes an uneven, uncentered float conversion.
          //   So the policy here is to force an even range for
          //    symmetrical +/- float conversion. (0.0 in the middle.)
          //   Treat value '0' (-64 pan) and '1' (-63 pan) the same.
          //--------------------------------------------------------------
          if(bias != 0 && ((max - min) & 0x1))
            ++min;

          const double d_min = (double)min;
          const double d_max = (double)max;
          const double d_range = double(max - min);

          const double m_val_delta = muse_round2micro(a_fact_delta * d_range);

          double d_fin_val = 0.0;
          // Get the current or last valid value.
          double d_cur_val =  mp->hwDCtrlState(chan, m_ctlnum);
          if(MusECore::MidiController::dValIsUnknown(d_cur_val))
            d_cur_val = mp->lastValidHWDCtrlState(chan, m_ctlnum);
          if(MusECore::MidiController::dValIsUnknown(d_cur_val))
          {
            if(!mctl->initValIsUnknown())
              d_cur_val = double(mctl->initVal()) + double(bias);
          }
          if(MusECore::MidiController::dValIsUnknown(d_cur_val))
            d_fin_val = 0.0;
          else
          {
            d_cur_val = d_cur_val - double(mctl->bias());
            if(m_ctlnum == MusECore::CTRL_VOLUME)
            {
              d_fin_val = muse_val2dbr(d_cur_val / d_max) * 2.0;
              d_fin_val += d_comp_val_delta;
              d_fin_val = d_max * muse_db2val(d_fin_val / 2.0);
            }
            else
              d_fin_val = d_cur_val + m_val_delta;
          }

          if(d_fin_val < d_min)
            d_fin_val = d_min;
          if(d_fin_val > d_max)
            d_fin_val = d_max;

          d_fin_val += double(mctl->bias());

          // False = linear not dB because we are doing the conversion here.
          mp->putControllerValue(port, chan, m_ctlnum, d_fin_val, false);

          // Trip the wait flag.
          wait_required = true;
        }
      }
      else
      {
        // Make sure we're not doing the same track more than once.
        if(doneAudioTracks.contains(t))
          continue;
        doneAudioTracks.append(t);

        MusECore::AudioTrack* at = static_cast<MusECore::AudioTrack*>(t);
        MusECore::iCtrlList icl = at->controller()->find(id);
        if(icl == at->controller()->end())
          continue;
        MusECore::CtrlList* cl = icl->second;

        double d_cur_val = cl->curVal();
        // The audio volume can go above 0dB (amplification) while
        //  the top midi value of 127 represents 0dB. Cut it off at 0dB.
        const double d_min = cl->minVal();
        const double d_max = cl->maxVal();
        const double d_range = d_max - d_min;
        const double d_val_delta = a_fact_delta * d_range;

        if(id == MusECore::AC_VOLUME)
        {
          if(d_cur_val <= 0.0)
            d_cur_val = MusEGlobal::config.minSlider;
          else
            d_cur_val = muse_val2dbr(d_cur_val);

          d_cur_val += d_comp_val_delta;
          if(d_cur_val < MusEGlobal::config.minSlider)
            d_cur_val = MusEGlobal::config.minSlider;
          if(d_cur_val > 10.0)
            d_cur_val = 10.0;

          d_cur_val = muse_db2val(d_cur_val);
        }
        else
          d_cur_val += d_val_delta;

        if(d_cur_val < d_min)
          d_cur_val = d_min;
        if(d_cur_val > d_max)
          d_cur_val = d_max;

        // Hack: Be sure to ignore in ScrDirect mode since we get both pressed AND changed signals.
        // ScrDirect mode is one-time only on press with modifier.
        //if(scrollMode != SliderBase::ScrDirect)
          at->recordAutomation(id, d_cur_val);
        at->setParam(id, d_cur_val);  // Schedules a timed control change.
        at->enableController(id, false);

        // Trip the wait flag.
        wait_required = true;
      }
    }
  }

  // This is a DELTA operation. Unfortunately we may need to WAIT for the hw controls to update
  //  in the audio thread before we can apply ANOTHER delta to the soon-to-be 'current' value.
  if(wait_required)
    MusEGlobal::audio->msgAudioWait();
}

} // namespace MusEGui
