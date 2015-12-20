//=========================================================
//  MusE
//  Linux Music Editor
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  meter_slider.cpp
//  (C) Copyright 2015 Tim E. Real (terminator356 on sourceforge)
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

#include <QVBoxLayout>
#include <QMouseEvent>

#include "compact_patch_edit.h"
#include "compact_slider.h"
#include "elided_label.h"

namespace MusEGui {

CompactPatchEdit::CompactPatchEdit(QWidget *parent, const char *name,
               Qt::Orientation orient, CompactSlider::ScalePos scalePos, QColor fillColor)
            : QFrame(parent)

{
  setObjectName(name);
  
  _id           = -1;
  _currentPatch = 0;
  
  _patchNameLabel = new ElidedLabel(this, Qt::ElideMiddle);
  _patchNameLabel->setObjectName("CompactPatchEditLabel");
  _patchNameLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
  
  _HBank = new CompactSlider(this, "CompactPatchEditHBank", orient, scalePos, tr("Hi"), QString(), QString(), QString(), fillColor);
  _LBank = new CompactSlider(this, "CompactPatchEditLBank", orient, scalePos, tr("Lo"), QString(), QString(), QString(), fillColor);
  _Prog = new CompactSlider(this, "CompactPatchEditProg", orient, scalePos, tr("Prg"), QString(), QString(), QString(), fillColor);

  _HBank->setHasOffMode(true);
  _LBank->setHasOffMode(true);
  _Prog->setHasOffMode(true);
  
  _HBank->setRange(0, 127, 1.0);
  _HBank->setValueDecimals(0);

  _LBank->setRange(0, 127, 1.0);
  _LBank->setValueDecimals(0);

  _Prog->setRange(0, 127, 1.0);
  _Prog->setValueDecimals(0);

  _patchNameLabel->setContentsMargins(0, 0, 0, 0);
  _HBank->setContentsMargins(0, 0, 0, 0);
  _LBank->setContentsMargins(0, 0, 0, 0);
  _Prog->setContentsMargins(0, 0, 0, 0);
  
//   _HBank->setMargins(0, 0);
//   _LBank->setMargins(0, 0);
//   _Prog->setMargins(0, 0);
  _HBank->setMargins(1, 1);
  _LBank->setMargins(1, 1);
  _Prog->setMargins(1, 1);
  
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);
  
  layout->addWidget(_patchNameLabel);
  layout->addWidget(_HBank);
  layout->addWidget(_LBank);
  layout->addWidget(_Prog);
  
  //connect(_patchNameLabel, SIGNAL(valueStateChanged(double,bool,int)), SLOT(HBankValueStateChanged(double,bool,int)));
  
  connect(_HBank, SIGNAL(valueStateChanged(double,bool,int)), 
                  SLOT(HBankValueStateChanged(double,bool,int)));
  connect(_LBank, SIGNAL(valueStateChanged(double,bool,int)), 
                  SLOT(LBankValueStateChanged(double,bool,int)));
  connect(_Prog,  SIGNAL(valueStateChanged(double,bool,int)), 
                  SLOT(ProgValueStateChanged(double,bool,int)));
  
  connect(_HBank, SIGNAL(sliderDoubleClicked(QPoint,int,Qt::MouseButtons,Qt::KeyboardModifiers)), 
                  SLOT(HBankDoubleClicked(QPoint,int,Qt::MouseButtons,Qt::KeyboardModifiers)));
  connect(_LBank, SIGNAL(sliderDoubleClicked(QPoint,int,Qt::MouseButtons,Qt::KeyboardModifiers)), 
                  SLOT(LBankDoubleClicked(QPoint,int,Qt::MouseButtons,Qt::KeyboardModifiers)));
  connect(_Prog,  SIGNAL(sliderDoubleClicked(QPoint,int,Qt::MouseButtons,Qt::KeyboardModifiers)), 
                  SLOT(ProgDoubleClicked(QPoint,int,Qt::MouseButtons,Qt::KeyboardModifiers)));

