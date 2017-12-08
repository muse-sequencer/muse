//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midiport.cpp,v 1.21.2.15 2009/12/07 20:11:51 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
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

#include <set>

#include <QMenu>
#include <QApplication>

#include "mididev.h"
#include "midiport.h"
#include "midictrl.h"
#include "midi.h"
#include "midiseq.h"
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
#include "muse_math.h"

namespace MusEGlobal {
MusECore::MidiPort midiPorts[MIDI_PORTS];
}

namespace MusECore {

MidiControllerList defaultManagedMidiController;

//LockFreeMultiBuffer<MidiPlayEvent> MidiPort::_eventFifos;
//LockFreeMPSCBuffer<MidiPlayEvent, 16384> MidiPort::_eventBuffers;
LockFreeMPSCRingBuffer<MidiPlayEvent> *MidiPort::_eventBuffers = 
  new LockFreeMPSCRingBuffer<MidiPlayEvent>(16384);

//---------------------------------------------------------
//   initMidiPorts
//---------------------------------------------------------

void initMidiPorts()
      {
      // REMOVE Tim. autoconnect. Added.
      // Create the ring buffers needed for the various threads
      //  wishing to communicate changes to the underlying data 
      //  structures such as controllers.
//       MidiPort::eventFifos().createBuffer(MidiPort::PlayFifo, 4096);
//       MidiPort::eventFifos().createBuffer(MidiPort::GuiFifo, 4096);
//       MidiPort::eventFifos().createBuffer(MidiPort::OSCFifo, 4096);
//       MidiPort::eventFifos().createBuffer(MidiPort::JackFifo, 4096);
//       MidiPort::eventFifos().createBuffer(MidiPort::ALSAFifo, 4096);
      
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
            
            port->changeInstrument(registerMidiInstrument("GM"));
            port->syncInfo().setPort(i);
            // Set the first channel on the first port to auto-connect to midi track outputs.
            if(i == 0)
            {
              port->setDefaultOutChannels(1);
              port->setDefaultInChannels(1);
            }
            }
      }

//---------------------------------------------------------
//   MidiPort
//---------------------------------------------------------

MidiPort::MidiPort()
   : _state("not configured")
      {
      // REMOVE Tim. autoconnect. Added.
//       _gui2AudioFifo = new LockFreeBuffer<Gui2AudioFifoStruct>(512);
//       _osc2AudioFifo = new LockFreeBuffer<MidiPlayEvent>(512);
//       _osc2GuiFifo = new LockFreeBuffer<MidiPlayEvent>(512);
//       _eventFifos = new LockFreeMultiBuffer<Gui2AudioFifoStruct>();
//       _eventFifos->createBuffer(PlayFifo, 512);
//       _eventFifos->createBuffer(GuiFifo, 512);
//       _eventFifos->createBuffer(OSCFifo, 512);
//       _eventFifos->createBuffer(JackFifo, 512);
//       _eventFifos->createBuffer(ALSAFifo, 512);
      
      _initializationsSent = false;  
      _defaultInChannels  = 0;
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
//       if(_eventFifos)
//         delete _eventFifos;
//       delete _osc2GuiFifo;
//       delete _osc2AudioFifo;
//       delete _gui2AudioFifo;
      delete _controller;
      }

//---------------------------------------------------------
//   guiVisible
//---------------------------------------------------------

bool MidiPort::guiVisible() const
      {
      if(!_device)
        return false;
      SynthI* synth = 0;
      if(_device->isSynti())
        synth = static_cast<SynthI*>(_device);
      if(!synth)
        return false;
      return synth->guiVisible();
      }

//---------------------------------------------------------
//   showGui
//---------------------------------------------------------

void MidiPort::showGui(bool v)
{
  if(!_device)
    return;
  SynthI* synth = 0;
  if(_device->isSynti())
    synth = static_cast<SynthI*>(_device);
  if(!synth)
    return;
  synth->showGui(v);
}

//---------------------------------------------------------
//   hasGui
//---------------------------------------------------------

bool MidiPort::hasGui() const
      {
      if(!_device)
        return false;
      SynthI* synth = 0;
      if(_device->isSynti())
        synth = static_cast<SynthI*>(_device);
      if(!synth)
        return false;
      return synth->hasGui();
      }

//---------------------------------------------------------
//   nativeGuiVisible
//---------------------------------------------------------

bool MidiPort::nativeGuiVisible() const
      {
      if(!_device)
        return false;
      SynthI* synth = 0;
      if(_device->isSynti())
        synth = static_cast<SynthI*>(_device);
      if(!synth)
        return false;
      return synth->nativeGuiVisible();
      }

//---------------------------------------------------------
//   showNativeGui
//---------------------------------------------------------

void MidiPort::showNativeGui(bool v)
{
  if(!_device)
    return;
  SynthI* synth = 0;
  if(_device->isSynti())
    synth = static_cast<SynthI*>(_device);
  if(!synth)
    return;
  synth->showNativeGui(v);
}

//---------------------------------------------------------
//   hasNativeGui
//---------------------------------------------------------

bool MidiPort::hasNativeGui() const
      {
      if(!_device)
        return false;
      SynthI* synth = 0;
      if(_device->isSynti())
        synth = static_cast<SynthI*>(_device);
      if(!synth)
        return false;
      return synth->hasNativeGui();
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
                          mp->changeInstrument(genericMidiInstrument);
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
//   To be called from realtime audio thread only.
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

//   unsigned last_tick = 0;
  unsigned last_frame = 0;
  MusECore::MidiInstrument* instr = instrument();
  if(instr && MusEGlobal::config.midiSendInit && (force || !_initializationsSent))
  {
    // Send the Instrument Init sequences.
    EventList* events = instr->midiInit();
    if(!events->empty())
    {
      for(iEvent ie = events->begin(); ie != events->end(); ++ie) 
      {
// REMOVE Tim. autoconnect. Changed.                  
//         const unsigned tick = ie->second.tick();
//         if(tick > last_tick)
//           last_tick = tick;
        //const unsigned int frame = MusEGlobal::tempomap.tick2frame(ie->second.tick()) + MusEGlobal::audio->curSyncFrame();
        //if(frame > last_frame)
        //  last_frame = frame;
        
        if(ie->second.type() == Sysex)
          last_frame += sysexDuration(ie->second.dataLen());
        
// REMOVE Tim. autoconnect. Changed.
//         MusECore::MidiPlayEvent ev(tick, port, 0, ie->second);
//         _device->putEvent(ev);
        //MusECore::MidiPlayEvent ev(frame, port, 0, ie->second);
        MusECore::MidiPlayEvent ev(last_frame + MusEGlobal::audio->curSyncFrame(), port, 0, ie->second);
        //_device->putEvent(ev, MidiDevice::PlayFifo, MidiDevice::NotLate);
        // Let the play events list sort it to be safe.
//         _device->addScheduledEvent(ev);
//         _device->putUserEvent(ev, MidiDevice::NotLate);
        _device->putEvent(ev, MidiDevice::NotLate);
      }
      // Give a bit of time for the last Init sysex to settle?
//       last_tick += 100;
      last_frame += 100;
    }
    _initializationsSent = true; // Mark as having been sent.
  }
    
  // Send the Instrument controller default values.
//   sendInitialControllers(last_tick);
  sendInitialControllers(last_frame);

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
    else if((*imt)->type() == MusECore::Track::NEW_DRUM)
    {
      for(int i = 0; i < DRUM_MAPSIZE; ++i)
      {
        // Default to track port if -1 and track channel if -1.
        int mport = (*imt)->drummap()[i].port;
        if(mport == -1)
          mport = (*imt)->outPort();
        int mchan = (*imt)->drummap()[i].channel;
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
// REMOVE Tim. autoconnect. Changed.
//             // Retry added. Use default attempts and delay. 
//             _device->putEventWithRetry(MidiPlayEvent(start_time, port, chan,  
//               ME_CONTROLLER, ctl, mc->initVal() + mc->bias()));
//             _device->putEvent(MidiPlayEvent(start_time, port, chan,  
//               ME_CONTROLLER, ctl, mc->initVal() + mc->bias()), MidiDevice::PlayFifo, MidiDevice::NotLate);
            _device->putEvent(MidiPlayEvent(start_time, port, chan,
              ME_CONTROLLER, ctl, mc->initVal() + mc->bias()), MidiDevice::NotLate);
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
// REMOVE Tim. autoconnect. Changed.
//         // Retry added. Use default attempts and delay. 
//         _device->putEventWithRetry(MidiPlayEvent(start_time, port, channel,
//           ME_CONTROLLER, cntrl, val));                          
//         _device->putEvent(MidiPlayEvent(start_time, port, channel,
//           ME_CONTROLLER, cntrl, val), MidiDevice::PlayFifo, MidiDevice::NotLate);                          
        //_device->putUserEvent(MidiPlayEvent(start_time, port, channel,
        _device->putEvent(MidiPlayEvent(start_time, port, channel,
          ME_CONTROLLER, cntrl, val), MidiDevice::NotLate);
        // Set it once so the 'last HW value' is set, and control knobs are positioned at the value...
        setHwCtrlState(channel, cntrl, val);
      }
  }
              
  return rv;
}
      
//---------------------------------------------------------
//   changeInstrument
//   If audio is running (and not idle) this should only be called by the rt audio thread.
//---------------------------------------------------------

void MidiPort::changeInstrument(MidiInstrument* i)
{
  if(_instrument == i)
    return;
  _instrument = i;
  _initializationsSent = false;
  updateDrumMaps();
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
            name = QString("%1:%2")
                .arg(port->portno() + 1)
                .arg(port->portname());
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
//   To be called from realtime audio thread only.
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
// REMOVE Tim. autoconnect. Changed.
//         _device->putEventWithRetry(MidiPlayEvent(0, portno(), chan, ME_CONTROLLER, ctl, v));
//         _device->putEvent(MidiPlayEvent(0, portno(), chan, ME_CONTROLLER, ctl, v), MidiDevice::PlayFifo, MidiDevice::NotLate);
        //_device->putUserEvent(MidiPlayEvent(0, portno(), chan, ME_CONTROLLER, ctl, v), MidiDevice::NotLate);
        _device->putEvent(MidiPlayEvent(0, portno(), chan, ME_CONTROLLER, ctl, v), MidiDevice::NotLate);
        
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
// REMOVE Tim. autoconnect. Changed.
//           _device->putEvent(ev);
//         _device->putEvent(ev, MidiDevice::PlayFifo, MidiDevice::NotLate);
        //_device->putUserEvent(ev, MidiDevice::NotLate);
        _device->putEvent(ev, MidiDevice::NotLate);
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
// REMOVE Tim. autoconnect. Changed.
//     _device->putEvent(ev);
//     _device->putEvent(ev, MidiDevice::PlayFifo, MidiDevice::NotLate);
    //_device->putUserEvent(ev, MidiDevice::NotLate);
    _device->putEvent(ev, MidiDevice::NotLate);
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
// REMOVE Tim. autoconnect. Changed.
//            _device->putEvent(event);
//            _device->putEvent(event, MidiDevice::PlayFifo, MidiDevice::NotLate);
           //_device->putUserEvent(event, MidiDevice::NotLate);
           _device->putEvent(event, MidiDevice::NotLate);
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
// REMOVE Tim. autoconnect. Changed.
//            _device->putEvent(event);
//            _device->putEvent(event, MidiDevice::PlayFifo, MidiDevice::NotLate);
           //_device->putUserEvent(event, MidiDevice::NotLate);
           _device->putEvent(event, MidiDevice::NotLate);
            }
      }

//---------------------------------------------------------
//   sendStop
//---------------------------------------------------------

void MidiPort::sendStop()
      {
      if (_device) {
            MidiPlayEvent event(0, 0, 0, ME_STOP, 0, 0);
// REMOVE Tim. autoconnect. Changed.
//            _device->putEvent(event);
//            _device->putEvent(event, MidiDevice::PlayFifo, MidiDevice::NotLate);
           //_device->putUserEvent(event, MidiDevice::NotLate);
           _device->putEvent(event, MidiDevice::NotLate);
            }
      }

//---------------------------------------------------------
//   sendClock
//---------------------------------------------------------

void MidiPort::sendClock()
      {
      if (_device) {
            MidiPlayEvent event(0, 0, 0, ME_CLOCK, 0, 0);
// REMOVE Tim. autoconnect. Changed.
//            _device->putEvent(event);
//            _device->putEvent(event, MidiDevice::PlayFifo, MidiDevice::NotLate);
           _device->putEvent(event, MidiDevice::NotLate);
            }
      }

//---------------------------------------------------------
//   sendContinue
//---------------------------------------------------------

void MidiPort::sendContinue()
      {
      if (_device) {
            MidiPlayEvent event(0, 0, 0, ME_CONTINUE, 0, 0);
// REMOVE Tim. autoconnect. Changed.
//            _device->putEvent(event);
//            _device->putEvent(event, MidiDevice::PlayFifo, MidiDevice::NotLate);
           _device->putEvent(event, MidiDevice::NotLate);
            }
      }

//---------------------------------------------------------
//   sendSongpos
//---------------------------------------------------------

