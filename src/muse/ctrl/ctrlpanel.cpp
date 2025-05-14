//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ctrlpanel.cpp,v 1.10.2.9 2009/06/14 05:24:45 terminator356 Exp $
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2012, 2017 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

//#include <stdio.h>
#include <list>

#include "ctrlpanel.h"
#include "ctrlcanvas.h"

#include <QSizePolicy>
//#include <QTimer>
#include <QColor>
#include <QStyle>

#include "muse_math.h"

#include "globaldefs.h"
#include "app.h"
#include "globals.h"
#include "midictrl.h"
#include "instruments/minstrument.h"
#include "mididev.h"
#include "icons.h"
//#include "event.h"
#include "part.h"
#include "midiedit/drummap.h"
#include "gconfig.h"
#include "song.h"
#include "utils.h"

#include "audio.h"
#include "midi_consts.h"
//#include "menutitleitem.h"
#include "popupmenu.h"
#include "helper.h"

// Forwards from header:
//#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QAction>
#include "midieditor.h"
#include "midi_controller.h"
#include "midiport.h"
#include "track.h"
//#include "ctrlcanvas.h"
#include "compact_knob.h"
#include "compact_slider.h"
#include "lcd_widgets.h"

namespace MusEGui {

//---------------------------------------------------------
//   CtrlPanel
//---------------------------------------------------------

CtrlPanel::CtrlPanel(QWidget* parent, MidiEditor* e, CtrlCanvas* c, const char* name)
   : QWidget(parent)
      {
      setObjectName(name);
      inHeartBeat = true;

      //setFocusPolicy(Qt::NoFocus);

      _knob = nullptr;
      _slider = nullptr;
      _patchEdit = nullptr;
      _veloPerNoteButton = nullptr;

      _preferKnobs = MusEGlobal::config.preferKnobsVsSliders;
      _showval = MusEGlobal::config.showControlValues;
      editor = e;
      ctrlcanvas = c;
      setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
      vbox = new QVBoxLayout;
      QHBoxLayout* bbox = new QHBoxLayout;
      bbox->setSpacing (0);
      vbox->addLayout(bbox);
      vbox->addStretch();
      kbox = new QHBoxLayout;
      vbox->addLayout(kbox);
      vbox->addStretch();
      vbox->setContentsMargins(0, 0, 0, 0);
      bbox->setContentsMargins(0, 0, 0, 0);
      kbox->setContentsMargins(0, 0, 0, 0);
      vbox->setSpacing (0);
      kbox->setSpacing(0);
      lspacer = nullptr;
      rspacer = nullptr;

      // Select controller
      selCtrl = new CompactToolButton(this);
      selCtrl->setIcon(*midiControllerSelectSVGIcon);
      selCtrl->setIconSize(QSize(14, 14));
      selCtrl->setHasFixedIconSize(true);
      selCtrl->setContentsMargins(4, 4, 4, 4);
      selCtrl->setFocusPolicy(Qt::NoFocus);
      selCtrl->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));
      selCtrl->setToolTip(tr("Select controller"));
      
      // Remove panel (destroy button)
      CompactToolButton* destroy = new CompactToolButton(this);
      destroy->setIcon(*midiControllerRemoveSVGIcon);
      destroy->setIconSize(QSize(14, 14));
      destroy->setHasFixedIconSize(true);
      destroy->setContentsMargins(4, 4, 4, 4);
      destroy->setFocusPolicy(Qt::NoFocus);
      destroy->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));
      destroy->setToolTip(tr("Remove panel"));

      connect(selCtrl, SIGNAL(clicked()), SLOT(ctrlPopup()));
      connect(destroy, SIGNAL(clicked()), SIGNAL(destroyPanel()));
      
      _track = nullptr;
      _ctrl = nullptr;
      _dnum = -1;
      
      bbox->addStretch();
      bbox->addWidget(selCtrl);
      bbox->addWidget(destroy);
      bbox->addStretch();

      configChanged();

      connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), SLOT(songChanged(MusECore::SongChangedStruct_t)));
      connect(MusEGlobal::muse, SIGNAL(configChanged()), SLOT(configChanged()));
      connect(MusEGlobal::heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
      inHeartBeat = false;
      setLayout(vbox);
      // When closing/(re)loading, we must wait for this to delete to avoid crashes
      //  due to still active external connections like heartBeatTimer.
      MusEGlobal::muse->addPendingObjectDestruction(this);
      }

