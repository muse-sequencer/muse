//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midiport.cpp,v 1.21.2.15 2009/12/07 20:11:51 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2012 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#include <set>

#include <QMenu>
#include <QApplication>

#include "mididev.h"
#include "midiport.h"
#include "midictrl.h"
#include "midi.h"
#include "minstrument.h"
#include "xml.h"
#include "gconfig.h"
#include "globals.h"
#include "globaldefs.h"
#include "mpevent.h"
#include "synth.h"
#include "app.h"
#include "song.h"
#include "menutitleitem.h"
#include "icons.h"
#include "track.h"
#include "drummap.h"
#include "audio.h"

namespace MusEGlobal {
MusECore::MidiPort midiPorts[MIDI_PORTS];
}

namespace MusECore {


MidiControllerList defaultManagedMidiController;

//---------------------------------------------------------
//   initMidiPorts
//---------------------------------------------------------

void initMidiPorts()
      {
      defaultManagedMidiController.add(&pitchCtrl);
      defaultManagedMidiController.add(&programCtrl);
      defaultManagedMidiController.add(&volumeCtrl);
      defaultManagedMidiController.add(&panCtrl);
      defaultManagedMidiController.add(&reverbSendCtrl);
      defaultManagedMidiController.add(&chorusSendCtrl);
      defaultManagedMidiController.add(&variationSendCtrl);
        
      for (int i = 0; i < MIDI_PORTS; ++i) {
            MidiPort* port = &MusEGlobal::midiPorts[i];
            
            //
            // create minimum set of managed controllers
            // to make midi mixer operational
            //
            port->addDefaultControllers();
            
            port->setInstrument(registerMidiInstrument("GM")); 
            port->syncInfo().setPort(i);
            // p4.0.17 Set the first channel on the first port to auto-connect to midi track outputs.
            if(i == 0)
              port->setDefaultOutChannels(1);      
            }
      }

//---------------------------------------------------------
//   MidiPort
//---------------------------------------------------------

MidiPort::MidiPort()
   : _state("not configured")
      {
      _initializationsSent = false;  
      _defaultInChannels  = (1 << MIDI_CHANNELS) -1;  // p4.0.17 Default is now to connect to all channels.
      _defaultOutChannels = 0;
      _device     = 0;
      _instrument = 0;
      _controller = new MidiCtrlValListList();
      _foundInSongFile = false;
      }

//---------------------------------------------------------
//   MidiPort
//---------------------------------------------------------

MidiPort::~MidiPort()
      {
      delete _controller;
      }

//---------------------------------------------------------
//   guiVisible
//---------------------------------------------------------

bool MidiPort::guiVisible() const
      {
      return _device ? _device->guiVisible() : false;
      }

//---------------------------------------------------------
//   showGui
//---------------------------------------------------------

void MidiPort::showGui(bool v)
{
  if(_device) 
    _device->showGui(v);
}

//---------------------------------------------------------
//   hasGui
//---------------------------------------------------------

bool MidiPort::hasGui() const
      {
      return _device ? _device->hasGui() : false;
      }

//---------------------------------------------------------
//   nativeGuiVisible
//---------------------------------------------------------

bool MidiPort::nativeGuiVisible() const
      {
      return _device ? _device->nativeGuiVisible() : false;
      }

//---------------------------------------------------------
//   showNativeGui
//---------------------------------------------------------

void MidiPort::showNativeGui(bool v)
{
  if(_device) 
    _device->showNativeGui(v);
}

//---------------------------------------------------------
//   hasNativeGui
//---------------------------------------------------------

bool MidiPort::hasNativeGui() const
      {
      return _device ? _device->hasNativeGui() : false;
      }

//---------------------------------------------------------
//   setDevice
//---------------------------------------------------------

void MidiPort::setMidiDevice(MidiDevice* dev)
      {
      if (_device) {
            if (_device->isSynti())
                  _instrument = genericMidiInstrument;
            _device->setPort(-1);
            _device->close();
            _initializationsSent = false;
            // Wait until upcoming process call has finished. Otherwise Jack may crash!
            MusEGlobal::audio->msgAudioWait();
            }
      if (dev) {
            for (int i = 0; i < MIDI_PORTS; ++i) {
                  MidiPort* mp = &MusEGlobal::midiPorts[i];
                  if (mp->device() == dev) {
                        if(dev->isSynti())
                          mp->setInstrument(genericMidiInstrument);
                        // move device
                        _state = mp->state();
                        mp->clearDevice();
                        break;
                        }
                  }
            _device = dev;
            if (_device->isSynti()) {
                  SynthI* s = (SynthI*)_device;
                  _instrument = s;
                  }
            _state = _device->open();
            _device->setPort(portno());
            _initializationsSent = false;
            }  

      else
            clearDevice();
      }

//---------------------------------------------------------
//   sendPendingInitializations
//   Return true if success.
//---------------------------------------------------------

bool MidiPort::sendPendingInitializations(bool force)
{
  if(!_device || !(_device->openFlags() & 1))   // Not writable?
    return false;
  
  bool rv = true;
  int port = portno();
  
  //
  // test for explicit instrument initialization
  //

  unsigned last_tick = 0;
  MusECore::MidiInstrument* instr = instrument();
  if(instr && MusEGlobal::config.midiSendInit && (force || !_initializationsSent))
  {
    // Send the Instrument Init sequences.
    EventList* events = instr->midiInit();
    if(!events->empty())
    {
      for(iEvent ie = events->begin(); ie != events->end(); ++ie) 
      {
        unsigned tick = ie->second.tick();
        if(tick > last_tick)
          last_tick = tick;
        MusECore::MidiPlayEvent ev(tick, port, 0, ie->second);
        _device->putEvent(ev);
      }
      // Give a bit of time for the last Init sysex to settle?
      last_tick += 100;
    }
    _initializationsSent = true; // Mark as having been sent.
  }
    
  // Send the Instrument controller default values.
  sendInitialControllers(last_tick);

  return rv;
}
      
//---------------------------------------------------------
//   sendInitialControllers
//   Return true if success.
//---------------------------------------------------------

bool MidiPort::sendInitialControllers(unsigned start_time)
{
  bool rv = true;
  int port = portno();
  
  // Find all channels of this port used in the song...
  bool usedChans[MIDI_CHANNELS];
  int usedChanCount = 0;
  for(int i = 0; i < MIDI_CHANNELS; ++i)
    usedChans[i] = false;
  if(MusEGlobal::song->click() && MusEGlobal::clickPort == port)
  {
    usedChans[MusEGlobal::clickChan] = true;
    ++usedChanCount;
  }
  bool drum_found = false;
  for(ciMidiTrack imt = MusEGlobal::song->midis()->begin(); imt != MusEGlobal::song->midis()->end(); ++imt)
  {
    if((*imt)->type() == MusECore::Track::DRUM)
    {
      if(!drum_found)
      {
        drum_found = true; 
        for(int i = 0; i < DRUM_MAPSIZE; ++i)
        {
          // Default to track port if -1 and track channel if -1.
          int mport = MusEGlobal::drumMap[i].port;
          if(mport == -1)
            mport = (*imt)->outPort();
          int mchan = MusEGlobal::drumMap[i].channel;
          if(mchan == -1)
            mchan = (*imt)->outChannel();
          if(mport != port || usedChans[mchan])
            continue;
          usedChans[mchan] = true;
          ++usedChanCount;
          if(usedChanCount >= MIDI_CHANNELS)
            break;  // All are used, done searching.
        }
      }
    }
    else
    {
      if((*imt)->outPort() != port || usedChans[(*imt)->outChannel()])
        continue;
      usedChans[(*imt)->outChannel()] = true;
      ++usedChanCount;
    }
    
    if(usedChanCount >= MIDI_CHANNELS)
      break;  // All are used, done searching.
  }

  // NOT for syntis. Use midiState and/or initParams for that. 
  if(MusEGlobal::config.midiSendInit && MusEGlobal::config.midiSendCtlDefaults && _instrument && !_device->isSynti())
  {
    MidiControllerList* cl = _instrument->controller();
    MidiController* mc;
    for(ciMidiController imc = cl->begin(); imc != cl->end(); ++imc) 
    {
      mc = imc->second;
      for(int chan = 0; chan < MIDI_CHANNELS; ++chan)
      {
        if(!usedChans[chan])
          continue;  // This channel on this port is not used in the song.
        ciMidiCtrlValList i;
        // Look for an initial value for this midi controller, on this midi channel, in the song...
        for(i = _controller->begin(); i != _controller->end(); ++i) 
        {
          int channel = i->first >> 24;
          int cntrl   = i->first & 0xffffff;
          int val     = i->second->hwVal();
          if(channel == chan && cntrl == mc->num() && val != CTRL_VAL_UNKNOWN)
            break;
        }  
        // If no initial value was found for this midi controller, on this midi channel, in the song...
        if(i == _controller->end())
        {
          // If the instrument's midi controller has an initial value, send it now.
          if(mc->initVal() != CTRL_VAL_UNKNOWN)
          {
            int ctl = mc->num();
            // Note the addition of bias!
            // Retry added. Use default attempts and delay. 
            _device->putEventWithRetry(MidiPlayEvent(start_time, port, chan,  
              ME_CONTROLLER, ctl, mc->initVal() + mc->bias()));
            // Set it once so the 'last HW value' is set, and control knobs are positioned at the value...
            // Set it again so that control labels show 'off'...
            setHwCtrlStates(chan, ctl, CTRL_VAL_UNKNOWN, mc->initVal() + mc->bias());
          }    
        }    
      }
    }
  }

  // init HW controller state
  for (iMidiCtrlValList i = _controller->begin(); i != _controller->end(); ++i) 
  {
      int channel = i->first >> 24;
      if(!usedChans[channel])
        continue;  // This channel on this port is not used in the song.
      int cntrl   = i->first & 0xffffff;
      int val     = i->second->hwVal();
      if (val != CTRL_VAL_UNKNOWN) 
      {
        // Retry added. Use default attempts and delay. 
        _device->putEventWithRetry(MidiPlayEvent(start_time, port, channel,
          ME_CONTROLLER, cntrl, val));                          
        // Set it once so the 'last HW value' is set, and control knobs are positioned at the value...
        setHwCtrlState(channel, cntrl, val);
      }
  }
              
  return rv;
}
      
//---------------------------------------------------------
//   setInstrument
//---------------------------------------------------------

void MidiPort::setInstrument(MidiInstrument* i)
{
  _instrument = i;
  _initializationsSent = false;
}

//---------------------------------------------------------
//   clearDevice
//---------------------------------------------------------

void MidiPort::clearDevice()
      {
      _device = 0;
      _initializationsSent = false;
      _state  = "not configured";
      }

//---------------------------------------------------------
//   portno
//---------------------------------------------------------

int MidiPort::portno() const
      {
      for (int i = 0; i < MIDI_PORTS; ++i) {
            if (&MusEGlobal::midiPorts[i] == this)
                  return i;
            }
      return -1;
      }

//---------------------------------------------------------
//   midiPortsPopup
//---------------------------------------------------------

QMenu* midiPortsPopup(QWidget* parent, int checkPort, bool includeDefaultEntry)
      {
      QMenu* p = new QMenu(parent);
      QMenu* subp = 0;
      QAction *act = 0;
      QString name;
      const int openConfigId = MIDI_PORTS;
      const int defaultId    = MIDI_PORTS + 1;
      
      // Warn if no devices available. Add an item to open midi config. 
      int pi = 0;
      for( ; pi < MIDI_PORTS; ++pi)
      {
        MusECore::MidiDevice* md = MusEGlobal::midiPorts[pi].device();
        if(md && (md->rwFlags() & 1))   
          break;
      }
      if(pi == MIDI_PORTS)
      {
        act = p->addAction(qApp->translate("@default", QT_TRANSLATE_NOOP("@default", "Warning: No output devices!")));
        act->setCheckable(false);
        act->setData(-1);
        p->addSeparator();
      }
      act = p->addAction(QIcon(*MusEGui::settings_midiport_softsynthsIcon), qApp->translate("@default", QT_TRANSLATE_NOOP("@default", "Open midi config...")));
      act->setCheckable(false);
      act->setData(openConfigId);  
      p->addSeparator();
      
      p->addAction(new MusEGui::MenuTitleItem(qApp->translate("@default", QT_TRANSLATE_NOOP("@default", "Output port/device")), p));

      if(includeDefaultEntry)
      {
        act = p->addAction(qApp->translate("@default", QT_TRANSLATE_NOOP("@default", "default")));
        act->setCheckable(false);
        act->setData(defaultId); 
      }
        
      for (int i = 0; i < MIDI_PORTS; ++i) {
            MidiPort* port = &MusEGlobal::midiPorts[i];
            MusECore::MidiDevice* md = port->device();
            if(md && md->isSynti()) //make deleted audio softsynths not show in select dialog
            {
               MusECore::AudioTrack *_track = static_cast<MusECore::AudioTrack *>(static_cast<MusECore::SynthI *>(md));
               MusECore::TrackList* tl = MusEGlobal::song->tracks();
               if(tl->find(_track) == tl->end())
                  continue;
            }
            if(md && !(md->rwFlags() & 1) && (i != checkPort))                     // Only writeable ports, or current one.
              continue;
            name.sprintf("%d:%s", port->portno()+1, port->portname().toLatin1().constData());
            if(md || (i == checkPort))   
            {  
              act = p->addAction(name);
              act->setData(i);
              act->setCheckable(true);
              act->setChecked(i == checkPort);
            }  

            if(!md)
            {
              if(!subp)                  // No submenu yet? Create it now.
              {
                subp = new QMenu(p);
                subp->setTitle(qApp->translate("@default", QT_TRANSLATE_NOOP("@default", "Empty ports")));
              }  
              act = subp->addAction(QString().setNum(i+1));
              act->setData(i);
              act->setCheckable(true);
              act->setChecked(i == checkPort);
            }  
          }  
      if(subp)
        p->addMenu(subp);
      return p;
      }

//---------------------------------------------------------
//   portname
//---------------------------------------------------------

const QString& MidiPort::portname() const
      {
      static const QString none(QT_TRANSLATE_NOOP("@default", "<none>"));
      if (_device)
            return _device->name();
      else
            return none;
      }

//---------------------------------------------------------
//   tryCtrlInitVal
//---------------------------------------------------------

void MidiPort::tryCtrlInitVal(int chan, int ctl, int val)
{
  // Look for an initial value in the song for this midi controller, on this midi channel... (p4.0.27)
  iMidiCtrlValList i = _controller->find(chan, ctl);
  if(i != _controller->end()) 
  {
    int v = i->second->value(0);   // Value at tick 0.
    if(v != CTRL_VAL_UNKNOWN)
    {
      if(_device)
        _device->putEventWithRetry(MidiPlayEvent(0, portno(), chan, ME_CONTROLLER, ctl, v));                          
        
      // Set it once so the 'last HW value' is set, and control knobs are positioned at the value...
      setHwCtrlState(chan, ctl, v);
      
      return;
    }  
  }
  
  // No initial value was found in the song for this midi controller on this midi channel. Try the instrument...
  if(_instrument)
  {
    MidiControllerList* cl = _instrument->controller();
    ciMidiController imc = cl->find(ctl);  
    if(imc != cl->end())
    {
      MidiController* mc = imc->second;
      int initval = mc->initVal();
      
      // Initialize from either the instrument controller's initial value, or the supplied value.
      if(initval != CTRL_VAL_UNKNOWN)
      {
        if(_device)
        {
          MidiPlayEvent ev(0, portno(), chan, ME_CONTROLLER, ctl, initval + mc->bias());
          _device->putEvent(ev);
          // Retry added. Use default attempts and delay. p4.0.15
          //_device->putEventWithRetry(ev);
        }  
        setHwCtrlStates(chan, ctl, CTRL_VAL_UNKNOWN, initval + mc->bias());
        
        return;
      }
    }  
  }
  
  // No initial value was found in the song or instrument for this midi controller. Just send the given value.
  if(_device)
  {
    MidiPlayEvent ev(0, portno(), chan, ME_CONTROLLER, ctl, val);
    _device->putEvent(ev);
  }  
  setHwCtrlStates(chan, ctl, CTRL_VAL_UNKNOWN, val);
}      
      
//---------------------------------------------------------
//   sendGmInitValues
//---------------------------------------------------------

void MidiPort::sendGmInitValues()
{
  for (int i = 0; i < MIDI_CHANNELS; ++i) {
        // By T356. Initialize from instrument controller if it has an initial value, otherwise use the specified value.
        // Tested: Ultimately, a track's controller stored values take priority by sending any 'zero time' value 
        //  AFTER these GM/GS/XG init routines are called via initDevices().
        tryCtrlInitVal(i, CTRL_PROGRAM,      0);
        tryCtrlInitVal(i, CTRL_PITCH,        0);
        tryCtrlInitVal(i, CTRL_VOLUME,     100);
        tryCtrlInitVal(i, CTRL_PANPOT,      64);
        tryCtrlInitVal(i, CTRL_REVERB_SEND, 40);
        tryCtrlInitVal(i, CTRL_CHORUS_SEND,  0);
        }
}

//---------------------------------------------------------
//   sendGsInitValues
//---------------------------------------------------------

void MidiPort::sendGsInitValues()
{
  sendGmInitValues();
}

//---------------------------------------------------------
//   sendXgInitValues
//---------------------------------------------------------

void MidiPort::sendXgInitValues()
{
  for (int i = 0; i < MIDI_CHANNELS; ++i) {
        // By T356. Initialize from instrument controller if it has an initial value, otherwise use the specified value.
        tryCtrlInitVal(i, CTRL_PROGRAM, 0);
        tryCtrlInitVal(i, CTRL_MODULATION, 0);
        tryCtrlInitVal(i, CTRL_PORTAMENTO_TIME, 0);
        tryCtrlInitVal(i, CTRL_VOLUME, 0x64);
        tryCtrlInitVal(i, CTRL_PANPOT, 0x40);
        tryCtrlInitVal(i, CTRL_EXPRESSION, 0x7f);
        tryCtrlInitVal(i, CTRL_SUSTAIN, 0x0);
        tryCtrlInitVal(i, CTRL_PORTAMENTO, 0x0);
        tryCtrlInitVal(i, CTRL_SOSTENUTO, 0x0);
        tryCtrlInitVal(i, CTRL_SOFT_PEDAL, 0x0);
        tryCtrlInitVal(i, CTRL_HARMONIC_CONTENT, 0x40);
        tryCtrlInitVal(i, CTRL_RELEASE_TIME, 0x40);
        tryCtrlInitVal(i, CTRL_ATTACK_TIME, 0x40);
        tryCtrlInitVal(i, CTRL_BRIGHTNESS, 0x40);
        tryCtrlInitVal(i, CTRL_REVERB_SEND, 0x28);
        tryCtrlInitVal(i, CTRL_CHORUS_SEND, 0x0);
        tryCtrlInitVal(i, CTRL_VARIATION_SEND, 0x0);
        }
}

//---------------------------------------------------------
//   sendGmOn
//    send GM-On message to midi device and keep track
//    of device state
//---------------------------------------------------------

void MidiPort::sendGmOn()
      {
      sendSysex(gmOnMsg, gmOnMsgLen);
      }

//---------------------------------------------------------
//   sendGsOn
//    send Roland GS-On message to midi device and keep track
//    of device state
//---------------------------------------------------------

void MidiPort::sendGsOn()
      {
      sendSysex(gsOnMsg2, gsOnMsg2Len);
      sendSysex(gsOnMsg3, gsOnMsg3Len);
      }

//---------------------------------------------------------
//   sendXgOn
//    send Yamaha XG-On message to midi device and keep track
//    of device state
//---------------------------------------------------------

void MidiPort::sendXgOn()
      {
      sendSysex(xgOnMsg, xgOnMsgLen);
      }

//---------------------------------------------------------
//   sendSysex
//    send SYSEX message to midi device
//---------------------------------------------------------

void MidiPort::sendSysex(const unsigned char* p, int n)
      {
      if (_device) {
            MidiPlayEvent event(0, 0, ME_SYSEX, p, n);
           _device->putEvent(event);
            }
      }

//---------------------------------------------------------
//   sendMMCLocate
//---------------------------------------------------------

void MidiPort::sendMMCLocate(unsigned char ht, unsigned char m, unsigned char s, unsigned char f, unsigned char sf, int devid)
{
  unsigned char msg[mmcLocateMsgLen];
  memcpy(msg, mmcLocateMsg, mmcLocateMsgLen);
  if(devid != -1)
    msg[1]  = devid;
  else
    msg[1]  = _syncInfo.idOut();
  msg[6]    = ht;
  msg[7]    = m;
  msg[8]    = s;
  msg[9]    = f;
  msg[10]   = sf;
  sendSysex(msg, mmcLocateMsgLen);
}

//---------------------------------------------------------
//   sendMMCStop
//---------------------------------------------------------

void MidiPort::sendMMCStop(int devid)
{
  unsigned char msg[mmcStopMsgLen];
  memcpy(msg, mmcStopMsg, mmcStopMsgLen);
  if(devid != -1)
    msg[1] = devid;
  else
    msg[1] = _syncInfo.idOut();
  sendSysex(msg, mmcStopMsgLen);
}

//---------------------------------------------------------
//   sendMMCDeferredPlay
//---------------------------------------------------------

void MidiPort::sendMMCDeferredPlay(int devid)
{
  unsigned char msg[mmcDeferredPlayMsgLen];
  memcpy(msg, mmcDeferredPlayMsg, mmcDeferredPlayMsgLen);
  if(devid != -1)
    msg[1] = devid;
  else
    msg[1] = _syncInfo.idOut();
  sendSysex(msg, mmcDeferredPlayMsgLen);
}

//---------------------------------------------------------
//   sendStart
//---------------------------------------------------------

void MidiPort::sendStart()
      {
      if (_device) {
            MidiPlayEvent event(0, 0, 0, ME_START, 0, 0);
           _device->putEvent(event);
            }
      }

//---------------------------------------------------------
//   sendStop
//---------------------------------------------------------

void MidiPort::sendStop()
      {
      if (_device) {
            MidiPlayEvent event(0, 0, 0, ME_STOP, 0, 0);
           _device->putEvent(event);
            }
      }

//---------------------------------------------------------
//   sendClock
//---------------------------------------------------------

void MidiPort::sendClock()
      {
      if (_device) {
            MidiPlayEvent event(0, 0, 0, ME_CLOCK, 0, 0);
           _device->putEvent(event);
            }
      }

//---------------------------------------------------------
//   sendContinue
//---------------------------------------------------------

void MidiPort::sendContinue()
      {
      if (_device) {
            MidiPlayEvent event(0, 0, 0, ME_CONTINUE, 0, 0);
           _device->putEvent(event);
            }
      }

//---------------------------------------------------------
//   sendSongpos
//---------------------------------------------------------

void MidiPort::sendSongpos(int pos)
      {
      if (_device) {
            MidiPlayEvent event(0, 0, 0, ME_SONGPOS, pos, 0);
           _device->putEvent(event);
            }
      }

//---------------------------------------------------------
//   addManagedController
//---------------------------------------------------------

MidiCtrlValList* MidiPort::addManagedController(int channel, int ctrl)
      {
      iMidiCtrlValList cl = _controller->find(channel, ctrl);
      if (cl == _controller->end()) {
            MidiCtrlValList* pvl = new MidiCtrlValList(ctrl);
            _controller->add(channel, pvl);
            return pvl;
            }
      else      
        return cl->second;
      }

//---------------------------------------------------------
//   addDefaultControllers
//---------------------------------------------------------

void MidiPort::addDefaultControllers()
{
  for (int i = 0; i < MIDI_CHANNELS; ++i) {
        for(ciMidiController imc = defaultManagedMidiController.begin(); imc != defaultManagedMidiController.end(); ++imc)
          addManagedController(i, imc->second->num());   
        _automationType[i] = AUTO_READ;
        }
}
      
//---------------------------------------------------------
//   limitValToInstrCtlRange
//---------------------------------------------------------

int MidiPort::limitValToInstrCtlRange(MidiController* mc, int val)
{
  if(!_instrument || !mc || val == CTRL_VAL_UNKNOWN)
    return val;
    
  //MidiController* mc = imc->second;
  int mn = mc->minVal();
  int mx = mc->maxVal();
  int bias = mc->bias();
  
  // Subtract controller bias from value.
  val -= bias;
  
  // Limit value to controller range.
  if(val < mn)
    val = mn;
  else
  if(val > mx)
    val = mx;
    
  // Re-add controller bias to value.
  val += bias;
  
  return val;
}
            
int MidiPort::limitValToInstrCtlRange(int ctl, int val)
{
  if(!_instrument || val == CTRL_VAL_UNKNOWN)
    return val;
    
  MidiControllerList* cl = _instrument->controller();

  // FIXME: This might be optimized by calling midiController instead,
  //         and simply asking if it's a drum controller. Saves one list iteration.
  // Is it a drum controller?
  MidiController *mc = drumController(ctl);
  if(!mc)
  {
    // It's not a drum controller. Find it as a regular controller instead.
    iMidiController imc = cl->find(ctl);
    if(imc != cl->end())
      mc = imc->second;
  }
  
  // If it's a valid controller, limit the value to the instrument controller range.
  if(mc)
    return limitValToInstrCtlRange(mc, val);
  
  return val;
}
            
//---------------------------------------------------------
//   sendHwCtrlState
//   Return true if it is OK to go ahead and deliver the event.
//---------------------------------------------------------

bool MidiPort::sendHwCtrlState(const MidiPlayEvent& ev, bool forceSend)
      {
      if (ev.type() == ME_CONTROLLER) {
      
            int da = ev.dataA();
            int db = ev.dataB();

            int chn = ev.channel();

            // TODO: Maybe update CTRL_HBANK/CTRL_LBANK - but ONLY if they are specifically
            //        defined by the user in the controller list.
            if(da == CTRL_HBANK)
            {
              int hb = da & 0xff;
              int lb = 0xff;
              int pr = 0xff;
              if(_device)
              {
                _device->curOutParamNums(chn)->currentProg(&pr, &lb, NULL);
                _device->curOutParamNums(chn)->BANKH = hb;
              }
              db = (hb << 16) | (lb << 8) | pr;
              da = CTRL_PROGRAM;
            }
            else if(da == CTRL_LBANK)
            {
              int hb = 0xff;
              int lb = da & 0xff;
              int pr = 0xff;
              if(_device)
              {
                _device->curOutParamNums(chn)->currentProg(&pr, NULL, &hb);
                _device->curOutParamNums(chn)->BANKL = lb;
              }
              db = (hb << 16) | (lb << 8) | pr;
              da = CTRL_PROGRAM;
            }
            else if(da == CTRL_PROGRAM)
            {
              // TODO: Maybe update CTRL_HBANK/CTRL_LBANK - but ONLY if they are specifically
              //        defined by the user in the controller list.
            }
            else
              db = limitValToInstrCtlRange(da, db);
              
            
            if(!setHwCtrlState(chn, da, db)) {
                if (MusEGlobal::debugMsg && forceSend)
                  printf("sendHwCtrlState: State already set. Forcing anyway...\n");
                if (!forceSend)
                  return false;
              }
            }
      else
      if (ev.type() == ME_POLYAFTER)    // REMOVE Tim. Or keep? ...
      {
            int pitch = ev.dataA() & 0x7f;
            int ctl = (CTRL_POLYAFTER & ~0xff) | pitch;
            int db = limitValToInstrCtlRange(ctl, ev.dataB());
            if(!setHwCtrlState(ev.channel(), ctl, db)) {
              if (!forceSend)
                return false;
            }
      }
      else
      if (ev.type() == ME_AFTERTOUCH)  // ... Same
      {
            int da = limitValToInstrCtlRange(CTRL_AFTERTOUCH, ev.dataA());
            if(!setHwCtrlState(ev.channel(), CTRL_AFTERTOUCH, da)) {
              if (!forceSend)
                return false;
            }
      }
      else
      if (ev.type() == ME_PITCHBEND) 
      {
            int da = limitValToInstrCtlRange(CTRL_PITCH, ev.dataA());
            if(!setHwCtrlState(ev.channel(), CTRL_PITCH, da)) {
              if (!forceSend)
                return false;
            }
      }
      else
      if (ev.type() == ME_PROGRAM) 
      {
        int chn = ev.channel();
        int hb = 0xff;
        int lb = 0xff;
        int pr = ev.dataA() & 0xff;
        if(_device)
        {
          _device->curOutParamNums(chn)->currentProg(NULL, &lb, &hb);
          _device->curOutParamNums(chn)->PROG = pr;
        }
        int full_prog = (hb << 16) | (lb << 8) | pr;
        if(!setHwCtrlState(chn, CTRL_PROGRAM, full_prog)) {
          if (!forceSend)
              return false;
        }
      }
      
      return true;
      }

//---------------------------------------------------------
//   sendEvent
//    return true, if event cannot be delivered
//---------------------------------------------------------

bool MidiPort::sendEvent(const MidiPlayEvent& ev, bool forceSend)
      {
      if(!sendHwCtrlState(ev, forceSend))
        return false;

      if (!_device) {
          if (MusEGlobal::debugMsg)
            printf("no device for this midi port\n");
          return true;
          }
      return _device->putEvent(ev);
      }

//---------------------------------------------------------
//   lastValidHWCtrlState
//---------------------------------------------------------

int MidiPort::lastValidHWCtrlState(int ch, int ctrl) const
{
      ch &= 0xff;
      ciMidiCtrlValList cl = ((const MidiCtrlValListList*)_controller)->find(ch, ctrl);
      if (cl == _controller->end()) {
            return CTRL_VAL_UNKNOWN;
            }
      MidiCtrlValList* vl = cl->second;
      return vl->lastValidHWVal();
}

//---------------------------------------------------------
//   hwCtrlState
//---------------------------------------------------------

int MidiPort::hwCtrlState(int ch, int ctrl) const
      {
      ch &= 0xff;
      ciMidiCtrlValList cl = ((const MidiCtrlValListList*)_controller)->find(ch, ctrl);
      if (cl == _controller->end())
            return CTRL_VAL_UNKNOWN;
      MidiCtrlValList* vl = cl->second;
      return vl->hwVal();
      }

//---------------------------------------------------------
//   setHwCtrlState
//   Returns false if value is already equal, true if value is set.
//---------------------------------------------------------

bool MidiPort::setHwCtrlState(int ch, int ctrl, int val)
      {
      // By T356. This will create a new value list if necessary, otherwise it returns the existing list. 
      MidiCtrlValList* vl = addManagedController(ch, ctrl);

      return vl->setHwVal(val);
      }

//---------------------------------------------------------
//   setHwCtrlStates
//   Sets current and last HW values.
//   Handy for forcing labels to show 'off' and knobs to show specific values 
//    without having to send two messages.
//   Returns false if both values are already set, true if either value is changed.
//---------------------------------------------------------

bool MidiPort::setHwCtrlStates(int ch, int ctrl, int val, int lastval)
      {
      // This will create a new value list if necessary, otherwise it returns the existing list. 
      MidiCtrlValList* vl = addManagedController(ch, ctrl);

      return vl->setHwVals(val, lastval);
      }


//---------------------------------------------------------
//   setControllerVal
//   This function sets a controller value, 
//   creating the controller if necessary.
//   Returns true if a value was actually added or replaced.
//---------------------------------------------------------

bool MidiPort::setControllerVal(int ch, int tick, int ctrl, int val, Part* part)
{
      MidiCtrlValList* pvl;
      iMidiCtrlValList cl = _controller->find(ch, ctrl);
      if (cl == _controller->end()) 
      {
        pvl = new MidiCtrlValList(ctrl);
        _controller->add(ch, pvl);
      }
      else
        pvl = cl->second;
        
      return pvl->addMCtlVal(tick, val, part);
}

//---------------------------------------------------------
//   getCtrl
//---------------------------------------------------------

int MidiPort::getCtrl(int ch, int tick, int ctrl) const
      {
      iMidiCtrlValList cl = _controller->find(ch, ctrl);
      if (cl == _controller->end())
            return CTRL_VAL_UNKNOWN;

      return cl->second->value(tick);
      }

int MidiPort::getCtrl(int ch, int tick, int ctrl, Part* part) const
      {
      iMidiCtrlValList cl = _controller->find(ch, ctrl);
      if (cl == _controller->end())
            return CTRL_VAL_UNKNOWN;

      return cl->second->value(tick, part);
      }
//---------------------------------------------------------
//   deleteController
//---------------------------------------------------------

void MidiPort::deleteController(int ch, int tick, int ctrl, Part* part)
    {
      iMidiCtrlValList cl = _controller->find(ch, ctrl);
      if (cl == _controller->end()) {
            if (MusEGlobal::debugMsg)
                  printf("deleteController: controller %d(0x%x) for channel %d not found size %zd\n",
                     ctrl, ctrl, ch, _controller->size());
            return;
            }
      
      cl->second->delMCtlVal(tick, part);
      }

//---------------------------------------------------------
//   midiController
//---------------------------------------------------------

MidiController* MidiPort::midiController(int num, bool createIfNotFound) const
      {
      if (_instrument) {
            MidiControllerList* mcl = _instrument->controller();
            for (iMidiController i = mcl->begin(); i != mcl->end(); ++i) {
                  int cn = i->second->num();
                  if (cn == num)
                        return i->second;
                  // wildcard?
                  if (i->second->isPerNoteController() && ((cn & ~0xff) == (num & ~0xff)))
                        return i->second;
                  }
            }
      
      for (iMidiController i = defaultMidiController.begin(); i != defaultMidiController.end(); ++i) {
            int cn = i->second->num();
            if (cn == num)
                  return i->second;
            // wildcard?
            if (i->second->isPerNoteController() && ((cn & ~0xff) == (num & ~0xff)))
                  return i->second;
            }
      
      if(!createIfNotFound)
        return NULL;

      QString name = midiCtrlName(num);
      int min = 0;
      int max = 127;
      
      MidiController::ControllerType t = midiControllerType(num);
      switch (t) {
            case MidiController::RPN:
            case MidiController::NRPN:
            case MidiController::Controller7:
            case MidiController::PolyAftertouch:
            case MidiController::Aftertouch:
                  max = 127;
                  break;
            case MidiController::Controller14:
            case MidiController::RPN14:
            case MidiController::NRPN14:
                  max = 16383;
                  break;
            case MidiController::Program:
                  max = 0xffffff;
                  break;
            case MidiController::Pitch:
                  max = 8191;
                  min = -8192;
                  break;
            case MidiController::Velo:        // cannot happen
                  break;
            }
      MidiController* c = new MidiController(name, num, min, max, 0);
      defaultMidiController.add(c);
      return c;
      }

//---------------------------------------------------------
//   drumController
//   Returns instrument drum controller if ctl is a drum controller number.
//   Otherwise returns zero. 
//---------------------------------------------------------

MidiController* MidiPort::drumController(int ctl)
{
  if(!_instrument)
    return 0;
    
  MidiControllerList* cl = _instrument->controller();
  
  // If it's an RPN, NRPN, RPN14, or NRPN14 controller...
  if(((ctl - CTRL_RPN_OFFSET >= 0) && (ctl - CTRL_RPN_OFFSET <= 0xffff)) ||
     ((ctl - CTRL_NRPN_OFFSET >= 0) && (ctl - CTRL_NRPN_OFFSET <= 0xffff)) ||
     ((ctl - CTRL_RPN14_OFFSET >= 0) && (ctl - CTRL_RPN14_OFFSET <= 0xffff)) ||
     ((ctl - CTRL_NRPN14_OFFSET >= 0) && (ctl - CTRL_NRPN14_OFFSET <= 0xffff)) ||
     ((ctl - CTRL_INTERNAL_OFFSET >= 0) && (ctl - CTRL_INTERNAL_OFFSET <= 0xffff)))  // Include internals
  {
    // Does the instrument have a drum controller to match this controller's number?
    iMidiController imc = cl->find(ctl | 0xff);
    if(imc != cl->end())
      // Yes, it's a drum controller. Return a pointer to it.
      return imc->second;  
  }
  
  return 0;
}
            
//---------------------------------------------------------
//   writeRouting    
//---------------------------------------------------------

void MidiPort::writeRouting(int level, Xml& xml) const
{
      // If this device is not actually in use by the song, do not write any routes.
      // This prevents bogus routes from being saved and propagated in the med file.
      // p4.0.17 Reverted. Allow ports with no device to save.
      //if(!device())
      //  return;
     
      QString s;
      
      for (ciRoute r = _outRoutes.begin(); r != _outRoutes.end(); ++r) 
      {
        if(r->type == Route::TRACK_ROUTE && !r->name().isEmpty())
        {
          // Ignore Midi Port to Audio Input routes. Handled by Track route writer. p4.0.14 Tim.
          if(r->track && r->track->type() == Track::AUDIO_INPUT)
            continue;
            
          s = QT_TRANSLATE_NOOP("@default", "Route");
          if(r->channel != -1)
            s += QString(QT_TRANSLATE_NOOP("@default", " channel=\"%1\"")).arg(r->channel);
          xml.tag(level++, s.toLatin1().constData());
          
          xml.tag(level, "source mport=\"%d\"/", portno());
          
          s = QT_TRANSLATE_NOOP("@default", "dest");
          s += QString(QT_TRANSLATE_NOOP("@default", " name=\"%1\"/")).arg(Xml::xmlString(r->name()));
          xml.tag(level, s.toLatin1().constData());
          
          xml.etag(level--, "Route");
        }
      }
}
    
// p4.0.17 Turn off if and when multiple output routes supported.
#if 1
//---------------------------------------------------------
//   setPortExclusiveDefOutChan    
//---------------------------------------------------------

void setPortExclusiveDefOutChan(int port, int c) 
{ 
  if(port < 0 || port >= MIDI_PORTS)
    return;
  MusEGlobal::midiPorts[port].setDefaultOutChannels(c);
  for(int i = 0; i < MIDI_PORTS; ++i)
    if(i != port)
      MusEGlobal::midiPorts[i].setDefaultOutChannels(0);
}
#endif

} // namespace MusECore
