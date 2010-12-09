//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midiport.cpp,v 1.21.2.15 2009/12/07 20:11:51 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//=========================================================

//#include "config.h"

#include <QMenu>

#include "mididev.h"
#include "midiport.h"
#include "midictrl.h"
#include "midi.h"
#include "minstrument.h"
//#include "instruments/minstrument.h"   // p4.0.2
#include "xml.h"
#include "globals.h"
#include "mpevent.h"
#include "synth.h"
#include "app.h"
#include "song.h"

//#ifdef DSSI_SUPPORT
//#include "dssihost.h"
//#endif

MidiPort midiPorts[MIDI_PORTS];

//---------------------------------------------------------
//   initMidiPorts
//---------------------------------------------------------

void initMidiPorts()
      {
      for (int i = 0; i < MIDI_PORTS; ++i) {
            MidiPort* port = &midiPorts[i];
            port->setInstrument(genericMidiInstrument);
            port->syncInfo().setPort(i);
            }
      }

//---------------------------------------------------------
//   MidiPort
//---------------------------------------------------------

MidiPort::MidiPort()
   : _state("not configured")
      {
      _device     = 0;
      _instrument = 0;
      _controller = new MidiCtrlValListList();
      _foundInSongFile = false;

      //
      // create minimum set of managed controllers
      // to make midi mixer operational
      //
      for (int i = 0; i < MIDI_CHANNELS; ++i) {
            addManagedController(i, CTRL_PROGRAM);
            addManagedController(i, CTRL_VOLUME);
            addManagedController(i, CTRL_PANPOT);
            _automationType[i] = AUTO_READ;
            }
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
      return _instrument ? _instrument->guiVisible() : false;
      }

//---------------------------------------------------------
//   hasGui
//---------------------------------------------------------

bool MidiPort::hasGui() const
      {
      return _instrument ? _instrument->hasGui() : false;
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
            }
      if (dev) {
            for (int i = 0; i < MIDI_PORTS; ++i) {
                  MidiPort* mp = &midiPorts[i];
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

            // By T356. Send all instrument controller initial (default) values to all midi channels now,
            //  except where explicitly initialized in the song.
            // By sending ALL instrument controller initial values, even if those controllers are NOT
            //  in the song, we can ensure better consistency between songs. 
            // For example: A song is loaded which has a 'reverb level' controller initial value of '100'.
            // Then a song is loaded which has no such controller (hence no explicit initial value).
            // The 'reverb level' controller would still be at '100', and could adversely affect the song,
            //  but if the instrument has an available initial value of say '0', it will be used instead.
            //
            //if(_instrument)
            // p3.3.39 NOT for syntis! Use midiState an/or initParams for that.
            if(_instrument && !_device->isSynti())
            {
              MidiControllerList* cl = _instrument->controller();
              MidiController* mc;
              for(ciMidiController imc = cl->begin(); imc != cl->end(); ++imc) 
              {
                //mc = *imc;
                mc = imc->second;
                for(int chan = 0; chan < MIDI_CHANNELS; ++chan)
                {
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
                      
///#ifdef DSSI_SUPPORT
                      // Exclude dssi synths from this, as some of them have hundreds of controls.
                      // Another difference is dssi synth devices (usually) have readable default port values,
                      //  unlike a midi output port, which cannot be queried for a current or default value,
                      //  so we blindly send values here. Also some dssi have a different default mechanism or
                      //  storage systems for parameters, with complex GUIs with their own manipulation schemes.   
                      // Another difference is dssi controls are best manipulated as ladspa controls -
                      //  (they ARE ladspa controls). This is stuff I mainly put for midi ports and MESS...
                      // I DO allow midi control of those ladspa controls, so our midi controls shall be updated here...
                      // p3.3.39 Only non-syntis! Use midiState an/or initParams for that.
                      ///if(!_device->isSynti() || (dynamic_cast<DssiSynthIF*>(((SynthI*)_device)->sif()) == 0))
                      ///{  
///#endif
                        // Note the addition of bias!
                        _device->putEvent(MidiPlayEvent(0, portno(), chan,
                          ME_CONTROLLER, ctl, mc->initVal() + mc->bias()));
///#ifdef DSSI_SUPPORT
                      ///}
///#endif
                        
                      // Set it once so the 'last HW value' is set, and control knobs are positioned at the value...
                      //setHwCtrlState(chan, ctl, mc->initVal() + mc->bias());
                      // Set it again so that control labels show 'off'...
                      //setHwCtrlState(chan, ctl, CTRL_VAL_UNKNOWN);
                      setHwCtrlStates(chan, ctl, CTRL_VAL_UNKNOWN, mc->initVal() + mc->bias());
                    }    
                  }    
                }
              }
            }

            // init HW controller state
            // p3.3.39 NOT for syntis! Use midiState an/or initParams for that.
            if(!_device->isSynti())
            {
              for (iMidiCtrlValList i = _controller->begin(); i != _controller->end(); ++i) {
                  int channel = i->first >> 24;
                  int cntrl   = i->first & 0xffffff;
                  int val     = i->second->hwVal();
                  if (val != CTRL_VAL_UNKNOWN) {
                        
                        
///#ifdef DSSI_SUPPORT
                      // Not for dssi synths...
                      ///if(!_device->isSynti() || (dynamic_cast<DssiSynthIF*>(((SynthI*)_device)->sif()) == 0))
                      ///{  
///#endif
                        _device->putEvent(MidiPlayEvent(0, portno(), channel,
                          ME_CONTROLLER, cntrl, val));
///#ifdef DSSI_SUPPORT
                      ///}
///#endif
                        
                        // Set it once so the 'last HW value' is set, and control knobs are positioned at the value...
                        setHwCtrlState(channel, cntrl, val);
                        // Set it again so that control labels show 'off'...
                        //setHwCtrlState(channel, cntrl, CTRL_VAL_UNKNOWN);
                        //setHwCtrlStates(channel, cntrl, CTRL_VAL_UNKNOWN, val);
                        }
                  }
              }
            }  

      else
            clearDevice();
      }