void CtrlPanel::buildPanel()
{
  if(!_track || !_ctrl)
  {
    if(_veloPerNoteButton)
    { kbox->removeWidget(_veloPerNoteButton); _veloPerNoteButton->deleteLater(); _veloPerNoteButton = nullptr; }

    if(_slider) { kbox->removeWidget(_slider); _slider->deleteLater(); _slider = nullptr; }

    if(_knob) { kbox->removeWidget(_knob); _knob->deleteLater(); _knob = nullptr; }

    if(_patchEdit) { kbox->removeWidget(_patchEdit); _patchEdit->deleteLater(); _patchEdit = nullptr; }

    if(lspacer) { kbox->removeItem(lspacer); delete lspacer; lspacer = nullptr; }

    if(rspacer) { kbox->removeItem(rspacer); delete rspacer; rspacer = nullptr; }

    return;
  }
  
  if(_dnum == MusECore::CTRL_VELOCITY)
  {
    if(_slider) { kbox->removeWidget(_slider); _slider->deleteLater(); _slider = nullptr; }

    if(_knob) { kbox->removeWidget(_knob); _knob->deleteLater(); _knob = nullptr; }

    if(_patchEdit) { kbox->removeWidget(_patchEdit); _patchEdit->deleteLater(); _patchEdit = nullptr; }

    if(!lspacer)
    {
      lspacer = new QSpacerItem(0, 0);
      kbox->addSpacerItem(lspacer);
    }

    if(!_veloPerNoteButton)
    {
      _veloPerNoteButton = new CompactToolButton(this);
      _veloPerNoteButton->setIcon(*velocityPerNoteSVGIcon);
      _veloPerNoteButton->setIconSize(QSize(19, 19));
      _veloPerNoteButton->setHasFixedIconSize(true);
      _veloPerNoteButton->setContentsMargins(2, 2, 2, 2);
      _veloPerNoteButton->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));

      _veloPerNoteButton->setFocusPolicy(Qt::NoFocus);
      _veloPerNoteButton->setCheckable(true);
      _veloPerNoteButton->setToolTip(tr("All/Per-note velocity mode"));
      if(ctrlcanvas)
        _veloPerNoteButton->setChecked(ctrlcanvas->perNoteVeloMode());

      connect(_veloPerNoteButton, SIGNAL(clicked()), SLOT(velPerNoteClicked()));

      kbox->addWidget(_veloPerNoteButton);
    }

    if(!rspacer)
    {
      rspacer = new QSpacerItem(0, 0);
      kbox->addSpacerItem(rspacer);
    }
  }
  else
  {
    if(lspacer) { kbox->removeItem(lspacer); delete lspacer; lspacer = nullptr; }

    if(rspacer) { kbox->removeItem(rspacer); delete rspacer; rspacer = nullptr; }

    if(_dnum == MusECore::CTRL_PROGRAM)
    {
      if(_veloPerNoteButton)
      { kbox->removeWidget(_veloPerNoteButton); _veloPerNoteButton->deleteLater(); _veloPerNoteButton = nullptr; }

      if(_slider) { kbox->removeWidget(_slider); _slider->deleteLater(); _slider = nullptr; }

      if(_knob) { kbox->removeWidget(_knob); _knob->deleteLater(); _knob = nullptr; }

      if(!_patchEdit)
      {
        _patchEdit = new LCDPatchEdit(this);
        _patchEdit->setReadoutOrientation(LCDPatchEdit::PatchVertical);
        _patchEdit->setValue(MusECore::CTRL_VAL_UNKNOWN);
        _patchEdit->setFocusPolicy(Qt::NoFocus);
        // Don't allow anything here, it interferes with the CompactPatchEdit which sets it's own controls' tooltips.
        //control->setToolTip(d->_toolTipText);
        _patchEdit->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
        _patchEdit->setContentsMargins(0, 0, 0, 0);

        _patchEdit->setMaxAliasedPointSize(MusEGlobal::config.maxAliasedPointSize);
        if(_patchEdit->font() != MusEGlobal::config.fonts[1])
        {
          _patchEdit->setFont(MusEGlobal::config.fonts[1]);
          _patchEdit->setStyleSheet(MusECore::font2StyleSheetFull(MusEGlobal::config.fonts[1]));
        }

        connect(_patchEdit, SIGNAL(valueChanged(int,int)), SLOT(patchCtrlChanged(int)));
        connect(_patchEdit, SIGNAL(rightClicked(const QPoint&, int)), SLOT(ctrlRightClicked(const QPoint&, int)));

        kbox->addWidget(_patchEdit);
      }
    }
    else
    {
      if(_preferKnobs)
      {
        if(_veloPerNoteButton)
        { kbox->removeWidget(_veloPerNoteButton); _veloPerNoteButton->deleteLater(); _veloPerNoteButton = nullptr; }

        if(_slider) { kbox->removeWidget(_slider); _slider->deleteLater(); _slider = nullptr; }

        if(_patchEdit) { kbox->removeWidget(_patchEdit); _patchEdit->deleteLater(); _patchEdit = nullptr; }

        if(!_knob)
        {
          _knob = new CompactKnob(this, "CtrlPanelKnob", CompactKnob::Bottom);
          _knob->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
          _knob->setToolTip(tr("Manual adjust (Ctrl-double-click on/off)"));
          //_knob->setFocusPolicy(Qt::NoFocus);
          _knob->setRange(0.0, 127.0, 1.0);
          _knob->setValue(0.0);
          _knob->setHasOffMode(true);
          _knob->setOff(true);
          _knob->setValueDecimals(0);
          _knob->setFaceColor(MusEGlobal::config.midiControllerSliderColor);
          _knob->setStep(1.0);
          _knob->setShowLabel(false);
          _knob->setShowValue(true);
          _knob->setEnableValueToolTips(false);      // FIXME: Tooltip just gets in the way!
          _knob->setShowValueToolTipsOnHover(false); //

          if(_knob->font() != MusEGlobal::config.fonts[1])
          {
            _knob->setFont(MusEGlobal::config.fonts[1]);
            _knob->setStyleSheet(MusECore::font2StyleSheetFull(MusEGlobal::config.fonts[1]));
          }

          connect(_knob, SIGNAL(valueStateChanged(double,bool,int,int)), SLOT(ctrlChanged(double,bool,int,int)));
          connect(_knob, SIGNAL(sliderRightClicked(const QPoint&, int)), SLOT(ctrlRightClicked(const QPoint&, int)));

          kbox->addWidget(_knob);
        }
      }
      else
      {
        if(_veloPerNoteButton)
        { kbox->removeWidget(_veloPerNoteButton); _veloPerNoteButton->deleteLater(); _veloPerNoteButton = nullptr; }

        if(_knob) { kbox->removeWidget(_knob); _knob->deleteLater(); _knob = nullptr; }

        if(_patchEdit) { kbox->removeWidget(_patchEdit); _patchEdit->deleteLater(); _patchEdit = nullptr; }

        if(!_slider)
        {
          _slider = new CompactSlider(this, "CtrlPanelSlider");
          _slider->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
          _slider->setToolTip(tr("Manual adjust (Ctrl-double-click on/off)"));
          //_slider->setFocusPolicy(Qt::NoFocus);
          _slider->setRange(0.0, 127.0, 1.0);
          _slider->setValue(0.0);
          _slider->setHasOffMode(true);
          _slider->setOff(true);
          _slider->setValueDecimals(0);
          _slider->setBarColor(MusEGlobal::config.sliderBackgroundColor);
          _slider->setStep(1.0);
          _slider->setMaxAliasedPointSize(MusEGlobal::config.maxAliasedPointSize);
          _slider->setEnableValueToolTips(false);      // FIXME: Tooltip just gets in the way!
          _slider->setShowValueToolTipsOnHover(false); //

          if(_slider->font() != MusEGlobal::config.fonts[1])
          {
            _slider->setFont(MusEGlobal::config.fonts[1]);
            _slider->setStyleSheet(MusECore::font2StyleSheetFull(MusEGlobal::config.fonts[1]));
          }

          connect(_slider, SIGNAL(valueStateChanged(double,bool,int,int)), SLOT(ctrlChanged(double,bool,int,int)));
          connect(_slider, SIGNAL(sliderRightClicked(const QPoint&, int)), SLOT(ctrlRightClicked(const QPoint&, int)));

          kbox->addWidget(_slider);
        }
      }
    }
  }
}

