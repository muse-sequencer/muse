//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mididev.cpp,v 1.10.2.6 2009/11/05 03:14:35 terminator356 Exp $
//
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

#include <config.h>

#include <QMessageBox>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "midictrl.h"
#include "song.h"
#include "midi.h"
#include "midiport.h"
#include "mididev.h"
#include "config.h"
#include "gconfig.h"
#include "globals.h"
#include "audio.h"
#include "midiseq.h"
#include "sync.h"
#include "midiitransform.h"
#include "part.h"
//#include "mpevent.h"

#ifdef MIDI_DRIVER_MIDI_SERIAL
extern void initMidiSerial();
#endif
extern bool initMidiAlsa();
extern bool initMidiJack();

MidiDeviceList midiDevices;
extern void processMidiInputTransformPlugins(MEvent&);

extern unsigned int volatile lastExtMidiSyncTick;

//---------------------------------------------------------
//   initMidiDevices
//---------------------------------------------------------

void initMidiDevices()
      {
#ifdef MIDI_DRIVER_MIDI_SERIAL
      initMidiSerial();
#endif
      if(initMidiAlsa())
          {
          QMessageBox::critical(NULL, "MusE fatal error.", "MusE failed to initialize the\n" 
                                                          "Alsa midi subsystem, check\n"
                                                          "your configuration.");
          exit(-1);
          }
      
      if(initMidiJack())
          {
          QMessageBox::critical(NULL, "MusE fatal error.", "MusE failed to initialize the\n" 
                                                          "Jack midi subsystem, check\n"
                                                          "your configuration.");
          exit(-1);
          }
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

void MidiDevice::init()
      {
      _readEnable = false;
      _writeEnable = false;
      _rwFlags       = 3;
      _openFlags     = 3;
      _port          = -1;
      // _nextPlayEvent = _playEvents.begin(); // Removed p4.0.15 Tim.
      }

//---------------------------------------------------------
//   MidiDevice
//---------------------------------------------------------

MidiDevice::MidiDevice()
      {
      ///_recBufFlipped = false;
      //_tmpRecordCount = 0;
      for(unsigned int i = 0; i < MIDI_CHANNELS + 1; ++i)
        _tmpRecordCount[i] = 0;
      
      _sysexFIFOProcessed = false;
      //_sysexWritingChunks = false;
      _sysexReadingChunks = false;
      
      init();
      }

MidiDevice::MidiDevice(const QString& n)
   : _name(n)
      {
      ///_recBufFlipped = false;
      //_tmpRecordCount = 0;
      for(unsigned int i = 0; i < MIDI_CHANNELS + 1; ++i)
        _tmpRecordCount[i] = 0;
      
      _sysexFIFOProcessed = false;
      //_sysexWritingChunks = false;
      _sysexReadingChunks = false;
      
      init();
      }

//---------------------------------------------------------
//   filterEvent
//    return true if event filtered
//---------------------------------------------------------

//static bool filterEvent(const MEvent& event, int type, bool thru)
bool filterEvent(const MEvent& event, int type, bool thru)
      {
      switch(event.type()) {
            case ME_NOTEON:
            case ME_NOTEOFF:
                  if (type & MIDI_FILTER_NOTEON)
                        return true;
                  break;
            case ME_POLYAFTER:
                  if (type & MIDI_FILTER_POLYP)
                        return true;
                  break;
            case ME_CONTROLLER:
                  if (type & MIDI_FILTER_CTRL)
                        return true;
                  if (!thru && (midiFilterCtrl1 == event.dataA()
                     || midiFilterCtrl2 == event.dataA()
                     || midiFilterCtrl3 == event.dataA()
                     || midiFilterCtrl4 == event.dataA())) {
                        return true;
                        }
                  break;
            case ME_PROGRAM:
                  if (type & MIDI_FILTER_PROGRAM)
                        return true;
                  break;
            case ME_AFTERTOUCH:
                  if (type & MIDI_FILTER_AT)
                        return true;
                  break;
            case ME_PITCHBEND:
                  if (type & MIDI_FILTER_PITCH)
                        return true;
                  break;
            case ME_SYSEX:
                  if (type & MIDI_FILTER_SYSEX)
                        return true;
                  break;
            default:
                  break;
            }
      return false;
      }

//---------------------------------------------------------
//   afterProcess
//    clear all recorded events after a process cycle
//---------------------------------------------------------

void MidiDevice::afterProcess()
{
  //while (_tmpRecordCount--)
  //  _recordFifo.remove();
  
  for(unsigned int i = 0; i < MIDI_CHANNELS + 1; ++i)
  {
    while (_tmpRecordCount[i]--)
      _recordFifo[i].remove();
  } 
}

//---------------------------------------------------------
//   beforeProcess
//    "freeze" fifo for this process cycle
//---------------------------------------------------------

void MidiDevice::beforeProcess()
{
  //if (!jackPort(0).isZero())
  //      audioDriver->collectMidiEvents(this, jackPort(0));
  
  //_tmpRecordCount = _recordFifo.getSize();
  for(unsigned int i = 0; i < MIDI_CHANNELS + 1; ++i)
    _tmpRecordCount[i] = _recordFifo[i].getSize();
  
  // Reset this.
  _sysexFIFOProcessed = false;
}

/*
//---------------------------------------------------------
//   getEvents
//---------------------------------------------------------

void MidiDevice::getEvents(unsigned , unsigned , int ch, MPEventList* dst)  //from //to
{
  for (int i = 0; i < _tmpRecordCount; ++i) {
        const MidiPlayEvent& ev = _recordFifo.peek(i);
        if (ch == -1 || (ev.channel() == ch))
              dst->insert(ev);
        }
  
  //while(!recordFifo.isEmpty())
  //{
  //  MidiPlayEvent e(recordFifo.get());
  //  if (ch == -1 || (e.channel() == ch))
  //        dst->insert(e);
  //}  
}
*/

/*
//---------------------------------------------------------
//   recordEvent
//---------------------------------------------------------

MREventList* MidiDevice::recordEvents()        
{ 
  // Return which list is NOT currently being filled with incoming midi events. By T356.
  if(_recBufFlipped) 
    return &_recordEvents; 
  else  
    return &_recordEvents2; 
}
*/

//---------------------------------------------------------
//   recordEvent
//---------------------------------------------------------

void MidiDevice::recordEvent(MidiRecordEvent& event)
      {
      // p3.3.35
      // TODO: Tested, but record resolution not so good. Switch to wall clock based separate list in MidiDevice. And revert this line.
      //event.setTime(audio->timestamp());
      event.setTime(extSyncFlag.value() ? lastExtMidiSyncTick : audio->timestamp());
      
      //printf("MidiDevice::recordEvent event time:%d\n", event.time());
      
      // Added by Tim. p3.3.8
      
      // By T356. Set the loop number which the event came in at.
      //if(audio->isRecording())
      if(audio->isPlaying())
        event.setLoopNum(audio->loopCount());
      
      if (midiInputTrace) {
            printf("MidiInput: ");
            event.dump();
            }

      int typ = event.type();
      
      if(_port != -1)
      {
        int idin = midiPorts[_port].syncInfo().idIn();
        
// p3.3.26 1/23/10 Section was disabled, enabled by Tim.
//#if 0
  
        //---------------------------------------------------
        // filter some SYSEX events
        //---------------------------------------------------
  
        if (typ == ME_SYSEX) {
              const unsigned char* p = event.data();
              int n = event.len();
              if (n >= 4) {
                    if ((p[0] == 0x7f)
                      //&& ((p[1] == 0x7f) || (p[1] == rxDeviceId))) {
                      && ((p[1] == 0x7f) || (idin == 0x7f) || (p[1] == idin))) {
                          if (p[2] == 0x06) {
                                //mmcInput(p, n);
                                midiSeq->mmcInput(_port, p, n);
                                return;
                                }
                          if (p[2] == 0x01) {
                                //mtcInputFull(p, n);
                                midiSeq->mtcInputFull(_port, p, n);
                                return;
                                }
                          }
                    else if (p[0] == 0x7e) {
                          //nonRealtimeSystemSysex(p, n);
                          midiSeq->nonRealtimeSystemSysex(_port, p, n);
                          return;
                          }
                    }
              }
          else    
            // p3.3.26 1/23/10 Moved here from alsaProcessMidiInput(). Anticipating Jack midi support, so don't make it ALSA specific. Tim. 
            // Trigger general activity indicator detector. Sysex has no channel, don't trigger.
            midiPorts[_port].syncInfo().trigActDetect(event.channel());
              
//#endif

      }
      
      //
      //  process midi event input filtering and
      //    transformation
      //

      processMidiInputTransformPlugins(event);

      if (filterEvent(event, midiRecordType, false))
            return;

      if (!applyMidiInputTransformation(event)) {
            if (midiInputTrace)
                  printf("   midi input transformation: event filtered\n");
            return;
            }

      //
      // transfer noteOn and Off events to gui for step recording and keyboard
      // remote control (changed by flo93: added noteOff-events)
      //
      if (typ == ME_NOTEON) {
            int pv = ((event.dataA() & 0xff)<<8) + (event.dataB() & 0xff);
            song->putEvent(pv);
            }
      else if (typ == ME_NOTEOFF) {
            int pv = ((event.dataA() & 0xff)<<8) + (0x00); //send an event with velo=0
            song->putEvent(pv);
            }
      
      ///if(_recBufFlipped)
      ///  _recordEvents2.add(event);     // add event to secondary list of recorded events
      ///else
      ///  _recordEvents.add(event);     // add event to primary list of recorded events
      
      //if(_recordFifo.put(MidiPlayEvent(event)))
      //  printf("MidiDevice::recordEvent: fifo overflow\n");
      
      // p3.3.38
      // Do not bother recording if it is NOT actually being used by a port.
      // Because from this point on, process handles things, by selected port.
      if(_port == -1)
        return;
      
      // Split the events up into channel fifos. Special 'channel' number 17 for sysex events.
      unsigned int ch = (typ == ME_SYSEX)? MIDI_CHANNELS : event.channel();
      if(_recordFifo[ch].put(MidiPlayEvent(event)))
        printf("MidiDevice::recordEvent: fifo channel %d overflow\n", ch);
      }

//---------------------------------------------------------
//   find
//---------------------------------------------------------

MidiDevice* MidiDeviceList::find(const QString& s, int typeHint)
      {
      for (iMidiDevice i = begin(); i != end(); ++i)
            if( (typeHint == -1 || typeHint == (*i)->deviceType()) && ((*i)->name() == s) )
                  return *i;
      return 0;
      }

iMidiDevice MidiDeviceList::find(const MidiDevice* dev)
      {
      for (iMidiDevice i = begin(); i != end(); ++i)
            if (*i == dev)
                  return i;
      return end();
      }

//---------------------------------------------------------
//   add
//---------------------------------------------------------

void MidiDeviceList::add(MidiDevice* dev)
      {
      bool gotUniqueName=false;
      int increment = 0;
      QString origname = dev->name();
      while (!gotUniqueName) {
            gotUniqueName = true;
            // check if the name's been taken
            for (iMidiDevice i = begin(); i != end(); ++i) {
                  const QString s = (*i)->name();
                  if (s == dev->name())
                        {
                        char incstr[4];
                        sprintf(incstr,"_%d",++increment);
                        //dev->setName(origname + incstr);
                        dev->setName(origname + QString(incstr));    // p4.0.0
                        gotUniqueName = false;
                        }
                  }
            }
      
      push_back(dev);
      }

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void MidiDeviceList::remove(MidiDevice* dev)
      {
      for (iMidiDevice i = begin(); i != end(); ++i) {
            if (*i == dev) {
                  erase(i);
                  break;
                  }
            }
      }

//---------------------------------------------------------
//   sendNullRPNParams
//---------------------------------------------------------

bool MidiDevice::sendNullRPNParams(int chn, bool nrpn)
{
  if(_port == -1)
    return false;  
    
  int nv = midiPorts[_port].nullSendValue();
  if(nv == -1)
    return false;  
  
  int nvh = (nv >> 8) & 0xff;
  int nvl = nv & 0xff;
  if(nvh != 0xff)
  {
    if(nrpn)
      putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_HNRPN, nvh & 0x7f));
    else
      putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_HRPN, nvh & 0x7f));
  }
  if(nvl != 0xff)
  {
    if(nrpn)
      putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_LNRPN, nvl & 0x7f));
    else  
      putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_LRPN, nvl & 0x7f));
  }
  return true;  
}

