//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mididev.cpp,v 1.10.2.6 2009/11/05 03:14:35 terminator356 Exp $
//
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Tim E. Real (terminator356 on users dot sourceforge dot net)
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
#include "audiodev.h"
#include "midiseq.h"
#include "sync.h"
#include "midiitransform.h"
#include "part.h"

namespace MusEGlobal {
MusECore::MidiDeviceList midiDevices;
}

namespace MusECore {

#ifdef MIDI_DRIVER_MIDI_SERIAL
extern void initMidiSerial();
#endif
extern bool initMidiAlsa();
extern bool initMidiJack();

extern void processMidiInputTransformPlugins(MEvent&);


//---------------------------------------------------------
//   initMidiDevices
//---------------------------------------------------------

void initMidiDevices()
      {
#ifdef MIDI_DRIVER_MIDI_SERIAL
      initMidiSerial();
#endif
      if(MusEGlobal::useAlsaWithJack || MusEGlobal::audioDevice->deviceType() != AudioDevice::JACK_AUDIO)
      {
        if(initMidiAlsa())
          {
          QMessageBox::critical(NULL, "MusE fatal error.", "MusE failed to initialize the\n" 
                                                          "Alsa midi subsystem, check\n"
                                                          "your configuration.");
          exit(-1);
          }
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
      stopPending    = false;         
      seekPending    = false;
      
      _readEnable    = false;
      _writeEnable   = false;
      _rwFlags       = 3;
      _openFlags     = 3;
      _port          = -1;
      }

//---------------------------------------------------------
//   MidiDevice
//---------------------------------------------------------

MidiDevice::MidiDevice()
      {
      for(unsigned int i = 0; i < MIDI_CHANNELS + 1; ++i)
        _tmpRecordCount[i] = 0;
      
      _sysexFIFOProcessed = false;
      _sysexReadingChunks = false;
      
      init();
      }

MidiDevice::MidiDevice(const QString& n)
   : _name(n)
      {
      for(unsigned int i = 0; i < MIDI_CHANNELS + 1; ++i)
        _tmpRecordCount[i] = 0;
      
      _sysexFIFOProcessed = false;
      _sysexReadingChunks = false;
      
      init();
      }

//---------------------------------------------------------
//   filterEvent
//    return true if event filtered
//---------------------------------------------------------

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
                  if (!thru && (MusEGlobal::midiFilterCtrl1 == event.dataA()
                     || MusEGlobal::midiFilterCtrl2 == event.dataA()
                     || MusEGlobal::midiFilterCtrl3 == event.dataA()
                     || MusEGlobal::midiFilterCtrl4 == event.dataA())) {
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
  for(unsigned int i = 0; i < MIDI_CHANNELS + 1; ++i)
    _tmpRecordCount[i] = _recordFifo[i].getSize();
  
  // Reset this.
  _sysexFIFOProcessed = false;
}

//---------------------------------------------------------
//   recordEvent
//---------------------------------------------------------

void MidiDevice::recordEvent(MidiRecordEvent& event)
      {
      // TODO: Tested, but record resolution not so good. Switch to wall clock based separate list in MidiDevice. 
      unsigned frame_ts = MusEGlobal::audio->timestamp();
#ifndef _AUDIO_USE_TRUE_FRAME_
      if(MusEGlobal::audio->isPlaying())
       frame_ts += MusEGlobal::segmentSize;  // Shift forward into this period if playing
#endif
      event.setTime(frame_ts);  
      event.setTick(MusEGlobal::lastExtMidiSyncTick);    
      
      if(MusEGlobal::audio->isPlaying())
        event.setLoopNum(MusEGlobal::audio->loopCount());
      
      if (MusEGlobal::midiInputTrace) {
            printf("MidiInput: ");
            event.dump();
            }

      int typ = event.type();
      
      if(_port != -1)
      {
        int idin = MusEGlobal::midiPorts[_port].syncInfo().idIn();
          
        //---------------------------------------------------
        // filter some SYSEX events
        //---------------------------------------------------
  
        if (typ == ME_SYSEX) {
              const unsigned char* p = event.data();
              int n = event.len();
              if (n >= 4) {
                    if ((p[0] == 0x7f)
                      && ((p[1] == 0x7f) || (idin == 0x7f) || (p[1] == idin))) {
                          if (p[2] == 0x06) {
                                MusEGlobal::midiSeq->mmcInput(_port, p, n);
                                return;
                                }
                          if (p[2] == 0x01) {
                                MusEGlobal::midiSeq->mtcInputFull(_port, p, n);
                                return;
                                }
                          }
                    else if (p[0] == 0x7e) {
                          MusEGlobal::midiSeq->nonRealtimeSystemSysex(_port, p, n);
                          return;
                          }
                    }
              }
          else    
            // Trigger general activity indicator detector. Sysex has no channel, don't trigger.
            MusEGlobal::midiPorts[_port].syncInfo().trigActDetect(event.channel());
      }
      
      //
      //  process midi event input filtering and
      //    transformation
      //

      processMidiInputTransformPlugins(event);

      if (filterEvent(event, MusEGlobal::midiRecordType, false))
            return;

      if (!applyMidiInputTransformation(event)) {
            if (MusEGlobal::midiInputTrace)
                  printf("   midi input transformation: event filtered\n");
            return;
            }

      //
      // transfer noteOn and Off events to gui for step recording and keyboard
      // remote control (changed by flo93: added noteOff-events)
      //
      if (typ == ME_NOTEON) {
            int pv = ((event.dataA() & 0xff)<<8) + (event.dataB() & 0xff);
            MusEGlobal::song->putEvent(pv);
            }
      else if (typ == ME_NOTEOFF) {
            int pv = ((event.dataA() & 0xff)<<8) + (0x00); //send an event with velo=0
            MusEGlobal::song->putEvent(pv);
            }
      
      // Do not bother recording if it is NOT actually being used by a port.
      // Because from this point on, process handles things, by selected port.
      if(_port == -1)
        return;
      
      // Split the events up into channel fifos. Special 'channel' number 17 for sysex events.
      unsigned int ch = (typ == ME_SYSEX)? MIDI_CHANNELS : event.channel();
      if(_recordFifo[ch].put(event))
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
                        dev->setName(origname + QString(incstr));    
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
    
  int nv = MusEGlobal::midiPorts[_port].nullSendValue();
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
//    NOTE: Since it waits, it should not be used in RT or other time-sensitive threads. 
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
//     a retry facility is now found in putEventWithRetry. )
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
                  if (!(MusEGlobal::song->mtype() == MT_GM && chn == 9)) {
                        int hb = (b >> 16) & 0xff;
                        int lb = (b >> 8) & 0xff;
                        int pr = b & 0x7f;
                        if (hb != 0xff)
                              putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_HBANK, hb));
                        if (lb != 0xff)
                              putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_LBANK, lb));
                        return putMidiEvent(MidiPlayEvent(0, 0, chn, ME_PROGRAM, pr, 0));
                        }
                  return false;   // Should absorb anyway and return, right?    p4.0.48 Tim.  
                  }
