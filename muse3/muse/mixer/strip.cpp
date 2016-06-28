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

#include <QToolButton>
#include <QLayout>
#include <QPalette>
#include <QColor>
#include <QFrame>
#include <QMouseEvent>
#include <QMenu>
#include <QSignalMapper>
#include <QString>
#include <QPainter>

#include "globals.h"
#include "gconfig.h"
#include "app.h"
#include "audio.h"
#include "song.h"
#include "track.h"
#include "strip.h"
#include "meter.h"
#include "utils.h"
#include "icons.h"
#include "undo.h"
#include "amixer.h"
#include "compact_slider.h"
#include "elided_label.h"

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
        
        if(d->_changedSlot)
          connect(control, SIGNAL(valueStateChanged(double,bool,int, int)), d->_changedSlot);
        if(d->_movedSlot)
          connect(control, SIGNAL(sliderMoved(double,int,bool)), d->_movedSlot);
        if(d->_pressedSlot)
          connect(control, SIGNAL(sliderPressed(int)), d->_pressedSlot);
        if(d->_releasedSlot)
          connect(control, SIGNAL(sliderReleased(int)), d->_releasedSlot);
        if(d->_rightClickedSlot)
          connect(control, SIGNAL(sliderRightClicked(QPoint,int)), d->_rightClickedSlot);

        switch(d->_componentType)
        {
          case controllerComponent:
            if(!d->_changedSlot)
              connect(control, SIGNAL(valueStateChanged(double,bool,int, int)), SLOT(controllerChanged(double,bool,int,int)));
            if(!d->_movedSlot)
              connect(control, SIGNAL(sliderMoved(double,int,bool)), SLOT(controllerMoved(double,int,bool)));
            if(!d->_pressedSlot)
              connect(control, SIGNAL(sliderPressed(int)), SLOT(controllerPressed(int)));
            if(!d->_releasedSlot)
              connect(control, SIGNAL(sliderReleased(int)), SLOT(controllerReleased(int)));
            if(!d->_rightClickedSlot)
              connect(control, SIGNAL(sliderRightClicked(QPoint,int)), SLOT(controllerRightClicked(QPoint,int)));
          break;
          
          case propertyComponent:
            if(!d->_changedSlot)
              connect(control, SIGNAL(valueStateChanged(double,bool,int, int)), SLOT(propertyChanged(double,bool,int,int)));
            if(!d->_movedSlot)
              connect(control, SIGNAL(sliderMoved(double,int,bool)), SLOT(propertyMoved(double,int,bool)));
            if(!d->_pressedSlot)
              connect(control, SIGNAL(sliderPressed(int)), SLOT(propertyPressed(int)));
            if(!d->_releasedSlot)
              connect(control, SIGNAL(sliderReleased(int)), SLOT(propertyReleased(int)));
            if(!d->_rightClickedSlot)
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

        if(d->_labelPressedSlot)
          connect(control, SIGNAL(pressed(QPoint,int,Qt::MouseButtons,Qt::KeyboardModifiers)), d->_labelPressedSlot);
        if(d->_labelReleasedSlot)
          connect(control, SIGNAL(released(QPoint,int,Qt::MouseButtons,Qt::KeyboardModifiers)), d->_labelReleasedSlot);

        switch(d->_componentType)
        {
          case propertyComponent:
            if(!d->_labelPressedSlot)
              connect(control, SIGNAL(pressed(QPoint,int,Qt::MouseButtons,Qt::KeyboardModifiers)), 
                      SLOT(labelPropertyPressed(QPoint,int,Qt::MouseButtons,Qt::KeyboardModifiers)));
            if(!d->_labelReleasedSlot)
              connect(control, SIGNAL(released(QPoint,int,Qt::MouseButtons,Qt::KeyboardModifiers)), 
                      SLOT(labelPropertyReleased(QPoint,int,Qt::MouseButtons,Qt::KeyboardModifiers)));
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
  
void ComponentRack::setComponentMinValue(const ComponentWidget& cw, double min)
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
        w->blockSignals(true);
        w->setMinValue(min);
        w->blockSignals(false);
      }
    }
    break;
  }
}

void ComponentRack::setComponentMaxValue(const ComponentWidget& cw, double max)
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
        w->blockSignals(true);
        w->setMaxValue(max);
        w->blockSignals(false);
      }
    }
    break;
  }
}

