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

#include <QString>

#include "midiport.h"
#include "midi_consts.h"
#include "midiseq.h"
#include "gconfig.h"
#include "globals.h"
#include "synth.h"
#include "app.h"
#include "song.h"
//#include "menutitleitem.h"
//#include "icons.h"
#include "track.h"
#include "drummap.h"
#include "audio.h"
#include "muse_math.h"
#include "sysex_helper.h"

// Forwards from header:
#include "mididev.h"
#include "instruments/minstrument.h"
#include "part.h"
#include "midi_controller.h"
#include "midictrl.h"
#include "mpevent.h"
#include "xml.h"

namespace MusEGlobal {
MusECore::MidiPort midiPorts[MusECore::MIDI_PORTS];
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
        
      for (int i = 0; i < MusECore::MIDI_PORTS; ++i) {
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

              // robert: removing the default init on several places to allow for the case
              // where you rather want the midi track to default to the last created port
              // this can only happen if there is _no_ default set
              //
              // port->setDefaultOutChannels(1);

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
      _initializationsSent = false;  
      _defaultInChannels  = 0;
      _defaultOutChannels = 0;
      _device     = 0;
      _instrument = 0;
      _tmpTrackIdx = -1;
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

void MidiPort::setMidiDevice(MidiDevice* dev, MidiInstrument* instrument)
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
            for (int i = 0; i < MusECore::MIDI_PORTS; ++i) {
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
            // If an instrument was given, use it. Otherwise don't touch the instrument.
            if(instrument)
              _instrument = instrument;
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
  if(!_device || !_device->writeEnable())   // Not writable?
    return false;
  
  bool rv = true;
  const int port = portno();
  
  //
  // test for explicit instrument initialization
  //

//   unsigned last_tick = 0;
  unsigned last_frame = 0;
  const MusECore::MidiInstrument* instr = instrument();
  if(instr && MusEGlobal::config.midiSendInit && (force || !_initializationsSent))
  {
    // Send the Instrument Init sequences.
    const EventList* events = instr->midiInit();
    if(!events->empty())
    {
      for(ciEvent ie = events->cbegin(); ie != events->cend(); ++ie)
      {
        if(ie->second.type() == Sysex)
          last_frame += sysexDuration(ie->second.dataLen(), MusEGlobal::sampleRate);
        MusECore::MidiPlayEvent ev = ie->second.asMidiPlayEvent(last_frame + MusEGlobal::audio->curSyncFrame(), port, 0);
        _device->putEvent(ev, MidiDevice::NotLate);
      }
      // Give a bit of time for the last Init sysex to settle?
      last_frame += 100;
    }
    _initializationsSent = true; // Mark as having been sent.
  }
    
  // Send the Instrument controller default values.
  sendInitialControllers(last_frame);

  return rv;
}
      
//---------------------------------------------------------
//   sendInitialControllers
//   Return true if success.
//---------------------------------------------------------

bool MidiPort::sendInitialControllers(unsigned start_time)
{
  if(!_device)
    return false;

  MusECore::MetronomeSettings* metro_settings = 
    MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

  bool rv = true;
  const int port = portno();
  
  // Find all channels of this port used in the song...
  bool usedChans[MusECore::MUSE_MIDI_CHANNELS];
  int usedChanCount = 0;
  for(int i = 0; i < MusECore::MUSE_MIDI_CHANNELS; ++i)
    usedChans[i] = false;
  if(MusEGlobal::song->click() && metro_settings->clickPort == port)
  {
    usedChans[metro_settings->clickChan] = true;
    ++usedChanCount;
  }
  for(ciMidiTrack imt = MusEGlobal::song->midis()->cbegin(); imt != MusEGlobal::song->midis()->cend(); ++imt)
  {
    if((*imt)->type() == MusECore::Track::DRUM)
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
        if(usedChanCount >= MusECore::MUSE_MIDI_CHANNELS)
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
    
    if(usedChanCount >= MusECore::MUSE_MIDI_CHANNELS)
      break;  // All are used, done searching.
  }

  // NOT for syntis. Use midiState and/or initParams for that. 
  if(MusEGlobal::config.midiSendInit && MusEGlobal::config.midiSendCtlDefaults && _instrument && !_device->isSynti())
  {
    MusECore::MidiControllerList* cl = new MusECore::MidiControllerList();

    const MidiController* mc;

    for(int chan = 0; chan < MusECore::MUSE_MIDI_CHANNELS; ++chan)
    {
      if(!usedChans[chan])
        continue;  // This channel on this port is not used in the song.

      const int patch = hwCtrlState(chan, CTRL_PROGRAM);
      cl->clear();
      _instrument->getControllers(cl, chan, patch);

      // Returns true if any of the EIGHT reserved General Midi (N)RPN control numbers are
      //  ALREADY defined as Controller7 or part of Controller14. Cached, for speed.
      const bool patch_rpn_reserved = _instrument->RPN_Ctrls_Reserved(chan, patch);
      if(!patch_rpn_reserved)
      {
        // Start by nulling these controllers, to avoid any accidental settings later.
        _device->putEvent(MidiPlayEvent(start_time, port, chan, ME_CONTROLLER, CTRL_HRPN, 0x7f), MidiDevice::NotLate);
        _device->putEvent(MidiPlayEvent(start_time, port, chan, ME_CONTROLLER, CTRL_LRPN, 0x7f), MidiDevice::NotLate);
        _device->putEvent(MidiPlayEvent(start_time, port, chan, ME_CONTROLLER, CTRL_HNRPN, 0x7f), MidiDevice::NotLate);
        _device->putEvent(MidiPlayEvent(start_time, port, chan, ME_CONTROLLER, CTRL_LNRPN, 0x7f), MidiDevice::NotLate);
      }

      for(ciMidiController imc = cl->begin(); imc != cl->end(); ++imc)
      {
        mc = imc->second;
        ciMidiCtrlValList i;

        switch(mc->num())
        {
          // Allow these initial values if the controller list does not have standard RPN. (The enums alias to generic numbers).
          // Do not send any of these initial values if the controller has standard RPN. (The enums are meaningful).
          // 1) It is impossible to know which should come first/last (ie. which was last adjusted) - the RPNs or the data.
          // 2) This is an ineffective way to set a default value of one single RPN controller. Better use our built-ins.
          case CTRL_HRPN:
          case CTRL_LRPN:
          case CTRL_HNRPN:
          case CTRL_LNRPN:
          case CTRL_HDATA:
          case CTRL_LDATA:
          case CTRL_DATA_INC:
          case CTRL_DATA_DEC:
            if(!patch_rpn_reserved)
              continue;
          break;

          default:
          break;
        }

        // Look for an initial value for this midi controller, on this midi channel, in the song...
        for(i = _controller->cbegin(); i != _controller->cend(); ++i)
        {
          const int channel = i->first >> 24;
          const int cntrl   = i->first & 0xffffff;
          const int val     = i->second->hwVal();
          if(channel == chan && cntrl == mc->num() && val != CTRL_VAL_UNKNOWN)
            break;
        }  
        // If no initial value was found for this midi controller, on this midi channel, in the song...
        if(i == _controller->cend())
        {
          // If the instrument's midi controller has an initial value, send it now.
          if(mc->initVal() != CTRL_VAL_UNKNOWN)
          {
            const int ctl = mc->num();
            // Note the addition of bias!
            _device->putEvent(MidiPlayEvent(start_time, port, chan,
              ME_CONTROLLER, ctl, mc->initVal() + mc->bias()), MidiDevice::NotLate);
            // Set it once so the 'last HW value' is set, and control knobs are positioned at the value...
            // Set it again so that control labels show 'off'...
            setHwCtrlStates(chan, ctl, CTRL_VAL_UNKNOWN, mc->initVal() + mc->bias());
          }    
        }    
      }
    }
    delete cl;
  }

  // Returns true if any of the EIGHT reserved General Midi (N)RPN control numbers are
  //  ALREADY defined as Controller7 or part of Controller14. Cached, for speed.
  const bool instr_rpn_reserved = _instrument && !_device->isSynti() && _instrument->RPN_Ctrls_Reserved();

  // init HW controller state
  for (ciMidiCtrlValList i = _controller->cbegin(); i != _controller->cend(); ++i)
  {
      const int channel = i->first >> 24;
      if(!usedChans[channel])
        continue;  // This channel on this port is not used in the song.
      const int cntrl = i->first & 0xffffff;

      // Allow these values if the controller list does not have standard RPN. (The enums alias to generic numbers).
      // Do not send any of these values if the controller has standard RPN. (The enums are meaningful).
      // 1) It is impossible to know which should come first/last (ie. which was last adjusted) - the RPNs or the data.
      // 2) This is an ineffective way to set a default value of one single RPN controller. Better use our built-ins.
      switch(cntrl)
      {
        case CTRL_HRPN:
        case CTRL_LRPN:
        case CTRL_HNRPN:
        case CTRL_LNRPN:
        case CTRL_HDATA:
        case CTRL_LDATA:
        case CTRL_DATA_INC:
        case CTRL_DATA_DEC:
          if(!instr_rpn_reserved)
            continue;
        break;

        default:
        break;
      }

      const int val = i->second->hwVal();
      if (val != CTRL_VAL_UNKNOWN)
      {
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
      for (int i = 0; i < MusECore::MIDI_PORTS; ++i) {
            if (&MusEGlobal::midiPorts[i] == this)
                  return i;
            }
      return -1;
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
//   sendSysex
//    send SYSEX message to midi device
//---------------------------------------------------------

void MidiPort::sendSysex(const unsigned char* p, int n)
      {
      if (_device) {
            MidiPlayEvent event(0, 0, ME_SYSEX, p, n);
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
  for (int i = 0; i < MusECore::MUSE_MIDI_CHANNELS; ++i) {
        for(ciMidiController imc = defaultManagedMidiController.begin(); imc != defaultManagedMidiController.end(); ++imc)
          addManagedController(i, imc->second->num());   
        _automationType[i] = AUTO_READ;
        }
}
      
//---------------------------------------------------------
//   limitValToInstrCtlRange
//---------------------------------------------------------

int MidiPort::limitValToInstrCtlRange(const MidiController* mc, int val)
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
            
int MidiPort::limitValToInstrCtlRange(int ctl, int val, int chan)
{
  if(!_instrument || val == CTRL_VAL_UNKNOWN)
    return val;
    
  // FIXME: This might be optimized by calling midiController instead,
  //         and simply asking if it's a drum controller. Saves one list iteration.
  // Is it a drum controller?
  // Note: Midnam apparently has no concept of drum controllers, so the fact
  //  that this only asks in the instrument's controllers and not the midnam is OK.
  const MidiController *mc = drumController(ctl);
  if(!mc)
  {
    // It's not a drum controller. Find it as a regular controller instead.
    const int patch = hwCtrlState(chan, CTRL_PROGRAM);
    mc = _instrument->findController(ctl, chan, patch);
  }
  
  // If it's a valid controller, limit the value to the instrument controller range.
  if(mc)
    return limitValToInstrCtlRange(mc, val);
  
  return val;
}

double MidiPort::limitValToInstrCtlRange(const MidiController* mc, double val)
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

double MidiPort::limitValToInstrCtlRange(int ctl, double val, int chan)
{
  if(!_instrument || int(val) == CTRL_VAL_UNKNOWN)
    return val;

  // FIXME: This might be optimized by calling midiController instead,
  //         and simply asking if it's a drum controller. Saves one list iteration.
  // Is it a drum controller?
  // Note: Midnam apparently has no concept of drum controllers, so the fact
  //  that this only asks in the instrument's controllers and not the midnam is OK.
  const MidiController *mc = drumController(ctl);
  if(!mc)
  {
    // It's not a drum controller. Find it as a regular controller instead.
    const int patch = hwCtrlState(chan, CTRL_PROGRAM);
    mc = _instrument->findController(ctl, chan, patch);
  }

  // If it's a valid controller, limit the value to the instrument controller range.
  if(mc)
    return limitValToInstrCtlRange(mc, val);

  return val;
}

//---------------------------------------------------------
//  createController
//   Creates a controller in this port's controller list.
//   Returns true if the controller was created.
//   To be called by gui thread only.
//---------------------------------------------------------

bool MidiPort::createController(int chan, int ctrl)
{
  if(ctrl < 0 || chan < 0 || chan >= MusECore::MUSE_MIDI_CHANNELS)
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
  const int ctrl = ev.translateCtrlNum();

  // Event not translatable to a controller?
  if(ctrl < 0)
    return true;

  // Make sure to create the controller if necessary.
  //createController(chan, ctrl);
  
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
  
  if(!MusEGlobal::song->putIpcOutEvent(ev))
  {
    fprintf(stderr, "MidiPort::putHwCtrlEvent: Error: gui2AudioFifo fifo overflow\n");
    return true;
  }
  
  return false;
}

//---------------------------------------------------------
//   putEvent
//   To be called from gui thread only.
//   Returns true if event cannot be delivered.
//---------------------------------------------------------

bool MidiPort::putEvent(const MidiPlayEvent& ev)
{
  // Send the event to the device first so that current parameters could be updated on process.
  bool res = false;
  if(_device)
  {
    res = !_device->putEvent(ev, MidiDevice::Late);
  }
  putHwCtrlEvent(ev);
  return res;
}

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
  MusECore::MidiController* mc = midiController(ctlnum, chan, false);
  if(!mc)
    return true;
  const int max = mc->maxVal();

  if(isDb)
    val = double(max) * muse_db2val(val / 2.0);

  const int i_new_val = MidiController::dValToInt(val);

  // Time-stamp the event.
  MidiPlayEvent ev(MusEGlobal::audio->curFrame(), port, chan, MusECore::ME_CONTROLLER, ctlnum, i_new_val);
  bool res = false;
  if(_device)
  {
    res = !_device->putEvent(ev, MidiDevice::Late);
  }

  putHwCtrlEvent(ev);
  return res;
}

//---------------------------------------------------------
//   handleGui2AudioEvent
//   To be called from audio thread only.
//   Returns true on success.
//   If createAsNeeded is true, automatically send a message to the gui thread to
//    create items such as controllers, and cache the events sent to it and re-put
//    them after the controller has been created.
//---------------------------------------------------------

bool MidiPort::handleGui2AudioEvent(const MidiPlayEvent& ev, bool createAsNeeded)
{
  const int chn = ev.channel();
  const int type = ev.type();
  const int i_dataA = ev.dataA();
  const double d_dataB = ev.dataB();
  const int i_dataB = MidiController::dValToInt(d_dataB);
  
  int fin_db = i_dataB;
  switch(type)
  {
    case ME_CONTROLLER:
      switch(i_dataA)
      {
        case CTRL_HBANK:
        {
          // Does the CTRL_PROGRAM controller exist?
          iMidiCtrlValList imcvl = _controller->find(chn, CTRL_PROGRAM);
          if(imcvl == _controller->end())
          {
            // Tell the gui to create the controller and add the value.
            if(createAsNeeded)
              return MusEGlobal::song->putIpcInEvent(ev);
            return false;
          }
          
          int hb = 0xff;
          if(!MidiController::iValIsUnknown(i_dataB))
            hb = i_dataB & 0xff;
          if(hb != 0xff)
            hb = limitValToInstrCtlRange(i_dataA, hb, chn);
          int lb = 0xff;
          int pr = 0xff;
          
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
          
          return true;
        }
        break;

        case CTRL_LBANK:
        {
          // Does the CTRL_PROGRAM controller exist?
          iMidiCtrlValList imcvl = _controller->find(chn, CTRL_PROGRAM);
          if(imcvl == _controller->end())
          {
            // Tell the gui to create the controller and add the value.
            if(createAsNeeded)
              return MusEGlobal::song->putIpcInEvent(ev);
            return false;
          }
          
          int hb = 0xff;
          int lb = 0xff;
          if(!MidiController::iValIsUnknown(i_dataB))
            lb = i_dataB & 0xff;
          if(lb != 0xff)
            lb = limitValToInstrCtlRange(i_dataA, lb, chn);
          int pr = 0xff;
          
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
          
          return true;
        }
        break;

        case CTRL_PROGRAM:
        {
          // TODO: Maybe update CTRL_HBANK/CTRL_LBANK - but ONLY if they are specifically
          //        defined by the user in the controller list.
            
          // Does the CTRL_PROGRAM controller exist?
          iMidiCtrlValList imcvl = _controller->find(chn, CTRL_PROGRAM);
          if(imcvl == _controller->end())
          {
            // Tell the gui to create the controller and add the value.
            if(createAsNeeded)
              return MusEGlobal::song->putIpcInEvent(ev);
            return false;
          }
          
          // Set the value. Be sure to update drum maps (and inform the gui).
          if(imcvl->second->setHwVal(fin_db))
            updateDrumMaps(chn, fin_db);
          
          return true;
        }
        break;
        
        default:
        {
          // Does the controller exist?
          iMidiCtrlValList imcvl = _controller->find(chn, i_dataA);
          if(imcvl == _controller->end())
          {
            // Tell the gui to create the controller and add the value.
            if(createAsNeeded)
              return MusEGlobal::song->putIpcInEvent(ev);
            return false;
          }

          fin_db = limitValToInstrCtlRange(i_dataA, i_dataB, chn);
          // Set the value.
          imcvl->second->setHwVal(fin_db);
          
          return true;
        }
        break;
      }
    break;
    
    case ME_POLYAFTER:
    {
      const int pitch = i_dataA & 0x7f;
      const int fin_da = (CTRL_POLYAFTER & ~0xff) | pitch;
      
      // Does the controller exist?
      iMidiCtrlValList imcvl = _controller->find(chn, fin_da);
      if(imcvl == _controller->end())
      {
        // Tell the gui to create the controller and add the value.
        if(createAsNeeded)
          return MusEGlobal::song->putIpcInEvent(ev);
        return false;
      }

      fin_db = limitValToInstrCtlRange(fin_da, i_dataB, chn);
      // Set the value.
      imcvl->second->setHwVal(fin_db);
      
      return true;
    }
    break;
    
    case ME_AFTERTOUCH:
    {
      // Does the controller exist?
      iMidiCtrlValList imcvl = _controller->find(chn, CTRL_AFTERTOUCH);
      if(imcvl == _controller->end())
      {
        // Tell the gui to create the controller and add the value.
        if(createAsNeeded)
          return MusEGlobal::song->putIpcInEvent(ev);
        return false;
      }

      fin_db = limitValToInstrCtlRange(CTRL_AFTERTOUCH, i_dataA, chn);
      // Set the value.
      imcvl->second->setHwVal(fin_db);
      
      return true;
    }
    break;
    
    case ME_PITCHBEND:
    {
      // Does the controller exist?
      iMidiCtrlValList imcvl = _controller->find(chn, CTRL_PITCH);
      if(imcvl == _controller->end())
      {
        // Tell the gui to create the controller and add the value.
        if(createAsNeeded)
          return MusEGlobal::song->putIpcInEvent(ev);
        return false;
      }

      fin_db = limitValToInstrCtlRange(CTRL_PITCH, i_dataA, chn);
      // Set the value.
      imcvl->second->setHwVal(fin_db);
      
      return true;
    }
    break;
    
    case ME_PROGRAM:
    {
      // Does the controller exist?
      iMidiCtrlValList imcvl = _controller->find(chn, CTRL_PROGRAM);
      if(imcvl == _controller->end())
      {
        // Tell the gui to create the controller and add the value.
        if(createAsNeeded)
          return MusEGlobal::song->putIpcInEvent(ev);
        return false;
      }

      int hb = 0xff;
      int lb = 0xff;
      int pr = 0xff;
      if(!MidiController::iValIsUnknown(i_dataA))
        pr = i_dataA & 0xff;
      //if(pr != 0xff)
      //  pr = limitValToInstrCtlRange(da, pr, chn);
      
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
      
      return true;
    }
    break;
    
    default:
    break;
  }
  
  return false;
}

//---------------------------------------------------------
//   sendHwCtrlState
//   Return true if it is OK to go ahead and deliver the event.
//---------------------------------------------------------

bool MidiPort::sendHwCtrlState(const MidiPlayEvent& ev, bool forceSend)
      {
      const int type = ev.type();
      const int chn = ev.channel();
      const int da = ev.dataA();
      int fin_da = da;
      const int db = ev.dataB();
      
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
                hb = limitValToInstrCtlRange(da, hb, chn);
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
            }
            break;

            case CTRL_LBANK:
            {
              int hb = 0xff;
              int lb = 0xff;
              if(!MidiController::iValIsUnknown(db))
                lb = db & 0xff;
              if(lb != 0xff)
                lb = limitValToInstrCtlRange(da, lb, chn);
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
        }
        break;
        
        case ME_AFTERTOUCH:
        {
          fin_da = CTRL_AFTERTOUCH;
          // This will create a new value list if necessary, otherwise it returns the existing list.
          // FIXME: This is not realtime safe because it may allocate.
          addManagedController(chn, fin_da);
        }
        break;
        
        case ME_PITCHBEND:
        {
          fin_da = CTRL_PITCH;
          // This will create a new value list if necessary, otherwise it returns the existing list.
          // FIXME: This is not realtime safe because it may allocate.
          addManagedController(chn, fin_da);
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
          //  pr = limitValToInstrCtlRange(da, pr, chn);
          
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

bool MidiPort::setHwCtrlState(const MidiPlayEvent& ev)
{
  const int port = ev.port();
  if(port < 0 || port >= MusECore::MIDI_PORTS)
    return false;
  
  // Handle the event. Tell the gui to create controllers as needed.
  return MusEGlobal::midiPorts[port].handleGui2AudioEvent(ev, true);
}

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
//   Returns true if a value was actually added.
//---------------------------------------------------------

bool MidiPort::setControllerVal(int ch, unsigned int tick, int ctrl, int val, Part* part)
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
    if(mt->type() != Track::DRUM)
      continue;
    port = mt->outPort();
    if(port < 0 || port >= MusECore::MIDI_PORTS || &MusEGlobal::midiPorts[port] != this)
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
  bool map_changed = false;
  MidiTrack* mt;
  for(iMidiTrack t = MusEGlobal::song->midis()->begin(); t != MusEGlobal::song->midis()->end(); ++t)
  {
    mt = *t;
    if(mt->type() != Track::DRUM)
      continue;
    port = mt->outPort();
    if(port < 0 || port >= MusECore::MIDI_PORTS || &MusEGlobal::midiPorts[port] != this)
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

int MidiPort::getCtrl(int ch, unsigned int tick, int ctrl) const
      {
      iMidiCtrlValList cl = _controller->find(ch, ctrl);
      if (cl == _controller->end())
            return CTRL_VAL_UNKNOWN;

      return cl->second->value(tick);
      }

int MidiPort::getCtrl(int ch, unsigned int tick, int ctrl, Part* part) const
      {
      iMidiCtrlValList cl = _controller->find(ch, ctrl);
      if (cl == _controller->end())
            return CTRL_VAL_UNKNOWN;

      return cl->second->value(tick, part);
      }

int MidiPort::getVisibleCtrl(int ch, unsigned int tick, int ctrl, bool inclMutedParts, bool inclMutedTracks, bool inclOffTracks) const
      {
      iMidiCtrlValList cl = _controller->find(ch, ctrl);
      if (cl == _controller->end())
            return CTRL_VAL_UNKNOWN;

      return cl->second->visibleValue(tick, inclMutedParts, inclMutedTracks, inclOffTracks);
      }

int MidiPort::getVisibleCtrl(int ch, unsigned int tick, int ctrl, Part* part, bool inclMutedParts, bool inclMutedTracks, bool inclOffTracks) const
      {
      iMidiCtrlValList cl = _controller->find(ch, ctrl);
      if (cl == _controller->end())
            return CTRL_VAL_UNKNOWN;

      return cl->second->visibleValue(tick, part, inclMutedParts, inclMutedTracks, inclOffTracks);
      }

//---------------------------------------------------------
//   deleteController
//---------------------------------------------------------

void MidiPort::deleteController(int ch, unsigned int tick, int ctrl, int val, Part* part)
    {
      iMidiCtrlValList cl = _controller->find(ch, ctrl);
      if (cl == _controller->end()) {
            if (MusEGlobal::debugMsg)
                  printf("deleteController: controller %d(0x%x) for channel %d not found size %zd\n",
                     ctrl, ctrl, ch, _controller->size());
            return;
            }
      
      cl->second->delMCtlVal(tick, part, val);
      }

//---------------------------------------------------------
//   midiController
//---------------------------------------------------------

MidiController* MidiPort::midiController(int num, int chan, bool createIfNotFound) const
      {
      MidiController* mc = nullptr;
      
      // Search the instrument's controller lists (including midnam controllers).
      if (_instrument) {
            const int patch = hwCtrlState(chan, CTRL_PROGRAM);
            mc = _instrument->findController(num, chan, patch);
            if(mc)
              return mc;
            }
      
      // Search the global default controller list.
      mc = defaultMidiController.findController(num);
      if(mc)
        return mc;
      
      if(!createIfNotFound)
        return nullptr;

      const QString name = midiCtrlName(num);
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
                  return nullptr;
                  break;
            }
      mc = new MidiController(name, num, min, max, 0, 0);
      defaultMidiController.add(mc);
      return mc;
      }

//---------------------------------------------------------
//   drumController
//   Returns instrument drum controller if ctl is a drum controller number.
//   Otherwise returns zero. 
//   NOTE: Midnam apparently has no concept of drum controllers, so the fact
//    that this only asks in the instrument's controllers and not the midnam is OK.
//---------------------------------------------------------

MidiController* MidiPort::drumController(int ctl)
{
  if(!_instrument)
    return nullptr;
  MidiControllerList* cl = _instrument->controller();
  if(!cl)
    return nullptr;
  return cl->perNoteController(ctl);
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
        if(r->type == Route::TRACK_ROUTE && r->track)
        {
          // Ignore Midi Port to Audio Input routes. Handled by Track route writer. p4.0.14 Tim.
          if(r->track->type() == Track::AUDIO_INPUT)
            continue;
            
          s = "Route";
          if(r->channel != -1)
            s += QString(" channel=\"%1\"").arg(r->channel);
          xml.tag(level++, s.toLatin1().constData());
          
          xml.tag(level, "source mport=\"%d\"/", portno());
          
          s = "dest";
          s += QString(" track=\"%1\"/").arg(MusEGlobal::song->tracks()->index(r->track));
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
  if(port < 0 || port >= MusECore::MIDI_PORTS)
    return;
  MusEGlobal::midiPorts[port].setDefaultOutChannels(c);
  for(int i = 0; i < MusECore::MIDI_PORTS; ++i)
    if(i != port)
      MusEGlobal::midiPorts[i].setDefaultOutChannels(0);
}
#endif

} // namespace MusECore
