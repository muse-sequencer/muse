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

// For debugging output: Uncomment the fprintf section.
// REMOVE Tim. autoconnect. Changed. Enabled. Disable when done.
//#define DEBUG_MIDI_DEVICE(dev, format, args...)  //fprintf(dev, format, ##args);
#define DEBUG_MIDI_DEVICE(dev, format, args...)  fprintf(dev, format, ##args);

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
// REMOVE Tim. autoconnect. Added.
      _extClockHistoryFifo = new LockFreeBuffer<ExtMidiClock>(extClockHistoryCapacity);
      _eventFifos = new LockFreeMultiBuffer<MidiPlayEvent>();
      _eventFifos->createBuffer(PlayFifo, 512);
      _eventFifos->createBuffer(GuiFifo, 512);
      _eventFifos->createBuffer(OSCFifo, 512);
      _eventFifos->createBuffer(JackFifo, 512);
      _eventFifos->createBuffer(ALSAFifo, 512);

      //_osc2AudioFifo = new LockFreeBuffer<MidiPlayEvent>(512);
      //_playStateExt = ExtMidiClock::ExternStopped;
      
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

// REMOVE Tim. autoconnect. Added.
MidiDevice::~MidiDevice() 
{
    if(_extClockHistoryFifo)
      delete _extClockHistoryFifo;
    //if(_osc2AudioFifo)
    //  delete _osc2AudioFifo;
    if(_eventFifos)
      delete _eventFifos;
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

// REMOVE Tim. autoconnect. Added.
//---------------------------------------------------------
//   midiClockInput
//    Midi clock (24 ticks / quarter note)
//---------------------------------------------------------

void MidiDevice::midiClockInput(unsigned int frame)
{
// REMOVE Tim. autoconnect. Added.
//   if(MusEGlobal::midiInputTrace)
//     fprintf(stderr, "midiClockInput port:%d frame:%u\n", port+1, frame);

//   MidiPort* mp = &MusEGlobal::midiPorts[port];

//   const int port = midiPort();
//   if(port < 0 || port >= MIDI_PORTS)
//     return;
// 
//   MidiPort* mp = &MusEGlobal::midiPorts[port];
//   
//   mp->syncInfo().trigMCSyncDetect();
  
//   // External sync not on? Not for the current in port? Clock in not turned on? Forget it.
//   if(!MusEGlobal::extSyncFlag.value() || port != MusEGlobal::config.curMidiSyncInPort || !mp->syncInfo().MCIn())
//     return;

  // REMOVE Tim. autoconnect. Added.
//   fprintf(stderr, "MidiDevice::midiClockInput: CLOCK port:%d time:%u\n", midiPort(), frame);
                                    
//   // Re-transmit clock to other devices if clock out turned on.
//   // Must be careful not to allow more than one clock input at a time.
//   // Would re-transmit mixture of multiple clocks - confusing receivers.
//   // Solution: Added MusEGlobal::curMidiSyncInPort.
//   // Maybe in MidiSyncContainer::processTimerTick(), call sendClock for the other devices, instead of here.
//   for(int p = 0; p < MIDI_PORTS; ++p)
//     if(p != port && MusEGlobal::midiPorts[p].syncInfo().MCOut())
//       MusEGlobal::midiPorts[p].sendClock();

//   MusEGlobal::lastExtMidiSyncFrame = MusEGlobal::curExtMidiSyncFrame;
//   MusEGlobal::curExtMidiSyncFrame = frame;

  // REMOVE Tim. autoconnect. Added.
//   if(MusEGlobal::lastExtMidiSyncFrame > MusEGlobal::curExtMidiSyncFrame)
//   {
//     fprintf(stderr, 
//       "MusE: Warning: MidiSyncContainer::midiClockInput(): lastExtMidiSyncFrame:%u > curExtMidiSyncFrame:%u Setting last to cur...\n", 
//       MusEGlobal::lastExtMidiSyncFrame, MusEGlobal::curExtMidiSyncFrame);
//     MusEGlobal::lastExtMidiSyncFrame = MusEGlobal::curExtMidiSyncFrame;
//   }
// //   unsigned int f_diff = MusEGlobal::curExtMidiSyncFrame - MusEGlobal::lastExtMidiSyncFrame;
  
  // REMOVE Tim. autoconnect. Added.
//   const int div = MusEGlobal::config.division/24;
//   // Put a midi clock record event into the clock history fifo. Ignore port and channel.
//   // Timestamp with the current frame.
// //   if(MusEGlobal::midiSyncContainer.extClockHistory())
// //     //MusEGlobal::midiSyncContainer.extClockHistory()->put(MusEGlobal::audio->curFrame());
// //     MusEGlobal::midiSyncContainer.extClockHistory()->put(ExtMidiClock(frame, playStateExt));
// //   if(MusEGlobal::midiSyncContainer.extClockHistory())
// //   {
// //     for(int i = 0; i < div; ++i)
// //     {
// //       const unsigned int idx = (double(i) / double(div)) * double(f_diff);
// //       const unsigned int s_frame = idx + MusEGlobal::lastExtMidiSyncFrame;
// //       MusEGlobal::midiSyncContainer.extClockHistory()->put(s_frame);
// //     }
// //   }
  
// REMOVE Tim. autoconnect. Changed.
//   if(MusEGlobal::playPendingFirstClock)
//   {
//     MusEGlobal::playPendingFirstClock = false;
// //     // Hopefully the transport will be ready by now, the seek upon start should mean the
// //     //  audio prefetch has already finished or at least started...
// //     // Must comfirm that play does not force a complete prefetch again, but don't think so...
//     if(!MusEGlobal::audio->isPlaying())
//       MusEGlobal::audioDevice->startTransport();
//   }
//   //if(MusEGlobal::playPendingFirstClock)
//   //-------------------------------
//   // State changes:
//   //-------------------------------
//   bool first_clock = false;
//   const ExtMidiClock::ExternState ext_state = MusEGlobal::midiSyncContainer.externalPlayState();
//   if(ext_state == ExtMidiClock::ExternStarting || ext_state == ExtMidiClock::ExternContinuing)
//   {
//     first_clock = true;
//     if(_playStateExt == ExtMidiClock::ExternStarting)
//             _playStateExt = ExtMidiClock::ExternStarted;
//     if(_playStateExt == ExtMidiClock::ExternContinuing)
//             _playStateExt = ExtMidiClock::ExternContinued;
//     if(MusEGlobal::audio->isRunning() && !MusEGlobal::audio->isPlaying() && MusEGlobal::checkAudioDevice())
//       MusEGlobal::audioDevice->startTransport();
//   }
  
  // Put a midi clock record event into the clock history fifo. Ignore port and channel.
  // Timestamp with the current frame.
  const ExtMidiClock ext_clk = MusEGlobal::midiSyncContainer.midiClockInput(midiPort(), frame);
  if(ext_clk.isValid() && extClockHistory())
    //MusEGlobal::midiSyncContainer.extClockHistory()->put(MusEGlobal::audio->curFrame());
//     extClockHistory()->put(ExtMidiClock(frame, _playStateExt, first_clock));
    //extClockHistory()->put(ExtMidiClock(frame, ext_state, first_clock));
    extClockHistory()->put(ext_clk);
//   //else DELETETHIS?
//   // This part will be run on the second and subsequent clocks, after start.
//   // Can't check audio state, might not be playing yet, we might miss some increments.
// // REMOVE Tim. autoconnect. Changed.
// //   if(playStateExt)
//   if(isRunning())
//   {
// // REMOVE Tim. autoconnect. Removed. Moved above.
// //                     int div = MusEGlobal::config.division/24;
//     MusEGlobal::midiExtSyncTicks += div;
//     MusEGlobal::lastExtMidiSyncTick = MusEGlobal::curExtMidiSyncTick;
//     MusEGlobal::curExtMidiSyncTick += div;
// 
// // REMOVE Tim. autoconnect. Changed.
// //     if(MusEGlobal::song->record() && MusEGlobal::lastExtMidiSyncTime > 0.0)
// //       double diff = MusEGlobal::curExtMidiSyncTime - MusEGlobal::lastExtMidiSyncTime;
// //       if(diff != 0.0)
//     if(MusEGlobal::song->record() && MusEGlobal::curExtMidiSyncFrame > MusEGlobal::lastExtMidiSyncFrame)
//     {
//       double diff = double(MusEGlobal::curExtMidiSyncFrame - MusEGlobal::lastExtMidiSyncFrame) / double(MusEGlobal::sampleRate);
//       if(diff != 0.0)
//       {
//         
//         if(_clockAveragerPoles == 0)
//         {
//           double real_tempo = 60.0/(diff * 24.0);
//           if(_tempoQuantizeAmount > 0.0)
//           {
//             double f_mod = fmod(real_tempo, _tempoQuantizeAmount);
//             if(f_mod < _tempoQuantizeAmount/2.0)
//               real_tempo -= f_mod;
//             else
//               real_tempo += _tempoQuantizeAmount - f_mod;
//           }
//           int new_tempo = ((1000000.0 * 60.0) / (real_tempo));
//           if(new_tempo != lastTempo)
//           {
//             lastTempo = new_tempo;
//             // Compute tick for this tempo - it is one step back in time.
//             int add_tick = MusEGlobal::curExtMidiSyncTick - div;
//             if(MusEGlobal::debugSync)
//               printf("adding new tempo tick:%d curExtMidiSyncTick:%d avg_diff:%f real_tempo:%f new_tempo:%d = %f\n", add_tick, MusEGlobal::curExtMidiSyncTick, diff, real_tempo, new_tempo, (double)((1000000.0 * 60.0)/new_tempo));
//             MusEGlobal::song->addExternalTempo(TempoRecEvent(add_tick, new_tempo));
//           }
//         }
//         else
//         {
//           double avg_diff = diff;
//           for(int pole = 0; pole < _clockAveragerPoles; ++pole)
//           {
//             timediff[pole][_avgClkDiffCounter[pole]] = avg_diff;
//             ++_avgClkDiffCounter[pole];
//             if(_avgClkDiffCounter[pole] >= _clockAveragerStages[pole])
//             {
//               _avgClkDiffCounter[pole] = 0;
//               _averagerFull[pole] = true;
//             }
// 
//             // Each averager needs to be full before we can pass the data to
//             //  the next averager or use the data if all averagers are full...
//             if(!_averagerFull[pole])
//               break;
//             else
//             {
//               avg_diff = 0.0;
//               for(int i = 0; i < _clockAveragerStages[pole]; ++i)
//                 avg_diff += timediff[pole][i];
//               avg_diff /= _clockAveragerStages[pole];
// 
//               int fin_idx = _clockAveragerPoles - 1;
// 
//               // On the first pole? Check for large differences.
//               if(_preDetect && pole == 0)
//               {
//                 double real_tempo = 60.0/(avg_diff * 24.0);
//                 double real_tempo_diff = fabs(real_tempo - _lastRealTempo);
// 
//                 // If the tempo changed a large amount, reset.
//                 if(real_tempo_diff >= 10.0)  // TODO: User-adjustable?
//                 {
//                   if(_tempoQuantizeAmount > 0.0)
//                   {
//                     double f_mod = fmod(real_tempo, _tempoQuantizeAmount);
//                     if(f_mod < _tempoQuantizeAmount/2.0)
//                       real_tempo -= f_mod;
//                     else
//                       real_tempo += _tempoQuantizeAmount - f_mod;
//                   }
//                   _lastRealTempo = real_tempo;
//                   int new_tempo = ((1000000.0 * 60.0) / (real_tempo));
// 
//                   if(new_tempo != lastTempo)
//                   {
//                     lastTempo = new_tempo;
//                     // Compute tick for this tempo - it is way back in time.
//                     int add_tick = MusEGlobal::curExtMidiSyncTick - _clockAveragerStages[0] * div;
//                     if(add_tick < 0)
//                     {
//                       printf("FIXME sync: adding restart tempo curExtMidiSyncTick:%d: add_tick:%d < 0 !\n", MusEGlobal::curExtMidiSyncTick, add_tick);
//                       add_tick = 0;
//                     }
//                     if(MusEGlobal::debugSync)
//                       printf("adding restart tempo tick:%d curExtMidiSyncTick:%d tick_idx_sub:%d avg_diff:%f real_tempo:%f real_tempo_diff:%f new_tempo:%d = %f\n", add_tick, MusEGlobal::curExtMidiSyncTick, _clockAveragerStages[0], avg_diff, real_tempo, real_tempo_diff, new_tempo, (double)((1000000.0 * 60.0)/new_tempo));
//                     MusEGlobal::song->addExternalTempo(TempoRecEvent(add_tick, new_tempo));
//                   }
// 
//                   // Reset all the poles.
//                   //for(int i = 0; i < clockAveragerPoles; ++i)
//                   // We have a value for this pole, let's keep it but reset the other poles.
//                   for(int i = 1; i < _clockAveragerPoles; ++i)
//                   {
//                     _avgClkDiffCounter[i] = 0;
//                     _averagerFull[i] = false;
//                   }
//                   break;
//                 }
//               }
// 
//               // On the last pole?
//               // All averagers need to be full before we can use the data...
//               if(pole == fin_idx)
//               {
//                 double real_tempo = 60.0/(avg_diff * 24.0);
//                 double real_tempo_diff = fabs(real_tempo - _lastRealTempo);
// 
//                 if(real_tempo_diff >= _tempoQuantizeAmount/2.0) // Anti-hysteresis
//                 {
//                   if(_tempoQuantizeAmount > 0.0)
//                   {
//                     double f_mod = fmod(real_tempo, _tempoQuantizeAmount);
//                     if(f_mod < _tempoQuantizeAmount/2.0)
//                       real_tempo -= f_mod;
//                     else
//                       real_tempo += _tempoQuantizeAmount - f_mod;
//                   }
//                   _lastRealTempo = real_tempo;
//                   int new_tempo = ((1000000.0 * 60.0) / (real_tempo));
// 
//                   if(new_tempo != lastTempo)
//                   {
//                     lastTempo = new_tempo;
//                     // Compute tick for this tempo - it is way back in time.
//                     int tick_idx_sub = 0;
//                     for(int i = 0; i <= pole; ++i)
//                       tick_idx_sub += _clockAveragerStages[i];
//                     // Compensate: Each pole > 0 has a delay one less than its number of stages.
//                     // For example three pole {8, 8, 8} has a delay of 22 not 24.
//                     tick_idx_sub -= pole;
//                     int add_tick = MusEGlobal::curExtMidiSyncTick - tick_idx_sub * div;
//                     if(add_tick < 0)
//                     {
//                       printf("FIXME sync: adding new tempo curExtMidiSyncTick:%d: add_tick:%d < 0 !\n", MusEGlobal::curExtMidiSyncTick, add_tick);
//                       add_tick = 0;
//                     }
//                     if(MusEGlobal::debugSync)
//                       printf("adding new tempo tick:%d curExtMidiSyncTick:%d tick_idx_sub:%d avg_diff:%f real_tempo:%f new_tempo:%d = %f\n", add_tick, MusEGlobal::curExtMidiSyncTick, tick_idx_sub, avg_diff, real_tempo, new_tempo, (double)((1000000.0 * 60.0)/new_tempo));
//                     MusEGlobal::song->addExternalTempo(TempoRecEvent(add_tick, new_tempo));
//                   }
//                 }
//               }
//             }
//           }
//         }
//       }
//     }
//   }
}

//---------------------------------------------------------
//   recordEvent
//---------------------------------------------------------

void MidiDevice::recordEvent(MidiRecordEvent& event)
      {
// Removed. Let the caller handle the timestamp. This was moved into alsa driver.
// Jack midi device already has its own recordEvent() - simply without the timestamp it does its own,
//  but we might now replace that override method with this default method.
//
//       // TODO: Tested, but record resolution not so good. Switch to wall clock based separate list in MidiDevice.
//       unsigned frame_ts = MusEGlobal::audio->timestamp();
// #ifndef _AUDIO_USE_TRUE_FRAME_
//       if(MusEGlobal::audio->isPlaying())
//        frame_ts += MusEGlobal::segmentSize;  // Shift forward into this period if playing
// #endif
//       event.setTime(frame_ts);
//       event.setTick(MusEGlobal::lastExtMidiSyncTick);
      
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

// REMOVE Tim. autoconnect. Added.
//---------------------------------------------------------
//   putEvent
//    Put a MidiPlayEvent from our gui thread to this device's process thread.
//    return true if event cannot be delivered
//---------------------------------------------------------

bool MidiDevice::putEvent(const MidiPlayEvent& ev, EventFifoIds id)
{
// TODO: Decide whether we want the driver cached values always updated like this,
//        even if not writeable or if error.
//   if(!_writeEnable)
//     return true;
      
  DEBUG_MIDI_DEVICE(stderr, "MidiDevice::putEvent dev:%d time:%d type:%d ch:%d A:%d B:%d\n", 
                    deviceType(), ev.time(), ev.type(), ev.channel(), ev.dataA(), ev.dataB());
  if (MusEGlobal::midiOutputTrace)
  {
    fprintf(stderr, "MidiOut: %s: <%s>: ", deviceTypeString().toLatin1().constData(), name().toLatin1().constData());
    ev.dump();
  }
  
//   bool rv = eventFifo.put(ev);
//   bool rv = _eventFifos->put(GuiFifo, ev);
  bool rv = _eventFifos->put(id, ev);
  if(rv)
    printf("MidiDevice::putEvent: FIFO id:%d overflow\n", id);
  
  return rv;
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

// REMOVE Tim. autoconnect. Added.
// //---------------------------------------------------------
// // preparePlayEventFifo
// // Transfer all fifos into the play events list...
// // To be called from audio thread only.
// //---------------------------------------------------------
// void MidiDevice::preparePlayEventFifo()
// {
//   const int ev_sz = eventFifo.getSize();
//   //const int osc_sz = _osc2AudioFifo->getSize();
//   for(int i = 0; i < ev_sz; ++i)
//     _playEvents.add(eventFifo.get());
//   //for(int i = 0; i < osc_sz; ++i)
//   //  _playEvents.add(_osc2AudioFifo->get());
// }
      
// REMOVE Tim. autoconnect. Added.
// To be called from audio thread only.
void MidiDevice::preparePlayEventFifo()
{
//   // First make sure to call the ancestor, to transfer all fifos into the play events list.
//   MidiDevice::preparePlayEventFifo();
  
  // Transfer the events in the list to the fifo.
  for(ciMPEvent impe = _playEvents.begin(); impe != _playEvents.end(); ++impe)
//     _playEventFifo->put(*impe);
    _eventFifos->put(PlayFifo, *impe);
  // Clear the list.
  _playEvents.clear();
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
//     const bool extsync = MusEGlobal::extSyncFlag.value();
//     const int frameOffset = MusEGlobal::audio->getFrameOffset();
    const unsigned nextTick = MusEGlobal::audio->nextTick();
//     const unsigned curTick = MusEGlobal::audio->tickPos(); // REMOVE Tim. autoconnect. Added.
    ciMPEvent k;

    //---------------------------------------------------
    //    Play any stuck notes which were put directly to the device
    //---------------------------------------------------

    for (k = _stuckNotes.begin(); k != _stuckNotes.end(); ++k) {
          if (k->time() >= nextTick)  
                break;
          MidiPlayEvent ev(*k);
// REMOVE Tim. autoconnect. Changed.
//           if(extsync)              // p3.3.25
//             ev.setTime(k->time());
//           else 
//             ev.setTime(MusEGlobal::tempomap.tick2frame(k->time()) + frameOffset);
          ev.setTime(MusEGlobal::audio->midiQueueTimeStamp(k->time()));
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

// REMOVE Tim. autoconnect. Changed.
// void MidiDevice::handleSeek()
// {
//   // If the device is not in use by a port, don't bother it.
//   if(_port == -1)
//     return;
//   
//   MidiPort* mp = &MusEGlobal::midiPorts[_port];
//   MidiInstrument* instr = mp->instrument();
//   MidiCtrlValListList* cll = mp->controller();
//   unsigned pos = MusEGlobal::audio->tickPos();
//   
//   //---------------------------------------------------
//   //    Send STOP 
//   //---------------------------------------------------
//     
//   // Don't send if external sync is on. The master, and our sync routing system will take care of that.  
//   if(!MusEGlobal::extSyncFlag.value())
//   {
//     if(mp->syncInfo().MRTOut())
//     {
//       // Shall we check for device write open flag to see if it's ok to send?...
//       //if(!(rwFlags() & 0x1) || !(openFlags() & 1))
//       //if(!(openFlags() & 1))
//       //  continue;
//       mp->sendStop();
//     }    
//   }
// 
//   //---------------------------------------------------
//   //    If playing, clear all notes and flush out any
//   //     stuck notes which were put directly to the device
//   //---------------------------------------------------
//   
//   if(MusEGlobal::audio->isPlaying()) 
//   {
//     _playEvents.clear();
//     for(iMPEvent i = _stuckNotes.begin(); i != _stuckNotes.end(); ++i) 
//     {
//       MidiPlayEvent ev(*i);
//       ev.setTime(0);
//       putEvent(ev);  // For immediate playback try putEvent, putMidiEvent, or sendEvent (for the optimizations).
//     }
//     _stuckNotes.clear();
//   }
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
//   if(MusEGlobal::song->click() && MusEGlobal::clickPort == _port)
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
//       if((*i).port() != _port)
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
//           if(mport != _port || usedChans[mchan])
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
//       if((*imt)->outPort() != _port || usedChans[(*imt)->outChannel()])
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
//       int fin_port = _port;
//       MidiPort* fin_mp = mp;
//       int fin_chan = chan;
//       int fin_ctlnum = ctlnum;
//       // Is it a drum controller event, according to the track port's instrument?
//       if(mp->drumController(ctlnum))
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

void MidiDevice::handleSeek()
{
  //---------------------------------------------------
  //    If playing, clear all notes and flush out any
  //     stuck notes which were put directly to the device
  //---------------------------------------------------
  
  if(MusEGlobal::audio->isPlaying()) 
  {
    // TODO: Don't clear, let it play whatever was scheduled.
    _playEvents.clear();
    for(iMPEvent i = _stuckNotes.begin(); i != _stuckNotes.end(); ++i) 
    {
      MidiPlayEvent ev(*i);
      ev.setTime(0);
      putEvent(ev);  // For immediate playback try putEvent, putMidiEvent, or sendEvent (for the optimizations).
    }
    _stuckNotes.clear();
  }
}

// REMOVE Tim. autoconnect. Added.
// void MidiDevice::flushMidiEvents()
// {
//   if(MusEGlobal::audio->isPlaying()) 
//   {
//     _playEvents.clear();
//     for(iMPEvent i = _stuckNotes.begin(); i != _stuckNotes.end(); ++i) 
//     {
//       MidiPlayEvent ev(*i);
//       ev.setTime(0);
//       putEvent(ev);  // For immediate playback try putEvent, putMidiEvent, or sendEvent (for the optimizations).
//     }
//     _stuckNotes.clear();
//   }
// }

} // namespace MusECore