//---------------------------------------------------------
//   clearDevice
//---------------------------------------------------------

void MidiPort::clearDevice()
      {
      _device = 0;
      _state  = "not configured";
      }

//---------------------------------------------------------
//   portno
//---------------------------------------------------------

int MidiPort::portno() const
      {
      for (int i = 0; i < MIDI_PORTS; ++i) {
            if (&midiPorts[i] == this)
                  return i;
            }
      return -1;
      }

//---------------------------------------------------------
//   midiPortsPopup
//---------------------------------------------------------

//QPopupMenu* midiPortsPopup(QWidget* parent)
QMenu* midiPortsPopup(QWidget* parent, int checkPort)
      {
      QMenu* p = new QMenu(parent);
      for (int i = 0; i < MIDI_PORTS; ++i) {
            MidiPort* port = &midiPorts[i];
            QString name;
            name.sprintf("%d:%s", port->portno()+1, port->portname().toLatin1().constData());
	    QAction *act = p->addAction(name);
	    act->setData(i);
            
            if(i == checkPort)
              act->setChecked(true);
          }  
      return p;
      }

//---------------------------------------------------------
//   portname
//---------------------------------------------------------

const QString& MidiPort::portname() const
      {
      //static const QString none("<none>");
      static const QString none(QT_TR_NOOP("<none>"));
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
  if(_instrument)
  {
    MidiControllerList* cl = _instrument->controller();
    //for(ciMidiController imc = cl->begin(); imc != cl->end(); ++imc)
    ciMidiController imc = cl->find(ctl);  
    if(imc != cl->end())
    {
      //MidiController* mc = *imc;
      MidiController* mc = imc->second;
      //int cnum = mc->num();
      //if(cnum == ctl)
      //{
        int initval = mc->initVal();
        
        // Initialize from either the instrument controller's initial value, or the supplied value.
        if(initval != CTRL_VAL_UNKNOWN)
        {
          if(_device)
          {
            //MidiPlayEvent ev(song->cpos(), portno(), chan, ME_CONTROLLER, ctl, initval + mc->bias());
            MidiPlayEvent ev(0, portno(), chan, ME_CONTROLLER, ctl, initval + mc->bias());
            _device->putEvent(ev);
          }  
          // Set it once so the 'last HW value' is set, and control knobs are positioned at the value...
          //setHwCtrlState(chan, ctl, initval + mc->bias());
          // Set it again so that control labels show 'off'...
          //setHwCtrlState(chan, ctl, CTRL_VAL_UNKNOWN);
          setHwCtrlStates(chan, ctl, CTRL_VAL_UNKNOWN, initval + mc->bias());
          
          return;
        }
    }  
  }
  
  if(_device)
  {
    //MidiPlayEvent ev(song->cpos(), portno(), chan, ME_CONTROLLER, ctl, val);
    MidiPlayEvent ev(0, portno(), chan, ME_CONTROLLER, ctl, val);
    _device->putEvent(ev);
  }  
  // Set it once so the 'last HW value' is set, and control knobs are positioned at the value...
  //setHwCtrlState(chan, ctl, val);
  // Set it again so that control labels show 'off'...
  //setHwCtrlState(chan, ctl, CTRL_VAL_UNKNOWN);
  setHwCtrlStates(chan, ctl, CTRL_VAL_UNKNOWN, val);
}      
      