#if 1 // if ALSA cannot handle RPN NRPN etc. DELETETHIS? remove the wrapping #if #endif
            
            if (a < CTRL_14_OFFSET) {          // 7 Bit Controller
                  putMidiEvent(ev);
                  }
            else if (a < CTRL_RPN_OFFSET) {     // 14 bit high resolution controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  int dataH = (b >> 7) & 0x7f;
                  int dataL = b & 0x7f;
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, ctrlH, dataH));
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, ctrlL, dataL));
                  }
            else if (a < CTRL_NRPN_OFFSET) {     // RPN 7-Bit Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_HRPN, ctrlH));
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_LRPN, ctrlL));
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_HDATA, b));
                  
                  // Select null parameters so that subsequent data controller
                  //  events do not upset the last *RPN controller.  Tim.
                  sendNullRPNParams(chn, false);
                }
            else if (a < CTRL_INTERNAL_OFFSET) {     // NRPN 7-Bit Controller
                  int ctrlH = (a >> 8) & 0x7f;
                  int ctrlL = a & 0x7f;
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_HNRPN, ctrlH));
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_LNRPN, ctrlL));
                  putMidiEvent(MidiPlayEvent(0, 0, chn, ME_CONTROLLER, CTRL_HDATA, b));
                  
                  sendNullRPNParams(chn, true);
                  }
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
//   processStuckNotes
//---------------------------------------------------------

