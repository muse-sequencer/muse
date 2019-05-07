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
#include "mitplugin.h"
#include "part.h"
#include "drummap.h"
#include "operations.h"
#include "helper.h"

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
          QMessageBox::critical(NULL, "MusE fatal error.", "MusE failed to initialize the\n" 
                                                          "Alsa midi subsystem, check\n"
                                                          "your configuration.");
          exit(-1);
          }
      }
#endif
      
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
//   addOperation
//---------------------------------------------------------

void MidiDeviceList::addOperation(MidiDevice* dev, PendingOperationList& ops)
{
  bool gotUniqueName=false;
  int increment = 0;
  const QString origname = dev->name();
  QString newName = origname;
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
      newName = origname + QString("_%1").arg(++increment);
      gotUniqueName = false;
    }    
    
    for(iMidiDevice i = begin(); i != end(); ++i) 
    {
      const QString s = (*i)->name();
      if(s == newName)
      {
        newName = origname + QString("_%1").arg(++increment);
        gotUniqueName = false;
      }
    }
  }
  
  if(origname != newName)
    dev->setName(newName);
  
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
    const bool extsync = MusEGlobal::extSyncFlag.value();
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

// REMOVE Tim. latency. Added.
//================================================
// BEGIN Latency correction/compensation routines.
//================================================

void MidiDevice::prepareLatencyScan() { 
  // Reset some latency info to prepare for (re)computation.
  // TODO: Instead of doing this blindly every cycle, do it only when
  //        a latency controller changes or a connection is made etc,
  //        ie only when something changes.
  _captureLatencyInfo.initialize();
  _playbackLatencyInfo.initialize();
}

bool MidiDevice::isLatencyInputTerminalMidi(bool capture)
{
  TrackLatencyInfo* tli = capture ? &_captureLatencyInfo : &_playbackLatencyInfo;

  const int port = midiPort();

  // Playback devices are considered a termination point.
  if(!capture || port < 0 || port >= MusECore::MIDI_PORTS)
  {
    tli->_isLatencyInputTerminal = true;
    return true;
  }

  MidiPort* mp = &MusEGlobal::midiPorts[port];

//   bool res = true;
  
  // REMOVE Tim. latency. Added. FLAG latency rec.
  // TODO Refine this with a case or something, specific for say Aux tracks, Group tracks etc.
//   if(!off() && ((!canRecordMonitor() || (canRecordMonitor() && isRecMonitored()))
//      //|| (canRecord() && recordFlag())
//      ))

  // If we're asking for the view from the record side, check if we're
  //  passing the signal through the track via monitoring.
//   if(off() || (canRecordMonitor() && (!MusEGlobal::config.monitoringAffectsLatency || !isRecMonitored())))
//      //&& canRecord() && !recordFlag())
    
    
//   if(!(openFlags() & 2 /*read*/))
//   {
// //     _latencyInfo._isLatencyInputTerminal = true;
//     tli->_isLatencyInputTerminal = true;
//     return true;
//   }
  
//     const RouteList* rl = outRoutes();
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
            
//             _latencyInfo._isLatencyInputTerminal = false;
            tli->_isLatencyInputTerminal = false;
            return false;
          }
        break;

        default:
        break;
      }
    }
//   }

//   _latencyInfo._isLatencyInputTerminal = res;
//   return res;
  tli->_isLatencyInputTerminal = true;
  return true;
}

bool MidiDevice::isLatencyOutputTerminalMidi(bool capture)
{
  TrackLatencyInfo* tli = capture ? &_captureLatencyInfo : &_playbackLatencyInfo;

  const int port = midiPort();

  // Playback devices are considered a termination point.
  if(!capture || port < 0 || port >= MusECore::MIDI_PORTS)
  {
    tli->_isLatencyInputTerminal = true;
    return true;
  }

  MidiPort* mp = &MusEGlobal::midiPorts[port];

//   bool res = true;
  
  // REMOVE Tim. latency. Added. FLAG latency rec.
  // TODO Refine this with a case or something, specific for say Aux tracks, Group tracks etc.
//   if(!off() && ((!canRecordMonitor() || (canRecordMonitor() && isRecMonitored()))
//      //|| (canRecord() && recordFlag())
//      ))

//   // If we're asking for the view from the record side, check if we're
//   //  passing the signal through the track via monitoring.
//   if(off() || (record && canRecordMonitor() && !isRecMonitored()))
//      //&& canRecord() && !recordFlag())
//   {
//     _latencyInfo._isLatencyOuputTerminal = true;
//     return true;
//   }
  
//   if(!off() && 
//     (!record || ((!canRecordMonitor() || (canRecordMonitor() && isRecMonitored()))
//      //|| (canRecord() && recordFlag())
//      )))
//   {

//     const RouteList* rl = outRoutes();
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
            
//             _latencyInfo._isLatencyInputTerminal = false;
            tli->_isLatencyInputTerminal = false;
            return false;
          }
        break;

        default:
        break;
      }
    }
//   }

//   _latencyInfo._isLatencyOutputTerminal = res;
//   return res;
//   _latencyInfo._isLatencyOutputTerminal = true;
  tli->_isLatencyOutputTerminal = true;
  return true;
}

bool MidiDevice::canDominateOutputLatencyMidi(bool capture) const
{
  if(capture)
    return true;
  return false;
}

bool MidiDevice::canDominateEndPointLatencyMidi(bool capture) const
{
  if(capture)
    return false;
  return true;
}

//---------------------------------------------------------
//   getInputDominanceLatencyInfo
//---------------------------------------------------------