//---------------------------------------------------------
//   putEventWithRetry
//    return true if event cannot be delivered
//    This method will try to putEvent 'tries' times, waiting 'delayUs' microseconds between tries.
//    NOTE: Since it waits, it should not be used in RT or other time-sensitive threads. p4.0.15 Tim.
//---------------------------------------------------------

bool MidiDevice::putEventWithRetry(const MidiPlayEvent& ev, int tries, long delayUs)
{
  // TODO: Er, probably not the best way to do this.
  //       Maybe try to correlate with actual audio buffer size instead of blind time delay.
  for( ; tries > 0; --tries)
  { 
    if(!putEvent(ev))  // Returns true if event cannot be delivered.
      return false;
      
    bool sleepOk = -1;
    while(sleepOk == -1)
      sleepOk = usleep(delayUs);   // FIXME: usleep is supposed to be depricated!
  }  
  return true;
}
      
//---------------------------------------------------------
//   putEvent
//    return true if event cannot be delivered
//    TODO: retry on controller putMidiEvent
//    (Note: Since putEvent is virtual and there are different versions, 
//     a retry facility is now found in putEventWithRetry. p4.0.15 Tim)
//---------------------------------------------------------

bool MidiDevice::putEvent(const MidiPlayEvent& ev)
      {
      if(!_writeEnable)
        //return true;
        return false;
        
      if (ev.type() == ME_CONTROLLER) {
            int a = ev.dataA();
            int b = ev.dataB();
            int chn = ev.channel();
            if (a == CTRL_PITCH) {
                  return putMidiEvent(MidiPlayEvent(0, 0, chn, ME_PITCHBEND, b, 0));
                  }
            if (a == CTRL_PROGRAM) {
                  // don't output program changes for GM drum channel
                  if (!(song->mtype() == MT_GM && chn == 9)) {
                        int hb = (b >> 16) & 0xff;
                        int lb = (b >> 8) & 0xff;
                        int pr = b & 0x7f;
                        if (hb != 0xff)
                              putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_HBANK, hb));
                        if (lb != 0xff)
                              putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_LBANK, lb));
                        return putMidiEvent(MidiPlayEvent(0, 0, chn, ME_PROGRAM, pr, 0));
                        }
                  }