  connect(_HBank, SIGNAL(sliderRightClicked(QPoint,int)), 
                  SLOT(anySliderRightClicked(QPoint,int)));
  connect(_LBank, SIGNAL(sliderRightClicked(QPoint,int)), 
                  SLOT(anySliderRightClicked(QPoint,int)));
  connect(_Prog,  SIGNAL(sliderRightClicked(QPoint,int)), 
                  SLOT(anySliderRightClicked(QPoint,int)));
  
//     void sliderPressed(int id);
//     void sliderReleased(int id);
//     void sliderMoved(double value, int id);
//     void sliderMoved(double value, int id, bool shift);
//     void sliderRightClicked(const QPoint &p, int id);
// 
//     
//     void _HBankChanged();
//     void _HBankDoubleCLicked();
//     void _LBankChanged();
//     void _LBankDoubleCLicked();
//     void _ProgramChanged();
//     void _ProgramDoubleClicked();
    
}

// Static.      
QSize CompactPatchEdit::getMinimumSizeHint(const QFontMetrics& fm,
                                        Qt::Orientation orient,
                                        CompactSlider::ScalePos scalePos,
                                        int xMargin,
                                        int yMargin)
{
  const QSize ctrl_sz = CompactSlider::getMinimumSizeHint(fm, orient, scalePos, xMargin, yMargin);
  
  // HACK Try to find the size of a label
//   QLabel* dummyLabel = new QLabel("WWW", this);
//   dummyLabel->setMargin(yMargin);
//   const QSize lbl_sz = dummyLabel->sizeHint();
  const int lbl_h = fm.height() + 2 * yMargin;
  
  //const int h = 3 * ctrl_sz.height() + lbl_sz.height();
  const int h = 3 * ctrl_sz.height() + lbl_h;
  switch(orient) {
        case Qt::Vertical:
              return QSize(16, h);
              break;
        case Qt::Horizontal:
              return QSize(16, h);
              break;
        }
  return QSize(10, 10);
}
      
bool CompactPatchEdit::isOff() const
{
  return _Prog->isOff();
}

void CompactPatchEdit::setOff(bool v)
{
  _HBank->setOff(v);
  _LBank->setOff(v);
  _Prog->setOff(v);
}

double CompactPatchEdit::value() const
{
//   const int hb = (_currentPatch >> 16) & 0xff;
//   const int lb = (_currentPatch >> 8) & 0xff;
//   const int pr = _currentPatch & 0xff;
  return _currentPatch;
}


void CompactPatchEdit::setValueState(double v, bool off)
{
  _currentPatch = int(v);
  const int hb = (_currentPatch >> 16) & 0xff;
  const int lb = (_currentPatch >> 8) & 0xff;
  const int pr = _currentPatch & 0xff;

  if(pr == 0xff)
    off = true;
  
  if(hb == 0xff)
    _HBank->setOff(true);
  else
    _HBank->setValueState(hb, off);
    
  if(lb == 0xff)
    _LBank->setOff(true);
  else
    _LBank->setValueState(lb, off);
  
  if(pr == 0xff)
    _Prog->setOff(true);
  else
    _Prog->setValueState(pr, off);
}

// void CompactPatchEdit::updateValueState()
// {
//   int hb = (_currentPatch >> 16) & 0xff;
//   int lb = (_currentPatch >> 8) & 0xff;
//   int pr = _currentPatch & 0xff;
//   
//   if(hb == 0xff)
//     _HBank->setOff(true);
//   
//   if(lb == 0xff)
//     _LBank->setOff(true);
//   
//   if(pr == 0xff)
//     _Prog->setOff(true);
//   
//   _HBank->setValueState(double(v), off);
//   
// }

// int CompactPatchEdit::setCurrentPatch(int patch)
// { 
//   if(_currentPatch == patch)
//     return;
//   
//   _currentPatch = patch;
//   
// }