TrackLatencyInfo& MidiDevice::getInputDominanceLatencyInfoMidi(bool capture)
{
      TrackLatencyInfo* tli = capture ? &_captureLatencyInfo : &_playbackLatencyInfo;
      
      // Have we been here before during this scan?
      // Just return the cached value.
    //       if(_latencyInfo._dominanceProcessed)
    //         return _latencyInfo;
      if(tli->_dominanceProcessed)
        return *tli;

      const int port = midiPort();

// //       // Playback devices are considered a termination point.
//       if(/*!capture ||*/ port < 0 || port >= MusECore::MIDI_PORTS)
//       {
//         tli->_dominanceProcessed = true;
//         return *tli;
//       }

//       MidiPort* mp = &MusEGlobal::midiPorts[port];

      
      
      
      
      
//       RouteList* rl = inRoutes();
      //const RouteList* rl = inRoutes();
      float route_worst_latency = 0.0f;
      float track_worst_chan_latency = 0.0f;
      
      // This value has a range from 0 (worst) to positive inf (best) or close to it.
//       float route_worst_out_corr = outputLatencyCorrection();
      // Get the default domination for this track type.
      bool can_dominate_out_lat = canDominateOutputLatencyMidi(capture);
      // Get the default correction ability for this track type.
      //bool can_correct_out_lat = canCorrectOutputLatency();

      // Gather latency info from all connected input branches,
      //  but ONLY if the track is not off.
//       if(!off())
      if((openFlags() & (capture ? 2 : 1)) && port >= 0 && port < MusECore::MIDI_PORTS)
      {
//         bool used_chans[MUSE_MIDI_CHANNELS];
//         for(int i = 0; i < MUSE_MIDI_CHANNELS; ++i)
//           used_chans[i] = false;
//         bool all_chans = false;

        bool item_found = false;
        // Only if monitoring is not available, or it is and in fact is monitored.
//         if(!canRecordMonitor() || (canRecordMonitor() && isRecMonitored()))
        // TODO Refine this with a case or something, specific for say Aux tracks, Group tracks etc.
        // REMOVE Tim. latency. Added. FLAG latency rec.
//         if((!canRecordMonitor() || (canRecordMonitor() && isRecMonitored()))
//             //|| (canRecord() && recordFlag())
//           )
//         const bool passthru =
//           !canRecordMonitor() || 
//           (MusEGlobal::config.monitoringAffectsLatency && isRecMonitored());
//           //|| (canRecord() && recordFlag());
        
        
        {
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
          const ciTrack tl_end = MusEGlobal::song->tracks()->cend();
          for(ciTrack it = MusEGlobal::song->tracks()->begin(); it != tl_end; ++it)
          {
            if(!(*it)->isMidiTrack())
              continue;
            MidiTrack* track = static_cast<MidiTrack*>(*it);
            if(track->outPort() != port)
              continue;

            const TrackLatencyInfo& li = track->getInputDominanceLatencyInfo();
            const bool passthru =
              !track->canRecordMonitor() || 
              (MusEGlobal::config.monitoringAffectsLatency && track->isRecMonitored());

            // TODO: FIXME: Where to store? We have no route to store it in.
            // Temporarily store these values conveniently in the actual route.
            // They will be used by the latency compensator in the audio process pass.
            //ir->canDominateLatency = li._canDominateOutputLatency;
            //ir->canCorrectOutputLatency = li._canCorrectOutputLatency;

            if(passthru)
            {
              // Is it the first found item?
              if(item_found)
              {
                // If any one of the branches can dominate the latency,
                //  that overrides any which cannot.
                if(li._canDominateOutputLatency)
                {
                  can_dominate_out_lat = true;
                  // Override the current worst value if the latency is greater,
                  //  but ONLY if the branch can dominate.
                  if(li._outputLatency > route_worst_latency)
                    route_worst_latency = li._outputLatency;
                }
              }
              else
              {
                item_found = true;
                // Override the defaults with this first item's values.
                can_dominate_out_lat = li._canDominateOutputLatency;
                // Override the default worst value, but ONLY if the branch can dominate.
                if(can_dominate_out_lat)
                  route_worst_latency = li._outputLatency;
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
                    if(ir->channel < -1 || ir->channel >= MUSE_MIDI_CHANNELS)
                      continue;

                    Track* track = ir->track;
//                     if(track->off()) // || 
//                       //(atrack->canRecordMonitor() && (MusEGlobal::config.monitoringAffectsLatency || !atrack->isRecMonitored())))
//                       //&& atrack->canRecord() && !atrack->recordFlag()))
//                       continue;
                   
//                     if(ir->channel < 0)
//                       all_chans = true;
//                     else
//                       used_chans[ir->channel] = true;
                      
                    const TrackLatencyInfo& li = track->getInputDominanceLatencyInfo();
                    const bool passthru =
                      !track->canRecordMonitor() || 
                      (MusEGlobal::config.monitoringAffectsLatency && track->isRecMonitored());

                    // Temporarily store these values conveniently in the actual route.
                    // They will be used by the latency compensator in the audio process pass.
                    ir->canDominateLatency = li._canDominateOutputLatency;
                    ir->canCorrectOutputLatency = li._canCorrectOutputLatency;

                    if(passthru)
                    {
                      // Is it the first found item?
                      if(item_found)
                      {
                        // If any one of the branches can dominate the latency,
                        //  that overrides any which cannot.
                        if(li._canDominateOutputLatency)
                        {
                          can_dominate_out_lat = true;
                          // Override the current worst value if the latency is greater,
                          //  but ONLY if the branch can dominate.
                          if(li._outputLatency > route_worst_latency)
                            route_worst_latency = li._outputLatency;
                        }
                      }
                      else
                      {
                        item_found = true;
                        // Override the defaults with this first item's values.
                        can_dominate_out_lat = li._canDominateOutputLatency;
                        // Override the default worst value, but ONLY if the branch can dominate.
                        if(can_dominate_out_lat)
                          route_worst_latency = li._outputLatency;
                      }
                    }
                  }
                break;

                default:
                break;
            }            
          }

#endif          

        }
        
        // Adjust for THIS device's contribution to latency.
        // The goal is to have equal latency output on all channels on this track.
//         for(int i = 0; i < track_out_channels; ++i)
        //for(int i = 0; i < MUSE_MIDI_CHANNELS; ++i)
        {
//           if(!used_chans[i])
//             continue;
//           const float lat = trackLatency(i);
          // TODO Revert. TESTING
          //const float lat = selfLatency(i, capture);
          const float lat = selfLatencyMidi(0, capture);
          if(lat > track_worst_chan_latency)
              track_worst_chan_latency = lat;
        }
      }
      
      // The absolute latency of signals leaving this track is the sum of
      //  any connected route latencies and this track's latency.
      tli->_trackLatency  = track_worst_chan_latency;
      tli->_outputLatency = track_worst_chan_latency + route_worst_latency;
      //tli->_outputAvailableCorrection = route_worst_out_corr;
      tli->_canDominateOutputLatency = can_dominate_out_lat;
      //tli->_canCorrectOutputLatency = can_correct_out_lat;
      tli->_canCorrectOutputLatency = canCorrectOutputLatencyMidi();
      // Take advantage of this first stage to initialize the track's
      //  correction value to zero.
      tli->_sourceCorrectionValue = 0.0f;
      // Take advantage of this first stage to initialize the track's
      //  write offset to zero.
      tli->_compensatorWriteOffset = 0;
      // Set whether this track is a branch end point.
      //_latencyInfo._isLatencyOuputTerminal = isLatencyOutputTerminal();

      tli->_dominanceProcessed = true;
      return *tli;
}