#if 1 // if ALSA cannot handle RPN NRPN etc.
            
            // p3.3.37
            //if (a < 0x1000) {          // 7 Bit Controller
            if (a < CTRL_14_OFFSET) {          // 7 Bit Controller
                  //putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, a, b));
                  putMidiEvent(ev);
                  }
            //else if (a < 0x20000) {     // 14 bit high resolution controller
            else if (a < CTRL_RPN_OFFSET) {     // 14 bit high resolution controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  int dataH = (b >> 7) & 0x7f;
                  int dataL = b & 0x7f;
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, ctrlH, dataH));
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, ctrlL, dataL));
                  }
            //else if (a < 0x30000) {     // RPN 7-Bit Controller
            else if (a < CTRL_NRPN_OFFSET) {     // RPN 7-Bit Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_HRPN, ctrlH));
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_LRPN, ctrlL));
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_HDATA, b));
                  
                  // Added by T356. Select null parameters so that subsequent data controller
                  //  events do not upset the last *RPN controller.
                  sendNullRPNParams(chn, false);
                }
            //else if (a < 0x40000) {     // NRPN 7-Bit Controller
            else if (a < CTRL_INTERNAL_OFFSET) {     // NRPN 7-Bit Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_HNRPN, ctrlH));
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_LNRPN, ctrlL));
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_HDATA, b));
                  
                  sendNullRPNParams(chn, true);
                  }
            //else if (a < 0x60000) {     // RPN14 Controller
            else if (a < CTRL_NRPN14_OFFSET) {     // RPN14 Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  int dataH = (b >> 7) & 0x7f;
                  int dataL = b & 0x7f;
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_HRPN, ctrlH));
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_LRPN, ctrlL));
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_HDATA, dataH));
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_LDATA, dataL));
                  
                  sendNullRPNParams(chn, false);
                  }
            //else if (a < 0x70000) {     // NRPN14 Controller
            else if (a < CTRL_NONE_OFFSET) {     // NRPN14 Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  int dataH = (b >> 7) & 0x7f;
                  int dataL = b & 0x7f;
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_HNRPN, ctrlH));
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_LNRPN, ctrlL));
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_HDATA, dataH));
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_LDATA, dataL));
                  
                  sendNullRPNParams(chn, true);
                  }
            else {
                  printf("putEvent: unknown controller type 0x%x\n", a);
                  }
            return false;