//---------------------------------------------------------
//   sendGmInitValues
//---------------------------------------------------------

void MidiPort::sendGmInitValues()
{
  for (int i = 0; i < MIDI_CHANNELS; ++i) {
        // Changed by T356. 
        //setHwCtrlState(i, CTRL_PROGRAM,      0);
        //setHwCtrlState(i, CTRL_PITCH,        0);
        //setHwCtrlState(i, CTRL_VOLUME,     100);
        //setHwCtrlState(i, CTRL_PANPOT,      64);
        //setHwCtrlState(i, CTRL_REVERB_SEND, 40);
        //setHwCtrlState(i, CTRL_CHORUS_SEND,  0);
        
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
        // Changed by T356. 
        //setHwCtrlState(i, CTRL_PROGRAM, 0);
        //setHwCtrlState(i, CTRL_MODULATION, 0);
        //setHwCtrlState(i, CTRL_PORTAMENTO_TIME, 0);
        //setHwCtrlState(i, CTRL_VOLUME, 0x64);
        //setHwCtrlState(i, CTRL_PANPOT, 0x40);
        //setHwCtrlState(i, CTRL_EXPRESSION, 0x7f);
        //setHwCtrlState(i, CTRL_SUSTAIN, 0x0);
        //setHwCtrlState(i, CTRL_PORTAMENTO, 0x0);
        //setHwCtrlState(i, CTRL_SOSTENUTO, 0x0);
        //setHwCtrlState(i, CTRL_SOFT_PEDAL, 0x0);
        //setHwCtrlState(i, CTRL_HARMONIC_CONTENT, 0x40);
        //setHwCtrlState(i, CTRL_RELEASE_TIME, 0x40);
        //setHwCtrlState(i, CTRL_ATTACK_TIME, 0x40);
        //setHwCtrlState(i, CTRL_BRIGHTNESS, 0x40);
        //setHwCtrlState(i, CTRL_REVERB_SEND, 0x28);
        //setHwCtrlState(i, CTRL_CHORUS_SEND, 0x0);
        //setHwCtrlState(i, CTRL_VARIATION_SEND, 0x0);
        
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
      //static unsigned char data2[] = { 0x41, 0x10, 0x42, 0x12, 0x40, 0x01, 0x33, 0x50, 0x3c };
      //static unsigned char data3[] = { 0x41, 0x10, 0x42, 0x12, 0x40, 0x01, 0x34, 0x50, 0x3b };
      //sendSysex(data2, sizeof(data2));
      //sendSysex(data3, sizeof(data3));
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
//   sendEvent
//    return true, if event cannot be delivered
//---------------------------------------------------------

bool MidiPort::sendEvent(const MidiPlayEvent& ev)
      {
      if (ev.type() == ME_CONTROLLER) {
      
//      printf("current sustain %d %d %d\n", hwCtrlState(ev.channel(),CTRL_SUSTAIN), CTRL_SUSTAIN, ev.dataA());

            // Added by T356.
            int da = ev.dataA();
            int db = ev.dataB();
            /*
            // Is it a drum controller?
            MidiController* mc = drumController(da);
            if(mc)
            {
              DrumMap* dm = &drumMap[da & 0x7f];
              int port = dm->port;
              MidiPort* mp = &midiPorts[port];
              // Is it NOT for this MidiPort?
              if(mp && (mp != this))
              {
                // Redirect the event to the mapped port and channel...
                da = (da & ~0xff) | (dm->anote & 0x7f);
                db = mp->limitValToInstrCtlRange(da, db);
                MidiPlayEvent nev(ev.time(), port, dm->channel, ME_CONTROLLER, da, db);
                if(!mp->setHwCtrlState(ev.channel(), da, db))
                  return false;
                if(!mp->device())
                  return true;
                return mp->device()->putEvent(nev);
              }
            }
            */
            db = limitValToInstrCtlRange(da, db);
            

            // Removed by T356.
            //
            //  optimize controller settings
            //
            //if (hwCtrlState(ev.channel(), ev.dataA()) == ev.dataB()) {
// printf("optimize ctrl %d %x val %d\n", ev.dataA(), ev.dataA(), ev.dataB());
            //      return false;
            //      }
// printf("set HW Ctrl State ch:%d 0x%x 0x%x\n", ev.channel(), ev.dataA(), ev.dataB());
            if(!setHwCtrlState(ev.channel(), da, db))
              return false;
            }
      else
      if (ev.type() == ME_PITCHBEND) 
      {
            int da = limitValToInstrCtlRange(CTRL_PITCH, ev.dataA());
            // Removed by T356.
            //if (hwCtrlState(ev.channel(), CTRL_PITCH) == ev.dataA()) 
            //  return false;
            
            if(!setHwCtrlState(ev.channel(), CTRL_PITCH, da))
              return false;
      }
      else
      if (ev.type() == ME_PROGRAM) 
      {
            if(!setHwCtrlState(ev.channel(), CTRL_PROGRAM, ev.dataA()))
              return false;
      }
      
      
      if (!_device)
            return true;
      return _device->putEvent(ev);
      }

//---------------------------------------------------------
//   lastValidHWCtrlState
//---------------------------------------------------------

int MidiPort::lastValidHWCtrlState(int ch, int ctrl) const
{
      ch &= 0xff;
      iMidiCtrlValList cl = _controller->find(ch, ctrl);
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
      iMidiCtrlValList cl = _controller->find(ch, ctrl);
      if (cl == _controller->end()) {
            //if (debugMsg)
            //      printf("hwCtrlState: chan %d ctrl 0x%x not found\n", ch, ctrl);
            return CTRL_VAL_UNKNOWN;
            }
      MidiCtrlValList* vl = cl->second;
      return vl->hwVal();
      }

//---------------------------------------------------------
//   setHwCtrlState
//   Returns false if value is already equal, true if value is set.
//---------------------------------------------------------

bool MidiPort::setHwCtrlState(int ch, int ctrl, int val)
      {
      // Changed by T356.
      //iMidiCtrlValList cl = _controller->find(ch, ctrl);
      //if (cl == _controller->end()) {
            // try to add new controller
      //      addManagedController(ch, ctrl);
//            muse->importController(ch, this, ctrl);
      //      cl = _controller->find(ch, ctrl);
      //      if (cl == _controller->end()) {
      //            if (debugMsg)
      //                  printf("setHwCtrlState(%d,0x%x,0x%x): not found\n", ch, ctrl, val);
      //            return;
      //            }
      //      }
      //MidiCtrlValList* vl = cl->second;
// printf("setHwCtrlState ch %d  ctrl %x  val %x\n", ch, ctrl, val);
      
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

// Removed by T356.
//---------------------------------------------------------
//   setCtrl
//    return true if new controller value added
//---------------------------------------------------------

//bool MidiPort::setCtrl(int ch, int tick, int ctrl, int val)
//      {
//      if (debugMsg)
//            printf("setCtrl(tick=%d val=%d)\n",tick,val);
//      iMidiCtrlValList cl = _controller->find(ch, ctrl);
//      if (cl == _controller->end()) {
//            if (debugMsg)
//                  printf("setCtrl: controller 0x%x for channel %d not found\n", ctrl, ch);
//            return false;
//            }
//      return cl->second->add(tick, val);
//      }

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
      if (cl == _controller->end()) {
            //if (debugMsg)
            //      printf("getCtrl: controller %d(0x%x) for channel %d not found size %zd\n",
            //         ctrl, ctrl, ch, _controller->size());
            return CTRL_VAL_UNKNOWN;
            }
      return cl->second->value(tick);
      }

int MidiPort::getCtrl(int ch, int tick, int ctrl, Part* part) const
      {
      iMidiCtrlValList cl = _controller->find(ch, ctrl);
      if (cl == _controller->end()) {
            //if (debugMsg)
            //      printf("getCtrl: controller %d(0x%x) for channel %d not found size %zd\n",
            //         ctrl, ctrl, ch, _controller->size());
            return CTRL_VAL_UNKNOWN;
            }
      return cl->second->value(tick, part);
      }
//---------------------------------------------------------
//   deleteController
//---------------------------------------------------------

void MidiPort::deleteController(int ch, int tick, int ctrl, Part* part)
    {
      iMidiCtrlValList cl = _controller->find(ch, ctrl);
      if (cl == _controller->end()) {
            if (debugMsg)
                  printf("deleteController: controller %d(0x%x) for channel %d not found size %zd\n",
                     ctrl, ctrl, ch, _controller->size());
            return;
            }
      
      cl->second->delMCtlVal(tick, part);
      }

//---------------------------------------------------------
//   midiController
//---------------------------------------------------------

MidiController* MidiPort::midiController(int num) const
      {
      if (_instrument) {
            MidiControllerList* mcl = _instrument->controller();
            for (iMidiController i = mcl->begin(); i != mcl->end(); ++i) {
                  int cn = i->second->num();
                  if (cn == num)
                        return i->second;
                  // wildcard?
                  if (((cn & 0xff) == 0xff) && ((cn & ~0xff) == (num & ~0xff)))
                        return i->second;
                  }
            }
      
      for (iMidiController i = defaultMidiController.begin(); i != defaultMidiController.end(); ++i) {
            int cn = i->second->num();
            if (cn == num)
                  return i->second;
            // wildcard?
            if (((cn & 0xff) == 0xff) && ((cn & ~0xff) == (num & ~0xff)))
                  return i->second;
            }
      
      
      QString name = midiCtrlName(num);
      int min = 0;
      int max = 127;
      
      MidiController::ControllerType t = midiControllerType(num);
      switch (t) {
            case MidiController::RPN:
            case MidiController::NRPN:
            case MidiController::Controller7:
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
     ((ctl - CTRL_NRPN14_OFFSET >= 0) && (ctl - CTRL_NRPN14_OFFSET <= 0xffff)))
  {
    // Does the instrument have a drum controller to match this controller's number?
    iMidiController imc = cl->find(ctl | 0xff);
    if(imc != cl->end())
      // Yes, it's a drum controller. Return a pointer to it.
      return imc->second;  
  }
  
  return 0;
}
            
int MidiPort::nullSendValue()
{ 
  return _instrument ? _instrument->nullSendValue() : -1; 
}

void MidiPort::setNullSendValue(int v)              
{ 
  if(_instrument) 
    _instrument->setNullSendValue(v); 
}

//---------------------------------------------------------
//   writeRouting    // p3.3.50
//---------------------------------------------------------

void MidiPort::writeRouting(int level, Xml& xml) const
{
      // If this device is not actually in use by the song, do not write any routes.
      // This prevents bogus routes from being saved and propagated in the med file.
      if(!device())
        return;
     
      QString s;
      
      for (ciRoute r = _outRoutes.begin(); r != _outRoutes.end(); ++r) 
      {
        if(r->type == Route::TRACK_ROUTE && !r->name().isEmpty())
        {
          //xml.tag(level++, "Route");
          
          s = QT_TR_NOOP("Route");
          if(r->channel != -1 && r->channel != 0)  
            s += QString(QT_TR_NOOP(" channelMask=\"%1\"")).arg(r->channel);  // Use new channel mask.
          xml.tag(level++, s.toLatin1().constData());
          
          xml.tag(level, "source mport=\"%d\"/", portno());
          
          s = QT_TR_NOOP("dest");
          s += QString(QT_TR_NOOP(" name=\"%1\"/")).arg(Xml::xmlString(r->name()));
          xml.tag(level, s.toLatin1().constData());
          
          xml.etag(level--, "Route");
        }
      }
}
    