QString CompactPatchEdit::patchName() const 
{ 
  return _patchNameLabel->text(); 
}

void CompactPatchEdit::setPatchName(const QString& patchName) 
{ 
  _patchNameLabel->setText(patchName); 
}

void CompactPatchEdit::mousePressEvent(QMouseEvent* e)
{
  if(_patchNameLabel->rect().contains(e->pos()))
  {
    if(e->buttons() == Qt::LeftButton)
    {
      e->accept();
      emit patchNameClicked();
      return;
    }
    else if(e->buttons() == Qt::RightButton)
    {
      e->accept();
      emit patchNameRightClicked();
      return;
    }
  }
  e->ignore();
  QFrame::mousePressEvent(e);
}

void CompactPatchEdit::HBankValueStateChanged(double val, bool off, int /*id*/)
{
  const int hb = int(val) & 0xff;
  _currentPatch = (_currentPatch & 0xffff) | (off ? 0xff0000 : (hb << 16));
  
  if(isOff())
  {
    _LBank->blockSignals(true);
    _LBank->setOff(false);
    _LBank->blockSignals(false);
    
    _Prog->blockSignals(true);
    _Prog->setOff(false);
    _Prog->blockSignals(false);
  }
  
  emit valueStateChanged(_currentPatch, isOff(), _id);
}

void CompactPatchEdit::LBankValueStateChanged(double val, bool off, int /*id*/)
{
  const int lb = int(val) & 0xff;
  _currentPatch = (_currentPatch & 0xff00ff) | (off ? 0xff00 : (lb << 8));

  if(isOff())
  {
    _HBank->blockSignals(true);
    _HBank->setOff(false);
    _HBank->blockSignals(false);
    
    _Prog->blockSignals(true);
    _Prog->setOff(false);
    _Prog->blockSignals(false);
  }
  
  emit valueStateChanged(_currentPatch, isOff(), _id);
}

void CompactPatchEdit::ProgValueStateChanged(double val, bool off, int /*id*/)
{
  const int pr = int(val) & 0xff;
  _currentPatch = (_currentPatch & 0xffff00) | pr;
  
  if(off)
  {
    _HBank->blockSignals(true);
    _HBank->setOff(off);
    _HBank->blockSignals(false);
    
    _LBank->blockSignals(true);
    _LBank->setOff(off);
    _LBank->blockSignals(false);
  }
  
  emit valueStateChanged(_currentPatch, isOff(), _id);
}

void CompactPatchEdit::HBankDoubleClicked(QPoint /*p*/, int /*id*/, Qt::MouseButtons buttons, Qt::KeyboardModifiers keys)
{
  if(buttons == Qt::LeftButton && keys == (Qt::ControlModifier | Qt::ShiftModifier))
  {
    
  }
}

void CompactPatchEdit::LBankDoubleClicked(QPoint /*p*/, int /*id*/, Qt::MouseButtons buttons, Qt::KeyboardModifiers keys)
{
  if(buttons == Qt::LeftButton && keys == (Qt::ControlModifier | Qt::ShiftModifier))
  {
    
  }
}

void CompactPatchEdit::ProgDoubleClicked(QPoint /*p*/, int /*id*/, Qt::MouseButtons buttons, Qt::KeyboardModifiers keys)
{
  if(buttons == Qt::LeftButton && keys == (Qt::ControlModifier | Qt::ShiftModifier))
  {
    
  }
}

void CompactPatchEdit::anySliderRightClicked(const QPoint &p, int /*id*/)
{
  emit sliderRightClicked(p, _id);
}