void MidiDevice::processStuckNotes() 
{
  // Must be playing for valid nextTickPos, right? But wasn't checked in Audio::processMidi().
  // MusEGlobal::audio->isPlaying() might not be true during seek right now.
  //if(MusEGlobal::audio->isPlaying())  
  {
    bool extsync = MusEGlobal::extSyncFlag.value();
    int frameOffset = MusEGlobal::audio->getFrameOffset();
    unsigned nextTick = MusEGlobal::audio->nextTick();
    iMPEvent k;
    for (k = _stuckNotes.begin(); k != _stuckNotes.end(); ++k) {
          if (k->time() >= nextTick)  
                break;
          MidiPlayEvent ev(*k);
          if(extsync)              // p3.3.25
            ev.setTime(k->time());
          else 
            ev.setTime(MusEGlobal::tempomap.tick2frame(k->time()) + frameOffset);
          _playEvents.add(ev);
          }
    _stuckNotes.erase(_stuckNotes.begin(), k);
  }  
}

//---------------------------------------------------------
//   handleStop
//---------------------------------------------------------

void MidiDevice::handleStop()
{
  // If the device is not in use by a port, don't bother it.
  if(_port == -1)
    return;
    
  MidiPort* mp = &MusEGlobal::midiPorts[_port];
  
  //---------------------------------------------------
  //    send midi stop
  //---------------------------------------------------
  
  // Don't send if external sync is on. The master, and our sync routing system will take care of that.   
  if(!MusEGlobal::extSyncFlag.value())
  {
    // Shall we check open flags? DELETETHIS 4?
    //if(!(dev->rwFlags() & 0x1) || !(dev->openFlags() & 1))
    //if(!(dev->openFlags() & 1))
    //  return;
          
    MidiSyncInfo& si = mp->syncInfo();
    if(si.MMCOut())
      mp->sendMMCStop();
    
    if(si.MRTOut()) 
    {
      mp->sendStop();
      //DELETETHIS 5?
      // Added check of option send continue not start. Hmm, is this required? Seems to make other devices unhappy.
      // (Could try now that this is in MidiDevice.)
      //if(!si.sendContNotStart())
      //  mp->sendSongpos(MusEGlobal::audio->tickPos() * 4 / MusEGlobal::config.division);
    }
  }  

  //---------------------------------------------------
  //    Clear all notes and handle stuck notes
  //---------------------------------------------------
  
  _playEvents.clear();
  for(iMPEvent i = _stuckNotes.begin(); i != _stuckNotes.end(); ++i) 
  {
    MidiPlayEvent ev = *i;
    ev.setTime(0);
    putEvent(ev);
  }
  _stuckNotes.clear();
  
  //---------------------------------------------------
  //    reset sustain
  //---------------------------------------------------
  
  for(int ch = 0; ch < MIDI_CHANNELS; ++ch) 
  {
    if(mp->hwCtrlState(ch, CTRL_SUSTAIN) == 127) 
    {
      MidiPlayEvent ev(0, _port, ch, ME_CONTROLLER, CTRL_SUSTAIN, 0);
      putEvent(ev);
    }
  }
  
  /* DELETETHIS 23
  //---------------------------------------------------
  //    send midi stop
  //---------------------------------------------------
  
  // Don't send if external sync is on. The master, and our sync routing system will take care of that.   
  if(!MusEGlobal::extSyncFlag.value())
  {
    MidiSyncInfo& si = mp->syncInfo();
    if(si.MMCOut())
      mp->sendMMCStop();
    
    if(si.MRTOut()) 
    {
      // Send STOP 
      mp->sendStop();
      // Added check of option send continue not start. Hmm, is this required? Seems to make other devices unhappy.
      // (Could try now that this is in MidiDevice.)
      //if(!si.sendContNotStart())
      //  mp->sendSongpos(MusEGlobal::audio->tickPos() * 4 / MusEGlobal::config.division);
    }
  }  
  */
}
      