#endif
            }
      return putMidiEvent(ev);
      }

//---------------------------------------------------------
//   handleStop
//---------------------------------------------------------

void MidiDevice::handleStop()
{
  // If the device is not in use by a port, don't bother it.
  if(_port == -1)
    return;
    
  //---------------------------------------------------
  //    Clear all notes and handle stuck notes
  //---------------------------------------------------
  
  _playEvents.clear();
  for(iMPEvent i = _stuckNotes.begin(); i != _stuckNotes.end(); ++i) 
  {
    MidiPlayEvent ev = *i;
    ev.setTime(0);
    _playEvents.add(ev);
  }
  _stuckNotes.clear();
  //setNextPlayEvent(_playEvents.begin());  
  
  
  //---------------------------------------------------
  //    reset sustain
  //---------------------------------------------------
  
  MidiPort* mp = &midiPorts[_port];
  for(int ch = 0; ch < MIDI_CHANNELS; ++ch) 
  {
    if(mp->hwCtrlState(ch, CTRL_SUSTAIN) == 127) 
    {
      //printf("send clear sustain!!!!!!!! port %d ch %d\n", i,ch);
      MidiPlayEvent ev(0, _port, ch, ME_CONTROLLER, CTRL_SUSTAIN, 0);
      putEvent(ev);
    }
  }
  
  //---------------------------------------------------
  //    send midi stop
  //---------------------------------------------------
  
  // Don't send if external sync is on. The master, and our sync routing system will take care of that.   p3.3.31
  if(!extSyncFlag.value())
  {
    // Shall we check open flags?
    //if(!(dev->rwFlags() & 0x1) || !(dev->openFlags() & 1))
    //if(!(dev->openFlags() & 1))
    //  return;
          
    MidiSyncInfo& si = mp->syncInfo();
    if(si.MMCOut())
      mp->sendMMCStop();
    
    if(si.MRTOut()) 
    {
      // Send STOP 
      mp->sendStop();
      
      // p3.3.31
      // Added check of option send continue not start.
      // Hmm, is this required? Seems to make other devices unhappy.
      // (Could try now that this is in MidiDevice. p4.0.22 )
      /*
      if(!si.sendContNotStart())
        mp->sendSongpos(audio->tickPos() * 4 / config.division);
      */  
    }
  }  
}
      
