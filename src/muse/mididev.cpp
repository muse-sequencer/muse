//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mididev.cpp,v 1.10.2.6 2009/11/05 03:14:35 terminator356 Exp $
//
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011, 2016 Tim E. Real (terminator356 on users dot sourceforge dot net)
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
#include "midi_consts.h"
#include "midiport.h"
#include "mididev.h"
#include "config.h"
#include "gconfig.h"
#include "globals.h"
#include "audio.h"
#include "audiodev.h"
#include "midiseq.h"
#include "midiitransform.h"
#include "mitplugin.h"
#include "part.h"
#include "drummap.h"
#include "helper.h"
#include "ticksynth.h"

// Forwards from header:
#include "xml.h"

// Undefine if and when multiple output routes are added to midi tracks.
#define _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_

// For debugging output: Uncomment the fprintf section.
//#define DEBUG_MIDI_DEVICE(dev, format, args...)  //fprintf(dev, format, ##args);

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

// Static.
const int MidiDevice::extClockHistoryCapacity = 1024;


//---------------------------------------------------------
//   initMidiDevices
//---------------------------------------------------------

void initMidiDevices()
      {
#ifdef MIDI_DRIVER_MIDI_SERIAL
      initMidiSerial();
#endif
#ifdef ALSA_SUPPORT
      if(MusEGlobal::config.enableAlsaMidiDriver ||                         // User setting
         MusEGlobal::useAlsaWithJack ||                                     // Command line override
         MusEGlobal::audioDevice->deviceType() != AudioDevice::JACK_AUDIO)  // Jack not running
      {
        if(initMidiAlsa())
          {
          QMessageBox::critical(nullptr, "MusE fatal error.", "MusE failed to initialize the\n" 
                                                          "Alsa midi subsystem, check\n"
                                                          "your configuration.");
          exit(-1);
          }
      }
#endif
      
      if(initMidiJack())
          {
          QMessageBox::critical(nullptr, "MusE fatal error.", "MusE failed to initialize the\n" 
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
      _extClockHistoryFifo = new LockFreeBuffer<ExtMidiClock>(extClockHistoryCapacity);
      
      // TODO: Scale these according to the current audio segment size.
      _playbackEventBuffers = new LockFreeMPSCRingBuffer<MidiPlayEvent>(1024);
      _userEventBuffers = new LockFreeMPSCRingBuffer<MidiPlayEvent>(1024);
      
      _sysExOutDelayedEvents = new std::vector<MidiPlayEvent>;
      // Initially reserve a fair number of items to hold potentially a lot 
      //  of messages when the sysex processor is busy (in the Sending state).
      _sysExOutDelayedEvents->reserve(1024);
      _stopFlag.store(false);

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
      for(unsigned int i = 0; i < MusECore::MUSE_MIDI_CHANNELS + 1; ++i)
        _tmpRecordCount[i] = 0;
      
      _sysexFIFOProcessed = false;
      
      init();
      }

MidiDevice::MidiDevice(const QString& n)
   : _name(n)
      {
      for(unsigned int i = 0; i < MusECore::MUSE_MIDI_CHANNELS + 1; ++i)
        _tmpRecordCount[i] = 0;
      
      _sysexFIFOProcessed = false;
      
      init();
      }

MidiDevice::~MidiDevice() 
{
    if(_sysExOutDelayedEvents)
      delete _sysExOutDelayedEvents;
    if(_extClockHistoryFifo)
      delete _extClockHistoryFifo;
    if(_userEventBuffers)
      delete _userEventBuffers;
    if(_playbackEventBuffers)
      delete _playbackEventBuffers;
}

QString MidiDevice::deviceTypeString() const
{
  switch(deviceType())
  {
    case ALSA_MIDI:
        return "ALSA";
    case JACK_MIDI:
        return "JACK";
    case SYNTH_MIDI:
    {
      const SynthI* s = dynamic_cast<const SynthI*>(this);
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
  for(unsigned int i = 0; i < MusECore::MUSE_MIDI_CHANNELS + 1; ++i)
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
  for(unsigned int i = 0; i < MusECore::MUSE_MIDI_CHANNELS + 1; ++i)
    _tmpRecordCount[i] = _recordFifo[i].getSize();
  
  // Reset this.
  _sysexFIFOProcessed = false;
}

//---------------------------------------------------------
//   midiClockInput
//    Midi clock (24 ticks / quarter note)
//---------------------------------------------------------

void MidiDevice::midiClockInput(unsigned int frame)
{
  // Put a midi clock record event into the clock history fifo. Ignore port and channel.
  // Timestamp with the current frame.
  const ExtMidiClock ext_clk = MusEGlobal::midiSyncContainer.midiClockInput(midiPort(), frame);
  if(ext_clk.isValid() && extClockHistory())
    extClockHistory()->put(ext_clk);
}

//---------------------------------------------------------
//   recordEvent
//---------------------------------------------------------

void MidiDevice::recordEvent(MidiRecordEvent& event)
      {
      if(MusEGlobal::audio->isPlaying())
        event.setLoopNum(MusEGlobal::audio->loopCount());
      
      if (MusEGlobal::midiInputTrace) {
            fprintf(stderr, "MidiInput: ");
            dumpMPEvent(&event);
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
                                MusEGlobal::midiSyncContainer.mmcInput(_port, p, n);
                                return;
                                }
                          if (p[2] == 0x01) {
                                MusEGlobal::midiSyncContainer.mtcInputFull(_port, p, n);
                                return;
                                }
                          }
                    else if (p[0] == 0x7e) {
                          MusEGlobal::midiSyncContainer.nonRealtimeSystemSysex(_port, p, n);
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
      else if (MusEGlobal::rcEnableCC && typ == ME_CONTROLLER) {
          char cc = static_cast<char>(event.dataA() & 0xff);
          printf("*** Input CC: %d\n", cc);
          MusEGlobal::song->putEventCC(cc);
      }

      // Do not bother recording if it is NOT actually being used by a port.
      // Because from this point on, process handles things, by selected port.
      if(_port == -1)
        return;
      
      // Split the events up into channel fifos. Special 'channel' number 17 for sysex events.
      unsigned int ch = (typ == ME_SYSEX)? MusECore::MUSE_MIDI_CHANNELS : event.channel();
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
      const QString origname = dev->name();
      QString newName = origname;
      while (!gotUniqueName) {
            gotUniqueName = true;
            // check if the name's been taken
            for (iMidiDevice i = begin(); i != end(); ++i) {
                  const QString s = (*i)->name();
                  if (s == newName)
                        {
                        newName = origname + QString("_%1").arg(++increment);
                        gotUniqueName = false;
                        }
                  }
            }
      if(origname != newName)
        dev->setName(newName);
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
//   resetCurParamNums
//   Reset output channel's current parameter numbers to -1.
//   All channels if chan = -1.
//---------------------------------------------------------

void MidiDevice::resetCurOutParamNums(int chan)
{
  if(chan == -1)
  {
    for(int i = 0; i < MusECore::MUSE_MIDI_CHANNELS; ++i)
      _curOutParamNums[i].resetParamNums();
    return;
  }
  _curOutParamNums[chan].resetParamNums();
}

//---------------------------------------------------------
//   putEvent
//    return true if event cannot be delivered
//---------------------------------------------------------

bool MidiDevice::putEvent(const MidiPlayEvent& ev, LatencyType latencyType, EventBufferType bufferType)
{
// TODO: Decide whether we want the driver cached values always updated like this,
//        even if not writeable or if error.
//   if(!_writeEnable)
//     return true;
  
  // Automatically shift the time forward if specified.
  MidiPlayEvent fin_ev = ev;
  switch(latencyType)
  {
    case NotLate:
    break;
    
    case Late:
      fin_ev.setTime(fin_ev.time() + pbForwardShiftFrames());
    break;
  }
  
  //DEBUG_MIDI_DEVICE(stderr, "MidiDevice::putUserEvent devType:%d time:%d type:%d ch:%d A:%d B:%d\n", 
  //                  deviceType(), fin_ev.time(), fin_ev.type(), fin_ev.channel(), fin_ev.dataA(), fin_ev.dataB());
  if (MusEGlobal::midiOutputTrace)
  {
    fprintf(stderr, "MidiDevice::putEvent: %s: <%s>: ", deviceTypeString().toLatin1().constData(), name().toLatin1().constData());
    dumpMPEvent(&fin_ev);
  }
  
  bool rv = true;
  switch(bufferType)
  {
    case PlaybackBuffer:
      rv = !_playbackEventBuffers->put(fin_ev);
    break;
    
    case UserBuffer:
      rv = !_userEventBuffers->put(fin_ev);
    break;
  }
  
  if(rv)
    fprintf(stderr, "MidiDevice::putEvent: Error: Device buffer overflow. bufferType:%d\n", bufferType);
  
  return rv;
}

//---------------------------------------------------------
//   processStuckNotes
//   To be called by audio thread only.
//---------------------------------------------------------

void MidiDevice::processStuckNotes() 
{
  // Must be playing for valid nextTickPos, right? But wasn't checked in Audio::processMidi().
  // MusEGlobal::audio->isPlaying() might not be true during seek right now.
  //if(MusEGlobal::audio->isPlaying())  
  {
    const bool extsync = MusEGlobal::extSyncFlag;
    const unsigned syncFrame = MusEGlobal::audio->curSyncFrame();
    const unsigned curTickPos = MusEGlobal::audio->tickPos();
    const unsigned nextTick = MusEGlobal::audio->nextTick();
    // What is the current transport frame?
    const unsigned int pos_fr = MusEGlobal::audio->pos().frame();
    // What is the (theoretical) next transport frame?
    const unsigned int next_pos_fr = pos_fr + MusEGlobal::audio->curCycleFrames();
    ciMPEvent k;

    //---------------------------------------------------
    //    Play any stuck notes which were put directly to the device
    //---------------------------------------------------

    for (k = _stuckNotes.begin(); k != _stuckNotes.end(); ++k) {
          MidiPlayEvent ev(*k);
          unsigned int off_tick = ev.time();
          // If external sync is not on, we can take advantage of frame accuracy but
          //  first we must allow the next tick position to be included in the search
          //  even if it is equal to the current tick position.
          if (extsync ? (off_tick >= nextTick) : (off_tick > nextTick))  
                break;
          unsigned int off_frame = 0;
          if(extsync)
          {
            if(off_tick < curTickPos)
              off_tick = curTickPos;
            off_frame = MusEGlobal::audio->extClockHistoryTick2Frame(off_tick - curTickPos) + MusEGlobal::segmentSize;
          }
          else
          {
            // What is the exact transport frame that the event should be played at?
            const unsigned int fr = MusEGlobal::tempomap.tick2frame(off_tick);
            // Is the event frame outside of the current transport frame range?
            if(fr >= next_pos_fr)
              break;
            off_frame = (fr < pos_fr) ? 0 : fr - pos_fr;
            off_frame += syncFrame;
          }
          ev.setTime(off_frame);

          _userEventBuffers->put(ev);
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
//   To be called by audio thread only.
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
  if(!MusEGlobal::extSyncFlag)
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

  setStopFlag(true);
  for(iMPEvent i = _stuckNotes.begin(); i != _stuckNotes.end(); ++i) 
  {
    MidiPlayEvent ev(*i);
    ev.setTime(0);  // Immediate processing. TODO Use curFrame?
    //ev.setTime(MusEGlobal::audio->midiQueueTimeStamp(ev.time()));
        putEvent(ev, MidiDevice::NotLate);
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
      ev.setTime(0);  // Immediate processing. TODO Use curFrame?
      //ev.setTime(MusEGlobal::audio->midiQueueTimeStamp(ev.time()));
      putEvent(ev, MidiDevice::NotLate);
            
      mel.erase(i);
    }
  }
  
  //---------------------------------------------------
  //    reset sustain
  //---------------------------------------------------
  
  for(int ch = 0; ch < MusECore::MUSE_MIDI_CHANNELS; ++ch) 
  {
    if(mp->hwCtrlState(ch, CTRL_SUSTAIN) == 127) 
    {
      MidiPlayEvent ev(0, _port, ch, ME_CONTROLLER, CTRL_SUSTAIN, 0); // Immediate processing. TODO Use curFrame?
      //ev.setTime(MusEGlobal::audio->midiQueueTimeStamp(ev.time()));
      putEvent(ev, MidiDevice::NotLate);
    }
  }
}
      
//---------------------------------------------------------
//   handleSeek
//   To be called by audio thread only.
//---------------------------------------------------------

void MidiDevice::handleSeek()
{
  //---------------------------------------------------
  //    If playing, clear all notes and flush out any
  //     stuck notes which were put directly to the device
  //---------------------------------------------------
  
  if(MusEGlobal::audio->isPlaying()) 
  {
    // TODO: Don't clear, let it play whatever was scheduled ?
    //setStopFlag(true);
    for(iMPEvent i = _stuckNotes.begin(); i != _stuckNotes.end(); ++i) 
    {
      MidiPlayEvent ev(*i);
      ev.setTime(0); // Immediate processing. TODO Use curFrame?
      //ev.setTime(MusEGlobal::audio->midiQueueTimeStamp(ev.time()));
      putEvent(ev, MidiDevice::NotLate);
    }
    _stuckNotes.clear();
  }
}

//================================================
// BEGIN Latency correction/compensation routines.
//================================================

void MidiDevice::prepareLatencyScan() { 
  // Reset some latency info to prepare for (re)computation.
  _captureLatencyInfo.initialize();
  _playbackLatencyInfo.initialize();
}

bool MidiDevice::isLatencyInputTerminalMidi(bool capture)
{
  TrackLatencyInfo* tli = capture ? &_captureLatencyInfo : &_playbackLatencyInfo;

  // Have we been here before during this scan?
  // Just return the cached value.
  if(tli->_isLatencyInputTerminalProcessed)
    return tli->_isLatencyInputTerminal;

  const int port = midiPort();

  // Playback devices are considered a termination point.
  if(!capture || port < 0 || port >= MusECore::MIDI_PORTS)
  {
    tli->_isLatencyInputTerminal = true;
    tli->_isLatencyInputTerminalProcessed = true;
    return true;
  }

  MidiPort* mp = &MusEGlobal::midiPorts[port];
  const RouteList* rl = mp->outRoutes();
  for (ciRoute ir = rl->begin(); ir != rl->end(); ++ir) {
    switch(ir->type)
    {
      case Route::TRACK_ROUTE:
        if(!ir->track)
          continue;
        if(ir->track->isMidiTrack())
        {
          Track* track = ir->track;
          if(track->off()) // || 
            //(atrack->canRecordMonitor() && (MusEGlobal::config.monitoringAffectsLatency || !atrack->isRecMonitored())))
              //&& atrack->canRecord() && !atrack->recordFlag()))
            continue;
          
          tli->_isLatencyInputTerminal = false;
          tli->_isLatencyInputTerminalProcessed = true;
          return false;
        }
      break;

      default:
      break;
    }
  }

  tli->_isLatencyInputTerminal = true;
  tli->_isLatencyInputTerminalProcessed = true;
  return true;
}

bool MidiDevice::isLatencyOutputTerminalMidi(bool capture)
{
  TrackLatencyInfo* tli = capture ? &_captureLatencyInfo : &_playbackLatencyInfo;

  // Have we been here before during this scan?
  // Just return the cached value.
  if(tli->_isLatencyOutputTerminalProcessed)
    return tli->_isLatencyOutputTerminal;

  const int port = midiPort();

  // Playback devices are considered a termination point.
  if(!capture || port < 0 || port >= MusECore::MIDI_PORTS)
  {
    tli->_isLatencyOutputTerminal = true;
    tli->_isLatencyOutputTerminalProcessed = true;
    return true;
  }

  MidiPort* mp = &MusEGlobal::midiPorts[port];
  const RouteList* rl = mp->outRoutes();
  for (ciRoute ir = rl->begin(); ir != rl->end(); ++ir) {
    switch(ir->type)
    {
      case Route::TRACK_ROUTE:
        if(!ir->track)
          continue;
        if(ir->track->isMidiTrack())
        {
          Track* track = ir->track;
          if(track->off()) // || 
            //(atrack->canRecordMonitor() && (MusEGlobal::config.monitoringAffectsLatency || !atrack->isRecMonitored())))
              //&& atrack->canRecord() && !atrack->recordFlag()))
            continue;
          
          tli->_isLatencyOutputTerminal = false;
          tli->_isLatencyOutputTerminalProcessed = true;
          return false;
        }
      break;

      default:
      break;
    }
  }

  tli->_isLatencyOutputTerminal = true;
  tli->_isLatencyOutputTerminalProcessed = true;
  return true;
}

//---------------------------------------------------------
//   getWorstSelfLatencyMidi
//---------------------------------------------------------

float MidiDevice::getWorstSelfLatencyMidi(bool capture)
{
  TrackLatencyInfo* tli = capture ? &_captureLatencyInfo : &_playbackLatencyInfo;

  // Have we been here before during this scan?
  // Just return the cached value.
  if(tli->_worstSelfLatencyMidiProcessed)
    return tli->_worstSelfLatencyMidi;

// REMOVE Tim. latency. Changed. TESTING. Reinstate.
//   for(int i = 0; i < MusECore::MUSE_MIDI_CHANNELS; ++i)
  {
    //if(!used_chans[i])
    //  continue;
//     const float lat = selfLatencyMidi(i, capture);
    const float lat = selfLatencyMidi(0, capture);
    //const float lat = selfLatencyMidi(i, 0 /*playback*/);
    if(lat > tli->_worstSelfLatencyMidi)
      tli->_worstSelfLatencyMidi = lat;
  }
  
  // The absolute latency of signals leaving this track is the sum of
  //  any connected route latencies and this track's latency.
  tli->_worstSelfLatencyMidiProcessed = true;
  return tli->_worstSelfLatencyMidi;
}

inline bool MidiDevice::canDominateOutputLatencyMidi(bool capture) const
{
  if(capture)
    return true;
  return false;
}

inline bool MidiDevice::canDominateInputLatencyMidi(bool /*capture*/) const
{
  return false;
}

inline bool MidiDevice::canDominateEndPointLatencyMidi(bool capture) const
{
  if(capture)
    return false;
  return true;
}

inline bool MidiDevice::canPassThruLatencyMidi(bool /*capture*/) const
{ 
  return true;
}

//---------------------------------------------------------
//   getDominanceInfoMidi
//---------------------------------------------------------

TrackLatencyInfo& MidiDevice::getDominanceInfoMidi(bool capture, bool input)
{
      TrackLatencyInfo* tli = capture ? &_captureLatencyInfo : &_playbackLatencyInfo;

      // Have we been here before during this scan?
      // Just return the cached value.
      if((input && tli->_canDominateInputProcessed) ||
        (!input && tli->_canDominateProcessed))
        return *tli;

      // Get the default domination for this track type.
      bool can_dominate_lat = input ? canDominateInputLatencyMidi(capture) : canDominateOutputLatencyMidi(capture);
      bool can_correct_lat = canCorrectOutputLatencyMidi();

      const bool passthru = canPassThruLatencyMidi(capture);

      bool item_found = false;

      const int port = midiPort();
      const int open_flags = openFlags();

      // Gather latency info from all connected input branches,
      //  but ONLY if the track is not off.
      // Currently there are no routes FROM tracks (audio or midi) TO midi capture devices,
      //  only TO midi playback devices.
      // CAUTION: The ABSENCE of the '!capture' caused an infinite loop crash, where
      //  MidiDevice::getDominanceInfoMidi called Track::getDominanceInfo which called
      //  MidiDevice::getDominanceInfoMidi again, and repeat inf...
      // When that happens, all the "Have we been here before...?" checks say 'no'
      //  because each call has not finished yet, where at the end we say 'yes'.
      // So I'm not sure how we could support the above future plan, if any.
      if(!capture && (open_flags & (/*capture ? 2 :*/ 1)) && (passthru || input) &&
        port >= 0 && port < MusECore::MIDI_PORTS)
      {
//         bool used_chans[MusECore::MUSE_MIDI_CHANNELS];
//         for(int i = 0; i < MusECore::MUSE_MIDI_CHANNELS; ++i)
//           used_chans[i] = false;
//         bool all_chans = false;

#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
        const MidiTrackList& tl = *MusEGlobal::song->midis();
        const MidiTrackList::size_type tl_sz = tl.size();
        for(MidiTrackList::size_type it = 0; it < tl_sz; ++it)
        {
          MidiTrack* track = static_cast<MidiTrack*>(tl[it]);
          if(track->outPort() != port)
            continue;
          
          //if((open_flags & (/*capture ? 2 :*/ 1)) && !track->off() && (passthru || input))
          if(!track->off())
          {
            const TrackLatencyInfo& li = track->getDominanceInfo(false);

            // Whether the branch can dominate or correct latency or if we
            //  want to allow unterminated input branches to
            //  participate in worst branch latency calculations.
            const bool participate = 
              (li._canCorrectOutputLatency ||
              li._canDominateOutputLatency ||
              MusEGlobal::config.correctUnterminatedInBranchLatency);

            if(participate)
            {
              // Is it the first found item?
              if(item_found)
              {
                // If any one of the branches can dominate the latency,
                //  that overrides any which cannot.
                if(li._canDominateOutputLatency)
                  can_dominate_lat = true;
                if(li._canCorrectOutputLatency)
                  can_correct_lat = true;
              }
              else
              {
                item_found = true;
                // Override the defaults with this first item's values.
                can_dominate_lat = li._canDominateOutputLatency;
                can_correct_lat = li._canCorrectOutputLatency;
              }
            }
          }
        }

#else

        MidiPort* mp = &MusEGlobal::midiPorts[port];
        RouteList* rl = mp->inRoutes();
        for (iRoute ir = rl->begin(); ir != rl->end(); ++ir)
        {
          switch(ir->type)
          {
              case Route::TRACK_ROUTE:
                if(!ir->track)
                  continue;
                
                if(ir->track->isMidiTrack())
                {
                  if(ir->channel < -1 || ir->channel >= MusECore::MUSE_MIDI_CHANNELS)
                    continue;

                  Track* track = ir->track;
//                     if(ir->channel < 0)
//                       all_chans = true;
//                     else
//                       used_chans[ir->channel] = true;
                    
                  //if((open_flags & (/*capture ? 2 :*/ 1)) && !track->off() && (passthru || input))
                  if(!track->off())
                  {
                    const TrackLatencyInfo& li = track->getDominanceInfo(false);

                    // Whether the branch can dominate or correct latency or if we
                    //  want to allow unterminated input branches to
                    //  participate in worst branch latency calculations.
                    const bool participate = 
                      (li._canCorrectOutputLatency ||
                      li._canDominateOutputLatency ||
                      MusEGlobal::config.correctUnterminatedInBranchLatency);

                    if(participate)
                    {
                      // Is it the first found item?
                      if(item_found)
                      {
                        // If any one of the branches can dominate the latency,
                        //  that overrides any which cannot.
                        if(li._canDominateOutputLatency)
                          can_dominate_lat = true;
                        if(li._canCorrectOutputLatency)
                          can_correct_lat = true;
                      }
                      else
                      {
                        item_found = true;
                        // Override the defaults with this first item's values.
                        can_dominate_lat = li._canDominateOutputLatency;
                        can_correct_lat = li._canCorrectOutputLatency;
                      }
                    }
                  }
                }
              break;

              default:
              break;
          }            
        }

#endif          

        // Special for the built-in metronome.
        //if(!capture)
        //{
          MusECore::MetronomeSettings* metro_settings = 
            MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

          if(metro_settings->midiClickFlag && metro_settings->clickPort == port)
          {
            //if((open_flags & (/*capture ? 2 :*/ 1)) && !MusECore::metronome->off() && (passthru || input))
            if(!MusECore::metronome->off())
            {
              const TrackLatencyInfo& li = MusECore::metronome->getDominanceInfoMidi(capture, false);

              // Whether the branch can dominate or correct latency or if we
              //  want to allow unterminated input branches to
              //  participate in worst branch latency calculations.
              const bool participate = 
                (li._canCorrectOutputLatency ||
                li._canDominateOutputLatency ||
                MusEGlobal::config.correctUnterminatedInBranchLatency);

              if(participate)
              {
                // Is it the first found item?
                if(item_found)
                {
                  // If any one of the branches can dominate the latency,
                  //  that overrides any which cannot.
                  if(li._canDominateOutputLatency)
                    can_dominate_lat = true;
                  if(li._canCorrectOutputLatency)
                    can_correct_lat = true;
                }
                else
                {
                  item_found = true;
                  // Override the defaults with this first item's values.
                  //route_worst_out_corr = li._outputAvailableCorrection;
                  can_dominate_lat = li._canDominateOutputLatency;
                  can_correct_lat = li._canCorrectOutputLatency;
                }
              }
            }
          }
        //}
      }
      
      // Set the correction of all connected input branches,
      //  but ONLY if the track is not off.
      if((open_flags & (capture ? 2 : 1)))
      {
        if(input)
        {
          tli->_canDominateInputLatency = can_dominate_lat;
        }
        else
        {
          tli->_canDominateOutputLatency = can_dominate_lat;
          // If any of the branches can dominate, then this node cannot correct.
          tli->_canCorrectOutputLatency = can_correct_lat && !can_dominate_lat;
        }
      }

      if(input)
        tli->_canDominateInputProcessed = true;
      else
        tli->_canDominateProcessed = true;

      return *tli;
}

//---------------------------------------------------------
//   getDominanceLatencyInfoMidi
//---------------------------------------------------------

TrackLatencyInfo& MidiDevice::getDominanceLatencyInfoMidi(bool capture, bool input)
{
      TrackLatencyInfo* tli = capture ? &_captureLatencyInfo : &_playbackLatencyInfo;

      // Have we been here before during this scan?
      // Just return the cached value.
      if((input && tli->_dominanceInputProcessed) ||
        (!input && tli->_dominanceProcessed))
        return *tli;

      float route_worst_latency = 0.0f;

      const bool passthru = canPassThruLatencyMidi(capture);

      bool item_found = false;

      const int open_flags = openFlags();

      float worst_self_latency = 0.0f;
      if(!input && (open_flags & (capture ? 2 : 1)))
        worst_self_latency = getWorstSelfLatencyMidi(capture);
      
      const int port = midiPort();

      // Gather latency info from all connected input branches,
      //  but ONLY if the track is not off.
      // Currently there are no routes FROM tracks (audio or midi) TO midi capture devices,
      //  only TO midi playback devices.
      // CAUTION: See the warning in getDominanceInfoMidi about infinite recursion.
      if(!capture && (open_flags & (/*capture ? 2 :*/ 1)) && (passthru || input) &&
        port >= 0 && port < MusECore::MIDI_PORTS)
      {
//         bool used_chans[MusECore::MUSE_MIDI_CHANNELS];
//         for(int i = 0; i < MusECore::MUSE_MIDI_CHANNELS; ++i)
//           used_chans[i] = false;
//         bool all_chans = false;

#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
        const MidiTrackList& tl = *MusEGlobal::song->midis();
        const MidiTrackList::size_type tl_sz = tl.size();
        for(MidiTrackList::size_type it = 0; it < tl_sz; ++it)
        {
          MidiTrack* track = static_cast<MidiTrack*>(tl[it]);
          if(track->outPort() != port)
            continue;
          
          //if((open_flags & (/*capture ? 2 :*/ 1)) && !track->off() && (passthru || input))
          if(!track->off())
          {
            const TrackLatencyInfo& li = track->getDominanceLatencyInfo(false);

            // Whether the branch can dominate or correct latency or if we
            //  want to allow unterminated input branches to
            //  participate in worst branch latency calculations.
            const bool participate = 
              (li._canCorrectOutputLatency ||
              li._canDominateOutputLatency ||
              MusEGlobal::config.correctUnterminatedInBranchLatency);

            if(participate)
            {
              // Is it the first found item?
              if(item_found)
              {
                // If any one of the branches can dominate the latency,
                //  that overrides any which cannot.
                if(li._canDominateOutputLatency)
                {
                  // Override the current worst value if the latency is greater,
                  //  but ONLY if the branch can dominate.
                  //if(li._outputLatency > route_worst_latency)
                  //  route_worst_latency = li._outputLatency;
                }
                // Override the current worst value if the latency is greater,
                //  but ONLY if the branch can dominate.
                if(li._outputLatency > route_worst_latency)
                  route_worst_latency = li._outputLatency;
              }
              else
              {
                item_found = true;
                // Override the default worst value, but ONLY if the branch can dominate.
                //if(li._canDominateOutputLatency)
                  route_worst_latency = li._outputLatency;
              }
            }
          }
        }

#else

        MidiPort* mp = &MusEGlobal::midiPorts[port];
        RouteList* rl = mp->inRoutes();
        for (iRoute ir = rl->begin(); ir != rl->end(); ++ir)
        {
          switch(ir->type)
          {
              case Route::TRACK_ROUTE:
                if(!ir->track)
                  continue;
                
                if(ir->track->isMidiTrack())
                {
                  if(ir->channel < -1 || ir->channel >= MusECore::MUSE_MIDI_CHANNELS)
                    continue;

                  Track* track = ir->track;
//                     if(ir->channel < 0)
//                       all_chans = true;
//                     else
//                       used_chans[ir->channel] = true;
                    
                  //if((open_flags & (/*capture ? 2 :*/ 1)) && !track->off() && (passthru || input))
                  if(!track->off())
                  {
                    const TrackLatencyInfo& li = track->getDominanceLatencyInfo(false);

                    // Whether the branch can dominate or correct latency or if we
                    //  want to allow unterminated input branches to
                    //  participate in worst branch latency calculations.
                    const bool participate = 
                      (li._canCorrectOutputLatency ||
                      li._canDominateOutputLatency ||
                      MusEGlobal::config.correctUnterminatedInBranchLatency);

                    if(participate)
                    {
                      // Is it the first found item?
                      if(item_found)
                      {
                        // If any one of the branches can dominate the latency,
                        //  that overrides any which cannot.
                        if(li._canDominateOutputLatency)
                        {
                          // Override the current worst value if the latency is greater,
                          //  but ONLY if the branch can dominate.
                          //if(li._outputLatency > route_worst_latency)
                          //  route_worst_latency = li._outputLatency;
                        }
                        // Override the current worst value if the latency is greater,
                        //  but ONLY if the branch can dominate.
                        if(li._outputLatency > route_worst_latency)
                          route_worst_latency = li._outputLatency;
                      }
                      else
                      {
                        item_found = true;
                        // Override the default worst value, but ONLY if the branch can dominate.
                        //if(li._canDominateOutputLatency)
                          route_worst_latency = li._outputLatency;
                      }
                    }
                  }
                }
              break;

              default:
              break;
          }            
        }

#endif          

        // Special for the built-in metronome.
        //if(!capture)
        //{
          MusECore::MetronomeSettings* metro_settings = 
            MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

          //if(sendMetronome())
          if(metro_settings->midiClickFlag && metro_settings->clickPort == port)
          {
            //if((open_flags & (/*capture ? 2 :*/ 1)) && !MusECore::metronome->off() && (passthru || input))
            if(!MusECore::metronome->off())
            {
              const TrackLatencyInfo& li = MusECore::metronome->getDominanceLatencyInfoMidi(capture, false);

              // Whether the branch can dominate or correct latency or if we
              //  want to allow unterminated input branches to
              //  participate in worst branch latency calculations.
              const bool participate = 
                (li._canCorrectOutputLatency ||
                li._canDominateOutputLatency ||
                MusEGlobal::config.correctUnterminatedInBranchLatency);

              if(participate)
              {
                // Is it the first found item?
                if(item_found)
                {
                  // If any one of the branches can dominate the latency,
                  //  that overrides any which cannot.
                  if(li._canDominateOutputLatency)
                  {
                    // Override the current worst value if the latency is greater,
                    //  but ONLY if the branch can dominate.
                    //if(li._outputLatency > route_worst_latency)
                    //  route_worst_latency = li._outputLatency;
                  }
                  // Override the current worst value if the latency is greater,
                  //  but ONLY if the branch can dominate.
                  if(li._outputLatency > route_worst_latency)
                    route_worst_latency = li._outputLatency;
                }
                else
                {
                  item_found = true;
                  // Override the default worst value, but ONLY if the branch can dominate.
                  //if(li._canDominateOutputLatency)
                    route_worst_latency = li._outputLatency;
                }
              }
            }
          }
        //}
      }
      
      // Set the correction of all connected input branches,
      //  but ONLY if the track is not off.
      if((open_flags & (capture ? 2 : 1)))
      {
        if(input)
        {
          tli->_inputLatency = route_worst_latency;
        }
        else
        {
          if(passthru)
          {
            tli->_outputLatency = worst_self_latency + route_worst_latency;
            tli->_inputLatency = route_worst_latency;
          }
          else
          {
            tli->_outputLatency = worst_self_latency + tli->_sourceCorrectionValue;
          }
        }
      }

      if(input)
        tli->_dominanceInputProcessed = true;
      else
        tli->_dominanceProcessed = true;

      return *tli;
}

//---------------------------------------------------------
//   setCorrectionLatencyInfoMidi
//---------------------------------------------------------

TrackLatencyInfo& MidiDevice::setCorrectionLatencyInfoMidi(bool capture, bool input, float finalWorstLatency, float callerBranchLatency)
{
  TrackLatencyInfo* tli = capture ? &_captureLatencyInfo : &_playbackLatencyInfo;

  const bool passthru = canPassThruLatencyMidi(capture);

  const int open_flags = openFlags();

  float worst_self_latency = 0.0f;
  if(!input && (open_flags & 1 /*write*/))
    worst_self_latency = getWorstSelfLatencyMidi(capture);
      
  // The _trackLatency should already be calculated in the dominance scan.
  const float branch_lat = callerBranchLatency + worst_self_latency;

  const int port = midiPort();
  // Currently there are no routes FROM tracks (audio or midi) TO midi capture devices,
  //  only TO midi playback devices.
  // CAUTION: See the warning in getDominanceInfoMidi about infinite recursion.
  if(!capture && (open_flags & 1 /*write*/) && (passthru || input) &&
    port >= 0 && port < MusECore::MIDI_PORTS)
  {
    // Set the correction of all connected input branches.
    // The _trackLatency should already be calculated in the dominance scan.
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
    const MidiTrackList& tl = *MusEGlobal::song->midis();
    const MidiTrackList::size_type tl_sz = tl.size();
    for(MidiTrackList::size_type it = 0; it < tl_sz; ++it)
    {
      MidiTrack* track = static_cast<MidiTrack*>(tl[it]);
      if(track->outPort() != port)
        continue;
      //if((open_flags & 1 /*write*/) && !track->off() && (passthru || input))
      if(!track->off())
        track->setCorrectionLatencyInfo(false, finalWorstLatency, branch_lat);
    }

#else

    MidiPort* mp = &MusEGlobal::midiPorts[port];
    RouteList* mrl = mp->inRoutes();
    for (iRoute ir = mrl->begin(); ir != mrl->end(); ++ir)
    {
      switch(ir->type)
      {
          case Route::TRACK_ROUTE:
            if(!ir->track)
              continue;
            
            if(ir->track->isMidiTrack())
            {
              if(ir->channel < -1 || ir->channel >= MusECore::MUSE_MIDI_CHANNELS)
                continue;
              Track* track = ir->track;
              //if((open_flags & 1 /*write*/) && !track->off() && (passthru || input))
              if(!track->off())
                track->setCorrectionLatencyInfo(false, finalWorstLatency, branch_lat);
            }
          break;

          default:
          break;
      }            
    }

#endif
    
    // Special for the built-in metronome.
    //if(!capture)
    //{
      MusECore::MetronomeSettings* metro_settings = 
        MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

      //if(sendMetronome())
      if(metro_settings->midiClickFlag && metro_settings->clickPort == port)
      {
        //if((open_flags & 1 /*write*/) && !MusECore::metronome->off() && (passthru || input))
        if(!MusECore::metronome->off())
          MusECore::metronome->setCorrectionLatencyInfoMidi(capture, false, finalWorstLatency, branch_lat);
      }
    //}
  }

  // Set the correction of all connected input branches,
  //  but ONLY if the track is not off.
  if(open_flags & 1 /*write*/ && !capture/*Tim*/)
  {
    if(input)
    {
    }
    else
    {
      if(canCorrectOutputLatencyMidi() && tli->_canCorrectOutputLatency)
      {
        float corr = 0.0f;
        if(MusEGlobal::config.commonProjectLatency)
          corr -= finalWorstLatency;

        corr -= branch_lat;
        // The _sourceCorrectionValue is initialized to zero.
        // Whichever calling branch needs the most correction gets it.
        if(corr < tli->_sourceCorrectionValue)
          tli->_sourceCorrectionValue = corr;
      }
    }
  }

  return *tli;
}

//---------------------------------------------------------
//   getLatencyInfoMidi
//---------------------------------------------------------

TrackLatencyInfo& MidiDevice::getLatencyInfoMidi(bool capture, bool input)
{
  TrackLatencyInfo* tli = capture ? &_captureLatencyInfo : &_playbackLatencyInfo;

  // Have we been here before during this scan?
  // Just return the cached value.
  if((input && tli->_inputProcessed) ||
    (!input && tli->_processed))
    return *tli;

  MusECore::MetronomeSettings* metro_settings = 
    MusEGlobal::metroUseSongSettings ? &MusEGlobal::metroSongSettings : &MusEGlobal::metroGlobalSettings;

  float route_worst_latency = tli->_inputLatency;

  const bool passthru = canPassThruLatencyMidi(capture);

  const int port = midiPort();
  const int open_flags = openFlags();

  if(passthru || input)
  {
    // Currently there are no routes FROM tracks (audio or midi) TO midi capture devices,
    //  only TO midi playback devices.
    // CAUTION: See the warning in getDominanceInfoMidi about infinite recursion.
    if(!capture && port >= 0 && port < MusECore::MIDI_PORTS)
    {
  #ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
      const MidiTrackList& tl = *MusEGlobal::song->midis();
      const MidiTrackList::size_type tl_sz = tl.size();
      for(MidiTrackList::size_type it = 0; it < tl_sz; ++it)
      {
        MidiTrack* track = static_cast<MidiTrack*>(tl[it]);
        if(track->outPort() != port)
          continue;

        // TODO: FIXME: Where to store? We have no route to store it in.
        // Default to zero.
        //ir->audioLatencyOut = 0.0f;

        if((open_flags & (/*capture ? 2 :*/ 1)) && !track->off())
        {
          TrackLatencyInfo& li = track->getLatencyInfo(false);

          const bool participate =
            (li._canCorrectOutputLatency ||
            li._canDominateOutputLatency ||
            MusEGlobal::config.correctUnterminatedInBranchLatency);

          if(participate)
          {
            // TODO: FIXME: Where to store? We have no route to store it in.
            // Prepare the latency value to be passed to the compensator's writer,
            //  by adjusting each route latency value. ie. the route with the worst-case
            //  latency will get ZERO delay, while routes having smaller latency will get
            //  MORE delay, to match all the signal timings together.
            // The route's audioLatencyOut should have already been calculated and
            //  conveniently stored in the route.
//             ir->audioLatencyOut = route_worst_latency - li._outputLatency;
//             // Should not happen, but just in case.
//             if((long int)ir->audioLatencyOut < 0)
//               ir->audioLatencyOut = 0.0f;

            // Special for Midi Tracks: We don't have Midi Track to Midi Port routes yet
            //  because we don't have multiple Midi Track outputs yet, only a single output port.
            // So we must store this information here just for Midi Tracks.
            li._latencyOutMidiTrack = route_worst_latency - li._outputLatency;
            // Should not happen, but just in case.
            if((long int)li._latencyOutMidiTrack < 0)
              li._latencyOutMidiTrack = 0.0f;
          }
        }
      }

  #else

      MidiPort* mp = &MusEGlobal::midiPorts[port];
      RouteList* rl = mp->inRoutes();
      for (iRoute ir = rl->begin(); ir != rl->end(); ++ir)
      {
            switch(ir->type)
            {
                case Route::TRACK_ROUTE:
                  if(!ir->track)
                    continue;

                  if(ir->track->isMidiTrack())
                  {
                    if(ir->channel < -1 || ir->channel >= MusECore::MUSE_MIDI_CHANNELS)
                      continue;

                    Track* track = ir->track;

                    // Default to zero.
                    ir->audioLatencyOut = 0.0f;

                    if((open_flags & (/*capture ? 2 :*/ 1)) && !track->off())
                    {
                      const TrackLatencyInfo& li = track->getLatencyInfo(false);
                      const bool participate =
                        (li._canCorrectOutputLatency ||
                        li._canDominateOutputLatency ||
                        MusEGlobal::config.correctUnterminatedInBranchLatency);

                      if(participate)
                      {
                        // Prepare the latency value to be passed to the compensator's writer,
                        //  by adjusting each route latency value. ie. the route with the worst-case
                        //  latency will get ZERO delay, while routes having smaller latency will get
                        //  MORE delay, to match all the signal timings together.
                        // The route's audioLatencyOut should have already been calculated and
                        //  conveniently stored in the route.
                        ir->audioLatencyOut = route_worst_latency - li._outputLatency;
                        // Should not happen, but just in case.
                        if((long int)ir->audioLatencyOut < 0)
                          ir->audioLatencyOut = 0.0f;
                      }
                    }
                  }
                break;

                default:
                break;
            }          
      }

  #endif

      // Special for the built-in metronome.
      //if(!capture)
      //{
        // TODO: FIXME: Where to store? We have no route to store it in.
        // Default to zero.
        //ir->audioLatencyOut = 0.0f;

        if((open_flags & (/*capture ? 2 :*/ 1)) && !MusECore::metronome->off() && // sendMetronome() &&
          metro_settings->midiClickFlag && metro_settings->clickPort == port)
        {
          TrackLatencyInfo& li = MusECore::metronome->getLatencyInfoMidi(capture, false);
          const bool participate =
            (li._canCorrectOutputLatency ||
            li._canDominateOutputLatency ||
            MusEGlobal::config.correctUnterminatedInBranchLatency);

          if(participate)
          {
            // TODO: FIXME: Where to store? We have no route to store it in.
            // Prepare the latency value to be passed to the compensator's writer,
            //  by adjusting each route latency value. ie. the route with the worst-case
            //  latency will get ZERO delay, while routes having smaller latency will get
            //  MORE delay, to match all the signal timings together.
            // The route's audioLatencyOut should have already been calculated and
            //  conveniently stored in the route.

//             ir->audioLatencyOut = route_worst_latency - ir->audioLatencyOut;
//             // Should not happen, but just in case.
//             if((long int)ir->audioLatencyOut < 0)
//               ir->audioLatencyOut = 0.0f;

            // Special for Midi Tracks: We don't have Midi Track to Midi Port routes yet
            //  because we don't have multiple Midi Track outputs yet, only a single output port.
            // So we must store this information here just for Midi Tracks.
//             li._latencyOutMidiTrack = route_worst_latency - li._outputLatency;
//             // Should not happen, but just in case.
//             if((long int)li._latencyOutMidiTrack < 0)
//               li._latencyOutMidiTrack = 0.0f;

            // Special for Midi Tracks: We don't have Midi Track to Midi Port routes yet
            //  because we don't have multiple Midi Track outputs yet, only a single output port.
            // So we must store this information here just for Midi Tracks.
            li._latencyOutMetronome = route_worst_latency - li._latencyOutMetronome;
            // Should not happen, but just in case.
            if((long int)li._latencyOutMetronome < 0)
              li._latencyOutMetronome = 0.0f;
          }
        }
      //}
    }
  }

  if(input)
    tli->_inputProcessed = true;
  else
    tli->_processed = true;

  return *tli;
}

//---------------------------------------------------------
//   latencyCompWriteOffset
//---------------------------------------------------------

inline unsigned long MidiDevice::latencyCompWriteOffsetMidi(bool capture) const
{
  return capture ? _captureLatencyInfo._compensatorWriteOffset : _playbackLatencyInfo._compensatorWriteOffset;
}

void MidiDevice::setLatencyCompWriteOffsetMidi(float worstCase, bool capture)
{
  TrackLatencyInfo* tli = capture ? &_captureLatencyInfo : &_playbackLatencyInfo;
  
  // If independent branches are NOT to affect project latency,
  //  then there should be no need for any extra delay in the branch.
  if(!MusEGlobal::config.commonProjectLatency)
  {
    tli->_compensatorWriteOffset = 0;
    //fprintf(stderr, "MidiDevice::setLatencyCompWriteOffset() name:%s capture:%d worstCase:%f _outputLatency:%f _compensatorWriteOffset:%lu\n",
    //        name().toLatin1().constData(), capture, worstCase, tli->_outputLatency, tli->_compensatorWriteOffset);
    return;
  }
    
  if(tli->_canDominateOutputLatency)
  {
    const long unsigned int wc = worstCase;
    const long unsigned int ol = tli->_outputLatency;
    if(ol > wc)
      tli->_compensatorWriteOffset = 0;
    else
      tli->_compensatorWriteOffset = wc - ol;
  }
  else
  {
//     if(tli->_outputLatency < 0)
      tli->_compensatorWriteOffset = 0;
//     else
//       tli->_compensatorWriteOffset = tli->_outputLatency;
  }

  //fprintf(stderr,
  //  "MidiDevice::setLatencyCompWriteOffset() name:%s capture:%d worstCase:%f"
  //  " _outputLatency:%f _canDominateOutputLatency:%d _compensatorWriteOffset:%lu\n",
  //     name().toLatin1().constData(), capture, worstCase, tli->_outputLatency,
  //     tli->_canDominateOutputLatency, tli->_compensatorWriteOffset);
}

//================================================
// END Latency correction/compensation routines.
//================================================


} // namespace MusECore