//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void CtrlPanel::heartBeat()
{
  if(editor && editor->deleting())  // Ignore while while deleting to prevent crash.
    return;
  
  inHeartBeat = true;
  
  if(_track && _ctrl && _dnum != -1)
  {
    if(_dnum != MusECore::CTRL_VELOCITY)
    {
      int outport = _track->outPort();
      int chan = _track->outChannel();
      int cdp = ctrlcanvas->getCurDrumPitch();
      if(_ctrl->isPerNoteController() && cdp >= 0)
      {
        if(_track->type() == MusECore::Track::DRUM)
        {
          // Default to track port if -1 and track channel if -1.
          outport = _track->drummap()[cdp].port;
          if(outport == -1)
            outport = _track->outPort();
          chan = _track->drummap()[cdp].channel;
          if(chan == -1)
            chan = _track->outChannel();
        }
      }

      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[outport];          
      MusECore::MidiCtrlValListList* mcvll = mp->controller();

      MusECore::ciMidiCtrlValList imcvl = mcvll->find(chan, _dnum);
      const bool enable = imcvl != mcvll->end() && !_track->off();

// REMOVE Tim. midi. Added.
// Although we do this for the midi strip control racks (and probably shouldn't),
//  we need the control to be enabled - user would expect to be able to adjust it
//  even if controller was not found - our code will create the controller.
//       if(_knob)
//       {
//         if(_knob->isEnabled() != enable)
//           _knob->setEnabled(enable);
//       }
//       else if(_slider)
//       {
//         if(_slider->isEnabled() != enable)
//           _slider->setEnabled(enable);
//       }

      if(enable)
      {
        MusECore::MidiCtrlValList* mcvl = imcvl->second;

        int hwVal = mcvl->hwVal();

        if(_patchEdit && _dnum == MusECore::CTRL_PROGRAM)
        {
          // Special for new LCD patch edit control: Need to give both current and last values.
          // Keeping a local last value with the control won't work.
          _patchEdit->blockSignals(true);
          _patchEdit->setLastValidPatch(mcvl->lastValidHWVal());
          _patchEdit->setLastValidBytes(mcvl->lastValidByte2(), mcvl->lastValidByte1(), mcvl->lastValidByte0());
          _patchEdit->setValue(hwVal);
          _patchEdit->blockSignals(false);
        }
        else
        {
          int min = 0;
          int max = 127;
          int bias = 0;
          int initval = 0;
          MusECore::MidiController* mc = mp->midiController(_dnum, chan);
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

          if(_knob)
          {
            const double c_dmin = _knob->minValue();
            const double c_dmax = _knob->maxValue();
            if(c_dmin != min && c_dmax != max)
            {
              _knob->blockSignals(true);
              _knob->setRange(dmin, dmax, 1.0);
              _knob->blockSignals(false);
            }
            else if(c_dmin != min)
            {
              _knob->blockSignals(true);
              _knob->setMinValue(min);
              _knob->blockSignals(false);
            }
            else if(c_dmax != max)
            {
              _knob->blockSignals(true);
              _knob->setMaxValue(max);
              _knob->blockSignals(false);
            }

            if(hwVal == MusECore::CTRL_VAL_UNKNOWN)
            {
              hwVal = mcvl->lastValidHWVal();
              if(hwVal == MusECore::CTRL_VAL_UNKNOWN)
              {
                hwVal = initval;
                if(!_knob->isOff() || hwVal != _knob->value())
                {
                  _knob->blockSignals(true);
                  _knob->setValueState(hwVal, true);
                  _knob->blockSignals(false);
                }
              }
              else
              {
                hwVal -= bias;
                if(!_knob->isOff() || hwVal != _knob->value())
                {
                  _knob->blockSignals(true);
                  _knob->setValueState(hwVal, true);
                  _knob->blockSignals(false);
                }
              }
            }
            else
            {
              hwVal -= bias;
              if(_knob->isOff() || hwVal != _knob->value())
              {
                _knob->blockSignals(true);
                _knob->setValueState(hwVal, false);
                _knob->blockSignals(false);
              }
            }
          }
          else if(_slider)
          {
            const double c_dmin = _slider->minValue();
            const double c_dmax = _slider->maxValue();
            if(c_dmin != min && c_dmax != max)
            {
              _slider->blockSignals(true);
              _slider->setRange(dmin, dmax, 1.0);
              _slider->blockSignals(false);
            }
            else if(c_dmin != min)
            {
              _slider->blockSignals(true);
              _slider->setMinValue(min);
              _slider->blockSignals(false);
            }
            else if(c_dmax != max)
            {
              _slider->blockSignals(true);
              _slider->setMaxValue(max);
              _slider->blockSignals(false);
            }

            if(hwVal == MusECore::CTRL_VAL_UNKNOWN)
            {
              hwVal = mcvl->lastValidHWVal();
              if(hwVal == MusECore::CTRL_VAL_UNKNOWN)
              {
                hwVal = initval;
                if(!_slider->isOff() || hwVal != _slider->value())
                {
                  _slider->blockSignals(true);
                  _slider->setValueState(hwVal, true);
                  _slider->blockSignals(false);
                }
              }
              else
              {
                hwVal -= bias;
                if(!_slider->isOff() || hwVal != _slider->value())
                {
                  _slider->blockSignals(true);
                  _slider->setValueState(hwVal, true);
                  _slider->blockSignals(false);
                }
              }
            }
            else
            {
              hwVal -= bias;
              if(_slider->isOff() || hwVal != _slider->value())
              {
                _slider->blockSignals(true);
                _slider->setValueState(hwVal, false);
                _slider->blockSignals(false);
              }
            }
          }
        }
      }
    }
  }

  inHeartBeat = false;
}

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void CtrlPanel::configChanged()
{ 
  songChanged(SC_CONFIG);

  // Detect when knobs are preferred and rebuild.
  if(_preferKnobs != MusEGlobal::config.preferKnobsVsSliders)
  {
    _preferKnobs = MusEGlobal::config.preferKnobsVsSliders;
    setController();
  }

  if(_patchEdit)
  {
    if(_patchEdit->font() != MusEGlobal::config.fonts[1])
    {
      _patchEdit->setFont(MusEGlobal::config.fonts[1]);
      _patchEdit->setStyleSheet(MusECore::font2StyleSheetFull(MusEGlobal::config.fonts[1]));
    }
    _patchEdit->setMaxAliasedPointSize(MusEGlobal::config.maxAliasedPointSize);
  }

  if(_knob)
  {
    if(_knob->font() != MusEGlobal::config.fonts[1])
    {
      _knob->setFont(MusEGlobal::config.fonts[1]);
      _knob->setStyleSheet(MusECore::font2StyleSheetFull(MusEGlobal::config.fonts[1]));
    }

    // Whether to show values along with labels for certain controls.
    //_knob->setShowValue(MusEGlobal::config.showControlValues);
  }

  if(_slider)
  {
    if(_slider->font() != MusEGlobal::config.fonts[1])
    {
      _slider->setFont(MusEGlobal::config.fonts[1]);
      _slider->setStyleSheet(MusECore::font2StyleSheetFull(MusEGlobal::config.fonts[1]));
    }

    _slider->setMaxAliasedPointSize(MusEGlobal::config.maxAliasedPointSize);
    // Whether to show values along with labels for certain controls.
    //_slider->setShowValue(MusEGlobal::config.showControlValues);
  }

  setControlColor();
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void CtrlPanel::songChanged(MusECore::SongChangedStruct_t /*flags*/)
{
  if(editor && editor->deleting())  // Ignore while while deleting to prevent crash.
    return; 
}

void CtrlPanel::patchCtrlChanged(int val)
{
  ctrlChanged(double(val), false, _dnum, 0);
}

//---------------------------------------------------------
//   ctrlChanged
//---------------------------------------------------------

void CtrlPanel::ctrlChanged(double val, bool off, int /*id*/, int /*scrollMode*/)
    {
      if (inHeartBeat)
            return;
      if(!_track || !_ctrl || _dnum == -1)
          return;
      
      int ival = lrint(val);
      int outport = _track->outPort();
      int chan = _track->outChannel();
      if(chan < 0 || chan >= MusECore::MUSE_MIDI_CHANNELS || outport < 0 || outport >= MusECore::MIDI_PORTS)
          return;
      int cdp = ctrlcanvas->getCurDrumPitch();
      if(_ctrl->isPerNoteController() && cdp >= 0)
      {
        if(_track->type() == MusECore::Track::DRUM)
        {
          // Default to track port if -1 and track channel if -1.
          outport = _track->drummap()[cdp].port;
          if(outport == -1)
            outport = _track->outPort();
          chan = _track->drummap()[cdp].channel;
          if(chan == -1)
            chan = _track->outChannel();
        }
      }

      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[outport];          
      // Shouldn't happen, but...
      if(off || ival < _ctrl->minVal() || ival > _ctrl->maxVal())
        ival = MusECore::CTRL_VAL_UNKNOWN;
      
      if(ival != MusECore::CTRL_VAL_UNKNOWN)
        // Auto bias...
        ival += _ctrl->bias();

      MusECore::MidiPlayEvent ev(MusEGlobal::audio->curFrame(), outport, chan, MusECore::ME_CONTROLLER, _dnum, ival);
      mp->putEvent(ev);
    }

//---------------------------------------------------------
//   setController
//---------------------------------------------------------

void CtrlPanel::setController()
{
  if(!_track || !_ctrl)
  {
    buildPanel();
    inHeartBeat = false;
    return;
  }

  MusECore::MidiPort* mp = &MusEGlobal::midiPorts[_track->outPort()];
  int ch = _track->outChannel();
  int cdp = ctrlcanvas->getCurDrumPitch();
  _dnum = _ctrl->num();
  if(_ctrl->isPerNoteController() && cdp >= 0)
  {
    if(_track->type() == MusECore::Track::DRUM)
    {
      _dnum = (_dnum & ~0xff) | _track->drummap()[cdp].anote;
      int mport = _track->drummap()[cdp].port;
      // Default to track port if -1 and track channel if -1.
      if(mport == -1)
        mport = _track->outPort();
      mp = &MusEGlobal::midiPorts[mport];
      ch = _track->drummap()[cdp].channel;
      if(ch == -1)
        ch = _track->outChannel();
    }
    else if(_track->type() == MusECore::Track::MIDI)
    {
      _dnum = (_dnum & ~0xff) | cdp; //FINDMICHJETZT does that work?
    }
  }

  buildPanel();

  if(_dnum == MusECore::CTRL_VELOCITY)
  {
  }
  else
  {
    MusECore::MidiCtrlValListList* mcvll = mp->controller();

    int mn; int mx; int v;
    if(_dnum == MusECore::CTRL_PROGRAM)
    {
      if(_patchEdit)
      {
        MusECore::ciMidiCtrlValList imcvl = mcvll->find(ch, _dnum);
        if(imcvl != mcvll->end())
        {
          MusECore::MidiCtrlValList* mcvl = imcvl->second;
          int hwVal = mcvl->hwVal();
          // Special for new LCD patch edit control: Need to give both current and last values.
          // Keeping a local last value with the control won't work.
          _patchEdit->blockSignals(true);
          _patchEdit->setLastValidPatch(mcvl->lastValidHWVal());
          _patchEdit->setLastValidBytes(mcvl->lastValidByte2(), mcvl->lastValidByte1(), mcvl->lastValidByte0());
          _patchEdit->setValue(hwVal);
          _patchEdit->blockSignals(false);
        }
      }
      else
      {
        mn = 1;
        mx = 128;
        v = mp->hwCtrlState(ch, _dnum);

        if(_knob)
          _knob->setRange(double(mn), double(mx), 1.0);
        else if(_slider)
          _slider->setRange(double(mn), double(mx), 1.0);
        if(v == MusECore::CTRL_VAL_UNKNOWN || ((v & 0xffffff) == 0xffffff))
        {
          int lastv = mp->lastValidHWCtrlState(ch, _dnum);
          if(lastv == MusECore::CTRL_VAL_UNKNOWN || ((lastv & 0xffffff) == 0xffffff))
          {
            int initv = _ctrl->initVal();
            if(initv == MusECore::CTRL_VAL_UNKNOWN || ((initv & 0xffffff) == 0xffffff))
              v = 1;
            else
              v = (initv + 1) & 0xff;
          }
          else
            v = (lastv + 1) & 0xff;

          if(v > 128)
            v = 128;
        }
        else
        {
          v = (v + 1) & 0xff;
          if(v > 128)
            v = 128;
        }

        if(_knob)
        {
          _knob->setValue(double(v));
        }
        else if(_slider)
        {
          _slider->setValue(double(v));
        }
      }
    }
    else
    {
      mn = _ctrl->minVal();
      mx = _ctrl->maxVal();
      v = mp->hwCtrlState(ch, _dnum);
      if(_knob)
        _knob->setRange(double(mn), double(mx), 1.0);
      else if(_slider)
        _slider->setRange(double(mn), double(mx), 1.0);
      if(v == MusECore::CTRL_VAL_UNKNOWN)
      {
        int lastv = mp->lastValidHWCtrlState(ch, _dnum);
        if(lastv == MusECore::CTRL_VAL_UNKNOWN)
        {
          if(_ctrl->initVal() == MusECore::CTRL_VAL_UNKNOWN)
            v = 0;
          else
            v = _ctrl->initVal();
        }
        else
          v = lastv - _ctrl->bias();
      }
      else
      {
        // Auto bias...
        v -= _ctrl->bias();
      }

      if(_knob)
      {
        _knob->setValue(double(v));
      }
      else if(_slider)
      {
        _slider->setValue(double(v));
      }
    }
  }

  setControlColor();
}

void CtrlPanel::setControlColor()
{
  if(_dnum == -1)
    return;
  
  QColor color = MusEGlobal::config.sliderBackgroundColor;

  switch(_dnum)
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

  if(_patchEdit)
  {
    _patchEdit->setReadoutColor(color);
    // Colours in these sections were not updating with stylesheet colours,
    //  because normally that's only done at creation, and we create these
    //  things once and keep them around waiting to be shown.
    // Anyway, this trick recommended by help. Tested OK.
    // Good demonstration of how to apply a stylesheet after a property has been changed.
    style()->unpolish(_patchEdit);
    style()->polish(_patchEdit);
  }

  if(_knob)
  {
    _knob->setFaceColor(color);
    style()->unpolish(_knob);
    style()->polish(_knob);
  }

  if(_slider)
  {
    _slider->setBorderColor(color);
    _slider->setBarColor(MusEGlobal::config.sliderBarColor);
    style()->unpolish(_slider);
    style()->polish(_slider);
  }
}

//---------------------------------------------------------
//   setHWController
//---------------------------------------------------------

void CtrlPanel::setHWController(MusECore::MidiTrack* t, MusECore::MidiController* ctrl) 
{ 
  inHeartBeat = true;
  
  _track = t; _ctrl = ctrl; 

  setController();

  inHeartBeat = false;
}

//---------------------------------------------------------
//   setHeight
//---------------------------------------------------------

void CtrlPanel::setHeight(int h)
      {
      setFixedHeight(h);
      }

//---------------------------------------------------
// ctrlPopup
//---------------------------------------------------
            
void CtrlPanel::ctrlPopup()
      {
      MusECore::PartList* parts  = editor->parts();
      MusECore::Part* part       = editor->curCanvasPart();
      int curDrumPitch           = ctrlcanvas->getCurDrumPitch();
      
      PopupMenu* pup = new PopupMenu(true);  // true = enable stay open. Don't bother with parent. 
      int est_width = populateMidiCtrlMenu(pup, parts, part, curDrumPitch);
      QPoint ep = mapToGlobal(QPoint(0,0));
      //int newx = ep.x() - ctrlMainPop->width();  // Too much! Width says 640. Maybe because it hasn't been shown yet  .
      int newx = ep.x() - est_width;  
      if(newx < 0)
        newx = 0;
      ep.setX(newx);
      connect(pup, SIGNAL(triggered(QAction*)), SLOT(ctrlPopupTriggered(QAction*)));
      pup->exec(ep);
      delete pup;
      selCtrl->setDown(false);
      }

//---------------------------------------------------------
//   ctrlPopupTriggered
//---------------------------------------------------------

void CtrlPanel::ctrlPopupTriggered(QAction* act)
{
  if(!act || (act->data().toInt() == -1))
    return;
  
  MusECore::Part* part       = editor->curCanvasPart();
  MusECore::MidiTrack* track = (MusECore::MidiTrack*)(part->track());
  int channel      = track->outChannel();
  MusECore::MidiPort* port   = &MusEGlobal::midiPorts[track->outPort()];
  MusECore::MidiCtrlValListList* cll = port->controller();
  const int min = channel << 24;
  const int max = min + 0x1000000;
  const int edit_ins = max + 3;
  const int velo = max + 0x101;
  int rv = act->data().toInt();
  
  if (rv == velo) {    // special case velocity
        emit controllerChanged(MusECore::CTRL_VELOCITY);
        }
  else if (rv == edit_ins) {            // edit instrument
        MusECore::MidiInstrument* instr = port->instrument();
        MusEGlobal::muse->startEditInstrument(instr ? instr->iname() : QString(), EditInstrumentControllers);
        }
  else {                           // Select a control
        MusECore::iMidiCtrlValList i = cll->find(channel, rv);
        if(i == cll->end())
        {
          MusECore::MidiCtrlValList* vl = new MusECore::MidiCtrlValList(rv);
          cll->add(channel, vl);
        }
        int num = rv;
        if(port->drumController(rv))
          num |= 0xff;
        emit controllerChanged(num);
      }
}

//---------------------------------------------------------
//   ctrlRightClicked
//---------------------------------------------------------

void CtrlPanel::ctrlRightClicked(const QPoint& p, int /*id*/)
{
  if(!editor->curCanvasPart() || !_ctrl)
    return;  
    
  int cdp = ctrlcanvas->getCurDrumPitch();
  int ctlnum = _ctrl->num();
  if(_track->isDrumTrack() &&
     _ctrl->isPerNoteController() && cdp >= 0)
    //ctlnum = (ctlnum & ~0xff) | MusEGlobal::drumMap[cdp].enote; DELETETHIS or which of them is correct?
    ctlnum = (ctlnum & ~0xff) | cdp;
  
  MusECore::MidiPart* part = dynamic_cast<MusECore::MidiPart*>(editor->curCanvasPart());
  MusEGlobal::song->execMidiAutomationCtlPopup(0, part, p, ctlnum);
}

//---------------------------------------------------------
//   velPerNoteClicked
//---------------------------------------------------------

void CtrlPanel::velPerNoteClicked()
{
  if(ctrlcanvas && _veloPerNoteButton && _veloPerNoteButton->isChecked() != ctrlcanvas->perNoteVeloMode())
    ctrlcanvas->setPerNoteVeloMode(_veloPerNoteButton->isChecked());
}

//---------------------------------------------------------
//   setVeloPerNoteMode
//---------------------------------------------------------

void CtrlPanel::setVeloPerNoteMode(bool v)
{
  if(_veloPerNoteButton && v != _veloPerNoteButton->isChecked())
    _veloPerNoteButton->setChecked(v);
}

} // namespace MusEGui