void ComponentRack::setComponentRange(const ComponentWidget& cw, double min, double max, 
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
        w->blockSignals(true);
        if(min != w->minValue() && max != w->maxValue())
          w->setRange(min, max, step, pageSize, mode);
        else if(min != w->minValue())
          w->setMinValue(max);
        else
          w->setMaxValue(max);
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
    }
  }
  
  return 0.0;
}

void ComponentRack::setComponentValue(const ComponentWidget& cw, double val)
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
        w->blockSignals(true);
        w->setValue(val);
        w->blockSignals(false);
        //cw->_currentValue = val;  // TODO ?
      }
    }
    break;
  }
}

void ComponentRack::setComponentText(const ComponentWidget& cw, const QString& text)
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
        w->blockSignals(true);
        w->setText(text);
        w->blockSignals(false);
      }
    }
    break;
  }
}

void ComponentRack::setComponentEnabled(const ComponentWidget& cw, bool enable)
{
  if(!cw._widget)
    return;

  // Nothing special for now. Just operate on the widget itself.
  cw._widget->setEnabled(enable);
}

//---------------------------------------------------------
//   configChanged
//   Catch when label font, or configuration min slider and meter values change, or viewable tracks etc.
//---------------------------------------------------------

void ComponentRack::configChanged() 
{ 
  for(ciComponentWidget ic = _components.begin(); ic != _components.end(); ++ic)
  {
    const ComponentWidget& cw = *ic;
    if(!cw._widget)
      continue;
    
    switch(cw._widgetType)
    {
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

const int Strip::FIXED_METER_WIDTH = 10;

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
      if (track->type() == MusECore::Track::AUDIO_OUTPUT) {
            if (val && track->recordFlag() == false) {
                  MusEGlobal::muse->bounceToFile((MusECore::AudioOutput*)track);
                  }
            MusEGlobal::audio->msgSetRecord((MusECore::AudioOutput*)track, val);
            if (!((MusECore::AudioOutput*)track)->recFile())
            {  
                  record->setChecked(false);
            }      
            return;
            }
      MusEGlobal::song->setRecordFlag(track, val);
      }
//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void Strip::heartBeat()
      {
      }

//---------------------------------------------------------
//   setLabelFont
//---------------------------------------------------------

void Strip::setLabelFont()
{
  // Use the new font #6 I created just for these labels (so far).
  // Set the label's font.
  label->setFont(MusEGlobal::config.fonts[6]);
  label->setStyleSheet(MusECore::font2StyleSheet(MusEGlobal::config.fonts[6]));
  // Dealing with a horizontally constrained label. Ignore vertical. Use a minimum readable point size.
  MusECore::autoAdjustFontSize(label, label->text(), false, true, MusEGlobal::config.fonts[6].pointSize(), 5); 
}

void Strip::paintEvent(QPaintEvent * /*ev*/)
{
  QPainter p(this);
  if (_highlight) {
    p.setPen(Qt::darkYellow);
    p.drawRect(0,0,width()-1,height()-1);
    p.drawRect(1,1,width()-2,height()-2);
  }
}

//---------------------------------------------------------
//   setLabelText
//---------------------------------------------------------

void Strip::setLabelText()
{
      QColor c;
      switch(track->type()) {
            case MusECore::Track::AUDIO_OUTPUT:
                  //c = Qt::green;
                  c = MusEGlobal::config.outputTrackLabelBg;
                  break;
            case MusECore::Track::AUDIO_GROUP:
                  //c = Qt::yellow;
                  c = MusEGlobal::config.groupTrackLabelBg;
                  break;
            case MusECore::Track::AUDIO_AUX:
                  //c = QColor(120, 255, 255);   // Light blue
                  c = MusEGlobal::config.auxTrackLabelBg;
                  break;
            case MusECore::Track::WAVE:
                  //c = Qt::magenta;
                  c = MusEGlobal::config.waveTrackLabelBg;
                  break;
            case MusECore::Track::AUDIO_INPUT:
                  //c = Qt::red;
                  c = MusEGlobal::config.inputTrackLabelBg;
                  break;
            case MusECore::Track::AUDIO_SOFTSYNTH:
                  //c = QColor(255, 130, 0);  // Med orange
                  c = MusEGlobal::config.synthTrackLabelBg;
                  break;
            case MusECore::Track::MIDI:
                  //c = QColor(0, 160, 255); // Med blue
                  c = MusEGlobal::config.midiTrackLabelBg;
                  break;
            case MusECore::Track::DRUM:
                  //c = QColor(0, 160, 255); // Med blue
                  c = MusEGlobal::config.drumTrackLabelBg;
                  break;
            case MusECore::Track::NEW_DRUM:
                  //c = QColor(0, 160, 255); // Med blue
                  c = MusEGlobal::config.newDrumTrackLabelBg;
                  break;
            default:
                  return;      
            }
      
      if (track->type() == MusECore::Track::AUDIO_AUX) {
          label->setText(((MusECore::AudioAux*)track)->auxName());
      } else {
          label->setText(track->name());
      }
      QPalette palette;
      QLinearGradient gradient(label->geometry().topLeft(), label->geometry().bottomLeft());
      gradient.setColorAt(0, c);
      gradient.setColorAt(0.5, c.lighter());
      gradient.setColorAt(1, c);
      palette.setBrush(label->backgroundRole(), gradient);
      label->setPalette(palette);
}

//---------------------------------------------------------
//   muteToggled
//---------------------------------------------------------

void Strip::muteToggled(bool val)
      {
      MusEGlobal::audio->msgSetTrackMute(track, val);
      MusEGlobal::song->update(SC_MUTE);
      }

//---------------------------------------------------------
//   soloToggled
//---------------------------------------------------------

void Strip::soloToggled(bool val)
      {
      MusEGlobal::audio->msgSetSolo(track, val);
      MusEGlobal::song->update(SC_SOLO);
      }

//---------------------------------------------------------
//   Strip
//    create mixer strip
//---------------------------------------------------------

Strip::Strip(QWidget* parent, MusECore::Track* t, bool hasHandle)
   : QFrame(parent)
      {
      setMouseTracking(true);
      _selected = false;
      _highlight = false;

      _curGridRow = 0;
      _userWidth = 0;
      autoType = 0;
      _visible = true;
      dragOn=false;
      setAttribute(Qt::WA_DeleteOnClose);
      iR            = 0;
      oR            = 0;
      
      ///setBackgroundRole(QPalette::Mid);
      setFrameStyle(Panel | Raised);
      setLineWidth(2);
      
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

      //label = new QLabel(this);
      // NOTE: This was required, otherwise the strip labels have no colour in the mixer only - track info OK !
      // Not sure why...
      label = new QLabel(this);
      label->setObjectName(track->cname());
      label->setTextFormat(Qt::PlainText);
      
      // Unfortunately for the mixer labels, QLabel doesn't support the BreakAnywhere flag.
      // Changed by Tim. p3.3.9
      //label->setAlignment(AlignCenter);
      //label->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
      // MusE-2 Tested: TextWrapAnywhere actually works, but in fact it takes precedence 
      //  over word wrap, so I found it is not really desirable. Maybe with a user setting...
      //label->setAlignment(Qt::AlignCenter | Qt::TextWordWrap | Qt::TextWrapAnywhere);
      // changed by Orcan: We can't use Qt::TextWordWrap in alignment in Qt4.
      label->setAlignment(Qt::AlignCenter);
      label->setWordWrap(true);
      label->setAutoFillBackground(true);
      label->setLineWidth(2);
      label->setFrameStyle(Sunken | StyledPanel);
      
      label->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum));
      
      setLabelText();
      setLabelFont();
      
      grid->addWidget(label, _curGridRow++, 0, 1, 3);
      }