//---------------------------------------------------------
//   handleSeek
//---------------------------------------------------------

void MidiDevice::handleSeek()
{
  // If the device is not in use by a port, don't bother it.
  if(_port == -1)
    return;
  
  //---------------------------------------------------
  //    If playing, clear all notes and handle stuck notes
  //---------------------------------------------------
  
  if(audio->isPlaying()) 
  {
    _playEvents.clear();
    for(iMPEvent i = _stuckNotes.begin(); i != _stuckNotes.end(); ++i) 
    {
      MidiPlayEvent ev = *i;
      ev.setTime(0);
      _playEvents.add(ev);
    }
    _stuckNotes.clear();
  }
  //else
    // Removed p4.0.15 Device now leaves beginning pointing at next event,
    //  immediately after playing some notes.  
    // NOTE: This removal needs testing. I'm not sure about this.
    //_playEvents.erase(_playEvents.begin(), _nextPlayEvent);  
  
  MidiPort* mp = &midiPorts[_port];
  MidiCtrlValListList* cll = mp->controller();
  int pos = audio->tickPos();
  
  //---------------------------------------------------
  //    Send new contoller values
  //---------------------------------------------------
    
  for(iMidiCtrlValList ivl = cll->begin(); ivl != cll->end(); ++ivl) 
  {
    MidiCtrlValList* vl = ivl->second;
    //int val = vl->value(pos);
    //if (val != CTRL_VAL_UNKNOWN) {
    //      int channel = ivl->first >> 24;
    //      el->add(MidiPlayEvent(0, port, channel, ME_CONTROLLER, vl->num(), val));
    //      }
    iMidiCtrlVal imcv = vl->iValue(pos);
    if(imcv != vl->end()) 
    {
      Part* p = imcv->second.part;
      unsigned t = (unsigned)imcv->first;
      // Do not add values that are outside of the part.
      if(p && t >= p->tick() && t < (p->tick() + p->lenTick()) )
        _playEvents.add(MidiPlayEvent(0, _port, ivl->first >> 24, ME_CONTROLLER, vl->num(), imcv->second.val));
    }
  }
  //_nextPlayEvent = (_playEvents.begin());    // Removed p4.0.15
  
  //---------------------------------------------------
  //    Send STOP and "set song position pointer"
  //---------------------------------------------------
    
  // Don't send if external sync is on. The master, and our sync routing system will take care of that.  p3.3.31
  if(!extSyncFlag.value())
  {
    if(mp->syncInfo().MRTOut())
    {
      // Shall we check for device write open flag to see if it's ok to send?...
      // This means obey what the user has chosen for read/write in the midi port config dialog,
      //  which already takes into account whether the device is writable or not.
      //if(!(rwFlags() & 0x1) || !(openFlags() & 1))
      //if(!(openFlags() & 1))
      //  continue;
      
      //int port = midiPort();
      
      // By checking for no port here (-1), (and out of bounds), it means
      //  the device must be assigned to a port for these MMC commands to be sent.
      // Without this check, interesting sync things can be done by the user without ever
      //  assigning any devices to ports ! 
      //if(port < 0 || port > MIDI_PORTS)
      //if(port < -1 || port > MIDI_PORTS)
      //  continue;
      
      int beat = (pos * 4) / config.division;
        
      //bool isPlaying = false;
      //if(state == PLAY)
      //  isPlaying = true;
      bool isPlaying = audio->isPlaying();  // Check this it includes LOOP1 and LOOP2 besides PLAY.  p4.0.22
        
      mp->sendStop();
      mp->sendSongpos(beat);
      if(isPlaying)
        mp->sendContinue();
    }    
  }
}