//---------------------------------------------------------
//   handleSeek
//---------------------------------------------------------

void MidiDevice::handleSeek()
{
  // If the device is not in use by a port, don't bother it.
  if(_port == -1)
    return;
  
  MidiPort* mp = &MusEGlobal::midiPorts[_port];
  MidiCtrlValListList* cll = mp->controller();
  int pos = MusEGlobal::audio->tickPos();
  
  //---------------------------------------------------
  //    Send STOP 
  //---------------------------------------------------
    
  // Don't send if external sync is on. The master, and our sync routing system will take care of that.  
  if(!MusEGlobal::extSyncFlag.value())
  {
    if(mp->syncInfo().MRTOut())
    {
      // Shall we check for device write open flag to see if it's ok to send?...
      //if(!(rwFlags() & 0x1) || !(openFlags() & 1))
      //if(!(openFlags() & 1))
      //  continue;
      mp->sendStop();
    }    
  }

  //---------------------------------------------------
  //    If playing, clear all notes and handle stuck notes
  //---------------------------------------------------
  
  if(MusEGlobal::audio->isPlaying()) 
  {
    _playEvents.clear();
    for(iMPEvent i = _stuckNotes.begin(); i != _stuckNotes.end(); ++i) 
    {
      MidiPlayEvent ev = *i;
      ev.setTime(0);
      //_playEvents.add(ev); 
      putEvent(ev);          // For immediate playback try putEvent, putMidiEvent, or sendEvent (for the optimizations) instead. 
      //mp->sendEvent(ev);
    }
    _stuckNotes.clear();
  }
  
  //---------------------------------------------------
  //    Send new controller values
  //---------------------------------------------------
    
  for(iMidiCtrlValList ivl = cll->begin(); ivl != cll->end(); ++ivl) 
  {
    MidiCtrlValList* vl = ivl->second;
    iMidiCtrlVal imcv = vl->iValue(pos);
    if(imcv != vl->end()) 
    {
      Part* p = imcv->second.part;
      // Don't send if part or track is muted or off.
      if(!p || p->mute())
        continue;
      Track* track = p->track();
      if(track && (track->isMute() || track->off()))   
        continue;
      unsigned t = (unsigned)imcv->first;
      // Do not add values that are outside of the part.
      if(p && t >= p->tick() && t < (p->tick() + p->lenTick()) )
        //_playEvents.add(MidiPlayEvent(0, _port, ivl->first >> 24, ME_CONTROLLER, vl->num(), imcv->second.val));
        // Use sendEvent to get the optimizations and limiting. But force if there's a value at this exact position.
        mp->sendEvent(MidiPlayEvent(0, _port, ivl->first >> 24, ME_CONTROLLER, vl->num(), imcv->second.val), imcv->first == pos);
        //mp->sendEvent(MidiPlayEvent(0, _port, ivl->first >> 24, ME_CONTROLLER, vl->num(), imcv->second.val), pos == 0 || imcv->first == pos);
    }
  }
  
  //---------------------------------------------------
  //    reset sustain
  //---------------------------------------------------
  
  for(int ch = 0; ch < MIDI_CHANNELS; ++ch) 
  {
    if(mp->hwCtrlState(ch, CTRL_SUSTAIN) == 127) 
    {
      MidiPlayEvent ev(0, _port, ch, ME_CONTROLLER, CTRL_SUSTAIN, 0);
      putEvent(ev);
    }
  }
  
  //---------------------------------------------------
  //    Send STOP and "set song position pointer"
  //---------------------------------------------------
    
  // Don't send if external sync is on. The master, and our sync routing system will take care of that.  
  if(!MusEGlobal::extSyncFlag.value())
  {
    if(mp->syncInfo().MRTOut())
    {
      //mp->sendStop();   // Moved above
      int beat = (pos * 4) / MusEGlobal::config.division;
      mp->sendSongpos(beat);
    }    
  }
}

} // namespace MusECore