// REMOVE Tim. latency. Added.
//---------------------------------------------------------
//   getDominanceLatencyInfo
//---------------------------------------------------------

TrackLatencyInfo& MidiDevice::getDominanceLatencyInfoMidi(bool capture)
{
  TrackLatencyInfo* tli = capture ? &_captureLatencyInfo : &_playbackLatencyInfo;
  
      // Have we been here before during this scan?
      // Just return the cached value.
//       if(_latencyInfo._dominanceProcessed)
//         return _latencyInfo;
      if(tli->_dominanceProcessed)
        return *tli;
      
      const int port = midiPort();
      
//       RouteList* rl = inRoutes();
      //const RouteList* rl = inRoutes();
      float route_worst_latency = 0.0f;
      float track_worst_chan_latency = 0.0f;
      
      // This value has a range from 0 (worst) to positive inf (best) or close to it.
//       float route_worst_out_corr = outputLatencyCorrection();
      // Get the default domination for this track type.
//       bool can_dominate_out_lat = canDominateOutputLatency();
      bool can_dominate_out_lat = canDominateOutputLatencyMidi(capture);
      // Get the default correction ability for this track type.
      //bool can_correct_out_lat = canCorrectOutputLatency();

      // Gather latency info from all connected input branches,
      //  but ONLY if the track is not off.
//       if(!off())
      if((openFlags() & (capture ? 2 : 1)) && port >= 0 && port < MusECore::MIDI_PORTS)
      {
//         bool used_chans[MUSE_MIDI_CHANNELS];
//         for(int i = 0; i < MUSE_MIDI_CHANNELS; ++i)
//           used_chans[i] = false;
//         bool all_chans = false;

        bool item_found = false;
        // Only if monitoring is not available, or it is and in fact is monitored.
//         if(!canRecordMonitor() || (canRecordMonitor() && isRecMonitored()))
        // TODO Refine this with a case or something, specific for say Aux tracks, Group tracks etc.
        // REMOVE Tim. latency. Added. FLAG latency rec.
//         if((!canRecordMonitor() || (canRecordMonitor() && isRecMonitored()))
//             //|| (canRecord() && recordFlag())
//           )

//         const bool passthru =
//           !canRecordMonitor() || 
//           (MusEGlobal::config.monitoringAffectsLatency && isRecMonitored());
//           //|| (canRecord() && recordFlag());
        
        
        {
          
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
          const ciTrack tl_end = MusEGlobal::song->tracks()->cend();
          for(ciTrack it = MusEGlobal::song->tracks()->begin(); it != tl_end; ++it)
          {
            if(!(*it)->isMidiTrack())
              continue;
            MidiTrack* track = static_cast<MidiTrack*>(*it);
            if(track->outPort() != port)
              continue;
            const TrackLatencyInfo& li = track->getDominanceLatencyInfo();

            // TODO: FIXME: Where to store? We have no route to store it in.
            // Temporarily store these values conveniently in the actual route.
            // They will be used by the latency compensator in the audio process pass.
            //ir->canDominateLatency = li._canDominateOutputLatency;
            //ir->canCorrectOutputLatency = li._canCorrectOutputLatency;

//             if(passthru)
            {
              // Is it the first found item?
              if(item_found)
              {
                // If any one of the branches can dominate the latency,
                //  that overrides any which cannot.
                if(li._canDominateOutputLatency)
                {
                  can_dominate_out_lat = true;
                  // Override the current worst value if the latency is greater,
                  //  but ONLY if the branch can dominate.
                  if(li._outputLatency > route_worst_latency)
                    route_worst_latency = li._outputLatency;
                }
              }
              else
              {
                item_found = true;
                // Override the defaults with this first item's values.
                can_dominate_out_lat = li._canDominateOutputLatency;
                // Override the default worst value, but ONLY if the branch can dominate.
                if(can_dominate_out_lat)
                  route_worst_latency = li._outputLatency;
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
                    if(ir->channel < -1 || ir->channel >= MUSE_MIDI_CHANNELS)
                      continue;

                    Track* track = ir->track;
//                     if(track->off()) // || 
//                       //(atrack->canRecordMonitor() && (MusEGlobal::config.monitoringAffectsLatency || !atrack->isRecMonitored())))
//                       //&& atrack->canRecord() && !atrack->recordFlag()))
//                       continue;
                   
//                     if(ir->channel < 0)
//                       all_chans = true;
//                     else
//                       used_chans[ir->channel] = true;
                      
                    const TrackLatencyInfo& li = track->getDominanceLatencyInfo();

                    // Temporarily store these values conveniently in the actual route.
                    // They will be used by the latency compensator in the audio process pass.
                    ir->canDominateLatency = li._canDominateOutputLatency;
                    ir->canCorrectOutputLatency = li._canCorrectOutputLatency;
                    
                    
//                     if(passthru)
                    {
                      // Is it the first found item?
                      if(item_found)
                      {
                        // If any one of the branches can dominate the latency,
                        //  that overrides any which cannot.
                        if(li._canDominateOutputLatency)
                        {
                          can_dominate_out_lat = true;
                          // Override the current worst value if the latency is greater,
                          //  but ONLY if the branch can dominate.
                          if(li._outputLatency > route_worst_latency)
                            route_worst_latency = li._outputLatency;
                        }
                      }
                      else
                      {
                        item_found = true;
                        // Override the defaults with this first item's values.
                        can_dominate_out_lat = li._canDominateOutputLatency;
                        // Override the default worst value, but ONLY if the branch can dominate.
                        if(can_dominate_out_lat)
                          route_worst_latency = li._outputLatency;
                      }
                    }
                  }
                break;

                default:
                break;
            }            
          }

#endif          

        }
        
        // Adjust for THIS track's contribution to latency.
        // The goal is to have equal latency output on all channels on this track.
//         for(int i = 0; i < track_out_channels; ++i)
        //for(int i = 0; i < MUSE_MIDI_CHANNELS; ++i)
        {
//           if(!used_chans[i])
//             continue;
//           const float lat = trackLatency(i);
          // TODO Revert. TESTING
          //const float lat = selfLatency(i, capture);
          const float lat = selfLatencyMidi(0, capture);
          if(lat > track_worst_chan_latency)
              track_worst_chan_latency = lat;
        }
      }
      
      // The absolute latency of signals leaving this track is the sum of
      //  any connected route latencies and this track's latency.
      tli->_trackLatency  = track_worst_chan_latency;
      tli->_outputLatency = track_worst_chan_latency + route_worst_latency;
      //tli->_outputAvailableCorrection = route_worst_out_corr;
      tli->_canDominateOutputLatency = can_dominate_out_lat;
      //tli->_canCorrectOutputLatency = can_correct_out_lat;
      tli->_canCorrectOutputLatency = canCorrectOutputLatencyMidi();
      // Take advantage of this first stage to initialize the track's
      //  correction value to zero.
      tli->_sourceCorrectionValue = 0.0f;
      // Take advantage of this first stage to initialize the track's
      //  write offset to zero.
      tli->_compensatorWriteOffset = 0;
      // Set whether this track is a branch end point.
      //tli->_isLatencyOuputTerminal = isLatencyOutputTerminal();

      tli->_dominanceProcessed = true;
      return *tli;
}

