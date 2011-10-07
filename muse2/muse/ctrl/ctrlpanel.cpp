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

//#include <QMenu>
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

//---------------------------------------------------------
//   CtrlPanel
//---------------------------------------------------------

CtrlPanel::CtrlPanel(QWidget* parent, MidiEditor* e, const char* name)
   : QWidget(parent)
      {
      setObjectName(name);
      inHeartBeat = true;
      //ctrlMainPop = 0;
      //ctrlSubPop = 0;
      editor = e;
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

      selCtrl = new QPushButton(tr("S"));
      selCtrl->setFont(MusEConfig::config.fonts[3]);
      selCtrl->setFixedHeight(20);
      selCtrl->setSizePolicy(
         QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
      selCtrl->setToolTip(tr("select controller"));
      
      ///pop = new QMenu;

      // destroy button
      QPushButton* destroy = new QPushButton(tr("X"));
      destroy->setFont(MusEConfig::config.fonts[3]);
      destroy->setFixedHeight(20);
      destroy->setSizePolicy(
         QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
      destroy->setToolTip(tr("remove panel"));
      // Cursor Position
      connect(selCtrl, SIGNAL(clicked()), SLOT(ctrlPopup()));
      connect(destroy, SIGNAL(clicked()), SIGNAL(destroyPanel()));
      
      _track = 0;
      _ctrl = 0;
      _val = CTRL_VAL_UNKNOWN;
      _dnum = -1;
      
      _knob = new MusEWidget::Knob;
      _knob->setFixedWidth(25);
      _knob->setFixedHeight(25);
      _knob->setToolTip(tr("manual adjust"));
      _knob->setRange(0.0, 127.0, 1.0);
      _knob->setValue(0.0);
      _knob->setEnabled(false);
      _knob->hide();
      _knob->setAltFaceColor(Qt::red);
      
      _dl = new MusEWidget::DoubleLabel(-1.0, 0.0, +127.0);
      _dl->setPrecision(0);
      _dl->setToolTip(tr("double click on/off"));
      _dl->setSpecialText(tr("off"));
      _dl->setFont(MusEConfig::config.fonts[1]);
      _dl->setBackgroundRole(QPalette::Mid);
      _dl->setFrame(true);
      _dl->setFixedWidth(36);
      _dl->setFixedHeight(15);
      _dl->setEnabled(false);
      _dl->hide();
      
      connect(_knob, SIGNAL(sliderMoved(double,int)), SLOT(ctrlChanged(double)));
      connect(_knob, SIGNAL(sliderRightClicked(const QPoint&, int)), SLOT(ctrlRightClicked(const QPoint&, int)));
      //connect(_knob, SIGNAL(sliderReleased(int)), SLOT(ctrlReleased(int)));
      connect(_dl, SIGNAL(valueChanged(double,int)), SLOT(ctrlChanged(double)));
      connect(_dl, SIGNAL(doubleClicked(int)), SLOT(labelDoubleClicked()));
      
      bbox->addStretch();
      bbox->addWidget(selCtrl);
      bbox->addWidget(destroy);
      bbox->addStretch();
      kbox->addStretch();
      kbox->addWidget(_knob);
      kbox->addStretch();
      dbox->addStretch();
      dbox->addWidget(_dl);
      dbox->addStretch();
      connect(MusEGlobal::heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
      inHeartBeat = false;
      setLayout(vbox);
      }
//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void CtrlPanel::heartBeat()
{
  inHeartBeat = true;
  
  if(_track && _ctrl && _dnum != -1)
  {
    //if(_dnum != CTRL_VELOCITY && _dnum != CTRL_PROGRAM)
    if(_dnum != CTRL_VELOCITY)
    {
      int outport;
      int chan;
      int cdi = editor->curDrumInstrument();
      if(_track->type() == Track::DRUM && ((_ctrl->num() & 0xff) == 0xff) && cdi != -1) //FINDMICHJETZT was ist das? und ähnliche dinger
      {
        outport = drumMap[cdi].port;
        chan = drumMap[cdi].channel;
      }  
      else  
      {
        outport = _track->outPort();
        chan = _track->outChannel();
      }
      MidiPort* mp = &midiPorts[outport];          
      
      int v = mp->hwCtrlState(chan, _dnum);
      if(v == CTRL_VAL_UNKNOWN)
      {
        // MusEWidget::DoubleLabel ignores the value if already set...
        _dl->setValue(_dl->off() - 1.0);
        _val = CTRL_VAL_UNKNOWN;
        v = mp->lastValidHWCtrlState(chan, _dnum);
        if(v != CTRL_VAL_UNKNOWN && ((_dnum != CTRL_PROGRAM) || ((v & 0xff) != 0xff) ))
        {
          if(_dnum == CTRL_PROGRAM)
            v = (v & 0x7f) + 1;
          else  
            // Auto bias...
            v -= _ctrl->bias();
          if(double(v) != _knob->value())
          {
            // Added by Tim. p3.3.6
            //printf("CtrlPanel::heartBeat setting knob\n");
            
            _knob->setValue(double(v));
          }  
        }
      }
      else
      if(v != _val)
      {
        _val = v;
        if(v == CTRL_VAL_UNKNOWN || ((_dnum == CTRL_PROGRAM) && ((v & 0xff) == 0xff) ))
        {
          // MusEWidget::DoubleLabel ignores the value if already set...
          //_dl->setValue(double(_ctrl->minVal() - 1));
          _dl->setValue(_dl->off() - 1.0);
        }
        else
        {
          if(_dnum == CTRL_PROGRAM)
            v = (v & 0x7f) + 1;
          else  
            // Auto bias...
            v -= _ctrl->bias();
          
          // Added by Tim. p3.3.6
          //printf("CtrlPanel::heartBeat setting knob and label\n");
          
          _knob->setValue(double(v));
          _dl->setValue(double(v));
        }  
      }  
    }  
  }  

  inHeartBeat = false;
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
  int cdi = editor->curDrumInstrument();
  if(_track->type() == Track::DRUM && ((_ctrl->num() & 0xff) == 0xff) && cdi != -1)
  {
    outport = drumMap[cdi].port;
    chan = drumMap[cdi].channel;
  }  
  else  
  {
    outport = _track->outPort();
    chan = _track->outChannel();
  }
  MidiPort* mp = &midiPorts[outport];          
  int lastv = mp->lastValidHWCtrlState(chan, _dnum);
  
  int curv = mp->hwCtrlState(chan, _dnum);
  
  if(_dnum == CTRL_PROGRAM)
  {
    if(curv == CTRL_VAL_UNKNOWN || ((curv & 0xffffff) == 0xffffff))
    {
      // If no value has ever been set yet, use the current knob value 
      //  (or the controller's initial value?) to 'turn on' the controller.
      if(lastv == CTRL_VAL_UNKNOWN || ((lastv & 0xffffff) == 0xffffff))
      {
        //int kiv = _ctrl->initVal());
        int kiv = lrint(_knob->value());
        --kiv;
        kiv &= 0x7f;
        kiv |= 0xffff00;
        //MidiPlayEvent ev(song->cpos(), outport, chan, ME_CONTROLLER, _dnum, kiv);
        MidiPlayEvent ev(0, outport, chan, ME_CONTROLLER, _dnum, kiv);
        audio->msgPlayMidiEvent(&ev);
      }
      else
      {
        //MidiPlayEvent ev(song->cpos(), outport, chan, ME_CONTROLLER, _dnum, lastv);
        MidiPlayEvent ev(0, outport, chan, ME_CONTROLLER, _dnum, lastv);
        audio->msgPlayMidiEvent(&ev);
      }
    }
    else
    {
      //if((curv & 0xffff00) == 0xffff00)
      //{
        ////if(mp->hwCtrlState(chan, _dnum) != CTRL_VAL_UNKNOWN)
          audio->msgSetHwCtrlState(mp, chan, _dnum, CTRL_VAL_UNKNOWN);
      //}
      //else
      //{
      //  MidiPlayEvent ev(song->cpos(), outport, chan, ME_CONTROLLER, _dnum, (curv & 0xffff00) | 0xff);
      //  audio->msgPlayMidiEvent(&ev);
      //}
    }  
  }
  else
  {
    if(curv == CTRL_VAL_UNKNOWN)
    {
      // If no value has ever been set yet, use the current knob value 
      //  (or the controller's initial value?) to 'turn on' the controller.
      if(lastv == CTRL_VAL_UNKNOWN)
      {
        //int kiv = _ctrl->initVal());
        int kiv = lrint(_knob->value());
        if(kiv < _ctrl->minVal())
          kiv = _ctrl->minVal();
        if(kiv > _ctrl->maxVal())
          kiv = _ctrl->maxVal();
        kiv += _ctrl->bias();
        //MidiPlayEvent ev(song->cpos(), outport, chan, ME_CONTROLLER, _dnum, kiv);
        MidiPlayEvent ev(0, outport, chan, ME_CONTROLLER, _dnum, kiv);
        audio->msgPlayMidiEvent(&ev);
      }
      else
      {
        //MidiPlayEvent ev(song->cpos(), outport, chan, ME_CONTROLLER, _dnum, lastv);
        MidiPlayEvent ev(0, outport, chan, ME_CONTROLLER, _dnum, lastv);
        audio->msgPlayMidiEvent(&ev);
      }
    }  
    else
    {
      //if(mp->hwCtrlState(chan, _dnum) != CTRL_VAL_UNKNOWN)
        audio->msgSetHwCtrlState(mp, chan, _dnum, CTRL_VAL_UNKNOWN);
    }    
  }
  song->update(SC_MIDI_CONTROLLER);
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
      int cdi = editor->curDrumInstrument();
      if(_track->type() == Track::DRUM && ((_ctrl->num() & 0xff) == 0xff) && cdi != -1)
      {
        outport = drumMap[cdi].port;
        chan = drumMap[cdi].channel;
      }  
      else  
      {
        outport = _track->outPort();
        chan = _track->outChannel();
      }
      MidiPort* mp = &midiPorts[outport];          
      int curval = mp->hwCtrlState(chan, _dnum);
      
      if(_dnum == CTRL_PROGRAM)
      {
        --ival; 
        ival &= 0x7f;
        
        if(curval == CTRL_VAL_UNKNOWN)
          ival |= 0xffff00;
        else
          ival |= (curval & 0xffff00);  
        //MidiPlayEvent ev(song->cpos(), outport, chan, ME_CONTROLLER, _dnum, ival);
        MidiPlayEvent ev(0, outport, chan, ME_CONTROLLER, _dnum, ival);
        audio->msgPlayMidiEvent(&ev);
      }
      else
      // Shouldn't happen, but...
      if((ival < _ctrl->minVal()) || (ival > _ctrl->maxVal()))
      {
        //if(mp->hwCtrlState(chan, _dnum) != CTRL_VAL_UNKNOWN)
        if(curval != CTRL_VAL_UNKNOWN)
          audio->msgSetHwCtrlState(mp, chan, _dnum, CTRL_VAL_UNKNOWN);
      }  
      else
      {
        // Auto bias...
        ival += _ctrl->bias();
      
        //MidiPlayEvent ev(song->cpos(), outport, chan, ME_CONTROLLER, _dnum, ival);
        MidiPlayEvent ev(0, outport, chan, ME_CONTROLLER, _dnum, ival);
        audio->msgPlayMidiEvent(&ev);
      }
      song->update(SC_MIDI_CONTROLLER);
    }

//---------------------------------------------------------
//   setHWController
//---------------------------------------------------------

void CtrlPanel::setHWController(MidiTrack* t, MidiController* ctrl) 
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
  
  MidiPort* mp;
  int ch;
  int cdi = editor->curDrumInstrument();
  _dnum = _ctrl->num();
  if(_track->type() == Track::DRUM && ((_dnum & 0xff) == 0xff) && cdi != -1)
  {
    _dnum = (_dnum & ~0xff) | drumMap[cdi].anote;
    mp = &midiPorts[drumMap[cdi].port];          
    ch = drumMap[cdi].channel;
  }  
  else  
  {
    mp = &midiPorts[_track->outPort()];
    ch = _track->outChannel();
  }
  
  //if(_dnum == CTRL_VELOCITY || _dnum == CTRL_PROGRAM)
  if(_dnum == CTRL_VELOCITY)
  {
    _knob->setEnabled(false);
    _dl->setEnabled(false);
    _knob->hide();
    _dl->hide();
  }
  else
  {
    _knob->setEnabled(true);
    _dl->setEnabled(true);
    double dlv;
    int mn; int mx; int v;
    if(_dnum == CTRL_PROGRAM)
    {
      mn = 1;
      mx = 128;
      v = mp->hwCtrlState(ch, _dnum);
      _val = v;
      _knob->setRange(double(mn), double(mx), 1.0);
      _dl->setRange(double(mn), double(mx));
      //_dl->setOff(double(mn - 1));
      if(v == CTRL_VAL_UNKNOWN || ((v & 0xffffff) == 0xffffff))
      {
        int lastv = mp->lastValidHWCtrlState(ch, _dnum);
        if(lastv == CTRL_VAL_UNKNOWN || ((lastv & 0xffffff) == 0xffffff))
        {
          int initv = _ctrl->initVal();
          if(initv == CTRL_VAL_UNKNOWN || ((initv & 0xffffff) == 0xffffff))
            v = 1;
          else  
            v = (initv + 1) & 0xff;
        }
        else  
          v = (lastv + 1) & 0xff;
        
        if(v > 128)
          v = 128;
        //dlv = mn - 1;
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
      //_dl->setOff(double(mn - 1));
      if(v == CTRL_VAL_UNKNOWN)
      {
        int lastv = mp->lastValidHWCtrlState(ch, _dnum);
        if(lastv == CTRL_VAL_UNKNOWN)
        {
          if(_ctrl->initVal() == CTRL_VAL_UNKNOWN)
            v = 0;
          else  
            v = _ctrl->initVal();
        }
        else  
          v = lastv - _ctrl->bias();
        //dlv = mn - 1;
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

#if 0
struct CI {
            QString s;
            bool used;
            CI(const QString& ss, bool u) : s(ss), used(u) {}
            };

//---------------------------------------------------------
//   ctrlPopup
//---------------------------------------------------------

void CtrlPanel::ctrlPopup()
      {
      //---------------------------------------------------
      // build list of midi controllers for current
      // MidiPort/channel
      //---------------------------------------------------

      PartList* parts  = editor->parts();
      Part* part       = editor->curCanvasPart();
      MidiTrack* track = (MidiTrack*)(part->track());
      int channel      = track->outChannel();
      MidiPort* port   = &midiPorts[track->outPort()];
      int curDrumInstrument = editor->curDrumInstrument();
      bool isDrum      = track->type() == Track::DRUM; //FINDMICHJETZT ist das wichtig?

      QMenu* pop = new QMenu;
      //pop->clear();
      pop->addAction(tr("Velocity"))->setData(1);
      
      MidiCtrlValListList* cll = port->controller();
      int min = channel << 24;
      int max = min + 0x1000000;

      std::list<CI> sList;
      typedef std::list<CI>::iterator isList;

      for (iMidiCtrlValList i = cll->lower_bound(min); i != cll->lower_bound(max); ++i) {
            MidiCtrlValList* cl = i->second;
            MidiController* c   = port->midiController(cl->num());
            // dont show drum specific controller if not a drum track
            if ((c->num() & 0xff) == 0xff) {
                  if (!isDrum)
                        continue;
                  // only show controller for curDrumInstrument:
                  if ((cl->num() & 0xff) != drumMap[curDrumInstrument].anote) {
                        continue;
                        }
                  }
            isList i = sList.begin();
            for (; i != sList.end(); ++i) {
                  if (i->s == c->name())
                        break;
                  }
            if (i == sList.end()) {
                  bool used = false;
                  for (iPart ip = parts->begin(); ip != parts->end(); ++ip) {
                        EventList* el = ip->second->events();
                        for (iEvent ie = el->begin(); ie != el->end(); ++ie) {
                              Event e = ie->second;
                              if ((e.type() == Controller) && (e.dataA() == cl->num())) {
                                    used = true;
                                    break;
                                    }
                              }
                        if (used)
                              break;
                        }
                  sList.push_back(CI(c->name(), used));
                  }
            }
      for (isList i = sList.begin(); i != sList.end(); ++i) {
            if (i->used)
                  pop->addAction(QIcon(*greendotIcon), i->s);
            else
                  pop->addAction(i->s);
            }

      pop->addAction(QIcon(*configureIcon), tr("add new ..."))->setData(2);
      QAction *act = pop->exec(selCtrl->mapToGlobal(QPoint(0,0)));
      selCtrl->setDown(false);

      if (!act)
      {
        delete pop;
        return;
      }
      
      int rv = act->data().toInt();
      QString s = act->text();
      delete pop;
      
      if (rv == 1) {    // special case velocity
            emit controllerChanged(CTRL_VELOCITY);
            }
      else if (rv == 2) {
            //
            // add new controller
            //
            QMenu* pop1 = new QMenu(this);
            //pop1->setCheckable(false); // Qt4 doc says not needed.
            //
            // populate popup with all controllers available for
            // current instrument
            //
            MidiInstrument* instr   = port->instrument();
            MidiControllerList* mcl = instr->controller();
            for (iMidiController ci = mcl->begin(); ci != mcl->end(); ++ci)
            {
                int num = ci->second->num();
                if (isDrum && ((num & 0xff) == 0xff))
                  num = (num & ~0xff) + drumMap[curDrumInstrument].anote;
                
                if(cll->find(channel, num) == cll->end())
                  pop1->addAction(ci->second->name());
            }
            QAction *act2 = pop1->exec(selCtrl->mapToGlobal(QPoint(0,0)));
            if (act2) {
                  QString s = act2->text();
                  MidiController* c;
                  for (iMidiController ci = mcl->begin(); ci != mcl->end(); ++ci) {
                        c = ci->second;
                        if (c->name() == s) {
                              int num = c->num();
                              if (isDrum && ((num & 0xff) == 0xff))
                                num = (num & ~0xff) + drumMap[curDrumInstrument].anote;
                              
                              if(cll->find(channel, num) == cll->end())
                              {
                                MidiCtrlValList* vl = new MidiCtrlValList(num);
                                
                                cll->add(channel, vl);
                                emit controllerChanged(c->num());
                                //song->update(SC_MIDI_CONTROLLER_ADD);
                              }
                              else 
                                emit controllerChanged(c->num());
                              break;
                              }
                        }
                  }
            delete pop1;   
            }
      else {
            ///QString s = act->text();
            iMidiCtrlValList i = cll->begin();
            for (; i != cll->end(); ++i) {
                  MidiCtrlValList* cl = i->second;
                  MidiController* c   = port->midiController(cl->num());
                  if (c->name() == s) {
                        emit controllerChanged(c->num());
                        break;
                        }
                  }
            if (i == cll->end()) {
                  printf("CtrlPanel: controller %s not found!", s.toLatin1().constData());
                  }
            }
      }

#else               // p4.0.25 Tim
struct CI {
            int num;
            QString s;
            bool used;
            bool instrument;
            CI(int n, const QString& ss, bool u, bool i) : num(n), s(ss), used(u), instrument(i) {}
            };

void CtrlPanel::ctrlPopup()
      {
      //---------------------------------------------------
      // build list of midi controllers for current
      // MidiPort/channel
      //---------------------------------------------------

      PartList* parts  = editor->parts();
      Part* part       = editor->curCanvasPart();
      MidiTrack* track = (MidiTrack*)(part->track());
      int channel      = track->outChannel();
      MidiPort* port   = &midiPorts[track->outPort()];
      int curDrumInstrument = editor->curDrumInstrument();
      bool isDrum      = track->type() == Track::DRUM; //FINDMICHJETZT ist das wichtig?
      MidiInstrument* instr = port->instrument();
      MidiControllerList* mcl = instr->controller();

      MidiCtrlValListList* cll = port->controller();
      int min = channel << 24;
      int max = min + 0x1000000;

      std::list<CI> sList;
      typedef std::list<CI>::iterator isList;

      for (iMidiCtrlValList i = cll->lower_bound(min); i != cll->lower_bound(max); ++i) {
            MidiCtrlValList* cl = i->second;
            MidiController* c   = port->midiController(cl->num());
            // dont show drum specific controller if not a drum track
            if ((c->num() & 0xff) == 0xff) {
                  if (!isDrum)
                        continue;
                  // only show controller for curDrumInstrument:
                  if ((cl->num() & 0xff) != drumMap[curDrumInstrument].anote) {
                        continue;
                        }
                  }
            isList i = sList.begin();
            for (; i != sList.end(); ++i) {
                  //if (i->s == c->name())
                  if (i->num == c->num())
                        break;
                  }
            if (i == sList.end()) {
                  bool used = false;
                  for (iPart ip = parts->begin(); ip != parts->end(); ++ip) {
                        EventList* el = ip->second->events();
                        for (iEvent ie = el->begin(); ie != el->end(); ++ie) {
                              Event e = ie->second;
                              if ((e.type() == Controller) && (e.dataA() == cl->num())) {
                                    used = true;
                                    break;
                                    }
                              }
                        if (used)
                              break;
                        }
                  //sList.push_back(CI(c->name(), used));
                  bool isinstr = ( mcl->find(c->num()) != mcl->end() );
                  int cnum = c->num();
                  // Need to distinguish between global default controllers and 
                  //  instrument defined controllers. Instrument takes priority over global
                  //  ie they 'overtake' definition of a global controller such that the
                  //  global def is no longer available.
                  sList.push_back(CI(cnum, 
                                  isinstr ? midiCtrlNumString(cnum, true) + c->name() : midiCtrlName(cnum, true), 
                                  used, isinstr));
                  }
            }
      
      MusEWidget::PopupMenu* ctrlMainPop = new MusEWidget::PopupMenu;
      
      //ctrlMainPop->addSeparator();
      ctrlMainPop->addAction(new MusEWidget::MenuTitleItem(tr("Instrument-defined"), ctrlMainPop));
      
      //ctrlMainPop->addAction(QIcon(*configureIcon), tr("Add ..."))->setData(max + 1);
      
      // Add instrument-defined controllers.
      for (isList i = sList.begin(); i != sList.end(); ++i) 
      {
        if(!i->instrument)
          continue;
        if (i->used)
          ctrlMainPop->addAction(QIcon(*greendotIcon), i->s)->setData(i->num);
        else
          ctrlMainPop->addAction(i->s)->setData(i->num);
      }

      ctrlMainPop->addAction(QIcon(*configureIcon), tr("Add ..."))->setData(max + 1);
      //ctrlMainPop->addAction(QIcon(*midi_edit_instrumentIcon), tr("Edit instruments"))->setData(max + 2);

      ctrlMainPop->addSeparator();
      ctrlMainPop->addAction(new MusEWidget::MenuTitleItem(tr("Others"), ctrlMainPop));
      
      //ctrlMainPop->addAction(QIcon(*configureIcon), tr("Add ..."))->setData(max + 3);
      
      ctrlMainPop->addAction(tr("Velocity"))->setData(max);
      
      // Add global default controllers (all controllers not found in instrument).
      for (isList i = sList.begin(); i != sList.end(); ++i) 
      {
        if(i->instrument)
          continue;
        if (i->used)
          ctrlMainPop->addAction(QIcon(*greendotIcon), i->s)->setData(i->num);
        else
          ctrlMainPop->addAction(i->s)->setData(i->num);
      }
      
      ctrlMainPop->addAction(QIcon(*configureIcon), tr("Add ..."))->setData(max + 3);

      //connect(ctrlMainPop, SIGNAL(hovered(QAction*)), SLOT(ctrlMainPopHovered(QAction*)));
      
      QAction *act = ctrlMainPop->exec(selCtrl->mapToGlobal(QPoint(0,0)));
      selCtrl->setDown(false);
      
      if (!act)
      {
        delete ctrlMainPop;
        return;
      }
      
      int rv = act->data().toInt();
      delete ctrlMainPop;
      
      if (rv == max) {    // special case velocity
            emit controllerChanged(CTRL_VELOCITY);
            }
      else if (rv == max + 1) {  // add new instrument controller
            
            MusEWidget::PopupMenu * ctrlSubPop = new MusEWidget::PopupMenu(this);
            ctrlSubPop->addAction(new MusEWidget::MenuTitleItem(tr("Instrument-defined"), ctrlSubPop));
            
            //
            // populate popup with all controllers available for
            // current instrument
            //
            
            //ctrlSubPop->addAction(QIcon(*midi_edit_instrumentIcon), tr("Edit instruments"))->setData(max + 2);
            
            for (iMidiController ci = mcl->begin(); ci != mcl->end(); ++ci)
            {
                int num = ci->second->num();
                if((num & 0xff) == 0xff)
                {
                  // dont show drum specific controller if not a drum track
                  if(!isDrum)
                    continue;
                  num = (num & ~0xff) + drumMap[curDrumInstrument].anote;
                }    

                if(cll->find(channel, num) == cll->end())
                  ctrlSubPop->addAction(midiCtrlNumString(num, true) + ci->second->name())->setData(num);
            }
            
            // Don't allow editing instrument if it's a synth
            if(!port->device() || port->device()->deviceType() != MidiDevice::SYNTH_MIDI)
              ctrlSubPop->addAction(QIcon(*midi_edit_instrumentIcon), tr("Edit instrument ..."))->setData(max + 2);
            
            //connect(ctrlSubPop, SIGNAL(hovered(QAction*)), SLOT(ctrlSubPopHovered(QAction*)));
            
            QAction *act2 = ctrlSubPop->exec(selCtrl->mapToGlobal(QPoint(0,0)));
            if (act2) 
            {
              int rv2 = act2->data().toInt();
              
              if (rv2 == max + 2)            // edit instrument
                MusEGlobal::muse->startEditInstrument();
              else                           // select new instrument control
              {
                MidiController* c;
                for (iMidiController ci = mcl->begin(); ci != mcl->end(); ++ci) 
                {
                      c = ci->second;
                      int num = c->num();
                      if (isDrum && ((num & 0xff) == 0xff))
                        num = (num & ~0xff) + drumMap[curDrumInstrument].anote;
                      
                      if(num != rv2)
                        continue;
                        
                      if(cll->find(channel, num) == cll->end())
                      {
                        MidiCtrlValList* vl = new MidiCtrlValList(num);
                        
                        cll->add(channel, vl);
                        emit controllerChanged(c->num());
                        //song->update(SC_MIDI_CONTROLLER_ADD);
                      }
                      else 
                        emit controllerChanged(c->num());
                      break;
                }
              }  
            }
            delete ctrlSubPop;   
            }
      
      //else if (rv == max + 2)             // edit instrument
      //      MusEGlobal::muse->startEditInstrument();
      
      else if (rv == max + 3) {             // add new other controller
            MusEWidget::PopupMenu* ctrlSubPop = new MusEWidget::PopupMenu(this);
            ctrlSubPop->addAction(new MusEWidget::MenuTitleItem(tr("Common Controls"), ctrlSubPop));
            
            for(int num = 0; num < 127; ++num)
              if(cll->find(channel, num) == cll->end())
                ctrlSubPop->addAction(midiCtrlName(num, true))->setData(num);
            QAction *act2 = ctrlSubPop->exec(selCtrl->mapToGlobal(QPoint(0,0)));
            if (act2) {
                  int rv2 = act2->data().toInt();
                  int num = rv2;
                  if (isDrum && ((num & 0xff) == 0xff))
                    num = (num & ~0xff) + drumMap[curDrumInstrument].anote;
                  if(cll->find(channel, num) == cll->end())
                  {
                    MidiCtrlValList* vl = new MidiCtrlValList(num);
                    
                    cll->add(channel, vl);
                    emit controllerChanged(rv2);
                    //song->update(SC_MIDI_CONTROLLER_ADD);
                  }
                  else 
                    emit controllerChanged(rv2);
                  }
            delete ctrlSubPop;   
            }
      else {                           // Select a control
            //QString s = act->text();
            iMidiCtrlValList i = cll->begin();
            for (; i != cll->end(); ++i) {
                  MidiCtrlValList* cl = i->second;
                  MidiController* c   = port->midiController(cl->num());
                  //if (c->name() == s) {
                  if (c->num() == rv) {
                        emit controllerChanged(c->num());
                        break;
                        }
                  }
            if (i == cll->end()) {
                  //printf("CtrlPanel: controller %s not found!", s.toLatin1().constData());
                  printf("CtrlPanel: controller number %d not found!", rv);
                  }
            }
      }
#endif

//---------------------------------------------------------
//   ctrlRightClicked
//---------------------------------------------------------

void CtrlPanel::ctrlRightClicked(const QPoint& p, int /*id*/)
{
  //if(!_knob->selectedFaceColor())
  //  _knob->selectFaceColor(true);
  //if(_dnum == -1)
  //  return;
  if(!editor->curCanvasPart() || !_ctrl)
    return;  
    
  int cdi = editor->curDrumInstrument();
  int ctlnum = _ctrl->num();
  if(_track->type() == Track::DRUM && ((ctlnum & 0xff) == 0xff) && cdi != -1)
    //ctlnum = (ctlnum & ~0xff) | drumMap[cdi].enote;
    ctlnum = (ctlnum & ~0xff) | cdi;
  
  MidiPart* part = dynamic_cast<MidiPart*>(editor->curCanvasPart());
  song->execMidiAutomationCtlPopup(0, part, p, ctlnum);
}

/*
//---------------------------------------------------------
//   ctrlReleased
//---------------------------------------------------------

void CtrlPanel::ctrlReleased(int id)
{
  //if(_knob->selectedFaceColor())
  //  _knob->selectFaceColor(false);
}
*/