//---------------------------------------------------------
//   Strip
//---------------------------------------------------------

Strip::~Strip()
      {
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
  // If going to OFF mode, need to update current 'manual' values from the automation values at this time...   
  if(t == AUTO_OFF && track->automationType() != AUTO_OFF) // && track->automationType() != AUTO_WRITE)
  {
    // May have a lot to do in updateCurValues, so try using idle.
    MusEGlobal::audio->msgIdle(true);
    track->setAutomationType(AutomationType(t));
    if(!track->isMidiTrack())
      (static_cast<MusECore::AudioTrack*>(track))->controller()->updateCurValues(MusEGlobal::audio->curFramePos());
    MusEGlobal::audio->msgIdle(false);
  }
  else
    // Try it within one message.
    MusEGlobal::audio->msgSetTrackAutomationType(track, t);   
  
  MusEGlobal::song->update(SC_AUTOMATION);
}
      
void Strip::resizeEvent(QResizeEvent* ev)
{
  DEBUG_STRIP(stderr, "Strip::resizeEvent\n");  
  QFrame::resizeEvent(ev);
  setLabelText();  
  setLabelFont();
}  

void Strip::updateRouteButtons()
{
  if (iR)
  {
      if (track->noInRoute())
      {
        iR->setStyleSheet("background-color:red;");
        iR->setToolTip(MusEGlobal::noInputRoutingToolTipWarn);
      }
      else
      {
        iR->setStyleSheet("");
        iR->setToolTip(MusEGlobal::inputRoutingToolTipBase);
      }
  }

  if (oR)
  {
    if (track->noOutRoute())
    {
      oR->setStyleSheet("background-color:red;");
      oR->setToolTip(MusEGlobal::noOutputRoutingToolTipWarn);
    }
    else
    {
      oR->setStyleSheet("");
      oR->setToolTip(MusEGlobal::outputRoutingToolTipBase);
    }
  }
}