//---------------------------------------------------------
//   getInputLatencyInfo
//---------------------------------------------------------

TrackLatencyInfo& MidiDevice::getInputLatencyInfoMidi(bool capture)
{
      TrackLatencyInfo* tli = capture ? &_captureLatencyInfo : &_playbackLatencyInfo;
      
      // Have we been here before during this scan?
      // Just return the cached value.
//       if(_latencyInfo._processed)
//         return _latencyInfo;
      if(tli->_processed)
        return *tli;
      
      const int port = midiPort();

//       RouteList* rl = inRoutes();
      //const RouteList* rl = inRoutes();
      float route_worst_latency = 0.0f;
      //float track_worst_chan_latency = 0.0f;
      
      // This value has a range from 0 (worst) to positive inf (best) or close to it.
      //float route_worst_out_corr = outputLatencyCorrection();
      // Get the default domination for this track type.
//       bool can_dominate_out_lat = canDominateOutputLatency();
      
      // Gather latency info from all connected input branches,
      //  but ONLY if the track is not off.
//       if(!off())
      if((openFlags() & (capture ? 2 : 1)) && port >= 0 && port < MusECore::MIDI_PORTS)
      {
        bool item_found = false;

#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
        const ciTrack tl_end = MusEGlobal::song->tracks()->cend();
        for(ciTrack it = MusEGlobal::song->tracks()->begin(); it != tl_end; ++it)
        {
          if(!(*it)->isMidiTrack())
            continue;
          MidiTrack* track = static_cast<MidiTrack*>(*it);
          if(track->outPort() != port)
            continue;
          const TrackLatencyInfo& li = track->getInputLatencyInfo();

          // TODO: FIXME: Where to store? We have no route to store it in.
          // Temporarily store these values conveniently in the actual route.
          // They will be used by the latency compensator in the audio process pass.
          //ir->audioLatencyOut = li._outputLatency;

          // Is it the first found item?
          if(item_found)
          {
              // Override the current worst value if the latency is greater,
              //  but ONLY if the branch can dominate.
              //if(passthru && li._outputLatency > route_worst_latency)
              if(li._outputLatency > route_worst_latency)
                route_worst_latency = li._outputLatency;

              //if(ir->audioLatencyOut > route_worst_latency)
              ////if(passthru && li._outputLatency > route_worst_latency)
                //route_worst_latency = ir->audioLatencyOut;
          }
          else
          {
            item_found = true;
            // Override the defaults with this first item's values.
            route_worst_latency = li._outputLatency;

            //route_worst_latency = ir->audioLatencyOut;
          }
        }

#else

        MidiPort* mp = &MusEGlobal::midiPorts[port];
        RouteList* rl = mp->inRoutes();
        for (iRoute ir = rl->begin(); ir != rl->end(); ++ir) {
              switch(ir->type)
              {
                  case Route::TRACK_ROUTE:
                    if(!ir->track)
                      continue;
                    
                    if(ir->track->isMidiTrack())
                    {
                      if(ir->channel < -1 || ir->channel >= MUSE_MIDI_CHANNELS)
                        continue;

                      Track* track = ir->track;
  //                     if(track->off()) // || 
  //                       //(atrack->canRecordMonitor() && (MusEGlobal::config.monitoringAffectsLatency || !atrack->isRecMonitored())))
  //                       //&& atrack->canRecord() && !atrack->recordFlag()))
  //                       continue;
                    
  //                     if(ir->channel < 0)
  //                       all_chans = true;
  //                     else
  //                       used_chans[ir->channel] = true;
                        
//                       const TrackLatencyInfo& li = track->getInputDominanceLatencyInfo();
                      const TrackLatencyInfo& li = track->getInputLatencyInfo();
                      // Temporarily store these values conveniently in the actual route.
                      // They will be used by the latency compensator in the audio process pass.
        //               ir->canDominateLatency = li._canDominateOutputLatency;
                      //ir->canCorrectOutputLatency = li._canCorrectOutputLatency;
                      ir->audioLatencyOut = li._outputLatency;

                      // Is it the first found item?
                      if(item_found)
                      {
                          // Override the current worst value if the latency is greater,
                          //  but ONLY if the branch can dominate.
                          if(ir->audioLatencyOut > route_worst_latency)
                          //if(passthru && li._outputLatency > route_worst_latency)
                            route_worst_latency = ir->audioLatencyOut;
                      }
                      else
                      {
                        item_found = true;
                        // Override the defaults with this first item's values.
                        route_worst_latency = ir->audioLatencyOut;
                      }
                    }
                  break;
                  
                  default:
                  break;
              }
          
          
          
//               if(ir->type != Route::TRACK_ROUTE || !ir->track || ir->track->isMidiTrack())
//                 continue;
//               AudioTrack* atrack = static_cast<AudioTrack*>(ir->track);
//   //             const int atrack_out_channels = atrack->totalOutChannels();
//   //             const int src_ch = ir->remoteChannel <= -1 ? 0 : ir->remoteChannel;
//   //             const int src_chs = ir->channels;
//   //             int fin_src_chs = src_chs;
//   //             if(src_ch + fin_src_chs >  atrack_out_channels)
//   //               fin_src_chs = atrack_out_channels - src_ch;
//   //             const int next_src_chan = src_ch + fin_src_chs;
//   //             // The goal is to have equal latency output on all channels on this track.
//   //             for(int i = src_ch; i < next_src_chan; ++i)
//   //             {
//   //               const float lat = atrack->trackLatency(i);
//   //               if(lat > worst_case_latency)
//   //                 worst_case_latency = lat;
//   //             }
//               const TrackLatencyInfo& li = atrack->getInputLatencyInfo();
//               
//               // Temporarily store these values conveniently in the actual route.
//               // They will be used by the latency compensator in the audio process pass.
// //               ir->canDominateLatency = li._canDominateOutputLatency;
//               //ir->canCorrectOutputLatency = li._canCorrectOutputLatency;
//               ir->audioLatencyOut = li._outputLatency;
//               
//   //             // Override the current worst value if the latency is greater,
//   //             //  but ONLY if the branch can dominate.
//   //             if(li._canDominateOutputLatency && li._outputLatency > route_worst_latency)
//   //               route_worst_latency = li._outputLatency;
//   //             // Override the current worst value if the latency is greater.
//   //             if(li._outputLatency > route_worst_latency)
//   //               route_worst_latency = li._outputLatency;
//               
//               // Is it the first found item?
//               if(item_found)
//               {
//                 // Override the current values with this item's values ONLY if required.
//                 
//                 //if(li._outputAvailableCorrection < route_worst_out_corr)
//                 //  route_worst_out_corr = li._outputAvailableCorrection;
//                 
//                 // If any one of the branches can dominate the latency,
//                 //  that overrides any which cannot.
// //                 if(li._canDominateOutputLatency)
// //                 {
// //                   can_dominate_out_lat = true;
// 
// //                   // Override the current worst value if the latency is greater,
// //                   //  but ONLY if the branch can dominate.
// //                   if(li._outputLatency > route_worst_latency)
// //                     route_worst_latency = li._outputLatency;
// // //                 }
// 
//                   // Override the current worst value if the latency is greater,
//                   //  but ONLY if the branch can dominate.
//                   if(ir->audioLatencyOut > route_worst_latency)
//                   //if(passthru && li._outputLatency > route_worst_latency)
//                     route_worst_latency = ir->audioLatencyOut;
//               }
//               else
//               {
//                 item_found = true;
//                 // Override the defaults with this first item's values.
//                 //route_worst_out_corr = li._outputAvailableCorrection;
// //                 can_dominate_out_lat = li._canDominateOutputLatency;
//                 // Override the default worst value, but ONLY if the branch can dominate.
// //                 if(can_dominate_out_lat)
// //                   route_worst_latency = li._outputLatency;
// 
//                 // Override the defaults with this first item's values.
//                 route_worst_latency = ir->audioLatencyOut;
//               }

        }

#endif
        
//         // Adjust for THIS track's contribution to latency.
//         // The goal is to have equal latency output on all channels on this track.
//         const int track_out_channels = totalProcessBuffers(); // totalOutChannels();
//         for(int i = 0; i < track_out_channels; ++i)
//         {
//           const float lat = trackLatency(i);
//           if(lat > track_worst_chan_latency)
//               track_worst_chan_latency = lat;
//         }
        
      
  //       // Now add the track's own correction value, if any.
  //       // The correction value is NEGATIVE, so simple summation is used.
  //       route_worst_latency += _latencyInfo._sourceCorrectionValue;

  //       // Override the current worst value if the track's own correction value is greater.
  //       // Note that the correction value is always NEGATIVE.
  //       if(_latencyInfo._sourceCorrectionValue > route_worst_latency)
  //         route_worst_latency = _latencyInfo._sourceCorrectionValue;
        
        // Now that we know the worst-case latency of the connected branches,
        //  adjust each of the conveniently stored temporary latency values
        //  in the routes according to whether they can dominate...
        
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
        //const ciTrack tl_end = MusEGlobal::song->tracks()->cend();
        for(ciTrack it = MusEGlobal::song->tracks()->begin(); it != tl_end; ++it)
        {
          if(!(*it)->isMidiTrack())
            continue;
          MidiTrack* track = static_cast<MidiTrack*>(*it);
          if(track->outPort() != port)
            continue;

          // TODO: FIXME: Where to store? We have no route to store it in.
          // Prepare the latency value to be passed to the compensator's writer,
          //  by adjusting each route latency value. ie. the route with the worst-case
          //  latency will get ZERO delay, while routes having smaller latency will get
          //  MORE delay, to match all the signal timings together.
          // The route's audioLatencyOut should have already been calculated and
          //  conveniently stored in the route.

//           ir->audioLatencyOut = route_worst_latency - ir->audioLatencyOut;
//           // Should not happen, but just in case.
//           if((long int)ir->audioLatencyOut < 0)
//             ir->audioLatencyOut = 0.0f;

          // TODO FIXME This probably won't work.
//           TrackLatencyInfo& li = track->getInputLatencyInfo();
//           li._outputLatency = route_worst_latency - li._outputLatency;
//           // Should not happen, but just in case.
//           if((long int)li._outputLatency < 0)
//             li._outputLatency = 0.0f;

          // Special for Midi Tracks: We don't have Midi Track to Midi Port routes yet
          //  because we don't have multiple Midi Track outputs yet, only a single output port.
          // So we must store this information here just for Midi Tracks.
          TrackLatencyInfo& li = track->getInputLatencyInfo();
          li._latencyOutMidiTrack = route_worst_latency - li._outputLatency;
          // Should not happen, but just in case.
          if((long int)li._latencyOutMidiTrack < 0)
            li._latencyOutMidiTrack = 0.0f;
        }

#else

        for (iRoute ir = rl->begin(); ir != rl->end(); ++ir)
        {
              switch(ir->type)
              {
                  case Route::TRACK_ROUTE:
                    if(!ir->track)
                      continue;
                    
                    if(ir->track->isMidiTrack())
                    {
                      if(ir->channel < -1 || ir->channel >= MUSE_MIDI_CHANNELS)
                        continue;

                      // Prepare the latency value to be passed to the compensator's writer,
                      //  by adjusting each route latency value. ie. the route with the worst-case
                      //  latency will get ZERO delay, while routes having smaller latency will get
                      //  MORE delay, to match all the signal timings together.
                      // The route's audioLatencyOut should have already been calculated and
                      //  conveniently stored in the route.
                      ir->audioLatencyOut = route_worst_latency - ir->audioLatencyOut;
                      // Should not happen, but just in case.
                      if((long int)ir->audioLatencyOut < 0)
                        ir->audioLatencyOut = 0.0f;
                    }
                  break;
                  
                  default:
                  break;
              }
          

          
          
          
//               if(ir->type != Route::TRACK_ROUTE || !ir->track || ir->track->isMidiTrack())
//                 continue;
//               
//               // If the branch cannot dominate the latency, force it to be
//               //  equal to the worst-case value.
//               //if(!ir->canDominateLatency)
//               // If the branch cannot correct the latency, force it to be
//               //  equal to the worst-case value.
// //               if(!ir->canCorrectOutputLatency)
// //                 ir->audioLatencyOut = route_worst_latency;
//               
//               
//               // Prepare the latency value to be passed to the compensator's writer,
//               //  by adjusting each route latency value. ie. the route with the worst-case
//               //  latency will get ZERO delay, while routes having smaller latency will get
//               //  MORE delay, to match all the signal timings together.
//               // The route's audioLatencyOut should have already been calculated and
//               //  conveniently stored in the route.
//               ir->audioLatencyOut = route_worst_latency - ir->audioLatencyOut;
//               // Should not happen, but just in case.
//               if((long int)ir->audioLatencyOut < 0)
//                 ir->audioLatencyOut = 0.0f;
        }

#endif
            
      }
      
      // The absolute latency of signals leaving this track is the sum of
      //  any connected route latencies and this track's latency.
//       _latencyInfo._trackLatency  = track_worst_chan_latency;
//       _latencyInfo._outputLatency = track_worst_chan_latency + route_worst_latency;
      // The _trackLatency should have been already calculated from the dominance scan.
//       _latencyInfo._outputLatency = _latencyInfo._trackLatency + route_worst_latency;
      tli->_outputLatency = tli->_trackLatency + route_worst_latency;
      //_latencyInfo._outputAvailableCorrection = route_worst_out_corr;
//       _latencyInfo._canDominateOutputLatency = can_dominate_out_lat;

      tli->_processed = true;
      return *tli;
}

