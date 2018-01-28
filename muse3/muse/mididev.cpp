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
      for(unsigned int i = 0; i < MIDI_CHANNELS + 1; ++i)
        _tmpRecordCount[i] = 0;
      
      _sysexFIFOProcessed = false;
      
      init();
      }

MidiDevice::MidiDevice(const QString& n)
   : _name(n)
      {
      for(unsigned int i = 0; i < MIDI_CHANNELS + 1; ++i)
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
    for(int i = 0; i < MIDI_CHANNELS; ++i)
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
    const unsigned nextTick = MusEGlobal::audio->nextTick();
    ciMPEvent k;

    //---------------------------------------------------
    //    Play any stuck notes which were put directly to the device
    //---------------------------------------------------

    for (k = _stuckNotes.begin(); k != _stuckNotes.end(); ++k) {
          if (k->time() >= nextTick)  
                break;
          MidiPlayEvent ev(*k);
          ev.setTime(MusEGlobal::audio->midiQueueTimeStamp(k->time()));
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
  
  for(int ch = 0; ch < MIDI_CHANNELS; ++ch) 
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

} // namespace MusECore