void Strip::mousePressEvent(QMouseEvent* ev)
{
  // Only one button at a time.
  if(ev->buttons() ^ ev->button())
  {
    //_resizeMode = ResizeModeNone;
    //unsetCursor();
    ev->accept();
    return;
  }

  if (ev->modifiers() & Qt::ControlModifier)
    setSelected(true);
  else {
    emit clearStripSelection();
    setSelected(true);
  }
  
  //if(_resizeMode == ResizeModeHovering)
  //{
    // Switch to dragging mode.
    //_resizeMode = ResizeModeDragging;
    //ev->ignore();
    //return;
  //}
  
  //  printf("strip mouse press event! %d\n",(int)ev->button());
  QPoint mousePos = QCursor::pos();
  mouseWidgetOffset = pos() - mousePos;

  if (ev->button() == Qt::RightButton) {
    QMenu* menu = new QMenu;
//    QAction *act = menu->addAction(tr("Remove track"));
//    act->setData(int(0));
//    menu->addSeparator();
    QAction *act = menu->addAction(tr("Hide strip"));
    act->setData(int(1));
    QPoint pt = QCursor::pos();
    act = menu->exec(pt, 0);
    if (!act)
    {
      delete menu;
      QFrame::mousePressEvent(ev);
      return;
    }

    DEBUG_STRIP("Menu finished, data returned %d\n", act->data().toInt());

    //if (act->data().toInt() == 0)
    //{
    //  DEBUG_STRIP(stderr, "Strip:: delete track\n");
    //  MusEGlobal::song->applyOperation(UndoOp(UndoOp::DeleteTrack, MusEGlobal::song->tracks()->index(track), track));
    //}
    /* else */
    if (act->data().toInt() == 1)
    {
      DEBUG_STRIP(stderr, "Strip:: setStripVisible false \n");
      setStripVisible(false);
      setVisible(false);
      MusEGlobal::song->update();
    }
    ev->accept();
    return;
  }
  QFrame::mousePressEvent(ev);
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
}

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
  //printf("strip mouse release event! %d\n",(int)ev->button());
  if (dragOn) {
    ((AudioMixerApp*)MusEGlobal::muse->mixer1Window())->moveStrip(this);
  }
  dragOn=false;
  QFrame::mouseReleaseEvent(ev);

}

void Strip::mouseMoveEvent(QMouseEvent*)
{
  bool isMoveStarted = false;

  //if (dragOn)
  //  printf("mouseMoveEvent and dragOn set!\n");
  
  if (MusEGlobal::muse->mixer1Window()) {
   isMoveStarted = ((AudioMixerApp*)MusEGlobal::muse->mixer1Window())->isMixerClicked();
  }
  if (isMoveStarted)
  {
    //printf("mouseMoveEvent isMoveStarted is set!\n");

    // we are located in the mixer and will try to move strip
    QPoint mousePos = QCursor::pos();
    move(mousePos + mouseWidgetOffset);
    dragOn = true;
    this->raise();
  }
  else {
    //printf("NO MOVE\n");
  }
  //QFrame::mouseMoveEvent(ev);
}

void Strip::keyPressEvent(QKeyEvent *ev)
{
  printf("Strip key pressed\n");
  ev->ignore();
}

void Strip::setSelected(bool v)
{
  if (v) {
    label->setFrameStyle(Raised | StyledPanel);
    setHighLight(true);
  }
  else {
    label->setFrameStyle(Sunken | StyledPanel);
    setHighLight(false);
  }

  _selected=v;
  emit trackSelected(track, false);
}

void Strip::setHighLight(bool highlight)
{
  _highlight = highlight;
  update();
}

} // namespace MusEGui