//---------------------------------------------------------
//   getLatencyInfo
//---------------------------------------------------------

TrackLatencyInfo& MidiDevice::getLatencyInfoMidi(bool capture)
{
      TrackLatencyInfo* tli = capture ? &_captureLatencyInfo : &_playbackLatencyInfo;

      // Have we been here before during this scan?
      // Just return the cached value.
//       if(_latencyInfo._processed)
//         return _latencyInfo;
      if(tli->_processed)
        return *tli;

      const int port = midiPort();

//       RouteList* rl = inRoutes();
      //const RouteList* rl = inRoutes();
      float route_worst_latency = 0.0f;
      //float track_worst_chan_latency = 0.0f;
      
      // This value has a range from 0 (worst) to positive inf (best) or close to it.
      //float route_worst_out_corr = outputLatencyCorrection();
      // Get the default domination for this track type.
//       bool can_dominate_out_lat = canDominateOutputLatency();
      
      // Gather latency info from all connected input branches,
      //  but ONLY if the track is not off.
//       if(!off())
      if((openFlags() & (capture ? 2 : 1)) && port >= 0 && port < MusECore::MIDI_PORTS)
      {
        bool item_found = false;

#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
        const ciTrack tl_end = MusEGlobal::song->tracks()->cend();
        for(ciTrack it = MusEGlobal::song->tracks()->begin(); it != tl_end; ++it)
        {
          if(!(*it)->isMidiTrack())
            continue;
          MidiTrack* track = static_cast<MidiTrack*>(*it);
          if(track->outPort() != port)
            continue;

          const TrackLatencyInfo& li = track->getLatencyInfo();
          
          // TODO: FIXME: Where to store? We have no route to store it in.
          // Temporarily store these values conveniently in the actual route.
          // They will be used by the latency compensator in the audio process pass.
          //ir->audioLatencyOut = li._outputLatency;

          // Is it the first found item?
          if(item_found)
          {
              // Override the current worst value if the latency is greater,
              //  but ONLY if the branch can dominate.
              //if(passthru && li._outputLatency > route_worst_latency)
              if(li._outputLatency > route_worst_latency)
                route_worst_latency = li._outputLatency;

              //if(ir->audioLatencyOut > route_worst_latency)
              ////if(passthru && li._outputLatency > route_worst_latency)
              //  route_worst_latency = ir->audioLatencyOut;
          }
          else
          {
            item_found = true;
            // Override the defaults with this first item's values.
            route_worst_latency = li._outputLatency;

            //route_worst_latency = ir->audioLatencyOut;
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
                      if(ir->channel < -1 || ir->channel >= MUSE_MIDI_CHANNELS)
                        continue;

                      Track* track = ir->track;
                      const TrackLatencyInfo& li = track->getLatencyInfo();
                      
                      // Temporarily store these values conveniently in the actual route.
                      // They will be used by the latency compensator in the audio process pass.
                      ir->audioLatencyOut = li._outputLatency;

                      // Is it the first found item?
                      if(item_found)
                      {
                          // Override the current worst value if the latency is greater,
                          //  but ONLY if the branch can dominate.
                          if(ir->audioLatencyOut > route_worst_latency)
                          //if(passthru && li._outputLatency > route_worst_latency)
                            route_worst_latency = ir->audioLatencyOut;
                      }
                      else
                      {
                        item_found = true;
                        // Override the defaults with this first item's values.
                        route_worst_latency = ir->audioLatencyOut;
                      }
                    }
                  break;
                  
                  default:
                  break;
              }          

              
              
//               if(ir->type != Route::TRACK_ROUTE || !ir->track || ir->track->isMidiTrack())
//                 continue;
//               AudioTrack* atrack = static_cast<AudioTrack*>(ir->track);
//   //             const int atrack_out_channels = atrack->totalOutChannels();
//   //             const int src_ch = ir->remoteChannel <= -1 ? 0 : ir->remoteChannel;
//   //             const int src_chs = ir->channels;
//   //             int fin_src_chs = src_chs;
//   //             if(src_ch + fin_src_chs >  atrack_out_channels)
//   //               fin_src_chs = atrack_out_channels - src_ch;
//   //             const int next_src_chan = src_ch + fin_src_chs;
//   //             // The goal is to have equal latency output on all channels on this track.
//   //             for(int i = src_ch; i < next_src_chan; ++i)
//   //             {
//   //               const float lat = atrack->trackLatency(i);
//   //               if(lat > worst_case_latency)
//   //                 worst_case_latency = lat;
//   //             }
//               const TrackLatencyInfo& li = atrack->getLatencyInfo();
//               
//               // Temporarily store these values conveniently in the actual route.
//               // They will be used by the latency compensator in the audio process pass.
// //               ir->canDominateLatency = li._canDominateOutputLatency;
//               //ir->canCorrectOutputLatency = li._canCorrectOutputLatency;
//               ir->audioLatencyOut = li._outputLatency;
//               
//   //             // Override the current worst value if the latency is greater,
//   //             //  but ONLY if the branch can dominate.
//   //             if(li._canDominateOutputLatency && li._outputLatency > route_worst_latency)
//   //               route_worst_latency = li._outputLatency;
//   //             // Override the current worst value if the latency is greater.
//   //             if(li._outputLatency > route_worst_latency)
//   //               route_worst_latency = li._outputLatency;
//               
//               // Is it the first found item?
//               if(item_found)
//               {
//                 // Override the current values with this item's values ONLY if required.
//                 
//                 //if(li._outputAvailableCorrection < route_worst_out_corr)
//                 //  route_worst_out_corr = li._outputAvailableCorrection;
//                 
//                 // If any one of the branches can dominate the latency,
//                 //  that overrides any which cannot.
// //                 if(li._canDominateOutputLatency)
// //                 {
// 
// //                   can_dominate_out_lat = true;
// //                   // Override the current worst value if the latency is greater,
// //                   //  but ONLY if the branch can dominate.
// //                   if(li._outputLatency > route_worst_latency)
// //                     route_worst_latency = li._outputLatency;
// // //                 }
//                   
//                   // Override the current worst value if the latency is greater,
//                   //  but ONLY if the branch can dominate.
//                   if(ir->audioLatencyOut > route_worst_latency)
//                   //if(passthru && li._outputLatency > route_worst_latency)
//                     route_worst_latency = ir->audioLatencyOut;
// //                   }
//               }
//               else
//               {
//                 item_found = true;
//                 // Override the defaults with this first item's values.
//                 //route_worst_out_corr = li._outputAvailableCorrection;
// //                 can_dominate_out_lat = li._canDominateOutputLatency;
//                 // Override the default worst value, but ONLY if the branch can dominate.
// //                 if(can_dominate_out_lat)
// //                   route_worst_latency = li._outputLatency;
// 
//                 // Override the defaults with this first item's values.
//                 route_worst_latency = ir->audioLatencyOut;
//               }
        }

#endif
        
//         // Adjust for THIS track's contribution to latency.
//         // The goal is to have equal latency output on all channels on this track.
//         const int track_out_channels = totalProcessBuffers(); // totalOutChannels();
//         for(int i = 0; i < track_out_channels; ++i)
//         {
//           const float lat = trackLatency(i);
//           if(lat > track_worst_chan_latency)
//               track_worst_chan_latency = lat;
//         }
        
      
  //       // Now add the track's own correction value, if any.
  //       // The correction value is NEGATIVE, so simple summation is used.
  //       route_worst_latency += _latencyInfo._sourceCorrectionValue;

  //       // Override the current worst value if the track's own correction value is greater.
  //       // Note that the correction value is always NEGATIVE.
  //       if(_latencyInfo._sourceCorrectionValue > route_worst_latency)
  //         route_worst_latency = _latencyInfo._sourceCorrectionValue;
        
        // Now that we know the worst-case latency of the connected branches,
        //  adjust each of the conveniently stored temporary latency values
        //  in the routes according to whether they can dominate...
        
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
        //const ciTrack tl_end = MusEGlobal::song->tracks()->cend();
        for(ciTrack it = MusEGlobal::song->tracks()->begin(); it != tl_end; ++it)
        {
          if(!(*it)->isMidiTrack())
            continue;
          MidiTrack* track = static_cast<MidiTrack*>(*it);
          if(track->outPort() != port)
            continue;

          // TODO: FIXME: Where to store? We have no route to store it in.
          // Prepare the latency value to be passed to the compensator's writer,
          //  by adjusting each route latency value. ie. the route with the worst-case
          //  latency will get ZERO delay, while routes having smaller latency will get
          //  MORE delay, to match all the signal timings together.
          // The route's audioLatencyOut should have already been calculated and
          //  conveniently stored in the route.
//           ir->audioLatencyOut = route_worst_latency - ir->audioLatencyOut;
//           // Should not happen, but just in case.
//           if((long int)ir->audioLatencyOut < 0)
//             ir->audioLatencyOut = 0.0f;

//           // TODO FIXME This probably won't work.
//           TrackLatencyInfo& li = track->getLatencyInfo();
//           li._outputLatency = route_worst_latency - li._outputLatency;
//           // Should not happen, but just in case.
//           if((long int)li._outputLatency < 0)
//             li._outputLatency = 0.0f;

          // Special for Midi Tracks: We don't have Midi Track to Midi Port routes yet
          //  because we don't have multiple Midi Track outputs yet, only a single output port.
          // So we must store this information here just for Midi Tracks.
          TrackLatencyInfo& li = track->getLatencyInfo();
          li._latencyOutMidiTrack = route_worst_latency - li._outputLatency;
          // Should not happen, but just in case.
          if((long int)li._latencyOutMidiTrack < 0)
            li._latencyOutMidiTrack = 0.0f;
        }

#else

        for (iRoute ir = rl->begin(); ir != rl->end(); ++ir)
        {
              switch(ir->type)
              {
                  case Route::TRACK_ROUTE:
                    if(!ir->track)
                      continue;
                    
                    if(ir->track->isMidiTrack())
                    {
                      if(ir->channel < -1 || ir->channel >= MUSE_MIDI_CHANNELS)
                        continue;

                      // Prepare the latency value to be passed to the compensator's writer,
                      //  by adjusting each route latency value. ie. the route with the worst-case
                      //  latency will get ZERO delay, while routes having smaller latency will get
                      //  MORE delay, to match all the signal timings together.
                      // The route's audioLatencyOut should have already been calculated and
                      //  conveniently stored in the route.
                      ir->audioLatencyOut = route_worst_latency - ir->audioLatencyOut;
                      // Should not happen, but just in case.
                      if((long int)ir->audioLatencyOut < 0)
                        ir->audioLatencyOut = 0.0f;
                    }
                  break;
                  
                  default:
                  break;
              }

              
              
//               if(ir->type != Route::TRACK_ROUTE || !ir->track || ir->track->isMidiTrack())
//                 continue;
//               
//               // If the branch cannot dominate the latency, force it to be
//               //  equal to the worst-case value.
//               //if(!ir->canDominateLatency)
//               // If the branch cannot correct the latency, force it to be
//               //  equal to the worst-case value.
// //               if(!ir->canCorrectOutputLatency)
// //                 ir->audioLatencyOut = route_worst_latency;
//               
//               
//               // Prepare the latency value to be passed to the compensator's writer,
//               //  by adjusting each route latency value. ie. the route with the worst-case
//               //  latency will get ZERO delay, while routes having smaller latency will get
//               //  MORE delay, to match all the signal timings together.
//               // The route's audioLatencyOut should have already been calculated and
//               //  conveniently stored in the route.
//               ir->audioLatencyOut = route_worst_latency - ir->audioLatencyOut;
//               // Should not happen, but just in case.
//               if((long int)ir->audioLatencyOut < 0)
//                 ir->audioLatencyOut = 0.0f;
        }

#endif
            
      }
      
      // The absolute latency of signals leaving this track is the sum of
      //  any connected route latencies and this track's latency.
