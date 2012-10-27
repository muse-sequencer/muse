//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ctrlpanel.cpp,v 1.10.2.9 2009/06/14 05:24:45 terminator356 Exp $
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
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
#include <list>

#include "ctrlpanel.h"
#include "ctrlcanvas.h"

#include <QAction>
#include <QPushButton>
#include <QSizePolicy>
#include <QHBoxLayout>
#include <QTimer>
#include <QVBoxLayout>

#include <math.h>

#include "app.h"
#include "globals.h"
#include "midictrl.h"
#include "instruments/minstrument.h"
#include "midiport.h"
#include "mididev.h"
#include "xml.h"
#include "icons.h"
#include "event.h"
#include "midieditor.h"
#include "track.h"
#include "part.h"
#include "midiedit/drummap.h"
#include "gconfig.h"
#include "song.h"
#include "knob.h"
#include "doublelabel.h"
#include "midi.h"
#include "audio.h"
#include "menutitleitem.h"
#include "popupmenu.h"
#include "helper.h"
#include "pixmap_button.h"

namespace MusEGui {

//---------------------------------------------------------
//   CtrlPanel
//---------------------------------------------------------

CtrlPanel::CtrlPanel(QWidget* parent, MidiEditor* e, CtrlCanvas* c, const char* name)
   : QWidget(parent)
      {
      setObjectName(name);
      inHeartBeat = true;
      editor = e;
      ctrlcanvas = c;
      setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
      QVBoxLayout* vbox = new QVBoxLayout;
      QHBoxLayout* bbox = new QHBoxLayout;
      bbox->setSpacing (0);
      vbox->addLayout(bbox);
      vbox->addStretch();
      QHBoxLayout* kbox = new QHBoxLayout;
      QHBoxLayout* dbox = new QHBoxLayout;
      vbox->addLayout(kbox);
      vbox->addLayout(dbox);
      vbox->addStretch();
      vbox->setContentsMargins(0, 0, 0, 0);
      bbox->setContentsMargins(0, 0, 0, 0);
      kbox->setContentsMargins(0, 0, 0, 0);
      dbox->setContentsMargins(0, 0, 0, 0);

      selCtrl = new QPushButton(tr("S"), this);
      selCtrl->setFocusPolicy(Qt::NoFocus);
      selCtrl->setFont(MusEGlobal::config.fonts[3]);
      selCtrl->setFixedHeight(20);
      selCtrl->setSizePolicy(
         QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
      selCtrl->setToolTip(tr("select controller"));
      
      // destroy button
      QPushButton* destroy = new QPushButton(tr("X"), this);
      destroy->setFocusPolicy(Qt::NoFocus);
      destroy->setFont(MusEGlobal::config.fonts[3]);
      destroy->setFixedHeight(20);
      destroy->setSizePolicy(
         QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
      destroy->setToolTip(tr("remove panel"));
      // Cursor Position
      connect(selCtrl, SIGNAL(clicked()), SLOT(ctrlPopup()));
      connect(destroy, SIGNAL(clicked()), SIGNAL(destroyPanel()));
      
      _track = 0;
      _ctrl = 0;
      _val = MusECore::CTRL_VAL_UNKNOWN;
      _dnum = -1;
      
      _knob = new Knob(this);
      _knob->setFixedWidth(25);
      _knob->setFixedHeight(25);
      _knob->setToolTip(tr("manual adjust"));
      _knob->setRange(0.0, 127.0, 1.0);
      _knob->setValue(0.0);
      _knob->setEnabled(false);
      _knob->hide();
      _knob->setAltFaceColor(Qt::red);
      
      _dl = new DoubleLabel(-1.0, 0.0, +127.0, this);
      _dl->setPrecision(0);
      _dl->setToolTip(tr("ctrl-double-click on/off"));
      _dl->setSpecialText(tr("off"));
      _dl->setFont(MusEGlobal::config.fonts[1]);
      _dl->setBackgroundRole(QPalette::Mid);
      _dl->setFrame(true);
      _dl->setFixedWidth(36);
      _dl->setFixedHeight(15);
      _dl->setEnabled(false);
      _dl->hide();
      
      connect(_knob, SIGNAL(sliderMoved(double,int)), SLOT(ctrlChanged(double)));
      connect(_knob, SIGNAL(sliderRightClicked(const QPoint&, int)), SLOT(ctrlRightClicked(const QPoint&, int)));
      connect(_dl, SIGNAL(valueChanged(double,int)), SLOT(ctrlChanged(double)));
      connect(_dl, SIGNAL(ctrlDoubleClicked(int)), SLOT(labelDoubleClicked()));
      
      _veloPerNoteButton = new PixmapButton(veloPerNote_OnIcon, veloPerNote_OffIcon, 2, this);  // Margin = 2
      _veloPerNoteButton->setFocusPolicy(Qt::NoFocus);
      _veloPerNoteButton->setCheckable(true);
      _veloPerNoteButton->setToolTip(tr("all/per-note velocity mode"));
      _veloPerNoteButton->setEnabled(false);
      _veloPerNoteButton->hide();
      connect(_veloPerNoteButton, SIGNAL(clicked()), SLOT(velPerNoteClicked()));
      
      bbox->addStretch();
      bbox->addWidget(selCtrl);
      bbox->addWidget(destroy);
      bbox->addStretch();
      kbox->addStretch();
      kbox->addWidget(_knob);
      kbox->addWidget(_veloPerNoteButton);
      kbox->addStretch();
      dbox->addStretch();
      dbox->addWidget(_dl);
      dbox->addStretch();
      
      connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedFlags_t)), SLOT(songChanged(MusECore::SongChangedFlags_t)));
      connect(MusEGlobal::muse, SIGNAL(configChanged()), SLOT(configChanged()));
      connect(MusEGlobal::heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
      inHeartBeat = false;
      setLayout(vbox);
      }
//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void CtrlPanel::heartBeat()
{
  if(editor->deleting())  // Ignore while while deleting to prevent crash.
    return;
  
  inHeartBeat = true;
  
  if(_track && _ctrl && _dnum != -1)
  {
    if(_dnum != MusECore::CTRL_VELOCITY)
    {
      int outport;
      int chan;
      int cdp = ctrlcanvas->getCurDrumPitch();
      if(_track->type() == MusECore::Track::DRUM && ((_ctrl->num() & 0xff) == 0xff) && cdp != -1)
      {
        outport = MusEGlobal::drumMap[cdp].port;
        chan = MusEGlobal::drumMap[cdp].channel;
      }  
      else  
      {
        outport = _track->outPort();
        chan = _track->outChannel();
      }
      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[outport];          
      
      int v = mp->hwCtrlState(chan, _dnum);
      if(v == MusECore::CTRL_VAL_UNKNOWN)
      {
        // MusEGui::DoubleLabel ignores the value if already set...
        _dl->setValue(_dl->off() - 1.0);
        _val = MusECore::CTRL_VAL_UNKNOWN;
        v = mp->lastValidHWCtrlState(chan, _dnum);
        if(v != MusECore::CTRL_VAL_UNKNOWN && ((_dnum != MusECore::CTRL_PROGRAM) || ((v & 0xff) != 0xff) ))
        {
          if(_dnum == MusECore::CTRL_PROGRAM)
            v = (v & 0x7f) + 1;
          else  
            // Auto bias...
            v -= _ctrl->bias();
          if (double(v) != _knob->value())
            _knob->setValue(double(v));
        }
      }
      else if(v != _val)
      {
        _val = v;
        if(v == MusECore::CTRL_VAL_UNKNOWN || ((_dnum == MusECore::CTRL_PROGRAM) && ((v & 0xff) == 0xff) ))
        {
          _dl->setValue(_dl->off() - 1.0);
        }
        else
        {
          if(_dnum == MusECore::CTRL_PROGRAM)
            v = (v & 0x7f) + 1;
          else  
            // Auto bias...
            v -= _ctrl->bias();
          
          _knob->setValue(double(v));
          _dl->setValue(double(v));
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
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void CtrlPanel::songChanged(MusECore::SongChangedFlags_t type)
{
  if(editor->deleting())  // Ignore while while deleting to prevent crash.
    return; 
  
  // Is it simply a midi controller value adjustment? Forget it.
  if(type == SC_MIDI_CONTROLLER)
    return;
            
  if(type & SC_CONFIG)
  {
    if(_veloPerNoteButton->isChecked() != MusEGlobal::config.velocityPerNote)
      _veloPerNoteButton->setChecked(MusEGlobal::config.velocityPerNote);  
  }
}

//---------------------------------------------------------
//   labelDoubleClicked
//---------------------------------------------------------

void CtrlPanel::labelDoubleClicked()
{
  if(!_track || !_ctrl || _dnum == -1)
      return;
  
  int outport;
  int chan;
  int cdp = ctrlcanvas->getCurDrumPitch();
  if(_track->type() == MusECore::Track::DRUM && ((_ctrl->num() & 0xff) == 0xff) && cdp != -1)
  {
    outport = MusEGlobal::drumMap[cdp].port;
    chan = MusEGlobal::drumMap[cdp].channel;
  }  
  else  
  {
    outport = _track->outPort();
    chan = _track->outChannel();
  }
  MusECore::MidiPort* mp = &MusEGlobal::midiPorts[outport];          
  int lastv = mp->lastValidHWCtrlState(chan, _dnum);
  
  int curv = mp->hwCtrlState(chan, _dnum);
  
//   if(_dnum == MusECore::CTRL_AFTERTOUCH)
//   {
//     // Auto bias...
//     ival += _ctrl->bias();
//     MusECore::MidiPlayEvent ev(0, outport, chan, MusECore::ME_AFTERTOUCH, ival, 0);
//     MusEGlobal::audio->msgPlayMidiEvent(&ev);
//   }
//   else 
  if(_dnum == MusECore::CTRL_PROGRAM)
  {
    if(curv == MusECore::CTRL_VAL_UNKNOWN || ((curv & 0xffffff) == 0xffffff))
    {
      // If no value has ever been set yet, use the current knob value 
      //  (or the controller's initial value?) to 'turn on' the controller.
      if(lastv == MusECore::CTRL_VAL_UNKNOWN || ((lastv & 0xffffff) == 0xffffff))
      {
        int kiv = lrint(_knob->value());
        --kiv;
        kiv &= 0x7f;
        kiv |= 0xffff00;
        MusECore::MidiPlayEvent ev(0, outport, chan, MusECore::ME_CONTROLLER, _dnum, kiv);
        MusEGlobal::audio->msgPlayMidiEvent(&ev);
      }
      else
      {
        MusECore::MidiPlayEvent ev(0, outport, chan, MusECore::ME_CONTROLLER, _dnum, lastv);
        MusEGlobal::audio->msgPlayMidiEvent(&ev);
      }
    }
    else
    {
      //if((curv & 0xffff00) == 0xffff00) DELETETHIS?
      //{
        ////if(mp->hwCtrlState(chan, _dnum) != MusECore::CTRL_VAL_UNKNOWN)
          MusEGlobal::audio->msgSetHwCtrlState(mp, chan, _dnum, MusECore::CTRL_VAL_UNKNOWN);
      //}
      //else
      //{
      //  MusECore::MidiPlayEvent ev(MusEGlobal::song->cpos(), outport, chan, MusECore::ME_CONTROLLER, _dnum, (curv & 0xffff00) | 0xff);
      //  MusEGlobal::audio->msgPlayMidiEvent(&ev);
      //}
    }  
  }
  else
  {
    if(curv == MusECore::CTRL_VAL_UNKNOWN)
    {
      // If no value has ever been set yet, use the current knob value 
      //  (or the controller's initial value?) to 'turn on' the controller.
      if(lastv == MusECore::CTRL_VAL_UNKNOWN)
      {
        int kiv = lrint(_knob->value());
        if(kiv < _ctrl->minVal())
          kiv = _ctrl->minVal();
        if(kiv > _ctrl->maxVal())
          kiv = _ctrl->maxVal();
        kiv += _ctrl->bias();
        MusECore::MidiPlayEvent ev(0, outport, chan, MusECore::ME_CONTROLLER, _dnum, kiv);
        MusEGlobal::audio->msgPlayMidiEvent(&ev);
      }
      else
      {
        MusECore::MidiPlayEvent ev(0, outport, chan, MusECore::ME_CONTROLLER, _dnum, lastv);
        MusEGlobal::audio->msgPlayMidiEvent(&ev);
      }
    }  
    else
    {
      MusEGlobal::audio->msgSetHwCtrlState(mp, chan, _dnum, MusECore::CTRL_VAL_UNKNOWN);
    }    
  }
  MusEGlobal::song->update(SC_MIDI_CONTROLLER);
}

//---------------------------------------------------------
//   ctrlChanged
//---------------------------------------------------------

void CtrlPanel::ctrlChanged(double val)
    {
      if (inHeartBeat)
            return;
      if(!_track || !_ctrl || _dnum == -1)
          return;
      
      int ival = lrint(val);
      
      int outport;
      int chan;
      int cdp = ctrlcanvas->getCurDrumPitch();
      if(_track->type() == MusECore::Track::DRUM && ((_ctrl->num() & 0xff) == 0xff) && cdp != -1)
      {
        outport = MusEGlobal::drumMap[cdp].port;
        chan = MusEGlobal::drumMap[cdp].channel;
      }  
      else  
      {
        outport = _track->outPort();
        chan = _track->outChannel();
      }
      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[outport];          
      int curval = mp->hwCtrlState(chan, _dnum);
      
//       if(_dnum == MusECore::CTRL_AFTERTOUCH)
//       {
//         // Auto bias...
//         ival += _ctrl->bias();
//         MusECore::MidiPlayEvent ev(0, outport, chan, MusECore::ME_AFTERTOUCH, ival, 0);
//         MusEGlobal::audio->msgPlayMidiEvent(&ev);
//       }
//       else
      if(_dnum == MusECore::CTRL_PROGRAM)
      {
        --ival; 
        ival &= 0x7f;
        
        if(curval == MusECore::CTRL_VAL_UNKNOWN)
          ival |= 0xffff00;
        else
          ival |= (curval & 0xffff00);  
        MusECore::MidiPlayEvent ev(0, outport, chan, MusECore::ME_CONTROLLER, _dnum, ival);
        MusEGlobal::audio->msgPlayMidiEvent(&ev);
      }
      else
      // Shouldn't happen, but...
      if((ival < _ctrl->minVal()) || (ival > _ctrl->maxVal()))
      {
        if(curval != MusECore::CTRL_VAL_UNKNOWN)
          MusEGlobal::audio->msgSetHwCtrlState(mp, chan, _dnum, MusECore::CTRL_VAL_UNKNOWN);
      }  
      else
      {
        // Auto bias...
        ival += _ctrl->bias();
      
        MusECore::MidiPlayEvent ev(0, outport, chan, MusECore::ME_CONTROLLER, _dnum, ival);
        MusEGlobal::audio->msgPlayMidiEvent(&ev);
      }
      MusEGlobal::song->update(SC_MIDI_CONTROLLER);
    }

//---------------------------------------------------------
//   setHWController
//---------------------------------------------------------

void CtrlPanel::setHWController(MusECore::MidiTrack* t, MusECore::MidiController* ctrl) 
{ 
  inHeartBeat = true;
  
  _track = t; _ctrl = ctrl; 
  
  if(!_track || !_ctrl)
  {
    _knob->setEnabled(false);
    _dl->setEnabled(false);
    _knob->hide();
    _dl->hide();
    inHeartBeat = false;
    return;
  }
  
  MusECore::MidiPort* mp;
  int ch;
  int cdp = ctrlcanvas->getCurDrumPitch();
  _dnum = _ctrl->num();
  if(_track->type() == MusECore::Track::DRUM && ((_dnum & 0xff) == 0xff) && cdp != -1)
  {
    _dnum = (_dnum & ~0xff) | MusEGlobal::drumMap[cdp].anote;
    mp = &MusEGlobal::midiPorts[MusEGlobal::drumMap[cdp].port];          
    ch = MusEGlobal::drumMap[cdp].channel;
  }  
  else if((_track->type() == MusECore::Track::NEW_DRUM || _track->type() == MusECore::Track::MIDI) && ((_dnum & 0xff) == 0xff) && cdp != -1)
  {
    _dnum = (_dnum & ~0xff) | cdp; //FINDMICHJETZT does that work?
    mp = &MusEGlobal::midiPorts[_track->outPort()];
    ch = _track->outChannel();
  }  
  else  
  {
    mp = &MusEGlobal::midiPorts[_track->outPort()];
    ch = _track->outChannel();
  }
  
  if(_dnum == MusECore::CTRL_VELOCITY)
  {
    _knob->setEnabled(false);
    _dl->setEnabled(false);
    _knob->hide();
    _dl->hide();
    _veloPerNoteButton->setEnabled(true);
    _veloPerNoteButton->show();
  }
  else
  {
    _knob->setEnabled(true);
    _dl->setEnabled(true);
    _veloPerNoteButton->setEnabled(false);
    _veloPerNoteButton->hide();
    double dlv;
    int mn; int mx; int v;
    if(_dnum == MusECore::CTRL_PROGRAM)
    {
      mn = 1;
      mx = 128;
      v = mp->hwCtrlState(ch, _dnum);
      _val = v;
      _knob->setRange(double(mn), double(mx), 1.0);
      _dl->setRange(double(mn), double(mx));
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
        dlv = _dl->off() - 1.0;
      }  
      else
      {
        v = (v + 1) & 0xff;
        if(v > 128)
          v = 128;
        dlv = double(v);
      }
    }
    else
    {
      mn = _ctrl->minVal();
      mx = _ctrl->maxVal();
      v = mp->hwCtrlState(ch, _dnum);
      _val = v;
      _knob->setRange(double(mn), double(mx), 1.0);
      _dl->setRange(double(mn), double(mx));
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
        dlv = _dl->off() - 1.0;
      }  
      else
      {
        // Auto bias...
        v -= _ctrl->bias();
        dlv = double(v);
      }
    }
    _knob->setValue(double(v));
    _dl->setValue(dlv);
    
    _knob->show();
    _dl->show();
    // Incomplete drawing sometimes. Update fixes it.
    _knob->update();
    _dl->update();
  }
  
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
  int curDrumPitch = ctrlcanvas->getCurDrumPitch();
  bool isDrum      = track->type() == MusECore::Track::DRUM;
  bool isNewDrum      = (track->type() == MusECore::Track::NEW_DRUM) || (track->type() == MusECore::Track::MIDI);
  MusECore::MidiInstrument* instr = port->instrument();
  MusECore::MidiControllerList* mcl = instr->controller();

  MusECore::MidiCtrlValListList* cll = port->controller();
  const int min = channel << 24;
  const int max = min + 0x1000000;

  const int add_ins_def = max + 1;
  const int add_other = max + 2;
  const int edit_ins = max + 3;
  
  const int velo = max + 0x101;

  int rv = act->data().toInt();
  
  if (rv == velo) {    // special case velocity
        emit controllerChanged(MusECore::CTRL_VELOCITY);
        }
  else if (rv == add_ins_def) {  // add new instrument controller
        
        PopupMenu * ctrlSubPop = new PopupMenu(this, true);  // true = enable stay open
        ctrlSubPop->addAction(new MenuTitleItem(tr("Instrument-defined"), ctrlSubPop));
        
        //
        // populate popup with all controllers available for
        // current instrument
        //
        
        for (MusECore::iMidiController ci = mcl->begin(); ci != mcl->end(); ++ci)
        {
            int num = ci->second->num();
            if((num & 0xff) == 0xff)
            {
              if (isDrum && curDrumPitch!=-1)
                num = (num & ~0xff) + MusEGlobal::drumMap[curDrumPitch].anote;
              else if (isNewDrum && curDrumPitch!=-1)
                num = (num & ~0xff) + curDrumPitch; //FINDMICH does this work?
              else // dont show drum specific controller if not a drum track
                continue;
            }    

            if(cll->find(channel, num) == cll->end())
              ctrlSubPop->addAction(MusECore::midiCtrlNumString(num, true) + ci->second->name())->setData(num);
        }
        
        // Don't allow editing instrument if it's a synth
        if(!port->device() || port->device()->deviceType() != MusECore::MidiDevice::SYNTH_MIDI)
          ctrlSubPop->addAction(QIcon(*midi_edit_instrumentIcon), tr("Edit instrument ..."))->setData(edit_ins);
        
        QAction *act2 = ctrlSubPop->exec(selCtrl->mapToGlobal(QPoint(0,0)));
        if (act2) 
        {
          int rv2 = act2->data().toInt();
          
          if (rv2 == edit_ins)            // edit instrument
            MusEGlobal::muse->startEditInstrument();
          else                           // select new instrument control
          {
            MusECore::MidiController* c;
            for (MusECore::iMidiController ci = mcl->begin(); ci != mcl->end(); ++ci) 
            {
                  c = ci->second;
                  int num = c->num();
                  if (isDrum && ((num & 0xff) == 0xff) && curDrumPitch!=-1)
                    num = (num & ~0xff) + MusEGlobal::drumMap[curDrumPitch].anote;
                  else if (isNewDrum && ((num & 0xff) == 0xff) && curDrumPitch!=-1)
                    num = (num & ~0xff) + curDrumPitch; //FINDMICHJETZT does this work?
                  
                  if(num != rv2)
                    continue;
                    
                  if(cll->find(channel, num) == cll->end())
                  {
                    MusECore::MidiCtrlValList* vl = new MusECore::MidiCtrlValList(num);
                    
                    cll->add(channel, vl);
                    emit controllerChanged(c->num());
                  }
                  else 
                    emit controllerChanged(c->num());
                  break;
            }
          }  
        }
        delete ctrlSubPop;   
        }
  
  //else if (rv == edit_ins)             // edit instrument
  //      MusEGlobal::muse->startEditInstrument();
  
  else if (rv == add_other) {             // add new other controller
        PopupMenu* ctrlSubPop = new PopupMenu(this, true);  // true = enable stay open
        ctrlSubPop->addAction(new MenuTitleItem(tr("Common Controls"), ctrlSubPop));
        
        for(int num = 0; num < 127; ++num)
          if(cll->find(channel, num) == cll->end())
            ctrlSubPop->addAction(MusECore::midiCtrlName(num, true))->setData(num);
        QAction *act2 = ctrlSubPop->exec(selCtrl->mapToGlobal(QPoint(0,0)));
        if (act2) {
              int rv2 = act2->data().toInt();
              int num = rv2;
              if (isDrum && ((num & 0xff) == 0xff) && curDrumPitch!=-1)
                num = (num & ~0xff) + MusEGlobal::drumMap[curDrumPitch].anote;
              if (isNewDrum && ((num & 0xff) == 0xff) && curDrumPitch!=-1)
                num = (num & ~0xff) + curDrumPitch; //FINDMICHJETZT does this work?

              if(cll->find(channel, num) == cll->end())
              {
                MusECore::MidiCtrlValList* vl = new MusECore::MidiCtrlValList(num);
                
                cll->add(channel, vl);
                emit controllerChanged(rv2);
              }
              else 
                emit controllerChanged(rv2);
              }
        delete ctrlSubPop;   
        }
  else {                           // Select a control
        MusECore::iMidiCtrlValList i = cll->begin();
        for (; i != cll->end(); ++i) {
              MusECore::MidiCtrlValList* cl = i->second;
              MusECore::MidiController* c   = port->midiController(cl->num());
              if (c->num() == rv) {
                    emit controllerChanged(c->num());
                    break;
                    }
              }
        if (i == cll->end()) {
              printf("CtrlPanel: controller number %d not found!", rv);
              }
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
  if(_track->type() == MusECore::Track::DRUM && ((ctlnum & 0xff) == 0xff) && cdp != -1)
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
  if(MusEGlobal::config.velocityPerNote != _veloPerNoteButton->isChecked())
  {
    MusEGlobal::config.velocityPerNote = _veloPerNoteButton->isChecked();  
    MusEGlobal::muse->changeConfig(false);  // Save settings? No, wait till close.
  }
}

} // namespace MusEGui