void MidiPort::sendSongpos(int pos)
      {
      if (_device) {
            MidiPlayEvent event(0, 0, 0, ME_SONGPOS, pos, 0);
// REMOVE Tim. autoconnect. Changed.
//            _device->putEvent(event);
//            _device->putEvent(event, MidiDevice::PlayFifo, MidiDevice::NotLate);
           _device->putEvent(event, MidiDevice::NotLate);
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

double MidiPort::limitValToInstrCtlRange(MidiController* mc, double val)
{
  if(!_instrument || !mc || int(val) == CTRL_VAL_UNKNOWN)
    return val;

  const double mn = double(mc->minVal());
  const double mx = double(mc->maxVal());
  const double bias = double(mc->bias());

  // Subtract controller bias from value.
  val -= double(bias);

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

double MidiPort::limitValToInstrCtlRange(int ctl, double val)
{
  if(!_instrument || int(val) == CTRL_VAL_UNKNOWN)
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

// REMOVE Tim. autoconnect. Changed.
// //---------------------------------------------------------
// //   stageEvent
// //   Prepares an event for putting into the gui2audio fifo.
// //   To be called from gui thread only.
// //   Returns true if the event was staged.
// //---------------------------------------------------------
// 
// bool MidiPort::stageEvent(MidiPlayEvent& dst, const MidiPlayEvent& src)
// {
//   PendingOperationList operations;
// 
//   const int chn = src.channel();
//   int ctrl = -1;
// 
//   if (src.type() == ME_CONTROLLER)
//   {
//         ctrl = src.dataA();
//         int db = src.dataB();
// 
//         // TODO: Maybe update CTRL_HBANK/CTRL_LBANK - but ONLY if they are specifically
//         //        defined by the user in the controller list.
//         if(ctrl == CTRL_HBANK)
//         {
//           ctrl = CTRL_PROGRAM;
//           dst = src;
//         }
//         else if(ctrl == CTRL_LBANK)
//         {
//           ctrl = CTRL_PROGRAM;
//           dst = src;
//         }
//         else if(ctrl == CTRL_PROGRAM)
//         {
//           dst = src;
//         }
//         else
//         {
//           db = limitValToInstrCtlRange(ctrl, db);
//           dst = MidiPlayEvent(src.time(), src.port(), chn, src.type(), ctrl, db);
//         }
//   }
//   else
//   if (src.type() == ME_POLYAFTER)
//   {
//         const int pitch = src.dataA() & 0x7f;
//         ctrl = (CTRL_POLYAFTER & ~0xff) | pitch;
//         const int db = limitValToInstrCtlRange(ctrl, src.dataB());
//         dst = MidiPlayEvent(src.time(), src.port(), chn, src.type(), ctrl, db);
//   }
//   else
//   if (src.type() == ME_AFTERTOUCH)
//   {
//         ctrl = CTRL_AFTERTOUCH;
//         const int da = limitValToInstrCtlRange(ctrl, src.dataA());
//         dst = MidiPlayEvent(src.time(), src.port(), chn, src.type(), ctrl, da);
//   }
//   else
//   if (src.type() == ME_PITCHBEND)
//   {
//         ctrl = CTRL_PITCH;
//         const int da = limitValToInstrCtlRange(ctrl, src.dataA());
//         dst = MidiPlayEvent(src.time(), src.port(), chn, src.type(), ctrl, da);
//   }
//   else
//   if (src.type() == ME_PROGRAM)
//   {
//     ctrl = CTRL_PROGRAM;
//     dst = src;
//   }
//   else
//     return false;
// 
//   // Make sure the controller exists, create it if not.
//   if(ctrl != -1)
//   {
//     iMidiCtrlValList cl = _controller->find(chn, ctrl);
//     if(cl == _controller->end())
//     {
//       PendingOperationItem poi(_controller, 0, chn, ctrl, PendingOperationItem::AddMidiCtrlValList);
//       if(operations.findAllocationOp(poi) == operations.end())
//       {
//         MidiCtrlValList* mcvl = new MidiCtrlValList(ctrl);
//         poi._mcvl = mcvl;
//         operations.add(poi);
//         // This waits for audio process thread to execute it.
//         MusEGlobal::audio->msgExecutePendingOperations(operations, true);
//       }
//     }
//   }
// 
//   return true;
// }

//---------------------------------------------------------
// stageEvent
// Prepares an event for putting into the gui2audio fifo.
// To be called from gui thread only. 
// Returns a valid source controller number (above zero), for the purpose of
//  the caller creating the controller with createController() if necessary. 
// Otherwise returns -1.
//---------------------------------------------------------

// int MidiPort::stageEvent(MidiPlayEvent& dst, const MidiPlayEvent& src)
// {
//   const int chn = src.channel();
// //   if(chn < 0 || chn >= MIDI_CHANNELS)
// //     return -1;
// 
//   int ctrl = -1;
// 
//   if (src.type() == ME_CONTROLLER)
//   {
//         ctrl = src.dataA();
//         int db = src.dataB();
// 
//         // TODO: Maybe update CTRL_HBANK/CTRL_LBANK - but ONLY if they are specifically
//         //        defined by the user in the controller list.
//         if(ctrl == CTRL_HBANK)
//         {
//           ctrl = CTRL_PROGRAM;
//           dst = src;
//         }
//         else if(ctrl == CTRL_LBANK)
//         {
//           ctrl = CTRL_PROGRAM;
//           dst = src;
//         }
//         else if(ctrl == CTRL_PROGRAM)
//         {
//           dst = src;
//         }
//         else
//         {
//           db = limitValToInstrCtlRange(ctrl, db);
//           dst = MidiPlayEvent(src.time(), src.port(), chn, src.type(), ctrl, db);
//         }
//   }
//   else
//   if (src.type() == ME_POLYAFTER)
//   {
//         const int pitch = src.dataA() & 0x7f;
//         ctrl = (CTRL_POLYAFTER & ~0xff) | pitch;
//         const int db = limitValToInstrCtlRange(ctrl, src.dataB());
//         dst = MidiPlayEvent(src.time(), src.port(), chn, src.type(), ctrl, db);
//   }
//   else
//   if (src.type() == ME_AFTERTOUCH)
//   {
//         ctrl = CTRL_AFTERTOUCH;
//         const int da = limitValToInstrCtlRange(ctrl, src.dataA());
//         dst = MidiPlayEvent(src.time(), src.port(), chn, src.type(), ctrl, da);
//   }
//   else
//   if (src.type() == ME_PITCHBEND)
//   {
//         ctrl = CTRL_PITCH;
//         const int da = limitValToInstrCtlRange(ctrl, src.dataA());
//         dst = MidiPlayEvent(src.time(), src.port(), chn, src.type(), ctrl, da);
//   }
//   else
//   if (src.type() == ME_PROGRAM)
//   {
//     ctrl = CTRL_PROGRAM;
//     dst = src;
//   }
//   else
//     dst = src;
// 
//   return ctrl;
// }

MidiPlayEvent MidiPort::stageEvent(const MidiPlayEvent& ev)
{
      const int type = ev.type();
      const int chn = ev.channel();
      const int i_dataA = ev.dataA();
//       int fin_da = i_dataA;
      const int i_dataB = ev.dataB();
      int fin_db = i_dataB;
 
//   const int chn = g2as._chan;
//   const int type = g2as._type;
  
//   const int i_dataA = g2as._dataA;
  
//   const double d_dataB = g2as._dataB;
//   const int i_dataB = MidiController::dValToInt(d_dataB);

//   int ctrl = -1;
//   int i_fin_val = 0;


  //const int type = ev.type();
  //const int chn = ev.channel();
  //const int da = ev.dataA();
  //int fin_da = i_dataA;
  //const int db = ev.dataB();
  //int fin_db = i_dataB;
  switch(type)
  {
    case ME_CONTROLLER:
      switch(i_dataA)
      {
        case CTRL_HBANK:
        {
          int hb = 0xff;
          if(!MidiController::iValIsUnknown(i_dataB))
            hb = i_dataB & 0xff;
          if(hb != 0xff)
            hb = limitValToInstrCtlRange(i_dataA, hb);
          int lb = 0xff;
          int pr = 0xff;
          
          iMidiCtrlValList imcvl = _controller->find(chn, CTRL_PROGRAM);
          // Does the CTRL_PROGRAM controller exist?
          if(imcvl == _controller->end())
            //return MidiPlayEvent();
            //return true;
            break;
          
          MidiCtrlValList* mcvl = imcvl->second;
          if(!mcvl->hwValIsUnknown())
          {
            const int hw_val = mcvl->hwVal();
            lb = (hw_val >> 8) & 0xff;
            pr = hw_val & 0xff;
          }
          
          if((hb != 0xff || lb != 0xff) && pr == 0xff)
            pr = 0x01;
          if(hb == 0xff && lb == 0xff && pr == 0xff)
            fin_db = CTRL_VAL_UNKNOWN;
          else
            fin_db = (hb << 16) | (lb << 8) | pr;
          
//           // Set the value. Be sure to update drum maps (and inform the gui).
//           if(mcvl->setHwVal(fin_db))
//             updateDrumMaps(chn, fin_db);
          
          return MidiPlayEvent(ev.time(), ev.port(), chn, ME_CONTROLLER, CTRL_PROGRAM, fin_db);
          //return false;
        }
        break;

        case CTRL_LBANK:
        {
          int hb = 0xff;
          int lb = 0xff;
          if(!MidiController::iValIsUnknown(i_dataB))
            lb = i_dataB & 0xff;
          if(lb != 0xff)
            lb = limitValToInstrCtlRange(i_dataA, lb);
          int pr = 0xff;
          
          iMidiCtrlValList imcvl = _controller->find(chn, CTRL_PROGRAM);
          // Does the CTRL_PROGRAM controller exist?
          if(imcvl == _controller->end())
            //return MidiPlayEvent();
            //return true;
            break;
          
          MidiCtrlValList* mcvl = imcvl->second;
          if(!mcvl->hwValIsUnknown())
          {
            const int hw_val = mcvl->hwVal();
            hb = (hw_val >> 16) & 0xff;
            pr = hw_val & 0xff;
          }
          
          if((hb != 0xff || lb != 0xff) && pr == 0xff)
            pr = 0x01;
          if(hb == 0xff && lb == 0xff && pr == 0xff)
            fin_db = CTRL_VAL_UNKNOWN;
          else
            fin_db = (hb << 16) | (lb << 8) | pr;
          
//           // Set the value. Be sure to update drum maps (and inform the gui).
//           if(mcvl->setHwVal(fin_db))
//             updateDrumMaps(chn, fin_db);
          
          return MidiPlayEvent(ev.time(), ev.port(), chn, ME_CONTROLLER, CTRL_PROGRAM, fin_db);
          //return false;
        }
        break;

        case CTRL_PROGRAM:
        {
          // TODO: Maybe update CTRL_HBANK/CTRL_LBANK - but ONLY if they are specifically
          //        defined by the user in the controller list.
            
          iMidiCtrlValList imcvl = _controller->find(chn, CTRL_PROGRAM);
          // Does the CTRL_PROGRAM controller exist?
          if(imcvl == _controller->end())
            //return MidiPlayEvent();
            //return true;
            break;
          
//           // Set the value. Be sure to update drum maps (and inform the gui).
//           if(imcvl->second->setHwVal(fin_db))
//             updateDrumMaps(chn, fin_db);
          
          return MidiPlayEvent(ev.time(), ev.port(), chn, ME_CONTROLLER, CTRL_PROGRAM, fin_db);
          //return false;
        }
        break;
        
        default:
        {
          iMidiCtrlValList imcvl = _controller->find(chn, i_dataA);
          // Does the controller exist?
          if(imcvl == _controller->end())
            //return MidiPlayEvent();
            //return true;
            break;

          fin_db = limitValToInstrCtlRange(i_dataA, i_dataB);
//           // Set the value.
//           imcvl->second->setHwVal(fin_db);
          
          return MidiPlayEvent(ev.time(), ev.port(), chn, ME_CONTROLLER, i_dataA, fin_db);
          //return false;
        }
        break;
      }
    break;
    
    case ME_POLYAFTER:
    {
      const int pitch = i_dataA & 0x7f;
      const int fin_da = (CTRL_POLYAFTER & ~0xff) | pitch;
      iMidiCtrlValList imcvl = _controller->find(chn, fin_da);
      
      // Does the controller exist?
      if(imcvl == _controller->end())
        //return MidiPlayEvent();
        //return true;
        break;

      fin_db = limitValToInstrCtlRange(fin_da, i_dataB);
//       // Set the value.
//       imcvl->second->setHwVal(fin_db);
      
      return MidiPlayEvent(ev.time(), ev.port(), chn, ME_CONTROLLER, fin_da, fin_db);
      //return false;
    }
    break;
    
    case ME_AFTERTOUCH:
    {
      iMidiCtrlValList imcvl = _controller->find(chn, CTRL_AFTERTOUCH);
      // Does the controller exist?
      if(imcvl == _controller->end())
        //return MidiPlayEvent();
        //return true;
        break;
      
      fin_db = limitValToInstrCtlRange(CTRL_AFTERTOUCH, i_dataA);
//       // Set the value.
//       imcvl->second->setHwVal(fin_db);
      
      return MidiPlayEvent(ev.time(), ev.port(), chn, ME_CONTROLLER, CTRL_AFTERTOUCH, fin_db);
      //return false;
    }
    break;
    
    case ME_PITCHBEND:
    {
      iMidiCtrlValList imcvl = _controller->find(chn, CTRL_PITCH);
      // Does the controller exist?
      if(imcvl == _controller->end())
        //return MidiPlayEvent();
        //return true;
        break;
      
      fin_db = limitValToInstrCtlRange(CTRL_PITCH, i_dataA);
//       // Set the value.
//       imcvl->second->setHwVal(fin_db);
      
      return MidiPlayEvent(ev.time(), ev.port(), chn, ME_CONTROLLER, CTRL_PITCH, fin_db);
      //return false;
    }
    break;
    
    case ME_PROGRAM:
    {
      int hb = 0xff;
      int lb = 0xff;
      int pr = 0xff;
      if(!MidiController::iValIsUnknown(i_dataA))
        pr = i_dataA & 0xff;
      //if(pr != 0xff)
      //  pr = limitValToInstrCtlRange(da, pr);
      
      iMidiCtrlValList imcvl = _controller->find(chn, CTRL_PROGRAM);
      // Does the CTRL_PROGRAM controller exist?
      if(imcvl == _controller->end())
        //return MidiPlayEvent();
        //return true;
        break;
      
      MidiCtrlValList* mcvl = imcvl->second;
      if(!mcvl->hwValIsUnknown())
      {
        const int hw_val = mcvl->hwVal();
        hb = (hw_val >> 16) & 0xff;
        lb = (hw_val >> 8) & 0xff;
      }
      
      if((hb != 0xff || lb != 0xff) && pr == 0xff)
        pr = 0x01;
      if(hb == 0xff && lb == 0xff && pr == 0xff)
        fin_db = CTRL_VAL_UNKNOWN;
      else
        fin_db = (hb << 16) | (lb << 8) | pr;
      
//       // Set the value. Be sure to update drum maps (and inform the gui).
//       if(mcvl->setHwVal(fin_db))
//         updateDrumMaps(chn, fin_db);
      
      return MidiPlayEvent(ev.time(), ev.port(), chn, ME_CONTROLLER, CTRL_PROGRAM, fin_db);
      //return false;
    }
    break;
    
    default:
      // For all other event types, just return true.
      //return MidiPlayEvent(g2as._time, g2as._port, chn, type, i_dataA, i_dataB);
      //return true;
    break;
  }
  
  return MidiPlayEvent(ev.time(), ev.port(), chn, type, i_dataA, i_dataB);
  //return true;
      
      
      
      
      
      
      
//       switch(type)
//       {
//         case ME_CONTROLLER:
//           switch(da)
//           {
//             case CTRL_HBANK:
//             {
//               int hb = 0xff;
//               if(!MidiController::iValIsUnknown(db))
//                 hb = db & 0xff;
//               if(hb != 0xff)
//                 hb = limitValToInstrCtlRange(da, hb);
//               int lb = 0xff;
//               int pr = 0xff;
//               
//               fin_da = CTRL_PROGRAM;
//               // This will create a new value list if necessary, otherwise it returns the existing list.
//               // FIXME: This is not realtime safe because it may allocate.
//               MidiCtrlValList* mcvl = addManagedController(chn, fin_da);
//               if(!mcvl->hwValIsUnknown())
//               {
//                 const int hw_val = mcvl->hwVal();
//                 lb = (hw_val >> 8) & 0xff;
//                 pr = hw_val & 0xff;
//               }
//               
//               if((hb != 0xff || lb != 0xff) && pr == 0xff)
//                 pr = 0x01;
//               if(hb == 0xff && lb == 0xff && pr == 0xff)
//                 fin_db = CTRL_VAL_UNKNOWN;
//               else
//                 fin_db = (hb << 16) | (lb << 8) | pr;
//             }
//             break;
// 
//             case CTRL_LBANK:
//             {
//               int hb = 0xff;
//               int lb = 0xff;
//               if(!MidiController::iValIsUnknown(db))
//                 lb = db & 0xff;
//               if(lb != 0xff)
//                 lb = limitValToInstrCtlRange(da, lb);
//               int pr = 0xff;
//               
//               fin_da = CTRL_PROGRAM;
//               // This will create a new value list if necessary, otherwise it returns the existing list.
//               // FIXME: This is not realtime safe because it may allocate.
//               MidiCtrlValList* mcvl = addManagedController(chn, fin_da);
//               if(!mcvl->hwValIsUnknown())
//               {
//                 const int hw_val = mcvl->hwVal();
//                 hb = (hw_val >> 16) & 0xff;
//                 pr = hw_val & 0xff;
//               }
//               
//               if((hb != 0xff || lb != 0xff) && pr == 0xff)
//                 pr = 0x01;
//               if(hb == 0xff && lb == 0xff && pr == 0xff)
//                 fin_db = CTRL_VAL_UNKNOWN;
//               else
//                 fin_db = (hb << 16) | (lb << 8) | pr;
//             }
//             break;
// 
//             case CTRL_PROGRAM:
//               // This will create a new value list if necessary, otherwise it returns the existing list.
//               // FIXME: This is not realtime safe because it may allocate.
//               addManagedController(chn, da);
//               // TODO: Maybe update CTRL_HBANK/CTRL_LBANK - but ONLY if they are specifically
//               //        defined by the user in the controller list.
//             break;
//             
//             default:
//               // This will create a new value list if necessary, otherwise it returns the existing list.
//               // FIXME: This is not realtime safe because it may allocate.
//               addManagedController(chn, da);
//               fin_db = limitValToInstrCtlRange(da, db);
//             break;
//           }
//         break;
//         
//         case ME_POLYAFTER:
//         {
//           const int pitch = da & 0x7f;
//           fin_da = (CTRL_POLYAFTER & ~0xff) | pitch;
//           // This will create a new value list if necessary, otherwise it returns the existing list.
//           // FIXME: This is not realtime safe because it may allocate.
//           addManagedController(chn, fin_da);
//           fin_db = limitValToInstrCtlRange(fin_da, db);
//         }
//         break;
//         
//         case ME_AFTERTOUCH:
//         {
//           fin_da = CTRL_AFTERTOUCH;
//           // This will create a new value list if necessary, otherwise it returns the existing list.
//           // FIXME: This is not realtime safe because it may allocate.
//           addManagedController(chn, fin_da);
//           fin_db = limitValToInstrCtlRange(fin_da, da);
//         }
//         break;
//         
//         case ME_PITCHBEND:
//         {
//           fin_da = CTRL_PITCH;
//           // This will create a new value list if necessary, otherwise it returns the existing list.
//           // FIXME: This is not realtime safe because it may allocate.
//           addManagedController(chn, fin_da);
//           fin_db = limitValToInstrCtlRange(fin_da, da);
//         }
//         break;
//         
//         case ME_PROGRAM:
//         {
//           int hb = 0xff;
//           int lb = 0xff;
//           int pr = 0xff;
//           if(!MidiController::iValIsUnknown(da))
//             pr = da & 0xff;
//           //if(pr != 0xff)
//           //  pr = limitValToInstrCtlRange(da, pr);
//           
//           fin_da = CTRL_PROGRAM;
//           // This will create a new value list if necessary, otherwise it returns the existing list.
//           // FIXME: This is not realtime safe because it may allocate.
//           MidiCtrlValList* mcvl = addManagedController(chn, fin_da);
//           if(!mcvl->hwValIsUnknown())
//           {
//             const int hw_val = mcvl->hwVal();
//             hb = (hw_val >> 16) & 0xff;
//             lb = (hw_val >> 8) & 0xff;
//           }
//           
//           if((hb != 0xff || lb != 0xff) && pr == 0xff)
//             pr = 0x01;
//           if(hb == 0xff && lb == 0xff && pr == 0xff)
//             fin_db = CTRL_VAL_UNKNOWN;
//           else
//             fin_db = (hb << 16) | (lb << 8) | pr;
//         }
//         break;
//         
//         default:
//           // For all other event types, just return true.
//           return true;
//         break;
//       }
//       
//       if(!setHwCtrlState(chn, da, db)) {
//           if (MusEGlobal::debugMsg && forceSend)
//             printf("sendHwCtrlState: State already set. Forcing anyway...\n");
//           if (!forceSend)
//             return false;
//         }
//         
//       return true;
  
  
  
  
  
  
// //   if(chn < 0 || chn >= MIDI_CHANNELS)
// //     return -1;
// 
//   int ctrl = -1;
//   MidiPlayEvent dst;
//   
//   const int type = ev.type();
//   const int chn = ev.channel();
//   const int da = ev.dataA();
//   int fin_da = da;
//   const int db = ev.dataB();
//   int fin_db = db;
//   
// 
//   switch(type)
//   {
//     case ME_CONTROLLER:
//       switch(da)
//       {
//         case CTRL_HBANK:
//         {
//           dst = ev;
//           ctrl = CTRL_PROGRAM;
//           
// //           int hb = 0xff;
// //           if(!MidiController::iValIsUnknown(db))
// //             hb = db & 0xff;
// //           if(hb != 0xff)
// //             hb = limitValToInstrCtlRange(da, hb);
// //           int lb = 0xff;
// //           int pr = 0xff;
// //           
// //           fin_da = CTRL_PROGRAM;
// //           // This will create a new value list if necessary, otherwise it returns the existing list.
// //           // FIXME: This is not realtime safe because it may allocate.
// //           MidiCtrlValList* mcvl = addManagedController(chn, fin_da);
// //           if(!mcvl->hwValIsUnknown())
// //           {
// //             const int hw_val = mcvl->hwVal();
// //             lb = (hw_val >> 8) & 0xff;
// //             pr = hw_val & 0xff;
// //           }
// //           
// //           if((hb != 0xff || lb != 0xff) && pr == 0xff)
// //             pr = 0x01;
// //           if(hb == 0xff && lb == 0xff && pr == 0xff)
// //             fin_db = CTRL_VAL_UNKNOWN;
// //           else
// //             fin_db = (hb << 16) | (lb << 8) | pr;
//         }
//         break;
// 
//         case CTRL_LBANK:
//         {
//           dst = ev;
//           ctrl = CTRL_PROGRAM;
//           
// //           int hb = 0xff;
// //           int lb = 0xff;
// //           if(!MidiController::iValIsUnknown(db))
// //             lb = db & 0xff;
// //           if(lb != 0xff)
// //             lb = limitValToInstrCtlRange(da, lb);
// //           int pr = 0xff;
// //           
// //           fin_da = CTRL_PROGRAM;
// //           // This will create a new value list if necessary, otherwise it returns the existing list.
// //           // FIXME: This is not realtime safe because it may allocate.
// //           MidiCtrlValList* mcvl = addManagedController(chn, fin_da);
// //           if(!mcvl->hwValIsUnknown())
// //           {
// //             const int hw_val = mcvl->hwVal();
// //             hb = (hw_val >> 16) & 0xff;
// //             pr = hw_val & 0xff;
// //           }
// //           
// //           if((hb != 0xff || lb != 0xff) && pr == 0xff)
// //             pr = 0x01;
// //           if(hb == 0xff && lb == 0xff && pr == 0xff)
// //             fin_db = CTRL_VAL_UNKNOWN;
// //           else
// //             fin_db = (hb << 16) | (lb << 8) | pr;
//         }
//         break;
// 
//         case CTRL_PROGRAM:
//           // This will create a new value list if necessary, otherwise it returns the existing list.
//           // FIXME: This is not realtime safe because it may allocate.
//           //addManagedController(chn, da);
//           // TODO: Maybe update CTRL_HBANK/CTRL_LBANK - but ONLY if they are specifically
//           //        defined by the user in the controller list.
//           dst = ev;
//           ctrl = CTRL_PROGRAM;
//         break;
//         
//         default:
//           // This will create a new value list if necessary, otherwise it returns the existing list.
//           // FIXME: This is not realtime safe because it may allocate.
//           //addManagedController(chn, da);
//           //fin_db = limitValToInstrCtlRange(da, db);
//           //dst = MidiPlayEvent(src.time(), src.port(), chn, src.type(), ctrl, db);
//           dst = ev;
//           ctrl = da;
//         break;
//       }
//     break;
//     
//     case ME_POLYAFTER:
//     {
//       const int pitch = da & 0x7f;
//       ctrl = (CTRL_POLYAFTER & ~0xff) | pitch;
// //       // This will create a new value list if necessary, otherwise it returns the existing list.
// //       // FIXME: This is not realtime safe because it may allocate.
// //       addManagedController(chn, fin_da);
// //       fin_db = limitValToInstrCtlRange(fin_da, db);
//         dst = MidiPlayEvent(ev.time(), ev.port(), chn, type, ctrl, db);
//     }
//     break;
//     
//     case ME_AFTERTOUCH:
//     {
//       ctrl = CTRL_AFTERTOUCH;
// //       // This will create a new value list if necessary, otherwise it returns the existing list.
// //       // FIXME: This is not realtime safe because it may allocate.
// //       addManagedController(chn, fin_da);
// //       fin_db = limitValToInstrCtlRange(fin_da, da);
//         dst = MidiPlayEvent(ev.time(), ev.port(), chn, type, ctrl, da);
//     }
//     break;
//     
//     case ME_PITCHBEND:
//     {
//       ctrl = CTRL_PITCH;
// //       // This will create a new value list if necessary, otherwise it returns the existing list.
// //       // FIXME: This is not realtime safe because it may allocate.
// //       addManagedController(chn, fin_da);
// //       fin_db = limitValToInstrCtlRange(fin_da, da);
// // //       dst = MidiPlayEvent(ev.time(), ev.port(), chn, ME_PITCHBEND, v & 0x7f, (v >> 7) & 0x7f), evBuffer))
// // //       if (event->type() == ME_PITCHBEND) {
// // //             int val = (event->dataB() << 7) + event->dataA();
// // //             val -= 8192;
// // //             event->setA(val);
// // //             }
//       dst.setA(((db << 7) + da) - 8192);
//     }
//     break;
//     
//     case ME_PROGRAM:
//     {
//       dst = src;
//       ctrl = CTRL_PROGRAM;
//       
// //       int hb = 0xff;
// //       int lb = 0xff;
// //       int pr = 0xff;
// //       if(!MidiController::iValIsUnknown(da))
// //         pr = da & 0xff;
// //       //if(pr != 0xff)
// //       //  pr = limitValToInstrCtlRange(da, pr);
// //       
// //       fin_da = CTRL_PROGRAM;
// //       // This will create a new value list if necessary, otherwise it returns the existing list.
// //       // FIXME: This is not realtime safe because it may allocate.
// //       MidiCtrlValList* mcvl = addManagedController(chn, fin_da);
// //       if(!mcvl->hwValIsUnknown())
// //       {
// //         const int hw_val = mcvl->hwVal();
// //         hb = (hw_val >> 16) & 0xff;
// //         lb = (hw_val >> 8) & 0xff;
// //       }
// //       
// //       if((hb != 0xff || lb != 0xff) && pr == 0xff)
// //         pr = 0x01;
// //       if(hb == 0xff && lb == 0xff && pr == 0xff)
// //         fin_db = CTRL_VAL_UNKNOWN;
// //       else
// //         fin_db = (hb << 16) | (lb << 8) | pr;
//     }
//     break;
//     
//     default:
//       // For all other event types, just return ctrl = -1.
//       dst = src;
//     break;
//   }
//   
//   return ctrl;
//       
// 
// 
// 
//   
//   
//   if (src.type() == ME_CONTROLLER)
//   {
//         ctrl = src.dataA();
//         int db = src.dataB();
// 
//         // TODO: Maybe update CTRL_HBANK/CTRL_LBANK - but ONLY if they are specifically
//         //        defined by the user in the controller list.
//         if(ctrl == CTRL_HBANK)
//         {
//           ctrl = CTRL_PROGRAM;
//           dst = src;
//         }
//         else if(ctrl == CTRL_LBANK)
//         {
//           ctrl = CTRL_PROGRAM;
//           dst = src;
//         }
//         else if(ctrl == CTRL_PROGRAM)
//         {
//           dst = src;
//         }
//         else
//         {
//           db = limitValToInstrCtlRange(ctrl, db);
//           dst = MidiPlayEvent(src.time(), src.port(), chn, src.type(), ctrl, db);
//         }
//   }
//   else
//   if (src.type() == ME_POLYAFTER)
//   {
//         const int pitch = src.dataA() & 0x7f;
//         ctrl = (CTRL_POLYAFTER & ~0xff) | pitch;
//         const int db = limitValToInstrCtlRange(ctrl, src.dataB());
//         dst = MidiPlayEvent(src.time(), src.port(), chn, src.type(), ctrl, db);
//   }
//   else
//   if (src.type() == ME_AFTERTOUCH)
//   {
//         ctrl = CTRL_AFTERTOUCH;
//         const int da = limitValToInstrCtlRange(ctrl, src.dataA());
//         dst = MidiPlayEvent(src.time(), src.port(), chn, src.type(), ctrl, da);
//   }
//   else
//   if (src.type() == ME_PITCHBEND)
//   {
//         ctrl = CTRL_PITCH;
//         const int da = limitValToInstrCtlRange(ctrl, src.dataA());
//         dst = MidiPlayEvent(src.time(), src.port(), chn, src.type(), ctrl, da);
//   }
//   else
//   if (src.type() == ME_PROGRAM)
//   {
//     ctrl = CTRL_PROGRAM;
//     dst = src;
//   }
//   else
//     dst = src;
// 
//   return ctrl;
}

//---------------------------------------------------------
//  createController
//   Creates a controller in this port's controller list.
//   Returns true if the controller was created.
//   To be called by gui thread only.
//---------------------------------------------------------

bool MidiPort::createController(int chan, int ctrl)
{
  if(ctrl < 0 || chan < 0 || chan >= MIDI_CHANNELS)
    return false;
    
  PendingOperationList operations;

  // Make sure the controller exists, create it if not.
  iMidiCtrlValList cl = _controller->find(chan, ctrl);
  if(cl != _controller->end())
    return false;
  
  PendingOperationItem poi(_controller, 0, chan, ctrl, PendingOperationItem::AddMidiCtrlValList);
  
  // This step is intended in case we ever pass an operations list to this function.
  if(operations.findAllocationOp(poi) != operations.end())
    return false;
  
  MidiCtrlValList* mcvl = new MidiCtrlValList(ctrl);
  poi._mcvl = mcvl;
  operations.add(poi);
  // This waits for audio process thread to execute it.
  MusEGlobal::audio->msgExecutePendingOperations(operations, true);

  return true;
}

//---------------------------------------------------------
//   putHwCtrlEvent
//   To be called from gui thread only.
//   Returns true if event cannot be delivered.
//---------------------------------------------------------

bool MidiPort::putHwCtrlEvent(const MidiPlayEvent& ev)
{
//   const int chan = ev.channel();
//   if(chan < 0 || chan >= MIDI_CHANNELS)
//     return true;
//   
//   // Stage the event.
//   MidiPlayEvent staged_ev;
//   const int ctrl = stageEvent(staged_ev, ev);
//   if(ctrl < 0)
//     return true;
  
  const int ctrl = ev.translateCtrlNum();

  // Event not translatable to a controller?
  if(ctrl < 0)
    return true;

//   // Make sure to create the controller if necessary.
//   createController(chan, ctrl);
  
  // Make sure the controller exists, create it if not.
  const int chan = ev.channel();
  ciMidiCtrlValList cl = _controller->find(chan, ctrl);
  // Controller does not exist?
  if(cl == _controller->end())
  {
    // Tell the gui thread to create and add a new controller.
    // It will store and re-deliver the events directly to the buffers
    //  after the controller is created.
    MusEGlobal::song->putIpcInEvent(ev);
    // Technically the event is being delivered.
    return false;
  }
  
//   if(_gui2AudioFifo->put(Gui2AudioFifoStruct(staged_ev)))
//   if(_gui2AudioFifo->put(Gui2AudioFifoStruct(ev)))
//   if(eventFifos().put(GuiFifo, Gui2AudioFifoStruct(ev)))
//   if(eventFifos().put(GuiFifo, ev))
  if(!eventBuffers()->put(ev))
  {
    fprintf(stderr, "MidiPort::putHwCtrlEvent: Error: gui2AudioFifo fifo overflow\n");
    return true;
  }
  
  return false;
}

// REMOVE Tim. autoconnect. Changed.
// //---------------------------------------------------------
// //   putEvent
// //   To be called from gui thread only.
// //   Returns true if event cannot be delivered.
// //---------------------------------------------------------
// 
// bool MidiPort::putEvent(const MidiPlayEvent& ev)
// {
//   const int chan = ev.channel();
// 
//   // Stage the event.
//   //MidiPlayEvent staged_ev;
// //   const int ctrl = stageEvent(staged_ev, ev);
//   const int ctrl = ev.translateCtrlNum();
//   // Event translatable to a controller?
//   if(ctrl >= 0)
//     // Make sure to create the controller if necessary.
//     createController(chan, ctrl);
//   
//   
//   // Send the event to the device first so that current parameters could be updated on process.
// //   MidiPlayEvent staged_ev;
// //   const bool stage_res = stageEvent(staged_ev, ev);
//   bool res = false;
//   if(_device)
//     // FIXME: Concurrency with putOSCEvent(). Need putOSCEvent() etc.
// //     res = _device->putEvent(ev);
// //     res = _device->putEvent(staged_ev);
// //     res = _device->putEvent(stageEvent(ev));
//     res = _device->eventFifos()->put(MidiDevice::GuiFifo, ev);
// //   if(ctrl >= 0 && _gui2AudioFifo->put(Gui2AudioFifoStruct(staged_ev)))
// //   if(ctrl >= 0 && _gui2AudioFifo->put(Gui2AudioFifoStruct(ev)))
// //   if(ctrl >= 0 && eventFifos().put(GuiFifo, Gui2AudioFifoStruct(ev)))
// //   if(eventFifos().put(GuiFifo, Gui2AudioFifoStruct(ev)))
//   if(eventFifos().put(GuiFifo, ev))
// //     fprintf(stderr, "MidiPort::putEvent: Error: gui2AudioFifo fifo overflow\n");
//     fprintf(stderr, "MidiPort::putEvent: Error: GuiFifo fifo overflow\n");
//   return res;
// }

//---------------------------------------------------------
//   putEvent
//   To be called from gui thread only.
//   Returns true if event cannot be delivered.
//---------------------------------------------------------

bool MidiPort::putEvent(const MidiPlayEvent& ev)
{
//   const int chan = ev.channel();
// 
//   // Stage the event.
//   //MidiPlayEvent staged_ev;
// //   const int ctrl = stageEvent(staged_ev, ev);
//   const int ctrl = ev.translateCtrlNum();
//   // Event translatable to a controller?
//   if(ctrl >= 0)
//     // Make sure to create the controller if necessary.
//     createController(chan, ctrl);
  
  
  // Send the event to the device first so that current parameters could be updated on process.
//   MidiPlayEvent staged_ev;
//   const bool stage_res = stageEvent(staged_ev, ev);
  bool res = false;
  if(_device)
  {
    // FIXME: Concurrency with putOSCEvent(). Need putOSCEvent() etc.
//     res = _device->putEvent(ev);
//     res = _device->putEvent(staged_ev);
//     res = _device->putEvent(stageEvent(ev));
//     res = _device->eventFifos()->put(MidiDevice::GuiFifo, ev);
//     res = !_device->userEventBuffers()->put(ev);
//     res = _device->putUserEvent(ev, MidiDevice::Late);
    res = !_device->putEvent(ev, MidiDevice::Late);
    //if(res)
    //  fprintf(stderr, "MidiPort::putEvent: Error: Device buffer overflow\n");
  }
//   if(ctrl >= 0 && _gui2AudioFifo->put(Gui2AudioFifoStruct(staged_ev)))
//   if(ctrl >= 0 && _gui2AudioFifo->put(Gui2AudioFifoStruct(ev)))
//   if(ctrl >= 0 && eventFifos().put(GuiFifo, Gui2AudioFifoStruct(ev)))
//   if(eventFifos().put(GuiFifo, Gui2AudioFifoStruct(ev)))
//   if(eventFifos().put(GuiFifo, ev))
//   if(!eventBuffers()->put(ev))
// //     fprintf(stderr, "MidiPort::putEvent: Error: gui2AudioFifo fifo overflow\n");
//     fprintf(stderr, "MidiPort::putEvent: Error: GuiFifo fifo overflow\n");
  putHwCtrlEvent(ev);
  return res;
}

//---------------------------------------------------------
//   putControllerIncrement
//   To be called from gui thread only.
//   Returns true if event cannot be delivered.
//---------------------------------------------------------

// TODO: An increment method seems possible: Wait for gui2audio to increment, then send to driver,
//        which incurs up to one extra segment delay (if Jack midi).

// REMOVE Tim. autoconnect. 
// bool MidiPort::putControllerIncrement(int port, int chan, int ctlnum, double incVal, bool isDb)
// {
//   iMidiCtrlValList imcvl = _controller->find(chan, ctlnum);
//   if(imcvl == _controller->end())
//     return true;
//   MidiCtrlValList* mcvl = imcvl->second;
// 
//   MusECore::MidiController* mc = midiController(ctlnum, false);
//   if(!mc)
//     return true;
//   const int max = mc->maxVal();
// 
//   double d_prev_val = mcvl->hwDVal();
//   if(mcvl->hwValIsUnknown())
//     d_prev_val = mcvl->lastValidHWDVal();
//   if(mcvl->lastHwValIsUnknown())
//   {
//     if(mc->initValIsUnknown())
//       d_prev_val = 0.0;
//     else
//       d_prev_val = double(mc->initVal());
//   }
// 
// //   const int i_prev_val = int(d_prev_val);
// 
//   if(isDb)
//     d_prev_val = muse_val2dbr(d_prev_val / double(max)) * 2.0;
// 
//   double d_new_val = d_prev_val + incVal;
// 
//   if(isDb)
//     d_new_val = double(max) * muse_db2val(d_new_val / 2.0);
// 
//   const int i_new_val = MidiController::dValToInt(d_new_val);
// //   const bool i_val_changed = i_new_val != i_prev_val;
// 
// // REMOVE Tim. autoconnect. Changed. Schedule for immediate playback.
// //   MusECore::MidiPlayEvent ev(MusEGlobal::song->cpos(), port, chan, MusECore::ME_CONTROLLER, ctlnum, i_new_val);
//   MusECore::MidiPlayEvent ev(0, port, chan, MusECore::ME_CONTROLLER, ctlnum, i_new_val);
//   // Send the event to the device first so that current parameters could be updated on process.
//   MidiPlayEvent staged_ev;
//   const bool stage_res = stageEvent(staged_ev, ev);
//   bool res = false;
//   if(_device)
// //   if(_device && i_val_changed)
//     res = _device->putEvent(ev);
// 
//   d_new_val = limitValToInstrCtlRange(ctlnum, d_new_val);
// 
//   // False = direct not increment because we are doing the increment here.
//   Gui2AudioFifoStruct g2as(staged_ev.time(), staged_ev.type(), staged_ev.channel(), staged_ev.dataA(), d_new_val, false);
//   if(stage_res && _gui2AudioFifo->put(g2as))
//     fprintf(stderr, "MidiPort::putControllerIncrement: Error: gui2AudioFifo fifo overflow\n");
//   return res;
// }

//---------------------------------------------------------
//   putControllerValue
//   To be called from gui thread only.
//   Returns true if event cannot be delivered.
//---------------------------------------------------------

bool MidiPort::putControllerValue(int port, int chan, int ctlnum, double val, bool isDb)
{
  iMidiCtrlValList imcvl = _controller->find(chan, ctlnum);
  if(imcvl == _controller->end())
    return true;

  // Don't create if not found.
  MusECore::MidiController* mc = midiController(ctlnum, false);
  if(!mc)
    return true;
  const int max = mc->maxVal();

//   double d_prev_val = mcvl->hwDVal();
//   if(mcvl->hwValIsUnknown())
//     d_prev_val = mcvl->lastValidHWDVal();
//   if(mcvl->lastHwValIsUnknown())
//   {
//     if(mc->initValIsUnknown())
//       d_prev_val = 0.0;
//     else
//       d_prev_val = double(mc->initVal());
//   }

//   const int i_prev_val = int(d_prev_val);

  if(isDb)
    val = double(max) * muse_db2val(val / 2.0);

  const int i_new_val = MidiController::dValToInt(val);
//   const bool i_val_changed = i_new_val != i_prev_val;

// REMOVE Tim. autoconnect. Changed. Schedule for immediate playback.
//   MusECore::MidiPlayEvent ev(MusEGlobal::song->cpos(), port, chan, MusECore::ME_CONTROLLER, ctlnum, i_new_val);
        // Time-stamp the event.
  MidiPlayEvent ev(MusEGlobal::audio->curFrame(), port, chan, MusECore::ME_CONTROLLER, ctlnum, i_new_val);
//   // Send the event to the device first so that current parameters could be updated on process.
//   MidiPlayEvent staged_ev;
//   const bool stage_res = stageEvent(staged_ev, ev);
  bool res = false;
  if(_device)
  {
//   if(_device && i_val_changed)
//     res = _device->putEvent(ev);
//     res = _device->eventFifos()->put(MidiDevice::GuiFifo, ev);
//     res = !_device->userEventBuffers()->put(ev);
//     res = _device->putUserEvent(ev, MidiDevice::Late);
    res = !_device->putEvent(ev, MidiDevice::Late);
//     if(res)
//       fprintf(stderr, "MidiPort::putControllerValue: Error: Device buffer overflow\n");
  }

//   val = limitValToInstrCtlRange(ctlnum, val);

  // False = direct not increment because we are doing the increment here.
//   Gui2AudioFifoStruct g2as(staged_ev.time(), staged_ev.type(), staged_ev.channel(), staged_ev.dataA(), val, false);
//   if(stage_res && _gui2AudioFifo->put(g2as))
//   if(eventFifos().put(GuiFifo, Gui2AudioFifoStruct(ev)))
//   if(eventFifos().put(GuiFifo, ev))
//   if(!eventBuffers()->put(ev))
// //     fprintf(stderr, "MidiPort::putControllerValue: Error: gui2AudioFifo fifo overflow\n");
//     fprintf(stderr, "MidiPort::putControllerValue: Error: GuiFifo fifo overflow\n");
  putHwCtrlEvent(ev);
  return res;
}

//---------------------------------------------------------
//   handleGui2AudioEvent
//   To be called from audio thread only.
//   Returns true if event cannot be delivered.
//---------------------------------------------------------

// bool MidiPort::handleGui2AudioEvent(const Gui2AudioFifoStruct& g2as)
// {
//   const int chn = g2as._chan;
//   
//   const int i_dataA = g2as._dataA;
//   
//   const double d_dataB = g2as._dataB;
//   const int i_dataB = MidiController::dValToInt(d_dataB);
//   
//   int ctrl = -1;
//   int i_fin_val = 0;
// 
//   if (g2as._type == ME_CONTROLLER)
//   {
//     ctrl = i_dataA;
//     i_fin_val = i_dataB;
// 
//     // TODO: Maybe update CTRL_HBANK/CTRL_LBANK - but ONLY if they are specifically
//     //        defined by the user in the controller list.
//     if(ctrl == CTRL_HBANK)
//     {
//       int hb = i_dataB & 0xff;
//       int lb = 0xff;
//       int pr = 0xff;
//       if(_device)
//       {
//         _device->curOutParamNums(chn)->currentProg(&pr, &lb, NULL);
//         _device->curOutParamNums(chn)->BANKH = hb;
//       }
//       i_fin_val = (hb << 16) | (lb << 8) | pr;
//       ctrl = CTRL_PROGRAM;
//     }
//     else if(ctrl == CTRL_LBANK)
//     {
//       int hb = 0xff;
//       int lb = i_dataB & 0xff;
//       int pr = 0xff;
//       if(_device)
//       {
//         _device->curOutParamNums(chn)->currentProg(&pr, NULL, &hb);
//         _device->curOutParamNums(chn)->BANKL = lb;
//       }
//       i_fin_val = (hb << 16) | (lb << 8) | pr;
//       ctrl = CTRL_PROGRAM;
//     }
//     else if(ctrl == CTRL_PROGRAM)
//     {
//       // TODO: Maybe update CTRL_HBANK/CTRL_LBANK - but ONLY if they are specifically
//       //        defined by the user in the controller list.
//     }
//     else
//     {
//     }
//   }
//   else
//   if (g2as._type == ME_POLYAFTER)
//   {
//     const int pitch = i_dataA & 0x7f;
//     ctrl = (CTRL_POLYAFTER & ~0xff) | pitch;
//     //const int db = limitValToInstrCtlRange(ctrl, ev.dataB());  // Already done by sender !
//     i_fin_val = i_dataB;
//   }
//   else
//   if (g2as._type == ME_AFTERTOUCH)  // ... Same
//   {
//     ctrl = CTRL_AFTERTOUCH;
//     //const int da = limitValToInstrCtlRange(ctrl, ev.dataA());  // Already done by sender !
//     i_fin_val = i_dataA;
//   }
//   else
//   if (g2as._type == ME_PITCHBEND)
//   {
//     ctrl = CTRL_PITCH;
//     //const int da = limitValToInstrCtlRange(ctrl, ev.dataA()); // Already done by sender !
//     i_fin_val = i_dataA;
//   }
//   else
//   if (g2as._type == ME_PROGRAM)
//   {
//     ctrl = CTRL_PROGRAM;
//     int hb = 0xff;
//     int lb = 0xff;
//     int pr = i_dataA & 0xff;
//     if(_device)
//     {
//       _device->curOutParamNums(chn)->currentProg(NULL, &lb, &hb);
//       _device->curOutParamNums(chn)->PROG = pr;
//     }
//     i_fin_val = (hb << 16) | (lb << 8) | pr;
//   }
//   else
//   {
// 
//   }
// 
//   // For now we are only interested in controllers not notes etc.
//   if(ctrl == -1)
//     return false;
// 
//   iMidiCtrlValList imcvl = _controller->find(chn, ctrl);
//   if(imcvl == _controller->end())
//   {
//     fprintf(stderr, "MidiPort::handleGui2AudioEvent Error: Controller:%d on chan:%d does not exist. Sender should have created it!\n", ctrl, chn);
//     return true;
//   }
// 
//   const double d_fin_val = double(i_fin_val);
//   
//   MidiCtrlValList* mcvl = imcvl->second;
//   const bool res = mcvl->setHwVal(d_fin_val);
//   // If program controller be sure to update drum maps (and inform the gui).
//   if(res && ctrl == CTRL_PROGRAM)
//     updateDrumMaps(chn, i_fin_val);
//   //return res;
// 
//   return false;
// }

// //---------------------------------------------------------
// //   handleGui2AudioEvent
// //   To be called from audio thread only.
// //   Returns true if event cannot be delivered.
// //---------------------------------------------------------
// 
// // MidiPlayEvent MidiPort::handleGui2AudioEvent(const Gui2AudioFifoStruct& g2as)
// MidiPlayEvent MidiPort::handleGui2AudioEvent(const MidiPlayEvent& ev)
// {
// //   const int chn = g2as._chan;
// //   const int type = g2as._type;
// //   const int i_dataA = g2as._dataA;
// //   const double d_dataB = g2as._dataB;
// //   const int i_dataB = MidiController::dValToInt(d_dataB);
// 
//   const unsigned int time = ev.time();
//   const int port = ev.port();
//   const int chn = ev.channel();
//   const int type = ev.type();
//   const int i_dataA = ev.dataA();
//   const double d_dataB = ev.dataB();
//   const int i_dataB = MidiController::dValToInt(d_dataB);
//   
// //   int ctrl = -1;
// //   int i_fin_val = 0;
// 
// 
//   int fin_db = i_dataB;
//   switch(type)
//   {
//     case ME_CONTROLLER:
//       switch(i_dataA)
//       {
//         case CTRL_HBANK:
//         {
//           int hb = 0xff;
//           if(!MidiController::iValIsUnknown(i_dataB))
//             hb = i_dataB & 0xff;
//           if(hb != 0xff)
//             hb = limitValToInstrCtlRange(i_dataA, hb);
//           int lb = 0xff;
//           int pr = 0xff;
//           
//           iMidiCtrlValList imcvl = _controller->find(chn, CTRL_PROGRAM);
//           // Does the CTRL_PROGRAM controller exist?
//           if(imcvl == _controller->end())
//             //return MidiPlayEvent();
//             //return true;
//             break;
//           
//           MidiCtrlValList* mcvl = imcvl->second;
//           if(!mcvl->hwValIsUnknown())
//           {
//             const int hw_val = mcvl->hwVal();
//             lb = (hw_val >> 8) & 0xff;
//             pr = hw_val & 0xff;
//           }
//           
//           if((hb != 0xff || lb != 0xff) && pr == 0xff)
//             pr = 0x01;
//           if(hb == 0xff && lb == 0xff && pr == 0xff)
//             fin_db = CTRL_VAL_UNKNOWN;
//           else
//             fin_db = (hb << 16) | (lb << 8) | pr;
//           
//           // Set the value. Be sure to update drum maps (and inform the gui).
//           if(mcvl->setHwVal(fin_db))
//             updateDrumMaps(chn, fin_db);
//           
// //           return MidiPlayEvent(g2as._time, g2as._port, chn, ME_CONTROLLER, CTRL_PROGRAM, fin_db);
//           return MidiPlayEvent(time, port, chn, ME_CONTROLLER, CTRL_PROGRAM, fin_db);
//           //return false;
//         }
//         break;
// 
//         case CTRL_LBANK:
//         {
//           int hb = 0xff;
//           int lb = 0xff;
//           if(!MidiController::iValIsUnknown(i_dataB))
//             lb = i_dataB & 0xff;
//           if(lb != 0xff)
//             lb = limitValToInstrCtlRange(i_dataA, lb);
//           int pr = 0xff;
//           
//           iMidiCtrlValList imcvl = _controller->find(chn, CTRL_PROGRAM);
//           // Does the CTRL_PROGRAM controller exist?
//           if(imcvl == _controller->end())
//             //return MidiPlayEvent();
//             //return true;
//             break;
//           
//           MidiCtrlValList* mcvl = imcvl->second;
//           if(!mcvl->hwValIsUnknown())
//           {
//             const int hw_val = mcvl->hwVal();
//             hb = (hw_val >> 16) & 0xff;
//             pr = hw_val & 0xff;
//           }
//           
//           if((hb != 0xff || lb != 0xff) && pr == 0xff)
//             pr = 0x01;
//           if(hb == 0xff && lb == 0xff && pr == 0xff)
//             fin_db = CTRL_VAL_UNKNOWN;
//           else
//             fin_db = (hb << 16) | (lb << 8) | pr;
//           
//           // Set the value. Be sure to update drum maps (and inform the gui).
//           if(mcvl->setHwVal(fin_db))
//             updateDrumMaps(chn, fin_db);
//           
// //           return MidiPlayEvent(g2as._time, g2as._port, chn, ME_CONTROLLER, CTRL_PROGRAM, fin_db);
//           return MidiPlayEvent(time, port, chn, ME_CONTROLLER, CTRL_PROGRAM, fin_db);
//           //return false;
//         }
//         break;
// 
//         case CTRL_PROGRAM:
//         {
//           // TODO: Maybe update CTRL_HBANK/CTRL_LBANK - but ONLY if they are specifically
//           //        defined by the user in the controller list.
//             
//           iMidiCtrlValList imcvl = _controller->find(chn, CTRL_PROGRAM);
//           // Does the CTRL_PROGRAM controller exist?
//           if(imcvl == _controller->end())
//             //return MidiPlayEvent();
//             //return true;
//             break;
//           
//           // Set the value. Be sure to update drum maps (and inform the gui).
//           if(imcvl->second->setHwVal(fin_db))
//             updateDrumMaps(chn, fin_db);
//           
// //           return MidiPlayEvent(g2as._time, g2as._port, chn, ME_CONTROLLER, CTRL_PROGRAM, fin_db);
//           return MidiPlayEvent(time, port, chn, ME_CONTROLLER, CTRL_PROGRAM, fin_db);
//           //return false;
//         }
//         break;
//         
//         default:
//         {
//           iMidiCtrlValList imcvl = _controller->find(chn, i_dataA);
//           // Does the controller exist?
//           if(imcvl == _controller->end())
//             //return MidiPlayEvent();
//             //return true;
//             break;
// 
//           fin_db = limitValToInstrCtlRange(i_dataA, i_dataB);
//           // Set the value.
//           imcvl->second->setHwVal(fin_db);
//           
// //           return MidiPlayEvent(g2as._time, g2as._port, chn, ME_CONTROLLER, i_dataA, fin_db);
//           return MidiPlayEvent(time, port, chn, ME_CONTROLLER, i_dataA, fin_db);
//           //return false;
//         }
//         break;
//       }
//     break;
//     
//     case ME_POLYAFTER:
//     {
//       const int pitch = i_dataA & 0x7f;
//       const int fin_da = (CTRL_POLYAFTER & ~0xff) | pitch;
//       iMidiCtrlValList imcvl = _controller->find(chn, fin_da);
//       
//       // Does the controller exist?
//       if(imcvl == _controller->end())
//         //return MidiPlayEvent();
//         //return true;
//         break;
// 
//       fin_db = limitValToInstrCtlRange(fin_da, i_dataB);
//       // Set the value.
//       imcvl->second->setHwVal(fin_db);
//       
// //       return MidiPlayEvent(g2as._time, g2as._port, chn, ME_CONTROLLER, fin_da, fin_db);
//       return MidiPlayEvent(time, port, chn, ME_CONTROLLER, fin_da, fin_db);
//       //return false;
//     }
//     break;
//     
//     case ME_AFTERTOUCH:
//     {
//       iMidiCtrlValList imcvl = _controller->find(chn, CTRL_AFTERTOUCH);
//       // Does the controller exist?
//       if(imcvl == _controller->end())
//         //return MidiPlayEvent();
//         //return true;
//         break;
//       
//       fin_db = limitValToInstrCtlRange(CTRL_AFTERTOUCH, i_dataA);
//       // Set the value.
//       imcvl->second->setHwVal(fin_db);
//       
// //       return MidiPlayEvent(g2as._time, g2as._port, chn, ME_CONTROLLER, CTRL_AFTERTOUCH, fin_db);
//       return MidiPlayEvent(time, port, chn, ME_CONTROLLER, CTRL_AFTERTOUCH, fin_db);
//       //return false;
//     }
//     break;
//     
//     case ME_PITCHBEND:
//     {
//       iMidiCtrlValList imcvl = _controller->find(chn, CTRL_PITCH);
//       // Does the controller exist?
//       if(imcvl == _controller->end())
//         //return MidiPlayEvent();
//         //return true;
//         break;
//       
//       fin_db = limitValToInstrCtlRange(CTRL_PITCH, i_dataA);
//       // Set the value.
//       imcvl->second->setHwVal(fin_db);
//       
// //       return MidiPlayEvent(g2as._time, g2as._port, chn, ME_CONTROLLER, CTRL_PITCH, fin_db);
//       return MidiPlayEvent(time, port, chn, ME_CONTROLLER, CTRL_PITCH, fin_db);
//       //return false;
//     }
//     break;
//     
//     case ME_PROGRAM:
//     {
//       int hb = 0xff;
//       int lb = 0xff;
//       int pr = 0xff;
//       if(!MidiController::iValIsUnknown(i_dataA))
//         pr = i_dataA & 0xff;
//       //if(pr != 0xff)
//       //  pr = limitValToInstrCtlRange(da, pr);
//       
//       iMidiCtrlValList imcvl = _controller->find(chn, CTRL_PROGRAM);
//       // Does the CTRL_PROGRAM controller exist?
//       if(imcvl == _controller->end())
//         //return MidiPlayEvent();
//         //return true;
//         break;
//       
//       MidiCtrlValList* mcvl = imcvl->second;
//       if(!mcvl->hwValIsUnknown())
//       {
//         const int hw_val = mcvl->hwVal();
//         hb = (hw_val >> 16) & 0xff;
//         lb = (hw_val >> 8) & 0xff;
//       }
//       
//       if((hb != 0xff || lb != 0xff) && pr == 0xff)
//         pr = 0x01;
//       if(hb == 0xff && lb == 0xff && pr == 0xff)
//         fin_db = CTRL_VAL_UNKNOWN;
//       else
//         fin_db = (hb << 16) | (lb << 8) | pr;
//       
//       // Set the value. Be sure to update drum maps (and inform the gui).
//       if(mcvl->setHwVal(fin_db))
//         updateDrumMaps(chn, fin_db);
//       
// //       return MidiPlayEvent(g2as._time, g2as._port, chn, ME_CONTROLLER, CTRL_PROGRAM, fin_db);
//       return MidiPlayEvent(time, port, chn, ME_CONTROLLER, CTRL_PROGRAM, fin_db);
//       //return false;
//     }
//     break;
//     
//     default:
//       // For all other event types, just return true.
//       //return MidiPlayEvent(time, port, chn, type, i_dataA, i_dataB);
//       //return true;
//     break;
//   }
//   
// //   return MidiPlayEvent(g2as._time, g2as._port, chn, type, i_dataA, i_dataB);
//   return MidiPlayEvent(time, port, chn, type, i_dataA, i_dataB);
//   //return true;
//       
// 
// 
// /*
//   
//   if (g2as._type == ME_CONTROLLER)
//   {
//     ctrl = i_dataA;
//     i_fin_val = i_dataB;
// 
//     // TODO: Maybe update CTRL_HBANK/CTRL_LBANK - but ONLY if they are specifically
//     //        defined by the user in the controller list.
//     if(ctrl == CTRL_HBANK)
//     {
//       int hb = i_dataB & 0xff;
//       int lb = 0xff;
//       int pr = 0xff;
//       if(_device)
//       {
//         _device->curOutParamNums(chn)->currentProg(&pr, &lb, NULL);
//         _device->curOutParamNums(chn)->BANKH = hb;
//       }
//       i_fin_val = (hb << 16) | (lb << 8) | pr;
//       ctrl = CTRL_PROGRAM;
//     }
//     else if(ctrl == CTRL_LBANK)
//     {
//       int hb = 0xff;
//       int lb = i_dataB & 0xff;
//       int pr = 0xff;
//       if(_device)
//       {
//         _device->curOutParamNums(chn)->currentProg(&pr, NULL, &hb);
//         _device->curOutParamNums(chn)->BANKL = lb;
//       }
//       i_fin_val = (hb << 16) | (lb << 8) | pr;
//       ctrl = CTRL_PROGRAM;
//     }
//     else if(ctrl == CTRL_PROGRAM)
//     {
//       // TODO: Maybe update CTRL_HBANK/CTRL_LBANK - but ONLY if they are specifically
//       //        defined by the user in the controller list.
//     }
//     else
//     {
//     }
//   }
//   else
//   if (g2as._type == ME_POLYAFTER)
//   {
//     const int pitch = i_dataA & 0x7f;
//     ctrl = (CTRL_POLYAFTER & ~0xff) | pitch;
//     //const int db = limitValToInstrCtlRange(ctrl, ev.dataB());  // Already done by sender !
//     i_fin_val = i_dataB;
//   }
//   else
//   if (g2as._type == ME_AFTERTOUCH)  // ... Same
//   {
//     ctrl = CTRL_AFTERTOUCH;
//     //const int da = limitValToInstrCtlRange(ctrl, ev.dataA());  // Already done by sender !
//     i_fin_val = i_dataA;
//   }
//   else
//   if (g2as._type == ME_PITCHBEND)
//   {
//     ctrl = CTRL_PITCH;
//     //const int da = limitValToInstrCtlRange(ctrl, ev.dataA()); // Already done by sender !
//     i_fin_val = i_dataA;
//   }
//   else
//   if (g2as._type == ME_PROGRAM)
//   {
//     ctrl = CTRL_PROGRAM;
//     int hb = 0xff;
//     int lb = 0xff;
//     int pr = i_dataA & 0xff;
//     if(_device)
//     {
//       _device->curOutParamNums(chn)->currentProg(NULL, &lb, &hb);
//       _device->curOutParamNums(chn)->PROG = pr;
//     }
//     i_fin_val = (hb << 16) | (lb << 8) | pr;
//   }
//   else
//   {
// 
//   }
// 
//   // For now we are only interested in controllers not notes etc.
//   if(ctrl == -1)
//     return false;
// 
//   iMidiCtrlValList imcvl = _controller->find(chn, ctrl);
//   if(imcvl == _controller->end())
//   {
//     fprintf(stderr, "MidiPort::handleGui2AudioEvent Error: Controller:%d on chan:%d does not exist. Sender should have created it!\n", ctrl, chn);
//     return true;
//   }
// 
//   const double d_fin_val = double(i_fin_val);
//   
//   MidiCtrlValList* mcvl = imcvl->second;
//   const bool res = mcvl->setHwVal(d_fin_val);
//   // If program controller be sure to update drum maps (and inform the gui).
//   if(res && ctrl == CTRL_PROGRAM)
//     updateDrumMaps(chn, i_fin_val);
//   //return res;
// 
//   return false;*/
// }

//---------------------------------------------------------
//   handleGui2AudioEvent
//   To be called from audio thread only.
//   Returns true if event cannot be delivered.
//---------------------------------------------------------

// MidiPlayEvent MidiPort::handleGui2AudioEvent(const Gui2AudioFifoStruct& g2as)
MidiPlayEvent MidiPort::handleGui2AudioEvent(const MidiPlayEvent& ev)
{
//   const int chn = g2as._chan;
//   const int type = g2as._type;
//   const int i_dataA = g2as._dataA;
//   const double d_dataB = g2as._dataB;
//   const int i_dataB = MidiController::dValToInt(d_dataB);

  const unsigned int time = ev.time();
  const int port = ev.port();
  const int chn = ev.channel();
  const int type = ev.type();
  const int i_dataA = ev.dataA();
  const double d_dataB = ev.dataB();
  const int i_dataB = MidiController::dValToInt(d_dataB);
  
//   int ctrl = -1;
//   int i_fin_val = 0;


  int fin_db = i_dataB;
  switch(type)
  {
    case ME_CONTROLLER:
      switch(i_dataA)
      {
        case CTRL_HBANK:
        {
          int hb = 0xff;
          if(!MidiController::iValIsUnknown(i_dataB))
            hb = i_dataB & 0xff;
          if(hb != 0xff)
            hb = limitValToInstrCtlRange(i_dataA, hb);
          int lb = 0xff;
          int pr = 0xff;
          
          iMidiCtrlValList imcvl = _controller->find(chn, CTRL_PROGRAM);
          // Does the CTRL_PROGRAM controller exist?
          if(imcvl == _controller->end())
            //return MidiPlayEvent();
            //return true;
            break;
          
          MidiCtrlValList* mcvl = imcvl->second;
          if(!mcvl->hwValIsUnknown())
          {
            const int hw_val = mcvl->hwVal();
            lb = (hw_val >> 8) & 0xff;
            pr = hw_val & 0xff;
          }
          
          if((hb != 0xff || lb != 0xff) && pr == 0xff)
            pr = 0x01;
          if(hb == 0xff && lb == 0xff && pr == 0xff)
            fin_db = CTRL_VAL_UNKNOWN;
          else
            fin_db = (hb << 16) | (lb << 8) | pr;
          
          // Set the value. Be sure to update drum maps (and inform the gui).
          if(mcvl->setHwVal(fin_db))
            updateDrumMaps(chn, fin_db);
          
//           return MidiPlayEvent(g2as._time, g2as._port, chn, ME_CONTROLLER, CTRL_PROGRAM, fin_db);
          return MidiPlayEvent(time, port, chn, ME_CONTROLLER, CTRL_PROGRAM, fin_db);
          //return false;
        }
        break;

        case CTRL_LBANK:
        {
          int hb = 0xff;
          int lb = 0xff;
          if(!MidiController::iValIsUnknown(i_dataB))
            lb = i_dataB & 0xff;
          if(lb != 0xff)
            lb = limitValToInstrCtlRange(i_dataA, lb);
          int pr = 0xff;
          
          iMidiCtrlValList imcvl = _controller->find(chn, CTRL_PROGRAM);
          // Does the CTRL_PROGRAM controller exist?
          if(imcvl == _controller->end())
            //return MidiPlayEvent();
            //return true;
            break;
          
          MidiCtrlValList* mcvl = imcvl->second;
          if(!mcvl->hwValIsUnknown())
          {
            const int hw_val = mcvl->hwVal();
            hb = (hw_val >> 16) & 0xff;
            pr = hw_val & 0xff;
          }
          
          if((hb != 0xff || lb != 0xff) && pr == 0xff)
            pr = 0x01;
          if(hb == 0xff && lb == 0xff && pr == 0xff)
            fin_db = CTRL_VAL_UNKNOWN;
          else
            fin_db = (hb << 16) | (lb << 8) | pr;
          
          // Set the value. Be sure to update drum maps (and inform the gui).
          if(mcvl->setHwVal(fin_db))
            updateDrumMaps(chn, fin_db);
          
//           return MidiPlayEvent(g2as._time, g2as._port, chn, ME_CONTROLLER, CTRL_PROGRAM, fin_db);
          return MidiPlayEvent(time, port, chn, ME_CONTROLLER, CTRL_PROGRAM, fin_db);
          //return false;
        }
        break;

        case CTRL_PROGRAM:
        {
          // TODO: Maybe update CTRL_HBANK/CTRL_LBANK - but ONLY if they are specifically
          //        defined by the user in the controller list.
            
          iMidiCtrlValList imcvl = _controller->find(chn, CTRL_PROGRAM);
          // Does the CTRL_PROGRAM controller exist?
          if(imcvl == _controller->end())
            //return MidiPlayEvent();
            //return true;
            break;
          
          // Set the value. Be sure to update drum maps (and inform the gui).
          if(imcvl->second->setHwVal(fin_db))
            updateDrumMaps(chn, fin_db);
          
//           return MidiPlayEvent(g2as._time, g2as._port, chn, ME_CONTROLLER, CTRL_PROGRAM, fin_db);
          return MidiPlayEvent(time, port, chn, ME_CONTROLLER, CTRL_PROGRAM, fin_db);
          //return false;
        }
        break;
        
        default:
        {
          iMidiCtrlValList imcvl = _controller->find(chn, i_dataA);
          // Does the controller exist?
          if(imcvl == _controller->end())
            //return MidiPlayEvent();
            //return true;
            break;

          fin_db = limitValToInstrCtlRange(i_dataA, i_dataB);
          // Set the value.
          imcvl->second->setHwVal(fin_db);
          
//           return MidiPlayEvent(g2as._time, g2as._port, chn, ME_CONTROLLER, i_dataA, fin_db);
          return MidiPlayEvent(time, port, chn, ME_CONTROLLER, i_dataA, fin_db);
          //return false;
        }
        break;
      }
    break;
    
    case ME_POLYAFTER:
    {
      const int pitch = i_dataA & 0x7f;
      const int fin_da = (CTRL_POLYAFTER & ~0xff) | pitch;
      iMidiCtrlValList imcvl = _controller->find(chn, fin_da);
      
      // Does the controller exist?
      if(imcvl == _controller->end())
        //return MidiPlayEvent();
        //return true;
        break;

      fin_db = limitValToInstrCtlRange(fin_da, i_dataB);
      // Set the value.
      imcvl->second->setHwVal(fin_db);
      
//       return MidiPlayEvent(g2as._time, g2as._port, chn, ME_CONTROLLER, fin_da, fin_db);
      return MidiPlayEvent(time, port, chn, ME_CONTROLLER, fin_da, fin_db);
      //return false;
    }
    break;
    
    case ME_AFTERTOUCH:
    {
      iMidiCtrlValList imcvl = _controller->find(chn, CTRL_AFTERTOUCH);
      // Does the controller exist?
      if(imcvl == _controller->end())
        //return MidiPlayEvent();
        //return true;
        break;
      
      fin_db = limitValToInstrCtlRange(CTRL_AFTERTOUCH, i_dataA);
      // Set the value.
      imcvl->second->setHwVal(fin_db);
      
//       return MidiPlayEvent(g2as._time, g2as._port, chn, ME_CONTROLLER, CTRL_AFTERTOUCH, fin_db);
      return MidiPlayEvent(time, port, chn, ME_CONTROLLER, CTRL_AFTERTOUCH, fin_db);
      //return false;
    }
    break;
    
    case ME_PITCHBEND:
    {
      iMidiCtrlValList imcvl = _controller->find(chn, CTRL_PITCH);
      // Does the controller exist?
      if(imcvl == _controller->end())
        //return MidiPlayEvent();
        //return true;
        break;
      
      fin_db = limitValToInstrCtlRange(CTRL_PITCH, i_dataA);
      // Set the value.
      imcvl->second->setHwVal(fin_db);
      
//       return MidiPlayEvent(g2as._time, g2as._port, chn, ME_CONTROLLER, CTRL_PITCH, fin_db);
      return MidiPlayEvent(time, port, chn, ME_CONTROLLER, CTRL_PITCH, fin_db);
      //return false;
    }
    break;
    
    case ME_PROGRAM:
    {
      int hb = 0xff;
      int lb = 0xff;
      int pr = 0xff;
      if(!MidiController::iValIsUnknown(i_dataA))
        pr = i_dataA & 0xff;
      //if(pr != 0xff)
      //  pr = limitValToInstrCtlRange(da, pr);
      
      iMidiCtrlValList imcvl = _controller->find(chn, CTRL_PROGRAM);
      // Does the CTRL_PROGRAM controller exist?
      if(imcvl == _controller->end())
        //return MidiPlayEvent();
        //return true;
        break;
      
      MidiCtrlValList* mcvl = imcvl->second;
      if(!mcvl->hwValIsUnknown())
      {
        const int hw_val = mcvl->hwVal();
        hb = (hw_val >> 16) & 0xff;
        lb = (hw_val >> 8) & 0xff;
      }
      
      if((hb != 0xff || lb != 0xff) && pr == 0xff)
        pr = 0x01;
      if(hb == 0xff && lb == 0xff && pr == 0xff)
        fin_db = CTRL_VAL_UNKNOWN;
      else
        fin_db = (hb << 16) | (lb << 8) | pr;
      
      // Set the value. Be sure to update drum maps (and inform the gui).
      if(mcvl->setHwVal(fin_db))
        updateDrumMaps(chn, fin_db);
      
//       return MidiPlayEvent(g2as._time, g2as._port, chn, ME_CONTROLLER, CTRL_PROGRAM, fin_db);
      return MidiPlayEvent(time, port, chn, ME_CONTROLLER, CTRL_PROGRAM, fin_db);
      //return false;
    }
    break;
    
    default:
      // For all other event types, just return true.
      //return MidiPlayEvent(time, port, chn, type, i_dataA, i_dataB);
      //return true;
    break;
  }
  
//   return MidiPlayEvent(g2as._time, g2as._port, chn, type, i_dataA, i_dataB);
  return MidiPlayEvent(time, port, chn, type, i_dataA, i_dataB);
  //return true;
}

//---------------------------------------------------------
//   processAudio2GuiEvent
//   To be called from gui thread only.
//   Returns true if event cannot be delivered.
//---------------------------------------------------------

bool MidiPort::processAudio2GuiEvent(MidiCtrlValList* mcvl, const MidiPlayEvent& ev)
{
//   const int chn = g2as._chan;
//   const int type = g2as._type;
//   const int i_dataA = g2as._dataA;
//   const double d_dataB = g2as._dataB;
//   const int i_dataB = MidiController::dValToInt(d_dataB);

//   const unsigned int time = ev.time();
//   const int port = ev.port();
  const int chn = ev.channel();
  const int type = ev.type();
  const int i_dataA = ev.dataA();
  const double d_dataB = ev.dataB();
  const int i_dataB = MidiController::dValToInt(d_dataB);
  
//   int ctrl = -1;
//   int i_fin_val = 0;


  int fin_db = i_dataB;
  switch(type)
  {
    case ME_CONTROLLER:
      switch(i_dataA)
      {
        case CTRL_HBANK:
        {
          int hb = 0xff;
          if(!MidiController::iValIsUnknown(i_dataB))
            hb = i_dataB & 0xff;
          if(hb != 0xff)
            hb = limitValToInstrCtlRange(i_dataA, hb);
          int lb = 0xff;
          int pr = 0xff;
          
//           iMidiCtrlValList imcvl = _controller->find(chn, CTRL_PROGRAM);
//           // Does the CTRL_PROGRAM controller exist?
//           if(imcvl == _controller->end())
//             //return MidiPlayEvent();
//             //return true;
//             break;
//           
//           MidiCtrlValList* mcvl = imcvl->second;
          if(!mcvl->hwValIsUnknown())
          {
            const int hw_val = mcvl->hwVal();
            lb = (hw_val >> 8) & 0xff;
            pr = hw_val & 0xff;
          }
          
          if((hb != 0xff || lb != 0xff) && pr == 0xff)
            pr = 0x01;
          if(hb == 0xff && lb == 0xff && pr == 0xff)
            fin_db = CTRL_VAL_UNKNOWN;
          else
            fin_db = (hb << 16) | (lb << 8) | pr;
          
          // Set the value. Be sure to update drum maps (and inform the gui).
          if(mcvl->setHwVal(fin_db))
            updateDrumMaps(chn, fin_db);
          
//           return MidiPlayEvent(g2as._time, g2as._port, chn, ME_CONTROLLER, CTRL_PROGRAM, fin_db);
//           return MidiPlayEvent(time, port, chn, ME_CONTROLLER, CTRL_PROGRAM, fin_db);
          return false;
        }
        break;

        case CTRL_LBANK:
        {
          int hb = 0xff;
          int lb = 0xff;
          if(!MidiController::iValIsUnknown(i_dataB))
            lb = i_dataB & 0xff;
          if(lb != 0xff)
            lb = limitValToInstrCtlRange(i_dataA, lb);
          int pr = 0xff;
          
//           iMidiCtrlValList imcvl = _controller->find(chn, CTRL_PROGRAM);
//           // Does the CTRL_PROGRAM controller exist?
//           if(imcvl == _controller->end())
//             //return MidiPlayEvent();
//             //return true;
//             break;
//           
//           MidiCtrlValList* mcvl = imcvl->second;
          if(!mcvl->hwValIsUnknown())
          {
            const int hw_val = mcvl->hwVal();
            hb = (hw_val >> 16) & 0xff;
            pr = hw_val & 0xff;
          }
          
          if((hb != 0xff || lb != 0xff) && pr == 0xff)
            pr = 0x01;
          if(hb == 0xff && lb == 0xff && pr == 0xff)
            fin_db = CTRL_VAL_UNKNOWN;
          else
            fin_db = (hb << 16) | (lb << 8) | pr;
          
          // Set the value. Be sure to update drum maps (and inform the gui).
          if(mcvl->setHwVal(fin_db))
            updateDrumMaps(chn, fin_db);
          
//           return MidiPlayEvent(g2as._time, g2as._port, chn, ME_CONTROLLER, CTRL_PROGRAM, fin_db);
//           return MidiPlayEvent(time, port, chn, ME_CONTROLLER, CTRL_PROGRAM, fin_db);
          return false;
        }
        break;

        case CTRL_PROGRAM:
        {
          // TODO: Maybe update CTRL_HBANK/CTRL_LBANK - but ONLY if they are specifically
          //        defined by the user in the controller list.
            
//           iMidiCtrlValList imcvl = _controller->find(chn, CTRL_PROGRAM);
//           // Does the CTRL_PROGRAM controller exist?
//           if(imcvl == _controller->end())
//             //return MidiPlayEvent();
//             //return true;
//             break;
//           
          // Set the value. Be sure to update drum maps (and inform the gui).
//           if(imcvl->second->setHwVal(fin_db))
          if(mcvl->setHwVal(fin_db))
            updateDrumMaps(chn, fin_db);
          
//           return MidiPlayEvent(g2as._time, g2as._port, chn, ME_CONTROLLER, CTRL_PROGRAM, fin_db);
//           return MidiPlayEvent(time, port, chn, ME_CONTROLLER, CTRL_PROGRAM, fin_db);
          return false;
        }
        break;
        
        default:
        {
//           iMidiCtrlValList imcvl = _controller->find(chn, i_dataA);
//           // Does the controller exist?
//           if(imcvl == _controller->end())
//             //return MidiPlayEvent();
//             //return true;
//             break;

          fin_db = limitValToInstrCtlRange(i_dataA, i_dataB);
          // Set the value.
//           imcvl->second->setHwVal(fin_db);
          mcvl->setHwVal(fin_db);
          
//           return MidiPlayEvent(g2as._time, g2as._port, chn, ME_CONTROLLER, i_dataA, fin_db);
//           return MidiPlayEvent(time, port, chn, ME_CONTROLLER, i_dataA, fin_db);
          return false;
        }
        break;
      }
    break;
    
    case ME_POLYAFTER:
    {
      const int pitch = i_dataA & 0x7f;
      const int fin_da = (CTRL_POLYAFTER & ~0xff) | pitch;
//       iMidiCtrlValList imcvl = _controller->find(chn, fin_da);
//       
//       // Does the controller exist?
//       if(imcvl == _controller->end())
//         //return MidiPlayEvent();
//         //return true;
//         break;

      fin_db = limitValToInstrCtlRange(fin_da, i_dataB);
      // Set the value.
//       imcvl->second->setHwVal(fin_db);
      mcvl->setHwVal(fin_db);
      
//       return MidiPlayEvent(g2as._time, g2as._port, chn, ME_CONTROLLER, fin_da, fin_db);
//       return MidiPlayEvent(time, port, chn, ME_CONTROLLER, fin_da, fin_db);
      return false;
    }
    break;
    
    case ME_AFTERTOUCH:
    {
//       iMidiCtrlValList imcvl = _controller->find(chn, CTRL_AFTERTOUCH);
//       // Does the controller exist?
//       if(imcvl == _controller->end())
//         //return MidiPlayEvent();
//         //return true;
//         break;
      
      fin_db = limitValToInstrCtlRange(CTRL_AFTERTOUCH, i_dataA);
      // Set the value.
//       imcvl->second->setHwVal(fin_db);
      mcvl->setHwVal(fin_db);
      
//       return MidiPlayEvent(g2as._time, g2as._port, chn, ME_CONTROLLER, CTRL_AFTERTOUCH, fin_db);
//       return MidiPlayEvent(time, port, chn, ME_CONTROLLER, CTRL_AFTERTOUCH, fin_db);
      return false;
    }
    break;
    
    case ME_PITCHBEND:
    {
//       iMidiCtrlValList imcvl = _controller->find(chn, CTRL_PITCH);
//       // Does the controller exist?
//       if(imcvl == _controller->end())
//         //return MidiPlayEvent();
//         //return true;
//         break;
      
      fin_db = limitValToInstrCtlRange(CTRL_PITCH, i_dataA);
      // Set the value.
//       imcvl->second->setHwVal(fin_db);
      mcvl->setHwVal(fin_db);
      
//       return MidiPlayEvent(g2as._time, g2as._port, chn, ME_CONTROLLER, CTRL_PITCH, fin_db);
//       return MidiPlayEvent(time, port, chn, ME_CONTROLLER, CTRL_PITCH, fin_db);
      return false;
    }
    break;
    
    case ME_PROGRAM:
    {
      int hb = 0xff;
      int lb = 0xff;
      int pr = 0xff;
      if(!MidiController::iValIsUnknown(i_dataA))
        pr = i_dataA & 0xff;
      //if(pr != 0xff)
      //  pr = limitValToInstrCtlRange(da, pr);
      
//       iMidiCtrlValList imcvl = _controller->find(chn, CTRL_PROGRAM);
//       // Does the CTRL_PROGRAM controller exist?
//       if(imcvl == _controller->end())
//         //return MidiPlayEvent();
//         //return true;
//         break;
//       
//       MidiCtrlValList* mcvl = imcvl->second;
      if(!mcvl->hwValIsUnknown())
      {
        const int hw_val = mcvl->hwVal();
        hb = (hw_val >> 16) & 0xff;
        lb = (hw_val >> 8) & 0xff;
      }
      
      if((hb != 0xff || lb != 0xff) && pr == 0xff)
        pr = 0x01;
      if(hb == 0xff && lb == 0xff && pr == 0xff)
        fin_db = CTRL_VAL_UNKNOWN;
      else
        fin_db = (hb << 16) | (lb << 8) | pr;
      
      // Set the value. Be sure to update drum maps (and inform the gui).
      if(mcvl->setHwVal(fin_db))
        updateDrumMaps(chn, fin_db);
      
//       return MidiPlayEvent(g2as._time, g2as._port, chn, ME_CONTROLLER, CTRL_PROGRAM, fin_db);
//       return MidiPlayEvent(time, port, chn, ME_CONTROLLER, CTRL_PROGRAM, fin_db);
      return false;
    }
    break;
    
    default:
      // For all other event types, just return true.
      //return MidiPlayEvent(time, port, chn, type, i_dataA, i_dataB);
      //return true;
    break;
  }
  
//   return MidiPlayEvent(g2as._time, g2as._port, chn, type, i_dataA, i_dataB);
//   return MidiPlayEvent(time, port, chn, type, i_dataA, i_dataB);
  return true;
}

//---------------------------------------------------------
//   processGui2AudioEvents
//   To be called from audio thread only.
//   Return true if error.
//---------------------------------------------------------

// bool MidiPort::processGui2AudioEvents()
// {
//   const int gui_sz = _gui2AudioFifo->getSize();
//   const int osc_sz = _osc2AudioFifo->getSize();
//   // Receive events sent from our gui thread to this audio thread.
//   //while(!_gui2AudioFifo->isEmpty())
//   for(int i = 0; i < gui_sz; ++i)
//     // Update hardware state so knobs and boxes are updated. Optimize to avoid re-setting existing values.
//     handleGui2AudioEvent(_gui2AudioFifo->get()); // Don't care about return value.
// 
//   // REMOVE Tim. autoconnect. Added.
//   // Receive events sent from OSC to this audio thread.
//   // FIXME: The two fifos' processing needs to be interleaved and sorted according to time.
//   // FIXME: Unlike our own gui thread, for the OSC thread we cannot allocate a controller
//   //         if it does not exist, either from the OSC thread or here. For that, the OSC thread
//   //         needs to instead inform our gui thread and then our gui thread sends the allocation
//   //         message to the audio thread. (That is, our required way of gui-to-audio communication.)
//   //        We can't have two different threads (our gui and OSC) messaging the audio thread like that.
//   //while(!_osc2AudioFifo->isEmpty())
//   for(int i = 0; i < osc_sz; ++i)
//   {
//     // Update hardware state so knobs and boxes are updated. Optimize to avoid re-setting existing values.
//     //const MidiPlayEvent ev = _osc2AudioFifo->get();
//     //handleGui2AudioEvent(ev); // Don't care about return value. Oops, wrong structure - this fifo doesn't take MPE's.
//     //sendHwCtrlState(ev); // Don't care about return value.
//     //const MidiPlayEvent staged_ev = handleGui2AudioEvent(ev);
//     //const MidiPlayEvent staged_ev = handleGui2AudioEvent(Gui2AudioFifoStruct(_osc2AudioFifo->get()));
//     const MidiPlayEvent ev = _osc2AudioFifo->get();
//     handleGui2AudioEvent(Gui2AudioFifoStruct(ev));
//     
//     // Send to the device as well.
//     if(device())
//       //device()->addScheduledEvent(ev);
//       device()->addScheduledEvent(stageEvent(ev));
//   }
//   return false;
// }

//---------------------------------------------------------
//   processGui2AudioEvents
//   To be called from audio thread only.
//   Return true if error.
//---------------------------------------------------------

// Static.
bool MidiPort::processGui2AudioEvents()
{
  // Receive hardware state events sent from various threads to this audio thread.
  // Update hardware state so gui controls are updated.
  // False = don't use the size snapshot, but update it.
//   const int sz = eventFifos().getSize(false);
  const int sz = eventBuffers()->getSize(false);
  MidiPlayEvent ev;
  for(int i = 0; i < sz; ++i)
  {
//     // True = use the size snapshot.
//     const Gui2AudioFifoStruct g2as = eventFifos().get(true);
//     const int port = g2as._port;
    // True = use the size snapshot.
//     const MidiPlayEvent ev = eventFifos().get(true);
    if(!eventBuffers()->get(ev))
      continue;
    const int port = ev.port();
    if(port < 0 || port >= MIDI_PORTS)
      continue;
//     MusEGlobal::midiPorts[port].handleGui2AudioEvent(g2as);
    MusEGlobal::midiPorts[port].handleGui2AudioEvent(ev);
  }
  return false;
}

// REMOVE Tim. autoconnect. Added.
// //---------------------------------------------------------
// //   putHwCtrlEvent
// //   To be called from OSC handler only.
// //   Returns true if event cannot be delivered.
// //---------------------------------------------------------
// 
// bool MidiPort::putOSCHwCtrlEvent(const MidiPlayEvent& ev)
// {
//   MidiPlayEvent staged_ev;
//   const bool stage_res = stageEvent(staged_ev, ev);
//   if(stage_res && _osc2AudioFifo->put(staged_ev))
//   {
//     fprintf(stderr, "MidiPort::putOSCHwCtrlEvent: Error: osc2AudioFifo fifo overflow\n");
//     return true;
//   }
//   return false;
// }

// //---------------------------------------------------------
// //   putOSCEvent
// //   To be called from OSC handler only.
// //   Returns true if event cannot be delivered.
// //---------------------------------------------------------
// 
// bool MidiPort::putOSCEvent(const MidiPlayEvent& ev)
// {
// //   // Send the event to the device first so that current parameters could be updated on process.
// //   MidiPlayEvent staged_ev;
// //   const bool stage_res = stageEvent(staged_ev, ev);
// //   bool res = false;
// //   if(_device)
// //     // FIXME: Concurrency with putEvent(). Need putOSCEvent() etc.
// //     res = _device->putEvent(ev);
// //   if(stage_res && _osc2AudioFifo->put(staged_ev))
// //     fprintf(stderr, "MidiPort::putOSCEvent: Error: osc2AudioFifo fifo overflow\n");
// //   return res;
// 
// //   MidiPlayEvent staged_ev;
// //   const int stage_ctrl = stageEvent(staged_ev, ev);
// //   if(stage_ctrl < 0)
// //     return true;
// //   stageEvent(staged_ev, ev);
//   
// //   if(_osc2AudioFifo->put(staged_ev))
//   if(_osc2AudioFifo->put(ev))
//   {
//     fprintf(stderr, "MidiPort::putOSCHwCtrlEvent: Error: osc2AudioFifo fifo overflow\n");
//     return true;
//   }
//   
//   return false;
// }

//---------------------------------------------------------
//   sendHwCtrlState
//   Return true if it is OK to go ahead and deliver the event.
//---------------------------------------------------------

// bool MidiPort::sendHwCtrlState(const MidiPlayEvent& ev, bool forceSend)
//       {
//       if (ev.type() == ME_CONTROLLER) {
//       
//             int da = ev.dataA();
//             int db = ev.dataB();
// 
//             int chn = ev.channel();
// 
//             // TODO: Maybe update CTRL_HBANK/CTRL_LBANK - but ONLY if they are specifically
//             //        defined by the user in the controller list.
//             if(da == CTRL_HBANK)
//             {
//               int hb = da & 0xff;
//               int lb = 0xff;
//               int pr = 0xff;
//               if(_device)
//               {
//                 _device->curOutParamNums(chn)->currentProg(&pr, &lb, NULL);
//                 _device->curOutParamNums(chn)->BANKH = hb;
//               }
//               db = (hb << 16) | (lb << 8) | pr;
//               da = CTRL_PROGRAM;
//             }
//             else if(da == CTRL_LBANK)
//             {
//               int hb = 0xff;
//               int lb = da & 0xff;
//               int pr = 0xff;
//               if(_device)
//               {
//                 _device->curOutParamNums(chn)->currentProg(&pr, NULL, &hb);
//                 _device->curOutParamNums(chn)->BANKL = lb;
//               }
//               db = (hb << 16) | (lb << 8) | pr;
//               da = CTRL_PROGRAM;
//             }
//             else if(da == CTRL_PROGRAM)
//             {
//               // TODO: Maybe update CTRL_HBANK/CTRL_LBANK - but ONLY if they are specifically
//               //        defined by the user in the controller list.
//             }
//             else
//               db = limitValToInstrCtlRange(da, db);
//               
//             
//             if(!setHwCtrlState(chn, da, db)) {
//                 if (MusEGlobal::debugMsg && forceSend)
//                   printf("sendHwCtrlState: State already set. Forcing anyway...\n");
//                 if (!forceSend)
//                   return false;
//               }
//             }
//       else
//       if (ev.type() == ME_POLYAFTER)    // REMOVE Tim. Or keep? ...
//       {
//             int pitch = ev.dataA() & 0x7f;
//             int ctl = (CTRL_POLYAFTER & ~0xff) | pitch;
//             int db = limitValToInstrCtlRange(ctl, ev.dataB());
//             if(!setHwCtrlState(ev.channel(), ctl, db)) {
//               if (!forceSend)
//                 return false;
//             }
//       }
//       else
//       if (ev.type() == ME_AFTERTOUCH)  // ... Same
//       {
//             int da = limitValToInstrCtlRange(CTRL_AFTERTOUCH, ev.dataA());
//             if(!setHwCtrlState(ev.channel(), CTRL_AFTERTOUCH, da)) {
//               if (!forceSend)
//                 return false;
//             }
//       }
//       else
//       if (ev.type() == ME_PITCHBEND) 
//       {
//             int da = limitValToInstrCtlRange(CTRL_PITCH, ev.dataA());
//             if(!setHwCtrlState(ev.channel(), CTRL_PITCH, da)) {
//               if (!forceSend)
//                 return false;
//             }
//       }
//       else
//       if (ev.type() == ME_PROGRAM) 
//       {
//         int chn = ev.channel();
//         int hb = 0xff;
//         int lb = 0xff;
//         int pr = ev.dataA() & 0xff;
//         if(_device)
//         {
//           _device->curOutParamNums(chn)->currentProg(NULL, &lb, &hb);
//           _device->curOutParamNums(chn)->PROG = pr;
//         }
//         int full_prog = (hb << 16) | (lb << 8) | pr;
//         if(!setHwCtrlState(chn, CTRL_PROGRAM, full_prog)) {
//           if (!forceSend)
//               return false;
//         }
//       }
//       
//       return true;
//       }
bool MidiPort::sendHwCtrlState(const MidiPlayEvent& ev, bool forceSend)
      {
      const int type = ev.type();
      const int chn = ev.channel();
      const int da = ev.dataA();
      int fin_da = da;
      const int db = ev.dataB();
//       int fin_db = db;
      
      switch(type)
      {
        case ME_CONTROLLER:
          switch(da)
          {
            case CTRL_HBANK:
            {
              int hb = 0xff;
              if(!MidiController::iValIsUnknown(db))
                hb = db & 0xff;
              if(hb != 0xff)
                hb = limitValToInstrCtlRange(da, hb);
              int lb = 0xff;
              int pr = 0xff;
              
              fin_da = CTRL_PROGRAM;
              // This will create a new value list if necessary, otherwise it returns the existing list.
              // FIXME: This is not realtime safe because it may allocate.
              MidiCtrlValList* mcvl = addManagedController(chn, fin_da);
              if(!mcvl->hwValIsUnknown())
              {
                const int hw_val = mcvl->hwVal();
                lb = (hw_val >> 8) & 0xff;
                pr = hw_val & 0xff;
              }
              
              if((hb != 0xff || lb != 0xff) && pr == 0xff)
                pr = 0x01;
//               if(hb == 0xff && lb == 0xff && pr == 0xff)
//                 fin_db = CTRL_VAL_UNKNOWN;
//               else
//                 fin_db = (hb << 16) | (lb << 8) | pr;
            }
            break;

            case CTRL_LBANK:
            {
              int hb = 0xff;
              int lb = 0xff;
              if(!MidiController::iValIsUnknown(db))
                lb = db & 0xff;
              if(lb != 0xff)
                lb = limitValToInstrCtlRange(da, lb);
              int pr = 0xff;
              
              fin_da = CTRL_PROGRAM;
              // This will create a new value list if necessary, otherwise it returns the existing list.
              // FIXME: This is not realtime safe because it may allocate.
              MidiCtrlValList* mcvl = addManagedController(chn, fin_da);
              if(!mcvl->hwValIsUnknown())
              {
                const int hw_val = mcvl->hwVal();
                hb = (hw_val >> 16) & 0xff;
                pr = hw_val & 0xff;
              }
              
              if((hb != 0xff || lb != 0xff) && pr == 0xff)
                pr = 0x01;
//               if(hb == 0xff && lb == 0xff && pr == 0xff)
//                 fin_db = CTRL_VAL_UNKNOWN;
//               else
//                 fin_db = (hb << 16) | (lb << 8) | pr;
            }
            break;

            case CTRL_PROGRAM:
              // This will create a new value list if necessary, otherwise it returns the existing list.
              // FIXME: This is not realtime safe because it may allocate.
              addManagedController(chn, da);
              // TODO: Maybe update CTRL_HBANK/CTRL_LBANK - but ONLY if they are specifically
              //        defined by the user in the controller list.
            break;
            
            default:
              // This will create a new value list if necessary, otherwise it returns the existing list.
              // FIXME: This is not realtime safe because it may allocate.
              addManagedController(chn, da);
//               fin_db = limitValToInstrCtlRange(da, db);
            break;
          }
        break;
        
        case ME_POLYAFTER:
        {
          const int pitch = da & 0x7f;
          fin_da = (CTRL_POLYAFTER & ~0xff) | pitch;
          // This will create a new value list if necessary, otherwise it returns the existing list.
          // FIXME: This is not realtime safe because it may allocate.
          addManagedController(chn, fin_da);
//           fin_db = limitValToInstrCtlRange(fin_da, db);
        }
        break;
        
        case ME_AFTERTOUCH:
        {
          fin_da = CTRL_AFTERTOUCH;
          // This will create a new value list if necessary, otherwise it returns the existing list.
          // FIXME: This is not realtime safe because it may allocate.
          addManagedController(chn, fin_da);
//           fin_db = limitValToInstrCtlRange(fin_da, da);
        }
        break;
        
        case ME_PITCHBEND:
        {
          fin_da = CTRL_PITCH;
          // This will create a new value list if necessary, otherwise it returns the existing list.
          // FIXME: This is not realtime safe because it may allocate.
          addManagedController(chn, fin_da);
//           fin_db = limitValToInstrCtlRange(fin_da, da);
        }
        break;
        
        case ME_PROGRAM:
        {
          int hb = 0xff;
          int lb = 0xff;
          int pr = 0xff;
          if(!MidiController::iValIsUnknown(da))
            pr = da & 0xff;
          //if(pr != 0xff)
          //  pr = limitValToInstrCtlRange(da, pr);
          
          fin_da = CTRL_PROGRAM;
          // This will create a new value list if necessary, otherwise it returns the existing list.
          // FIXME: This is not realtime safe because it may allocate.
          MidiCtrlValList* mcvl = addManagedController(chn, fin_da);
          if(!mcvl->hwValIsUnknown())
          {
            const int hw_val = mcvl->hwVal();
            hb = (hw_val >> 16) & 0xff;
            lb = (hw_val >> 8) & 0xff;
          }
          
          if((hb != 0xff || lb != 0xff) && pr == 0xff)
            pr = 0x01;
//           if(hb == 0xff && lb == 0xff && pr == 0xff)
//             fin_db = CTRL_VAL_UNKNOWN;
//           else
//             fin_db = (hb << 16) | (lb << 8) | pr;
        }
        break;
        
        default:
          // For all other event types, just return true.
          return true;
        break;
      }
      
      if(!setHwCtrlState(chn, da, db)) {
          if (MusEGlobal::debugMsg && forceSend)
            printf("sendHwCtrlState: State already set. Forcing anyway...\n");
          if (!forceSend)
            return false;
        }
        
      return true;
      
      
/*      
      if (ev.type() == ME_CONTROLLER) {
      
            int da = ev.dataA();
            int db = ev.dataB();

            int chn = ev.channel();

            // TODO: Maybe update CTRL_HBANK/CTRL_LBANK - but ONLY if they are specifically
            //        defined by the user in the controller list.
            if(da == CTRL_HBANK)
            {
//               int hb = da & 0xff;
              int hb = db & 0xff;
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
//               int lb = da & 0xff;
              int lb = db & 0xff;
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
      
      if(!setHwCtrlState(chn, da, db)) {
          if (MusEGlobal::debugMsg && forceSend)
            printf("sendHwCtrlState: State already set. Forcing anyway...\n");
          if (!forceSend)
            return false;
        }
        
      return true;*/
      }

// REMOVE Tim. autoconnect. Removed.
// //---------------------------------------------------------
// //   sendEvent
// //    return true, if event cannot be delivered
// //---------------------------------------------------------
// 
// bool MidiPort::sendEvent(const MidiPlayEvent& ev, bool forceSend)
//       {
//       if(!sendHwCtrlState(ev, forceSend))
//         return false;
// 
//       if (!_device) {
//           if (MusEGlobal::debugMsg)
//             printf("no device for this midi port\n");
//           return true;
//           }
//       return _device->putEvent(ev);
//       }

// REMOVE Tim. autoconnect. Added.
// //---------------------------------------------------------
// //   handleSeek
// //---------------------------------------------------------
// 
// void MidiPort::handleSeek()
// {
// //   // If the device is not in use by a port, don't bother it.
// //   if(_port == -1)
// //     return;
//   
// //   MidiPort* mp = &MusEGlobal::midiPorts[_port];
//   MidiInstrument* instr = instrument();
//   MidiCtrlValListList* cll = controller();
//   unsigned pos = MusEGlobal::audio->tickPos();
//   
//   //---------------------------------------------------
//   //    Send STOP 
//   //---------------------------------------------------
//     
//   // Don't send if external sync is on. The master, and our sync routing system will take care of that.  
//   if(!MusEGlobal::extSyncFlag.value())
//   {
// //     if(mp->syncInfo().MRTOut())
//     if(syncInfo().MRTOut())
//     {
//       // Shall we check for device write open flag to see if it's ok to send?...
//       //if(!(rwFlags() & 0x1) || !(openFlags() & 1))
//       //if(!(openFlags() & 1))
//       //  continue;
// //       mp->sendStop();
//       sendStop();
//     }    
//   }
// 
//   //---------------------------------------------------
//   //    If playing, clear all notes and flush out any
//   //     stuck notes which were put directly to the device
//   //---------------------------------------------------
//   
// //   if(MusEGlobal::audio->isPlaying()) 
// //   {
// //     _playEvents.clear();
// //     for(iMPEvent i = _stuckNotes.begin(); i != _stuckNotes.end(); ++i) 
// //     {
// //       MidiPlayEvent ev(*i);
// //       ev.setTime(0);
// //       putEvent(ev);  // For immediate playback try putEvent, putMidiEvent, or sendEvent (for the optimizations).
// //     }
// //     _stuckNotes.clear();
// //   }
//   if(_device)
//   {
//     _device->handleSeek();
//   }
// 
//   
//   //---------------------------------------------------
//   //    Send new controller values
//   //---------------------------------------------------
//     
//   // Find channels on this port used in the song...
//   bool usedChans[MIDI_CHANNELS];
//   int usedChanCount = 0;
//   for(int i = 0; i < MIDI_CHANNELS; ++i)
//     usedChans[i] = false;
// //   if(MusEGlobal::song->click() && MusEGlobal::clickPort == _port)
//   if(MusEGlobal::song->click() && 
//     MusEGlobal::clickPort >= 0 && MusEGlobal::clickPort < MIDI_PORTS &&
//     &MusEGlobal::midiPorts[MusEGlobal::clickPort] == this)
//   {
//     usedChans[MusEGlobal::clickChan] = true;
//     ++usedChanCount;
//   }
//   bool drum_found = false;
//   for(ciMidiTrack imt = MusEGlobal::song->midis()->begin(); imt != MusEGlobal::song->midis()->end(); ++imt)
//   {
//     //------------------------------------------------------------
//     //    While we are at it, flush out any track-related playback stuck notes
//     //     (NOT 'live' notes) which were not put directly to the device
//     //------------------------------------------------------------
//     MPEventList& mel = (*imt)->stuckNotes;
//     for(iMPEvent i = mel.begin(), i_next = i; i != mel.end(); i = i_next)
//     {
//       ++i_next;
// 
// //       if((*i).port() != _port)
//       const int port = (*i).port();
//       if(port < 0 || port >= MIDI_PORTS || &MusEGlobal::midiPorts[port] != this)
//         continue;
//       MidiPlayEvent ev(*i);
//       ev.setTime(0);
//       putEvent(ev); // For immediate playback try putEvent, putMidiEvent, or sendEvent (for the optimizations).
//       mel.erase(i);
//     }
//     
//     if((*imt)->type() == MusECore::Track::DRUM)
//     {
//       if(!drum_found)
//       {
//         drum_found = true; 
//         for(int i = 0; i < DRUM_MAPSIZE; ++i)
//         {
//           // Default to track port if -1 and track channel if -1.
//           int mport = MusEGlobal::drumMap[i].port;
//           if(mport == -1)
//             mport = (*imt)->outPort();
//           int mchan = MusEGlobal::drumMap[i].channel;
//           if(mchan == -1)
//             mchan = (*imt)->outChannel();
// //           if(mport != _port || usedChans[mchan])
//           if(mport < 0 || mport >= MIDI_PORTS || &MusEGlobal::midiPorts[mport] != this || usedChans[mchan])
//             continue;
//           usedChans[mchan] = true;
//           ++usedChanCount;
//           if(usedChanCount >= MIDI_CHANNELS)
//             break;  // All are used, done searching.
//         }
//       }
//     }
//     else
//     {
//       const int mport = (*imt)->outPort();
//       if(mport < 0 || mport >= MIDI_PORTS || &MusEGlobal::midiPorts[mport] != this || usedChans[(*imt)->outChannel()])
//         continue;
//       usedChans[(*imt)->outChannel()] = true;
//       ++usedChanCount;
//     }
// 
//     if(usedChanCount >= MIDI_CHANNELS)
//       break;    // All are used. Done searching.
//   }   
//   
//   for(iMidiCtrlValList ivl = cll->begin(); ivl != cll->end(); ++ivl) 
//   {
//     MidiCtrlValList* vl = ivl->second;
//     int chan = ivl->first >> 24;
//     if(!usedChans[chan])  // Channel not used in song?
//       continue;
//     int ctlnum = vl->num();
// 
//     // Find the first non-muted value at the given tick...
//     bool values_found = false;
//     bool found_value = false;
//     
//     iMidiCtrlVal imcv = vl->lower_bound(pos);
//     if(imcv != vl->end() && imcv->first == (int)pos)
//     {
//       for( ; imcv != vl->end() && imcv->first == (int)pos; ++imcv)
//       {
//         const Part* p = imcv->second.part;
//         if(!p)
//           continue;
//         // Ignore values that are outside of the part.
//         if(pos < p->tick() || pos >= (p->tick() + p->lenTick()))
//           continue;
//         values_found = true;
//         // Ignore if part or track is muted or off.
//         if(p->mute())
//           continue;
//         const Track* track = p->track();
//         if(track && (track->isMute() || track->off()))
//           continue;
//         found_value = true;
//         break;
//       }
//     }
//     else
//     {
//       while(imcv != vl->begin())
//       {
//         --imcv;
//         const Part* p = imcv->second.part;
//         if(!p)
//           continue;
//         // Ignore values that are outside of the part.
//         unsigned t = imcv->first;
//         if(t < p->tick() || t >= (p->tick() + p->lenTick()))
//           continue;
//         values_found = true;
//         // Ignore if part or track is muted or off.
//         if(p->mute())
//           continue;
//         const Track* track = p->track();
//         if(track && (track->isMute() || track->off()))
//           continue;
//         found_value = true;
//         break;
//       }
//     }
// 
//     if(found_value)
//     {
// //       int fin_port = _port;
// //       MidiPort* fin_mp = mp;
//       MidiPort* fin_mp = this;
//       int fin_chan = chan;
//       int fin_ctlnum = ctlnum;
//       // Is it a drum controller event, according to the track port's instrument?
// //       if(mp->drumController(ctlnum))
//       if(drumController(ctlnum))
//       {
//         if(const Part* p = imcv->second.part)
//         {
//           if(Track* t = p->track())
//           {
//             if(t->type() == MusECore::Track::NEW_DRUM)
//             {
//               MidiTrack* mt = static_cast<MidiTrack*>(t);
//               int v_idx = ctlnum & 0x7f;
//               fin_ctlnum = (ctlnum & ~0xff) | mt->drummap()[v_idx].anote;
//               int map_port = mt->drummap()[v_idx].port;
//               if(map_port != -1)
//               {
//                 fin_port = map_port;
//                 fin_mp = &MusEGlobal::midiPorts[fin_port];
//               }
//               int map_chan = mt->drummap()[v_idx].channel;
//               if(map_chan != -1)
//                 fin_chan = map_chan;
//             }
//           }
//         }
//       }
// 
//       // Don't bother sending any sustain values if not playing. Just set the hw state.
//       if(fin_ctlnum == CTRL_SUSTAIN && !MusEGlobal::audio->isPlaying())
//         fin_mp->setHwCtrlState(fin_chan, CTRL_SUSTAIN, imcv->second.val);
//       else
//       {
//         // Use sendEvent to get the optimizations and limiting. But force if there's a value at this exact position.
//         // NOTE: Why again was this forced? There was a reason. Think it was RJ in response to bug rep, then I modded.
//         // A reason not to force: If a straight line is drawn on graph, multiple identical events are stored
//         //  (which must be allowed). So seeking through them here sends them all redundantly, not good. // REMOVE Tim.
//         //fprintf(stderr, "MidiDevice::handleSeek: found_value: calling sendEvent: ctlnum:%d val:%d\n", ctlnum, imcv->second.val);
//         fin_mp->sendEvent(MidiPlayEvent(0, fin_port, fin_chan, ME_CONTROLLER, fin_ctlnum, imcv->second.val), false); //, imcv->first == pos);
//         //mp->sendEvent(MidiPlayEvent(0, _port, chan, ME_CONTROLLER, ctlnum, imcv->second.val), pos == 0 || imcv->first == pos);
//       }
//     }
// 
//     // Either no value was found, or they were outside parts, or pos is in the unknown area before the first value.
//     // Send instrument default initial values.  NOT for syntis. Use midiState and/or initParams for that. 
//     //if((imcv == vl->end() || !done) && !MusEGlobal::song->record() && instr && !isSynti()) 
//     // Hmm, without refinement we can only do this at position 0, due to possible 'skipped' values outside parts, above.
//     if(!values_found && MusEGlobal::config.midiSendCtlDefaults && !MusEGlobal::song->record() && pos == 0 && instr && !isSynti())
//     {
//       MidiControllerList* mcl = instr->controller();
//       ciMidiController imc = mcl->find(vl->num());
//       if(imc != mcl->end())
//       {
//         MidiController* mc = imc->second;
//         if(mc->initVal() != CTRL_VAL_UNKNOWN)
//         {
//           //fprintf(stderr, "MidiDevice::handleSeek: !values_found: calling sendEvent: ctlnum:%d val:%d\n", ctlnum, mc->initVal() + mc->bias());
//           // Use sendEvent to get the optimizations and limiting. No force sending. Note the addition of bias.
//           mp->sendEvent(MidiPlayEvent(0, _port, chan, ME_CONTROLLER, ctlnum, mc->initVal() + mc->bias()), false);
//         }
//       }
//     }
//   }
//   
//   //---------------------------------------------------
//   //    reset sustain
//   //---------------------------------------------------
//   
//   for(int ch = 0; ch < MIDI_CHANNELS; ++ch) 
//   {
//     if(mp->hwCtrlState(ch, CTRL_SUSTAIN) == 127) 
//     {
//       const MidiPlayEvent ev(0, _port, ch, ME_CONTROLLER, CTRL_SUSTAIN, 0);
//       putEvent(ev);
//     }
//   }
//   
//   //---------------------------------------------------
//   //    Send STOP and "set song position pointer"
//   //---------------------------------------------------
//     
//   // Don't send if external sync is on. The master, and our sync routing system will take care of that.  
//   if(!MusEGlobal::extSyncFlag.value())
//   {
//     if(mp->syncInfo().MRTOut())
//     {
//       //mp->sendStop();   // Moved above
//       int beat = (pos * 4) / MusEGlobal::config.division;
//       mp->sendSongpos(beat);
//     }    
//   }
// }
      
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
//   lastValidHWCtrlState
//---------------------------------------------------------

double MidiPort::lastValidHWDCtrlState(int ch, int ctrl) const
{
      ch &= 0xff;
      ciMidiCtrlValList cl = ((const MidiCtrlValListList*)_controller)->find(ch, ctrl);
      if (cl == _controller->end()) {
            return CTRL_VAL_UNKNOWN;
            }
      MidiCtrlValList* vl = cl->second;
      return vl->lastValidHWDVal();
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
//   hwDCtrlState
//---------------------------------------------------------

double MidiPort::hwDCtrlState(int ch, int ctrl) const
      {
      ch &= 0xff;
      ciMidiCtrlValList cl = ((const MidiCtrlValListList*)_controller)->find(ch, ctrl);
      if (cl == _controller->end())
            return CTRL_VAL_UNKNOWN;
      MidiCtrlValList* vl = cl->second;
      return vl->hwDVal();
      }

//---------------------------------------------------------
//   setHwCtrlState
//   If audio is running (and not idle) this should only be called by the rt audio thread.
//   Returns false if value is already equal, true if value is set.
//---------------------------------------------------------

bool MidiPort::setHwCtrlState(int ch, int ctrl, int val)
      {
      // This will create a new value list if necessary, otherwise it returns the existing list.
      MidiCtrlValList* vl = addManagedController(ch, ctrl);

      bool res = vl->setHwVal(val);
      // If program controller be sure to update drum maps (and inform the gui).
      if(res && ctrl == CTRL_PROGRAM)
        updateDrumMaps(ch, val);

      return res;
      }

bool MidiPort::setHwCtrlState(int ch, int ctrl, double val)
      {
      // This will create a new value list if necessary, otherwise it returns the existing list.
      MidiCtrlValList* vl = addManagedController(ch, ctrl);

      bool res = vl->setHwVal(val);
      // If program controller be sure to update drum maps (and inform the gui).
      if(res && ctrl == CTRL_PROGRAM)
        updateDrumMaps(ch, val);

      return res;
      }

//---------------------------------------------------------
//   setHwCtrlStates
//   If audio is running (and not idle) this should only be called by the rt audio thread.
//   Sets current and last HW values.
//   Handy for forcing labels to show 'off' and knobs to show specific values 
//    without having to send two messages.
//   Returns false if both values are already set, true if either value is changed.
//---------------------------------------------------------

bool MidiPort::setHwCtrlStates(int ch, int ctrl, int val, int lastval)
      {
      // This will create a new value list if necessary, otherwise it returns the existing list. 
      MidiCtrlValList* vl = addManagedController(ch, ctrl);

      // This is not perfectly ideal, drum maps should not have to (check) change if last hw val changed.
      bool res = vl->setHwVals(val, lastval);
      // If program controller be sure to update drum maps (and inform the gui).
      if(res && ctrl == CTRL_PROGRAM)
        updateDrumMaps(ch, val);

      return res;
      }

bool MidiPort::setHwCtrlStates(int ch, int ctrl, double val, double lastval)
      {
      // This will create a new value list if necessary, otherwise it returns the existing list.
      MidiCtrlValList* vl = addManagedController(ch, ctrl);

      // This is not perfectly ideal, drum maps should not have to (check) change if last hw val changed.
      bool res = vl->setHwVals(val, lastval);
      // If program controller be sure to update drum maps (and inform the gui).
      if(res && ctrl == CTRL_PROGRAM)
        updateDrumMaps(ch, val);

      return res;
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
//   updateDrumMaps
//   If audio is running (and not idle) this should only be called by the rt audio thread.
//   Returns true if maps were changed.
//---------------------------------------------------------

bool MidiPort::updateDrumMaps(int chan, int patch)
{
  int port;
  int tpatch;
  int tchan;
  bool map_changed = false;
  MidiTrack* mt;
  for(iMidiTrack t = MusEGlobal::song->midis()->begin(); t != MusEGlobal::song->midis()->end(); ++t)
  {
    mt = *t;
    if(mt->type() != Track::NEW_DRUM)
      continue;
    port = mt->outPort();
    if(port < 0 || port >= MIDI_PORTS || &MusEGlobal::midiPorts[port] != this)
      continue;
    tchan = mt->outChannel();
    if(tchan != chan)
      continue;
    tpatch = hwCtrlState(tchan, CTRL_PROGRAM);
    if(tpatch != patch)
      continue;
    if(mt->updateDrummap(false)) // false = don't signal gui thread, we'll do that here.
      map_changed = true;
  }

  if(map_changed)
  {
    // It is possible we are being called from gui thread already, in audio idle mode.
    // Will this still work, and not conflict with audio sending the same message?
    // Are we are not supposed to write to an fd from different threads?
    if(!MusEGlobal::audio || MusEGlobal::audio->isIdle())
      // Directly emit SC_DRUMMAP song changed signal.
      MusEGlobal::song->update(SC_DRUMMAP);
    else
      // Tell the gui to emit SC_DRUMMAP song changed signal.
      MusEGlobal::audio->sendMsgToGui('D'); // Drum map changed.

    return true;
  }

  return false;
}

//---------------------------------------------------------
//   updateDrumMaps
//   If audio is running (and not idle) this should only be called by the rt audio thread.
//   Returns true if maps were changed.
//---------------------------------------------------------

bool MidiPort::updateDrumMaps()
{
  int port;
  bool map_changed;
  MidiTrack* mt;
  for(iMidiTrack t = MusEGlobal::song->midis()->begin(); t != MusEGlobal::song->midis()->end(); ++t)
  {
    mt = *t;
    if(mt->type() != Track::NEW_DRUM)
      continue;
    port = mt->outPort();
    if(port < 0 || port >= MIDI_PORTS || &MusEGlobal::midiPorts[port] != this)
      continue;
    if(mt->updateDrummap(false)) // false = don't signal gui thread, we'll do that here.
      map_changed = true;
  }

  if(map_changed)
  {
    // It is possible we are being called from gui thread already, in audio idle mode.
    // Will this still work, and not conflict with audio sending the same message?
    // Are we are not supposed to write to an fd from different threads?
    //if(MusEGlobal::audio && MusEGlobal::audio->isIdle() && MusEGlobal::midiSeq && MusEGlobal::midiSeq->isIdle())
    if(!MusEGlobal::audio || MusEGlobal::audio->isIdle())
      // Directly emit SC_DRUMMAP song changed signal.
      //MusEGlobal::song->update(SC_DRUMMAP, true);
      MusEGlobal::song->update(SC_DRUMMAP);
    else
      // Tell the gui to emit SC_DRUMMAP song changed signal.
      MusEGlobal::audio->sendMsgToGui('D'); // Drum map changed.

    return true;
  }

  return false;
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

int MidiPort::getVisibleCtrl(int ch, int tick, int ctrl, bool inclMutedParts, bool inclMutedTracks, bool inclOffTracks) const
      {
      iMidiCtrlValList cl = _controller->find(ch, ctrl);
      if (cl == _controller->end())
            return CTRL_VAL_UNKNOWN;

      return cl->second->visibleValue(tick, inclMutedParts, inclMutedTracks, inclOffTracks);
      }

int MidiPort::getVisibleCtrl(int ch, int tick, int ctrl, Part* part, bool inclMutedParts, bool inclMutedTracks, bool inclOffTracks) const
      {
      iMidiCtrlValList cl = _controller->find(ch, ctrl);
      if (cl == _controller->end())
            return CTRL_VAL_UNKNOWN;

      return cl->second->visibleValue(tick, part, inclMutedParts, inclMutedTracks, inclOffTracks);
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
      MidiController* c = new MidiController(name, num, min, max, 0, 0);
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