//       _latencyInfo._trackLatency  = track_worst_chan_latency;
//       _latencyInfo._outputLatency = track_worst_chan_latency + route_worst_latency;
      // The _trackLatency should have been already calculated from the dominance scan.
//       _latencyInfo._outputLatency = _latencyInfo._trackLatency + route_worst_latency;
      tli->_outputLatency = tli->_trackLatency + route_worst_latency;
      //_latencyInfo._outputAvailableCorrection = route_worst_out_corr;
//       _latencyInfo._canDominateOutputLatency = can_dominate_out_lat;

      tli->_processed = true;
      return *tli;
}

//---------------------------------------------------------
//   latencyCompWriteOffset
//---------------------------------------------------------

unsigned long MidiDevice::latencyCompWriteOffsetMidi(bool capture) const
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
    // REMOVE Tim. latency. Added.
//     fprintf(stderr, "AudioTrack::setLatencyCompWriteOffset() name:%s capture:%d worstCase:%f _outputLatency:%f _compensatorWriteOffset:%lu\n",
//             name().toLatin1().constData(), capture, worstCase, tli->_outputLatency, tli->_compensatorWriteOffset);
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

  // REMOVE Tim. latency. Added.
//   fprintf(stderr, "AudioTrack::setLatencyCompWriteOffset() name:%s capture:%d worstCase:%f _outputLatency:%f _canDominateOutputLatency:%d _compensatorWriteOffset:%lu\n",
//           name().toLatin1().constData(), capture, worstCase, tli->_outputLatency, tli->_canDominateOutputLatency, tli->_compensatorWriteOffset);
}

//================================================
// END Latency correction/compensation routines.
//================================================


} // namespace MusECore

