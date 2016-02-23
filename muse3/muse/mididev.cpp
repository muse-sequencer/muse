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
#include "globaldefs.h"
#include "audio.h"
#include "audiodev.h"
#include "midiseq.h"
#include "sync.h"
#include "midiitransform.h"
#include "part.h"
#include "drummap.h"
#include "operations.h"


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
      
      _state         = QString("Closed");
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

QString MidiDevice::deviceTypeString()
{
  switch(deviceType())
  {
    case ALSA_MIDI:
        return "ALSA";
    case JACK_MIDI:
        return "JACK";
    case SYNTH_MIDI:
    {
      SynthI* s = dynamic_cast<SynthI*>(this);
      if(s && s->synth())
        return MusECore::synthType2String(s->synth()->synthType());
      else
        return "SYNTH";
    }
  }
  return "UNKNOWN";
}
      
void MidiDevice::setPort(int p)
{
  _port = p; 
  if(_port != -1)
    MusEGlobal::midiPorts[_port].clearInitSent();
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
            fprintf(stderr, "MidiInput: ");
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
                  fprintf(stderr, "   midi input transformation: event filtered\n");
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
        fprintf(stderr, "MidiDevice::recordEvent: fifo channel %d overflow\n", ch);
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
//   addOperation
//---------------------------------------------------------

void MidiDeviceList::addOperation(MidiDevice* dev, PendingOperationList& ops)
{
  bool gotUniqueName=false;
  int increment = 0;
  QString origname = dev->name();
  PendingOperationItem poi(this, dev, PendingOperationItem::AddMidiDevice);
  // check if the name's been taken
  while(!gotUniqueName) 
  {
    if(increment >= 10000)
    {
      fprintf(stderr, "MusE Error: MidiDeviceList::addOperation(): Out of 10000 unique midi device names!\n");
      return;        
    }
    gotUniqueName = true;
    // In the case of type AddMidiDevice, this searches for the name only.
    iPendingOperation ipo = ops.findAllocationOp(poi);
    if(ipo != ops.end())
    {
      PendingOperationItem& poif = *ipo;
      if(poif._midi_device == poi._midi_device)
        return;  // Device itself is already added! 
        
      // TODO: This and section below should be changed to simply: dev->setName(origname + QString::number(increment))  
      //        but first must be careful of localizations - will it give differing results?
      char incstr[4];
      sprintf(incstr,"_%d",++increment);
      dev->setName(origname + QString(incstr));
      
      gotUniqueName = false;
    }    
    
    for(iMidiDevice i = begin(); i != end(); ++i) 
    {
      const QString s = (*i)->name();
      if(s == dev->name())
      {
        char incstr[4];
        sprintf(incstr,"_%d",++increment);
        dev->setName(origname + QString(incstr));    
        gotUniqueName = false;
      }
    }
  }
  
  ops.add(poi);
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
//   resetCurParamNums
//   Reset output channel's current parameter numbers to -1.
//   All channels if chan = -1.
//---------------------------------------------------------

void MidiDevice::resetCurOutParamNums(int chan)
{
  if(chan == -1)
  {
    for(int i = 0; i < MIDI_CHANNELS; ++i)
      _curOutParamNums[i].resetParamNums();
    return;
  }
  _curOutParamNums[chan].resetParamNums();
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
      
    int sleepOk = -1;
    while(sleepOk == -1)
      sleepOk = usleep(delayUs);   // FIXME: usleep is supposed to be depricated!
  }  
  return true;
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
    const bool extsync = MusEGlobal::extSyncFlag.value();
    const int frameOffset = MusEGlobal::audio->getFrameOffset();
    const unsigned nextTick = MusEGlobal::audio->nextTick();
    ciMPEvent k;

    //---------------------------------------------------
    //    Play any stuck notes which were put directly to the device
    //---------------------------------------------------

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

    //------------------------------------------------------------
    //    To save time, playing of any track-related playback stuck notes (NOT 'live' notes)
    //     which were not put directly to the device, is done in Audio::processMidi().
    //------------------------------------------------------------
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
  //    Clear all notes and flush out any stuck notes
  //     which were put directly to the device
  //---------------------------------------------------

  _playEvents.clear();
  for(iMPEvent i = _stuckNotes.begin(); i != _stuckNotes.end(); ++i) 
  {
    MidiPlayEvent ev(*i);
    ev.setTime(0);
    putEvent(ev);
  }
  _stuckNotes.clear();
  
  //------------------------------------------------------------
  //    Flush out any track-related playback stuck notes (NOT 'live' notes)
  //     which were not put directly to the device
  //------------------------------------------------------------
  
  for(ciMidiTrack imt = MusEGlobal::song->midis()->begin(); imt != MusEGlobal::song->midis()->end(); ++imt)
  {
    MPEventList& mel = (*imt)->stuckNotes;
    for(iMPEvent i = mel.begin(), i_next = i; i != mel.end(); i = i_next) 
    {
      ++i_next;

      if((*i).port() != _port)
        continue;
      MidiPlayEvent ev(*i);
      ev.setTime(0);
      putEvent(ev); // For immediate playback try putEvent, putMidiEvent, or sendEvent (for the optimizations).
      mel.erase(i);
    }
  }
  
  //---------------------------------------------------
  //    reset sustain
  //---------------------------------------------------
  
  for(int ch = 0; ch < MIDI_CHANNELS; ++ch) 
  {
    if(mp->hwCtrlState(ch, CTRL_SUSTAIN) == 127) 
    {
      const MidiPlayEvent ev(0, _port, ch, ME_CONTROLLER, CTRL_SUSTAIN, 0);
      putEvent(ev);
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
  
  MidiPort* mp = &MusEGlobal::midiPorts[_port];
  MidiInstrument* instr = mp->instrument();
  MidiCtrlValListList* cll = mp->controller();
  unsigned pos = MusEGlobal::audio->tickPos();
  
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
  //    If playing, clear all notes and flush out any
  //     stuck notes which were put directly to the device
  //---------------------------------------------------
  
  if(MusEGlobal::audio->isPlaying()) 
  {
    _playEvents.clear();
    for(iMPEvent i = _stuckNotes.begin(); i != _stuckNotes.end(); ++i) 
    {
      MidiPlayEvent ev(*i);
      ev.setTime(0);
      putEvent(ev);  // For immediate playback try putEvent, putMidiEvent, or sendEvent (for the optimizations).
    }
    _stuckNotes.clear();
  }
  
  //---------------------------------------------------
  //    Send new controller values
  //---------------------------------------------------
    
  // Find channels on this port used in the song...
  bool usedChans[MIDI_CHANNELS];
  int usedChanCount = 0;
  for(int i = 0; i < MIDI_CHANNELS; ++i)
    usedChans[i] = false;
  if(MusEGlobal::song->click() && MusEGlobal::clickPort == _port)
  {
    usedChans[MusEGlobal::clickChan] = true;
    ++usedChanCount;
  }
  bool drum_found = false;
  for(ciMidiTrack imt = MusEGlobal::song->midis()->begin(); imt != MusEGlobal::song->midis()->end(); ++imt)
  {
    //------------------------------------------------------------
    //    While we are at it, flush out any track-related playback stuck notes
    //     (NOT 'live' notes) which were not put directly to the device
    //------------------------------------------------------------
    MPEventList& mel = (*imt)->stuckNotes;
    for(iMPEvent i = mel.begin(), i_next = i; i != mel.end(); i = i_next)
    {
      ++i_next;

      if((*i).port() != _port)
        continue;
      MidiPlayEvent ev(*i);
      ev.setTime(0);
      putEvent(ev); // For immediate playback try putEvent, putMidiEvent, or sendEvent (for the optimizations).
      mel.erase(i);
    }
    
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
          if(mport != _port || usedChans[mchan])
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
      if((*imt)->outPort() != _port || usedChans[(*imt)->outChannel()])
        continue;
      usedChans[(*imt)->outChannel()] = true;
      ++usedChanCount;
    }

    if(usedChanCount >= MIDI_CHANNELS)
      break;    // All are used. Done searching.
  }   
  
  for(iMidiCtrlValList ivl = cll->begin(); ivl != cll->end(); ++ivl) 
  {
    MidiCtrlValList* vl = ivl->second;
    int chan = ivl->first >> 24;
    if(!usedChans[chan])  // Channel not used in song?
      continue;
    int ctlnum = vl->num();

    // Find the first non-muted value at the given tick...
    bool values_found = false;
    bool found_value = false;
    
    iMidiCtrlVal imcv = vl->lower_bound(pos);
    if(imcv != vl->end() && imcv->first == (int)pos)
    {
      for( ; imcv != vl->end() && imcv->first == (int)pos; ++imcv)
      {
        const Part* p = imcv->second.part;
        if(!p)
          continue;
        // Ignore values that are outside of the part.
        if(pos < p->tick() || pos >= (p->tick() + p->lenTick()))
          continue;
        values_found = true;
        // Ignore if part or track is muted or off.
        if(p->mute())
          continue;
        const Track* track = p->track();
        if(track && (track->isMute() || track->off()))
          continue;
        found_value = true;
        break;
      }
    }
    else
    {
      while(imcv != vl->begin())
      {
        --imcv;
        const Part* p = imcv->second.part;
        if(!p)
          continue;
        // Ignore values that are outside of the part.
        unsigned t = imcv->first;
        if(t < p->tick() || t >= (p->tick() + p->lenTick()))
          continue;
        values_found = true;
        // Ignore if part or track is muted or off.
        if(p->mute())
          continue;
        const Track* track = p->track();
        if(track && (track->isMute() || track->off()))
          continue;
        found_value = true;
        break;
      }
    }

    if(found_value)
    {
      // Don't bother sending any sustain values if not playing. Just set the hw state.
      if(ctlnum == CTRL_SUSTAIN && !MusEGlobal::audio->isPlaying())
        mp->setHwCtrlState(chan, CTRL_SUSTAIN, imcv->second.val);
      else
        // Use sendEvent to get the optimizations and limiting. But force if there's a value at this exact position.
        // NOTE: Why again was this forced? There was a reason. Think it was RJ in response to bug rep, then I modded.
        // A reason not to force: If a straight line is drawn on graph, multiple identical events are stored
        //  (which must be allowed). So seeking through them here sends them all redundantly, not good. // REMOVE Tim.
        mp->sendEvent(MidiPlayEvent(0, _port, chan, ME_CONTROLLER, ctlnum, imcv->second.val), false); //, imcv->first == pos);
        //mp->sendEvent(MidiPlayEvent(0, _port, chan, ME_CONTROLLER, ctlnum, imcv->second.val), pos == 0 || imcv->first == pos);
    }

    // Either no value was found, or they were outside parts, or pos is in the unknown area before the first value.
    // Send instrument default initial values.  NOT for syntis. Use midiState and/or initParams for that. 
    //if((imcv == vl->end() || !done) && !MusEGlobal::song->record() && instr && !isSynti()) 
    // Hmm, without refinement we can only do this at position 0, due to possible 'skipped' values outside parts, above.
    if(!values_found && MusEGlobal::config.midiSendCtlDefaults && !MusEGlobal::song->record() && pos == 0 && instr && !isSynti())
    {
      MidiControllerList* mcl = instr->controller();
      ciMidiController imc = mcl->find(vl->num());
      if(imc != mcl->end())
      {
        MidiController* mc = imc->second;
        if(mc->initVal() != CTRL_VAL_UNKNOWN)
          // Use sendEvent to get the optimizations and limiting. No force sending. Note the addition of bias.
          mp->sendEvent(MidiPlayEvent(0, _port, chan, ME_CONTROLLER, ctlnum, mc->initVal() + mc->bias()), false);
      }
    }
  }
  
  //---------------------------------------------------
  //    reset sustain
  //---------------------------------------------------
  
  for(int ch = 0; ch < MIDI_CHANNELS; ++ch) 
  {
    if(mp->hwCtrlState(ch, CTRL_SUSTAIN) == 127) 
    {
      const MidiPlayEvent ev(0, _port, ch, ME_CONTROLLER, CTRL_SUSTAIN, 0);
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