/*

void CompactPatchEdit::HBankChanged()
      {
      if(!selected)
        return;
      MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
      int channel = track->outChannel();
      int port    = track->outPort();
      int hbank   = iHBank->value();
      int lbank   = iLBank->value();
      int prog    = iProgram->value();

      if (hbank > 0 && hbank < 129)
            hbank -= 1;
      else
            hbank = 0xff;
      if (lbank > 0 && lbank < 129)
            lbank -= 1;
      else
            lbank = 0xff;
      if (prog > 0 && prog < 129)
            prog -= 1;
      else
            prog = 0xff;

      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
      if(prog == 0xff && hbank == 0xff && lbank == 0xff)
      {
        program = MusECore::CTRL_VAL_UNKNOWN;
        ++_blockHeartbeatCount;
        if(mp->hwCtrlState(channel, MusECore::CTRL_PROGRAM) != MusECore::CTRL_VAL_UNKNOWN)
          MusEGlobal::audio->msgSetHwCtrlState(mp, channel, MusECore::CTRL_PROGRAM, MusECore::CTRL_VAL_UNKNOWN);
        --_blockHeartbeatCount;
        return;  
      }
      
      ++_blockHeartbeatCount;
      int np = mp->hwCtrlState(channel, MusECore::CTRL_PROGRAM);
      if(np == MusECore::CTRL_VAL_UNKNOWN)
      {
        np = mp->lastValidHWCtrlState(channel, MusECore::CTRL_PROGRAM);
        if(np != MusECore::CTRL_VAL_UNKNOWN)
        {
          lbank = (np & 0xff00) >> 8; 
          prog = np & 0xff;
          if(prog == 0xff)
            prog = 0;
          int ilbnk = lbank;
          int iprog = prog;
          if(ilbnk == 0xff)
            ilbnk = -1;
          ++ilbnk;
          ++iprog;
          iLBank->blockSignals(true);
          iProgram->blockSignals(true);
          iLBank->setValue(ilbnk);
          iProgram->setValue(iprog);
          iLBank->blockSignals(false);
          iProgram->blockSignals(false);
        }
      }
      
      if(prog == 0xff && (hbank != 0xff || lbank != 0xff))
      {
        prog = 0;
        iProgram->blockSignals(true);
        iProgram->setValue(1);
        iProgram->blockSignals(false);
      }  
      program = (hbank << 16) + (lbank << 8) + prog;
      MusECore::MidiPlayEvent ev(0, port, channel, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, program);
      MusEGlobal::audio->msgPlayMidiEvent(&ev);
      
      MusECore::MidiInstrument* instr = mp->instrument();
      iPatch->setText(instr->getPatchName(channel, program, track->isDrumTrack()));
//      updateTrackInfo();
      
      --_blockHeartbeatCount;
      }

void CompactPatchEdit::LBankChanged()
      {
      if(!selected)
        return;
      MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
      int channel = track->outChannel();
      int port    = track->outPort();
      int hbank   = iHBank->value();
      int lbank   = iLBank->value();
      int prog    = iProgram->value();

      if (hbank > 0 && hbank < 129)
            hbank -= 1;
      else
            hbank = 0xff;
      if (lbank > 0 && lbank < 129)
            lbank -= 1;
      else
            lbank = 0xff;
      if (prog > 0 && prog < 129)
            prog -= 1;
      else
            prog = 0xff;

      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];
      if(prog == 0xff && hbank == 0xff && lbank == 0xff)
      {
        program = MusECore::CTRL_VAL_UNKNOWN;
        ++_blockHeartbeatCount;
        if(mp->hwCtrlState(channel, MusECore::CTRL_PROGRAM) != MusECore::CTRL_VAL_UNKNOWN)
          MusEGlobal::audio->msgSetHwCtrlState(mp, channel, MusECore::CTRL_PROGRAM, MusECore::CTRL_VAL_UNKNOWN);
        --_blockHeartbeatCount;
        return;  
      }
      
      ++_blockHeartbeatCount;
      int np = mp->hwCtrlState(channel, MusECore::CTRL_PROGRAM);
      if(np == MusECore::CTRL_VAL_UNKNOWN)
      {
        np = mp->lastValidHWCtrlState(channel, MusECore::CTRL_PROGRAM);
        if(np != MusECore::CTRL_VAL_UNKNOWN)
        {
          hbank = (np & 0xff0000) >> 16; 
          prog = np & 0xff;
          if(prog == 0xff)
            prog = 0;
          int ihbnk = hbank;
          int iprog = prog;
          if(ihbnk == 0xff)
            ihbnk = -1;
          ++ihbnk;
          ++iprog;
          iHBank->blockSignals(true);
          iProgram->blockSignals(true);
          iHBank->setValue(ihbnk);
          iProgram->setValue(iprog);
          iHBank->blockSignals(false);
          iProgram->blockSignals(false);
        }
      }
      
      if(prog == 0xff && (hbank != 0xff || lbank != 0xff))
      {
        prog = 0;
        iProgram->blockSignals(true);
        iProgram->setValue(1);
        iProgram->blockSignals(false);
      }  
      program = (hbank << 16) + (lbank << 8) + prog;
      MusECore::MidiPlayEvent ev(0, port, channel, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, program);
      MusEGlobal::audio->msgPlayMidiEvent(&ev);
      
      MusECore::MidiInstrument* instr = mp->instrument();
      iPatch->setText(instr->getPatchName(channel, program, track->isDrumTrack()));
//      updateTrackInfo();
      
      --_blockHeartbeatCount;
      }

void CompactPatchEdit::ProgramChanged()
      {
      if(!selected)
        return;
      MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
      int channel = track->outChannel();
      int port    = track->outPort();
      int hbank   = iHBank->value();
      int lbank   = iLBank->value();
      int prog    = iProgram->value();

      if (hbank > 0 && hbank < 129)
            hbank -= 1;
      else
            hbank = 0xff;
      if (lbank > 0 && lbank < 129)
            lbank -= 1;
      else
            lbank = 0xff;
      if (prog > 0 && prog < 129)
            prog -= 1;
      else
            prog = 0xff;

      MusECore::MidiPort *mp = &MusEGlobal::midiPorts[port];
      if(prog == 0xff)
      {
        ++_blockHeartbeatCount;
        program = MusECore::CTRL_VAL_UNKNOWN;
        iHBank->blockSignals(true);
        iLBank->blockSignals(true);
        iHBank->setValue(0);
        iLBank->setValue(0);
        iHBank->blockSignals(false);
        iLBank->blockSignals(false);
        
        if(mp->hwCtrlState(channel, MusECore::CTRL_PROGRAM) != MusECore::CTRL_VAL_UNKNOWN)
          MusEGlobal::audio->msgSetHwCtrlState(mp, channel, MusECore::CTRL_PROGRAM, MusECore::CTRL_VAL_UNKNOWN);
        --_blockHeartbeatCount;
        return;
      }
      else
      {
        ++_blockHeartbeatCount;
        int np = mp->hwCtrlState(channel, MusECore::CTRL_PROGRAM);
        if(np == MusECore::CTRL_VAL_UNKNOWN)
        {
          np = mp->lastValidHWCtrlState(channel, MusECore::CTRL_PROGRAM);
          if(np != MusECore::CTRL_VAL_UNKNOWN)
          {
            hbank = (np & 0xff0000) >> 16;
            lbank = (np & 0xff00) >> 8; 
            int ihbnk = hbank;
            int ilbnk = lbank;
            if(ihbnk == 0xff)
              ihbnk = -1;
            if(ilbnk == 0xff)
              ilbnk = -1;
            ++ihbnk;
            ++ilbnk;
            iHBank->blockSignals(true);
            iLBank->blockSignals(true);
            iHBank->setValue(ihbnk);
            iLBank->setValue(ilbnk);
            iHBank->blockSignals(false);
            iLBank->blockSignals(false);
          }
        }
        program = (hbank << 16) + (lbank << 8) + prog;
        MusECore::MidiPlayEvent ev(0, port, channel, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, program);
        MusEGlobal::audio->msgPlayMidiEvent(&ev);
        
        MusECore::MidiInstrument* instr = mp->instrument();
        iPatch->setText(instr->getPatchName(channel, program, track->isDrumTrack()));
        
        --_blockHeartbeatCount;
      }
        
//      updateTrackInfo();
      }

void CompactPatchEdit::HBankDoubleCLicked()
{
  if(!selected)
    return;
  MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
  int port = track->outPort();
  int chan = track->outChannel();
  MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];  
  MusECore::MidiController* mctl = mp->midiController(MusECore::CTRL_PROGRAM);
  
  if(!track || !mctl)
      return;
  
  int lastv = mp->lastValidHWCtrlState(chan, MusECore::CTRL_PROGRAM);
  int curv = mp->hwCtrlState(chan, MusECore::CTRL_PROGRAM);
  
  if(curv == MusECore::CTRL_VAL_UNKNOWN)
  {
    // If no value has ever been set yet, use the current knob value 
    //  (or the controller's initial value?) to 'turn on' the controller.
    if(lastv == MusECore::CTRL_VAL_UNKNOWN)
    {
      int kiv = mctl->initVal();
      if(kiv == MusECore::CTRL_VAL_UNKNOWN)
        kiv = 0;

      ++_blockHeartbeatCount;
      MusECore::MidiPlayEvent ev(0, port, chan, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, kiv);
      MusEGlobal::audio->msgPlayMidiEvent(&ev);
      --_blockHeartbeatCount;
    }
    else
    {
      // TODO
//       int hbank = (lastv >> 16) & 0xff;
//       if(hbank == 0xff)
//         lastv &= 0xffff;
//       else
//         lastv |= 0xff0000;
      
      ++_blockHeartbeatCount;
      MusECore::MidiPlayEvent ev(0, port, chan, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, lastv);
      MusEGlobal::audio->msgPlayMidiEvent(&ev);
      --_blockHeartbeatCount;
    }
  }  
  else
  {      
    // TODO
//     int lasthb = 0xff;
//     if(lastv != MusECore::CTRL_VAL_UNKNOWN)
//       lasthb = (lastv >> 16) & 0xff;
//     
//     int hbank = (curv >> 16) & 0xff;
//     if(hbank == 0xff)
//     {
//       curv &= 0xffff;
//       if(lasthb != 0xff)
//         curv |= lasthb;
//     }
//     else
//       curv |= 0xff0000;
    
    if(mp->hwCtrlState(chan, MusECore::CTRL_PROGRAM) != MusECore::CTRL_VAL_UNKNOWN)
    {
      ++_blockHeartbeatCount;
      MusEGlobal::audio->msgSetHwCtrlState(mp, chan, MusECore::CTRL_PROGRAM, MusECore::CTRL_VAL_UNKNOWN);
      --_blockHeartbeatCount;
    }
  }
  
  MusEGlobal::song->update(SC_MIDI_CONTROLLER);
}

void CompactPatchEdit::LBankDoubleCLicked()
{
  if(!selected)
    return;
  MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
  int port = track->outPort();
  int chan = track->outChannel();
  MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];  
  MusECore::MidiController* mctl = mp->midiController(MusECore::CTRL_PROGRAM);
  
  if(!track || !mctl)
      return;
  
  int lastv = mp->lastValidHWCtrlState(chan, MusECore::CTRL_PROGRAM);
  int curv = mp->hwCtrlState(chan, MusECore::CTRL_PROGRAM);
  
  if(curv == MusECore::CTRL_VAL_UNKNOWN)
  {
    // If no value has ever been set yet, use the current knob value 
    //  (or the controller's initial value?) to 'turn on' the controller.
    if(lastv == MusECore::CTRL_VAL_UNKNOWN)
    {
      int kiv = mctl->initVal();
      if(kiv == MusECore::CTRL_VAL_UNKNOWN)
        kiv = 0xff0000;
      
      //MusECore::MidiPlayEvent ev(0, port, chan, MusECore::ME_CONTROLLER, num, kiv);
      MusECore::MidiPlayEvent ev(0, port, chan, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, kiv);
      MusEGlobal::audio->msgPlayMidiEvent(&ev);
    }
    else
    {
// TODO
//       int lbank = (lastv >> 8) & 0xff;
//       if(lbank == 0xff)
//         lastv &= 0xff00ff;
//       else
//       {
//         lastv |= 0xffff00;
//         //int hbank = (lastv >> 16) & 0xff;
//         //if(hbank != 0xff)
//         //  lastv |= 0xff0000;
//       }
      
      MusECore::MidiPlayEvent ev(0, port, chan, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, lastv);
      MusEGlobal::audio->msgPlayMidiEvent(&ev);
    }
  }  
  else
  {
// TODO
//     //int lasthb = 0xff;
//     int lastlb = 0xff;
//     if(lastv != MusECore::CTRL_VAL_UNKNOWN)
//     {
//       //lasthb = (lastv >> 16) & 0xff;
//       lastlb = (lastv >> 8) & 0xff;
//     }
//     
//     int lbank = (curv >> 8) & 0xff;
//     if(lbank == 0xff)
//     {
//       curv &= 0xff00ff;
//       if(lastlb != 0xff)
//         curv |= lastlb;
//     }
//     else
//     {
//       curv |= 0xffff00;
//       //int hbank = (curv >> 16) & 0xff;
//       //if(hbank != 0xff)
//       //  curv |= 0xff0000;
//     }
      
    if(mp->hwCtrlState(chan, MusECore::CTRL_PROGRAM) != MusECore::CTRL_VAL_UNKNOWN)
      MusEGlobal::audio->msgSetHwCtrlState(mp, chan, MusECore::CTRL_PROGRAM, MusECore::CTRL_VAL_UNKNOWN);
//     MusECore::MidiPlayEvent ev(0, port, chan, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, curv);
//     MusEGlobal::audio->msgPlayMidiEvent(&ev);
  }
  
  MusEGlobal::song->update(SC_MIDI_CONTROLLER);
}

void CompactPatchEdit::ProgramDoubleClicked()
{
  if(!selected)
    return;
  MusECore::MidiTrack* track = (MusECore::MidiTrack*)selected;
  int port = track->outPort();
  int chan = track->outChannel();
  MusECore::MidiPort* mp = &MusEGlobal::midiPorts[port];  
  MusECore::MidiController* mctl = mp->midiController(MusECore::CTRL_PROGRAM);
  
  if(!track || !mctl)
      return;
  
  int lastv = mp->lastValidHWCtrlState(chan, MusECore::CTRL_PROGRAM);
  int curv = mp->hwCtrlState(chan, MusECore::CTRL_PROGRAM);
  
  if(curv == MusECore::CTRL_VAL_UNKNOWN)
  {
    // If no value has ever been set yet, use the current knob value 
    //  (or the controller's initial value?) to 'turn on' the controller.
    if(lastv == MusECore::CTRL_VAL_UNKNOWN)
    {
      int kiv = mctl->initVal();
      if(kiv == MusECore::CTRL_VAL_UNKNOWN)
        kiv = 0xffff00;
      
      MusECore::MidiPlayEvent ev(0, port, chan, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, kiv);
      MusEGlobal::audio->msgPlayMidiEvent(&ev);
    }
    else
    {
      MusECore::MidiPlayEvent ev(0, port, chan, MusECore::ME_CONTROLLER, MusECore::CTRL_PROGRAM, lastv);
      MusEGlobal::audio->msgPlayMidiEvent(&ev);
    }
  }  
  else
  {
    if(mp->hwCtrlState(chan, MusECore::CTRL_PROGRAM) != MusECore::CTRL_VAL_UNKNOWN)
      MusEGlobal::audio->msgSetHwCtrlState(mp, chan, MusECore::CTRL_PROGRAM, MusECore::CTRL_VAL_UNKNOWN);
  }
  
  MusEGlobal::song->update(SC_MIDI_CONTROLLER);
}
*/



} // namespace MusEGui
