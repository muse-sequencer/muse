//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: sync.cpp,v 1.6.2.12 2009/06/20 22:20:41 terminator356 Exp $
//
//  (C) Copyright 2003 Werner Schweer (ws@seh.de)
//  (C) Copyright 2016 Tim E. Real (terminator356 on sourceforge.net)
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

//#include <stdlib.h>
#include "muse_math.h"

#include "sync.h"
#include "song.h"
#include "utils.h"
#include "midiport.h"
//#include "mididev.h"
#include "globals.h"
#include "midiseq.h"
#include "audio.h"
#include "audiodev.h"
#include "gconfig.h"
#include "xml.h"
#include "midi_consts.h"
#include "large_int.h"

namespace MusEGlobal {

bool debugSync = false;

int mtcType     = 1;
MusECore::MTC mtcOffset;
bool extSyncFlag = false;       // false - MASTER, true - SLAVE
bool timebaseMasterState = false;
// Hack when loading songs to force master.
bool timebaseMasterForceFlag = false;

static MusECore::MTC mtcCurTime;
static int mtcState;    // 0-7 next expected quarter message
static bool mtcValid;
static int mtcLost;
static bool mtcSync;    // receive complete mtc frame?

unsigned int syncSendFirstClockDelay = 1; // In milliseconds.
unsigned int volatile curExtMidiSyncTick = 0;
unsigned int volatile lastExtMidiSyncTick = 0;
unsigned int volatile curExtMidiSyncFrame = 0;
unsigned int volatile lastExtMidiSyncFrame = 0;
MusECore::MidiSyncInfo::SyncRecFilterPresetType syncRecFilterPreset = MusECore::MidiSyncInfo::SMALL;
double syncRecTempoValQuant = 1.0;

MusECore::MidiSyncContainer midiSyncContainer;

// Not used yet. DELETETHIS?
// static bool mcStart = false;
// static int mcStartTick;

// From the "Introduction to the Volatile Keyword" at Embedded dot com
/* A variable should be declared volatile whenever its value could change unexpectedly.
 ... <such as> global variables within a multi-threaded application
 ... So all shared global variables should be declared volatile */
unsigned int volatile midiExtSyncTicks = 0;

} // namespace MusEGlobal

namespace MusECore {

//---------------------------------------------------------
//  MidiSyncInfo
//---------------------------------------------------------

MidiSyncInfo::MidiSyncInfo()
{
  _port          = -1;
  _idOut         = 127;
  _idIn          = 127;
  _sendMC        = false;
  _sendMRT       = false;
  _sendMMC       = false;
  _sendMTC       = false;
  _recMC         = false;
  _recMRT        = false;
  _recMMC        = false;
  _recMTC        = false;

  _lastClkTime   = 0;
  _lastTickTime  = 0;
  _lastMRTTime   = 0;
  _lastMMCTime   = 0;
  _lastMTCTime   = 0;
  _clockTrig     = false;
  _tickTrig      = false;
  _MRTTrig       = false;
  _MMCTrig       = false;
  _MTCTrig       = false;
  _clockDetect   = false;
  _tickDetect    = false;
  _MRTDetect     = false;
  _MMCDetect     = false;
  _MTCDetect     = false;
  _recMTCtype    = 0;
  _recRewOnStart  = true;
  _actDetectBits = 0;
  for(int i = 0; i < MusECore::MUSE_MIDI_CHANNELS; ++i)
  {
    _lastActTime[i] = 0.0;
    _actTrig[i]     = false;
    _actDetect[i]   = false;
  }
}

//---------------------------------------------------------
//   operator =
//---------------------------------------------------------

MidiSyncInfo& MidiSyncInfo::operator=(const MidiSyncInfo &sp)
{
  copyParams(sp);

  _lastClkTime   = sp._lastClkTime;
  _lastTickTime  = sp._lastTickTime;
  _lastMRTTime   = sp._lastMRTTime;
  _lastMMCTime   = sp._lastMMCTime;
  _lastMTCTime   = sp._lastMTCTime;
  _clockTrig     = sp._clockTrig;
  _tickTrig      = sp._tickTrig;
  _MRTTrig       = sp._MRTTrig;
  _MMCTrig       = sp._MMCTrig;
  _MTCTrig       = sp._MTCTrig;
  _clockDetect   = sp._clockDetect;
  _tickDetect    = sp._tickDetect;
  _MRTDetect     = sp._MRTDetect;
  _MMCDetect     = sp._MMCDetect;
  _MTCDetect     = sp._MTCDetect;
  _recMTCtype    = sp._recMTCtype;
  for(int i = 0; i < MusECore::MUSE_MIDI_CHANNELS; ++i)
  {
    _lastActTime[i] = sp._lastActTime[i];
    _actTrig[i]     = sp._actTrig[i];
    _actDetect[i]   = sp._actDetect[i];
  }
  return *this;
}

//---------------------------------------------------------
//   copyParams
//---------------------------------------------------------

MidiSyncInfo& MidiSyncInfo::copyParams(const MidiSyncInfo &sp)
{
  _idOut         = sp._idOut;
  _idIn          = sp._idIn;
  _sendMC        = sp._sendMC;
  _sendMRT       = sp._sendMRT;
  _sendMMC       = sp._sendMMC;
  _sendMTC       = sp._sendMTC;
  setMCIn(sp._recMC);
  _recMRT        = sp._recMRT;
  _recMMC        = sp._recMMC;
  _recMTC        = sp._recMTC;
  _recRewOnStart = sp._recRewOnStart;
  return *this;
}

//---------------------------------------------------------
//  setTime
//---------------------------------------------------------

void MidiSyncInfo::setTime()
{
  // Note: CurTime() makes a system call to gettimeofday(),
  //  which apparently can be slow in some cases. So I avoid calling this function
  //  too frequently by calling it (at the heartbeat rate) in Song::beat().  T356
  uint64_t t = curTimeUS();

  if(_clockTrig)
  {
    _clockTrig = false;
    _lastClkTime = t;
  }
  else
  if(_clockDetect && (t - _lastClkTime >= 1000000UL)) // Set detect indicator timeout to about 1 second.
  {
    _clockDetect = false;
  }

  if(_tickTrig)
  {
    _tickTrig = false;
    _lastTickTime = t;
  }
  else
//   if(_tickDetect && (t - _lastTickTime) >= 1.0) // Set detect indicator timeout to about 1 second.
  if(_tickDetect && (t - _lastTickTime) >= 1000000UL) // Set detect indicator timeout to about 1 second.
    _tickDetect = false;

  if(_MRTTrig)
  {
    _MRTTrig = false;
    _lastMRTTime = t;
  }
  else
//   if(_MRTDetect && (t - _lastMRTTime) >= 1.0) // Set detect indicator timeout to about 1 second.
  if(_MRTDetect && (t - _lastMRTTime) >= 1000000UL) // Set detect indicator timeout to about 1 second.
  {
    _MRTDetect = false;
  }

  if(_MMCTrig)
  {
    _MMCTrig = false;
    _lastMMCTime = t;
  }
  else
//   if(_MMCDetect && (t - _lastMMCTime) >= 1.0) // Set detect indicator timeout to about 1 second.
  if(_MMCDetect && (t - _lastMMCTime) >= 1000000UL) // Set detect indicator timeout to about 1 second.
  {
    _MMCDetect = false;
  }

  if(_MTCTrig)
  {
    _MTCTrig = false;
    _lastMTCTime = t;
  }
  else
//   if(_MTCDetect && (t - _lastMTCTime) >= 1.0) // Set detect indicator timeout to about 1 second.
  if(_MTCDetect && (t - _lastMTCTime) >= 1000000UL) // Set detect indicator timeout to about 1 second.
  {
    _MTCDetect = false;
  }

  for(int i = 0; i < MusECore::MUSE_MIDI_CHANNELS; i++)
  {
    if(_actTrig[i])
    {
      _actTrig[i] = false;
      _lastActTime[i] = t;
    }
    else
//     if(_actDetect[i] && (t - _lastActTime[i]) >= 1.0) // Set detect indicator timeout to about 1 second.
    if(_actDetect[i] && (t - _lastActTime[i]) >= 1000000UL) // Set detect indicator timeout to about 1 second.
    {
      _actDetect[i] = false;
      _actDetectBits &= ~(1 << i);
    }
  }
}

//---------------------------------------------------------
//  setMCIn
//---------------------------------------------------------

void MidiSyncInfo::setMCIn(const bool v)
{
  _recMC = v;
}

//---------------------------------------------------------
//  setMRTIn
//---------------------------------------------------------

void MidiSyncInfo::setMRTIn(const bool v)
{
  _recMRT = v;
}

//---------------------------------------------------------
//  setMMCIn
//---------------------------------------------------------

void MidiSyncInfo::setMMCIn(const bool v)
{
  _recMMC = v;
}

//---------------------------------------------------------
//  setMTCIn
//---------------------------------------------------------

void MidiSyncInfo::setMTCIn(const bool v)
{
  _recMTC = v;
}

//---------------------------------------------------------
//  trigMCSyncDetect
//---------------------------------------------------------

void MidiSyncInfo::trigMCSyncDetect()
{
  _clockDetect = true;
  _clockTrig = true;
}

//---------------------------------------------------------
//  trigTickDetect
//---------------------------------------------------------

void MidiSyncInfo::trigTickDetect()
{
  _tickDetect = true;
  _tickTrig = true;
}

//---------------------------------------------------------
//  trigMRTDetect
//---------------------------------------------------------

void MidiSyncInfo::trigMRTDetect()
{
  _MRTDetect = true;
  _MRTTrig = true;
}

//---------------------------------------------------------
//  trigMMCDetect
//---------------------------------------------------------

void MidiSyncInfo::trigMMCDetect()
{
  _MMCDetect = true;
  _MMCTrig = true;
}

//---------------------------------------------------------
//  trigMTCDetect
//---------------------------------------------------------

void MidiSyncInfo::trigMTCDetect()
{
  _MTCDetect = true;
  _MTCTrig = true;
}

//---------------------------------------------------------
//  actDetect
//---------------------------------------------------------

bool MidiSyncInfo::actDetect(const int ch) const
{
  if(ch < 0 || ch >= MusECore::MUSE_MIDI_CHANNELS)
    return false;

  return _actDetect[ch];
}

//---------------------------------------------------------
//  trigActDetect
//---------------------------------------------------------

void MidiSyncInfo::trigActDetect(const int ch)
{
  if(ch < 0 || ch >= MusECore::MUSE_MIDI_CHANNELS)
    return;

  _actDetectBits |= (1 << ch);
  _actDetect[ch] = true;
  _actTrig[ch] = true;
}

//---------------------------------------------------------
//   isDefault
//---------------------------------------------------------

bool MidiSyncInfo::isDefault() const
{
  return(_idOut == 127 && _idIn == 127 && !_sendMC && !_sendMRT && !_sendMMC && !_sendMTC &&
     !_recMC && !_recMRT && !_recMMC && !_recMTC && _recRewOnStart);
}

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void MidiSyncInfo::read(Xml& xml)
      {
      for (;;) {
            Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                             if (tag == "idOut")
                              _idOut = xml.parseInt();
                        else if (tag == "idIn")
                              _idIn = xml.parseInt();
                        else if (tag == "sendMC")
                              _sendMC = xml.parseInt();
                        else if (tag == "sendMRT")
                              _sendMRT = xml.parseInt();
                        else if (tag == "sendMMC")
                              _sendMMC = xml.parseInt();
                        else if (tag == "sendMTC")
                              _sendMTC = xml.parseInt();
                        else if (tag == "recMC")
                              _recMC = xml.parseInt();
                        else if (tag == "recMRT")
                              _recMRT = xml.parseInt();
                        else if (tag == "recMMC")
                              _recMMC = xml.parseInt();
                        else if (tag == "recMTC")
                              _recMTC = xml.parseInt();
                        else if (tag == "recRewStart")
                              _recRewOnStart = xml.parseInt();
                        else
                              xml.unknown("midiSyncInfo");
                        break;
                  case Xml::TagEnd:
                        if (tag == "midiSyncInfo")
                            return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//  write
//---------------------------------------------------------

void MidiSyncInfo::write(int level, Xml& xml)
{
  if(isDefault())
    return;

  xml.tag(level++, "midiSyncInfo");

  if(_idOut != 127)
    xml.intTag(level, "idOut", _idOut);
  if(_idIn != 127)
    xml.intTag(level, "idIn", _idIn);

  if(_sendMC)
    xml.intTag(level, "sendMC", true);
  if(_sendMRT)
    xml.intTag(level, "sendMRT", true);
  if(_sendMMC)
    xml.intTag(level, "sendMMC", true);
  if(_sendMTC)
    xml.intTag(level, "sendMTC", true);

  if(_recMC)
    xml.intTag(level, "recMC", true);
  if(_recMRT)
    xml.intTag(level, "recMRT", true);
  if(_recMMC)
    xml.intTag(level, "recMMC", true);
  if(_recMTC)
    xml.intTag(level, "recMTC", true);
  if(!_recRewOnStart)
    xml.intTag(level, "recRewStart", false);

  xml.etag(level, "midiSyncInfo");
}


//---------------------------------------------------------
//   MidiSyncContainer
//---------------------------------------------------------

MidiSyncContainer::MidiSyncContainer()
{
// REMOVE Tim. clock. Removed.  
  _midiClock = 0;
  mclock1 = 0.0;
  mclock2 = 0.0;
  songtick1 = songtick2 = 0;
  lastTempo = 0;
  storedtimediffs = 0;
  playStateExt = ExtMidiClock::ExternStopped;
  recTick = 0;
  recTick1 = 0;
  recTick2 = 0;

  _clockAveragerStages = new int[16]; // Max stages is 16!

  _syncRecFilterPreset = MidiSyncInfo::SMALL;
  setSyncRecFilterPresetArrays();

  for(int i = 0; i < _clockAveragerPoles; ++i)
  {
    _avgClkDiffCounter[i] = 0;
    _averagerFull[i] = false;
  }
  _tempoQuantizeAmount = 1.0;
  _lastRealTempo      = 0.0;
}

MidiSyncContainer::~MidiSyncContainer()
{
    if(_clockAveragerStages)
      delete[] _clockAveragerStages;
}

//---------------------------------------------------------
//  mmcInput
//    Midi Machine Control Input received
//---------------------------------------------------------

void MidiSyncContainer::mmcInput(int port, const unsigned char* p, int n)
      {
      if (MusEGlobal::debugSync)
            fprintf(stderr, "mmcInput: n:%d %02x %02x %02x %02x\n",
               n, p[2], p[3], p[4], p[5]);

      MidiPort* mp = &MusEGlobal::midiPorts[port];
      MidiSyncInfo& msync = mp->syncInfo();
      // Trigger MMC detect in.
      msync.trigMMCDetect();
      // MMC locate SMPTE time code may contain format type bits. Grab them.
      if(p[3] == 0x44 && p[4] == 6 && p[5] == 1)
        msync.setRecMTCtype((p[6] >> 5) & 3);

      // MMC is not turned on? Forget it.
      if(!msync.MMCIn())
        return;

      switch(p[3]) {
            case MMC_Pause:
                  if (MusEGlobal::debugSync)
                        fprintf(stderr, "  MMC: PAUSE\n");
                  [[fallthrough]]; // not quite correct but lacking a better option, lets handle it like Stop
            case MMC_Stop:
                  if (MusEGlobal::debugSync)
                        fprintf(stderr, "  MMC: STOP\n");

                  playStateExt = ExtMidiClock::ExternStopped;

                  if (MusEGlobal::audio->isPlaying()) {
                        MusEGlobal::audio->msgPlay(false);
                  }

                  MusEGlobal::song->resetFastMove(); // reset FF RWD

                  alignAllTicks();
                  break;
            case MMC_Play:
                  if (MusEGlobal::debugSync)
                        fprintf(stderr, "  MMC: PLAY\n");
                  [[fallthrough]];
            case MMC_DeferredPlay:
                  if (MusEGlobal::debugSync)
                        fprintf(stderr, "  MMC: DEFERRED PLAY\n");
                  MusEGlobal::mtcState = 0;
                  MusEGlobal::mtcValid = false;
                  MusEGlobal::mtcLost  = 0;
                  MusEGlobal::mtcSync  = false;
                  alignAllTicks();
                  playStateExt = ExtMidiClock::ExternStarting;
                  if(MusEGlobal::audio->isRunning() && !MusEGlobal::audio->isPlaying() && MusEGlobal::checkAudioDevice()) 
                    MusEGlobal::audioDevice->startTransport();
                  break;
            case MMC_FastForward:
                  if (MusEGlobal::debugSync)
                        fprintf(stderr, "  MMC: FastForward\n");
                  MusEGlobal::song->putMMC_Command(MMC_FastForward);
                  break;
            case MMC_Rewind:
                  if (MusEGlobal::debugSync)
                        fprintf(stderr, "  MMC: REWIND\n");
                  MusEGlobal::song->putMMC_Command(MMC_Rewind);
                  break;
            case MMC_RecordStrobe:
                  if (MusEGlobal::debugSync)
                        fprintf(stderr, "  MMC: REC STROBE\n");
                  MusEGlobal::song->putMMC_Command(MMC_RecordStrobe);
                  break;
            case MMC_RecordExit:
                  if (MusEGlobal::debugSync)
                        fprintf(stderr, "  MMC: REC EXIT\n");
                  MusEGlobal::song->putMMC_Command(MMC_RecordExit);
                  break;
            case MMC_Reset:
                  if (MusEGlobal::debugSync)
                        fprintf(stderr, "  MMC: Reset\n");
                  MusEGlobal::song->putMMC_Command(MMC_Reset);
                  break;
            case MMC_Goto:
                  if (p[5] == 0) {
                        fprintf(stderr, "MMC: LOCATE IF not implemented\n");
                        break;
                        }
                  else if (p[5] == 1) {
                        if (!MusEGlobal::checkAudioDevice()) return;
                        MTC mtc(p[6] & 0x1f, p[7], p[8], p[9], p[10]);
                        int type = (p[6] >> 5) & 3;
                        // MTC time resolution is less than frame resolution. 
                        // Round up so that the reciprocal function (frame to time) matches value for value.
                        unsigned mmcPos = muse_multiply_64_div_64_to_64(mtc.timeUS(type), MusEGlobal::sampleRate, 1000000UL, LargeIntRoundUp);

                        Pos tp(mmcPos, false);
                        MusEGlobal::audioDevice->seekTransport(tp);
                        alignAllTicks();
                        if (MusEGlobal::debugSync) {
                              fprintf(stderr, "MMC: LOCATE mtc type:%d timeUS:%lu frame:%u mtc: ", type, (long unsigned)mtc.timeUS(), mmcPos);
                              mtc.print();
                              fprintf(stderr, "\n");
                              }
                        break;
                        }
                  [[fallthrough]];
            default:
                  fprintf(stderr, "MMC %x %x, unknown\n", p[3], p[4]); break;
            }
      }

//---------------------------------------------------------
//   mtcInputQuarter
//    process Quarter Frame Message
//---------------------------------------------------------

void MidiSyncContainer::mtcInputQuarter(int port, unsigned char c)
      {
      static int hour, min, sec, frame;

      int valL = c & 0xf;
      int valH = valL << 4;

      int _state = (c & 0x70) >> 4;
      if (MusEGlobal::mtcState != _state)
            MusEGlobal::mtcLost += _state - MusEGlobal::mtcState;
      MusEGlobal::mtcState = _state + 1;

      switch(_state) {
            case 7:
                  hour  = (hour  & 0x0f) | valH;
                  break;
            case 6:
                  hour  = (hour  & 0xf0) | valL;
                  break;
            case 5:
                  min   = (min   & 0x0f) | valH;
                  break;
            case 4:
                  min   = (min   & 0xf0) | valL;
                  break;
            case 3:
                  sec   = (sec   & 0x0f) | valH;
                  break;
            case 2:
                  sec   = (sec   & 0xf0) | valL;
                  break;
            case 1:
                  frame = (frame & 0x0f) | valH;
                  break;
            case 0:  frame = (frame & 0xf0) | valL;
                  break;
            }
      frame &= 0x1f;    // 0-29
      sec   &= 0x3f;    // 0-59
      min   &= 0x3f;    // 0-59
      int tmphour = hour;
      int type = (hour >> 5) & 3;
      hour  &= 0x1f;

      if(MusEGlobal::mtcState == 8)
      {
            MusEGlobal::mtcValid = (MusEGlobal::mtcLost == 0);
            MusEGlobal::mtcState = 0;
            MusEGlobal::mtcLost  = 0;
            if(MusEGlobal::mtcValid)
            {
                  MusEGlobal::mtcCurTime.set(hour, min, sec, frame);
                  if(port != -1)
                  {
                    MidiPort* mp = &MusEGlobal::midiPorts[port];
                    MidiSyncInfo& msync = mp->syncInfo();
                    msync.setRecMTCtype(type);
                    msync.trigMTCDetect();
                    // Not for the current in port? External sync not turned on? MTC in not turned on? Forget it.
                    if(port == MusEGlobal::config.curMidiSyncInPort && MusEGlobal::extSyncFlag && msync.MTCIn())
                    {
                      if(MusEGlobal::debugSync)
                        fprintf(stderr, "MidiSyncContainer::mtcInputQuarter hour byte:%x\n", (unsigned int)tmphour);
                      mtcSyncMsg(MusEGlobal::mtcCurTime, type, !MusEGlobal::mtcSync);
                    }
                  }
                  MusEGlobal::mtcSync = true;
            }
      }
      else if (MusEGlobal::mtcValid && (MusEGlobal::mtcLost == 0))
      {
            MusEGlobal::mtcCurTime.incQuarter(type);
      }
    }

//---------------------------------------------------------
//   mtcInputFull
//    process Frame Message
//---------------------------------------------------------

void MidiSyncContainer::mtcInputFull(int port, const unsigned char* p, int n)
      {
      if (MusEGlobal::debugSync)
            fprintf(stderr, "mtcInputFull\n");

      if (p[3] != 1) {
            if (p[3] != 2) {   // silently ignore user bits
                  fprintf(stderr, "unknown mtc msg subtype 0x%02x\n", p[3]);
                  dump(p, n);
                  }
            return;
            }
      int hour  = p[4];
      int min   = p[5];
      int sec   = p[6];
      int frame = p[7];

      frame &= 0x1f;    // 0-29
      sec   &= 0x3f;    // 0-59
      min   &= 0x3f;    // 0-59
      int type = (hour >> 5) & 3;
      hour &= 0x1f;

      MusEGlobal::mtcCurTime.set(hour, min, sec, frame);
      MusEGlobal::mtcState = 0;
      MusEGlobal::mtcValid = true;
      MusEGlobal::mtcLost  = 0;

      // Added by Tim.
      if(MusEGlobal::debugSync)
        fprintf(stderr, "mtcInputFull: timeUS:%lu stimeUS:%lu hour byte (all bits):%hhx\n",
                (long unsigned)MusEGlobal::mtcCurTime.timeUS(), (long unsigned)MusEGlobal::mtcCurTime.timeUS(type), p[4]);
      if(port != -1)
      {
        MidiPort* mp = &MusEGlobal::midiPorts[port];
        MidiSyncInfo& msync = mp->syncInfo();
        msync.setRecMTCtype(type);
        msync.trigMTCDetect();
        // MTC in not turned on? Forget it.
        if(msync.MTCIn())
        {
          // MTC time resolution is less than frame resolution. 
          // Round up so that the reciprocal function (frame to time) matches value for value.
          const unsigned t_frame = muse_multiply_64_div_64_to_64(MusEGlobal::mtcCurTime.timeUS(type), MusEGlobal::sampleRate, 1000000UL, LargeIntRoundUp);
          
          Pos tp(t_frame, false);
          MusEGlobal::audioDevice->seekTransport(tp);
          alignAllTicks();
        }
      }
    }

//---------------------------------------------------------
//   nonRealtimeSystemSysex
//---------------------------------------------------------

void MidiSyncContainer::nonRealtimeSystemSysex(int /*port*/, const unsigned char* p, int n)
      {
      switch(p[3]) {
            case 4:
                  fprintf(stderr, "NRT Setup\n");
                  break;
            default:
                  fprintf(stderr, "unknown NRT Msg 0x%02x\n", p[3]);
                  dump(p, n);
                  break;
           }
      }

//---------------------------------------------------------
//   setSongPosition
//    MidiBeat is a 14 Bit value. Each MidiBeat spans
//    6 MIDI Clocks. Inother words, each MIDI Beat is a
//    16th note (since there are 24 MIDI Clocks in a
//    quarter note).
//---------------------------------------------------------

void MidiSyncContainer::setSongPosition(int port, int midiBeat)
      {
      if (MusEGlobal::midiInputTrace)
            fprintf(stderr, "set song position port:%d %d\n", port, midiBeat);

      MusEGlobal::midiPorts[port].syncInfo().trigMRTDetect();

      if(!MusEGlobal::extSyncFlag || !MusEGlobal::midiPorts[port].syncInfo().MRTIn())
            return;

      // Re-transmit song position to other devices if clock out turned on.
      for(int p = 0; p < MusECore::MIDI_PORTS; ++p)
        if(p != port && MusEGlobal::midiPorts[p].syncInfo().MRTOut())
          MusEGlobal::midiPorts[p].sendSongpos(midiBeat);

      MusEGlobal::curExtMidiSyncTick = (MusEGlobal::config.division * midiBeat) / 4;
      MusEGlobal::lastExtMidiSyncTick = MusEGlobal::curExtMidiSyncTick;

      Pos pos(MusEGlobal::curExtMidiSyncTick, true);

      if (!MusEGlobal::checkAudioDevice()) return;

      MusEGlobal::audioDevice->seekTransport(pos);
      alignAllTicks(pos.frame());
      if (MusEGlobal::debugSync)
            fprintf(stderr, "setSongPosition %d\n", pos.tick());
      }



//---------------------------------------------------------
//   set all runtime variables to the "in sync" value
//---------------------------------------------------------
void MidiSyncContainer::alignAllTicks(int frameOverride)
      {
      unsigned curFrame;
      if (!frameOverride && MusEGlobal::audio)
        curFrame = MusEGlobal::audio->pos().frame();
      else
        curFrame = frameOverride;

      int tempo = MusEGlobal::tempomap.tempo(0);

      // use the last old values to give start values for the triple buffering
      int recTickSpan = recTick1 - recTick2;
      int songTickSpan = (int)(songtick1 - songtick2);    //prevent compiler warning:  casting double to int
      storedtimediffs = 0; // pretend there is no sync history

      mclock2=mclock1=0.0; // set all clock values to "in sync"

      recTick = (int) ((double(curFrame)/double(MusEGlobal::sampleRate)) *
                        double(MusEGlobal::config.division * 1000000.0) / double(tempo) //prevent compiler warning:  casting double to int
                );
      songtick1 = recTick - songTickSpan;
      if (songtick1 < 0)
        songtick1 = 0;
      songtick2 = songtick1 - songTickSpan;
      if (songtick2 < 0)
        songtick2 = 0;
      recTick1 = recTick - recTickSpan;
      if (recTick1 < 0)
        recTick1 = 0;
      recTick2 = recTick1 - recTickSpan;
      if (recTick2 < 0)
        recTick2 = 0;
      if (MusEGlobal::debugSync)
        fprintf(stderr, "alignAllTicks curFrame=%d recTick=%d tempo=%.3f frameOverride=%d\n",curFrame,recTick,(float)((1000000.0 * 60.0)/tempo), frameOverride);

      lastTempo = 0;
      for(int i = 0; i < _clockAveragerPoles; ++i)
      {
        _avgClkDiffCounter[i] = 0;
        _averagerFull[i] = false;
      }
      _lastRealTempo = 0.0;
      }

//---------------------------------------------------------
//   realtimeSystemInput
//    real time message received
//---------------------------------------------------------
void MidiSyncContainer::realtimeSystemInput(int port, int c)
      {

      if (MusEGlobal::midiInputTrace)
            fprintf(stderr, "realtimeSystemInput port:%d 0x%x\n", port+1, c);

      MidiPort* mp = &MusEGlobal::midiPorts[port];

      // Trigger on any tick, clock, or realtime command.
      if(c == ME_TICK) // Tick
        mp->syncInfo().trigTickDetect();
      else
        mp->syncInfo().trigMRTDetect(); // Other

      // External sync not on? Clock in not turned on? Otherwise realtime in not turned on?
      if(!MusEGlobal::extSyncFlag)
        return;
      if(!mp->syncInfo().MRTIn())
        return;


      switch(c) {
            case ME_TICK:  // midi tick  (every 10 msec)
                  //DELETETHIS 6
                  // FIXME: Unfinished? mcStartTick is uninitialized and Song::setPos doesn't set it either. Dangerous to allow this.
                  //if (mcStart) {
                  //      song->setPos(0, mcStartTick);
                  //      mcStart = false;
                  //      return;
                  //      }
                  break;
            case ME_START:  // start
                  // Re-transmit start to other devices if clock out turned on.
                  for(int p = 0; p < MusECore::MIDI_PORTS; ++p)
                    if(p != port && MusEGlobal::midiPorts[p].syncInfo().MRTOut())
                    {
                      // If we aren't rewinding on start, there's no point in re-sending start.
                      // Re-send continue instead, for consistency.
                      if(MusEGlobal::midiPorts[port].syncInfo().recRewOnStart())
                        MusEGlobal::midiPorts[p].sendStart();
                      else
                        MusEGlobal::midiPorts[p].sendContinue();
                    }
                  if (MusEGlobal::debugSync)
                        fprintf(stderr, "   start\n");


                  // DELETETHIS, remove the wrapping if(true)
                  if (1 /* !MusEGlobal::audio->isPlaying()*/ /*state == IDLE*/) {
                        if (!MusEGlobal::checkAudioDevice()) return;

                        playStateExt = ExtMidiClock::ExternStarting;
                        
                        // Rew on start option.
                        if(MusEGlobal::midiPorts[port].syncInfo().recRewOnStart())
                        {
                          MusEGlobal::curExtMidiSyncTick = 0;
                          MusEGlobal::lastExtMidiSyncTick = MusEGlobal::curExtMidiSyncTick;
                          MusEGlobal::audioDevice->seekTransport(Pos(0, false));
                        }

                        alignAllTicks();
                        storedtimediffs = 0;
                        MusEGlobal::midiExtSyncTicks = 0;
                        }
                  break;
            case ME_CONTINUE:  // continue
                  // Re-transmit continue to other devices if clock out turned on.
                  for(int p = 0; p < MusECore::MIDI_PORTS; ++p)
                    if(p != port && MusEGlobal::midiPorts[p].syncInfo().MRTOut())
                      MusEGlobal::midiPorts[p].sendContinue();

                  if (MusEGlobal::debugSync)
                        fprintf(stderr, "realtimeSystemInput continue\n");

                  //printf("continue:%f\n", curTime());

                  if (1 /* !MusEGlobal::audio->isPlaying() */ /*state == IDLE */) {
                        // Begin incrementing immediately upon first clock reception.
                        playStateExt = ExtMidiClock::ExternContinuing;
                        }
                  break;
            case ME_STOP:  // stop
                  {
                    // Stop the increment right away.
                    MusEGlobal::midiExtSyncTicks = 0;
                    playStateExt = ExtMidiClock::ExternStopped;

                    // Re-transmit stop to other devices if clock out turned on.
                    for(int p = 0; p < MusECore::MIDI_PORTS; ++p)
                      if(p != port && MusEGlobal::midiPorts[p].syncInfo().MRTOut())
                        MusEGlobal::midiPorts[p].sendStop();


                    if (MusEGlobal::audio->isPlaying())
                          MusEGlobal::audio->msgPlay(false);

                    if (MusEGlobal::debugSync)
                          fprintf(stderr, "realtimeSystemInput stop\n");

                    //DELETETHIS 7
                    // Just in case the process still runs a cycle or two and causes the
                    //  audio tick position to increment, reset the incrementer and force
                    //  the transport position to what the hardware thinks is the current position.
                    //MusEGlobal::midiExtSyncTicks = 0;
                    //Pos pos((MusEGlobal::config.division * lastStoppedBeat) / 4, true);
                    //Pos pos(MusEGlobal::curExtMidiSyncTick, true);
                    //MusEGlobal::audioDevice->seekTransport(pos);
                  }

                  break;
            //case 0xfd:  // unknown DELETETHIS 3
            //case ME_SENSE:  // active sensing
            //case ME_META:  // system reset (reset is 0xff same enumeration as file meta event)
            default:
                  break;
            }

      }

//---------------------------------------------------------
//   midiClockInput
//    Midi clock (24 ticks / quarter note)
//    Starts transport if necessary. Adds clock to tempo list.
//    Returns whether the clock was a 'first clock' after a start or continue message.
//---------------------------------------------------------

ExtMidiClock MidiSyncContainer::midiClockInput(int port, unsigned int frame)
{
  if(port < 0 || port >= MusECore::MIDI_PORTS)
    return ExtMidiClock();

  MidiPort* mp = &MusEGlobal::midiPorts[port];

  mp->syncInfo().trigMCSyncDetect();
  
  // External sync not on? Clock in not turned on? Otherwise realtime in not turned on?
  if(!MusEGlobal::extSyncFlag)
    return ExtMidiClock();
  if(!mp->syncInfo().MCIn())
    return ExtMidiClock();

  // Not for the current in port? Forget it.
  if(port != MusEGlobal::config.curMidiSyncInPort)
    return ExtMidiClock();

  //fprintf(stderr, "MidiSyncContainer::midiClockInput: CLOCK port:%d time:%u\n", port, frame);
                                    
  // Re-transmit clock to other devices if clock out turned on.
  // Must be careful not to allow more than one clock input at a time.
  // Would re-transmit mixture of multiple clocks - confusing receivers.
  // Solution: Added MusEGlobal::curMidiSyncInPort.
  // Maybe in MidiSyncContainer::processTimerTick(), call sendClock for the other devices, instead of here.
  for(int p = 0; p < MusECore::MIDI_PORTS; ++p)
    if(p != port && MusEGlobal::midiPorts[p].syncInfo().MCOut())
      MusEGlobal::midiPorts[p].sendClock();

  MusEGlobal::lastExtMidiSyncFrame = MusEGlobal::curExtMidiSyncFrame;
  MusEGlobal::curExtMidiSyncFrame = frame;

  if(MusEGlobal::lastExtMidiSyncFrame > MusEGlobal::curExtMidiSyncFrame)
  {
    fprintf(stderr, 
      "MusE: Warning: MidiSyncContainer::midiClockInput(): lastExtMidiSyncFrame:%u > curExtMidiSyncFrame:%u Setting last to cur...\n", 
      MusEGlobal::lastExtMidiSyncFrame, MusEGlobal::curExtMidiSyncFrame);
    MusEGlobal::lastExtMidiSyncFrame = MusEGlobal::curExtMidiSyncFrame;
  }
  
  const int div = MusEGlobal::config.division/24;
  
  //-------------------------------
  // State changes:
  //-------------------------------
  bool first_clock = false;
  if(playStateExt == ExtMidiClock::ExternStarting || playStateExt == ExtMidiClock::ExternContinuing)
  {
    first_clock = true;
    if(playStateExt == ExtMidiClock::ExternStarting)
      playStateExt = ExtMidiClock::ExternStarted;
    if(playStateExt == ExtMidiClock::ExternContinuing)
      playStateExt = ExtMidiClock::ExternContinued;
    if(MusEGlobal::audio->isRunning() && !MusEGlobal::audio->isPlaying() && MusEGlobal::checkAudioDevice())
      MusEGlobal::audioDevice->startTransport();
  }
  
  //else DELETETHIS?
  // This part will be run on the second and subsequent clocks, after start.
  // Can't check audio state, might not be playing yet, we might miss some increments.
  if(isRunning())
  {
    MusEGlobal::midiExtSyncTicks += div;
    MusEGlobal::lastExtMidiSyncTick = MusEGlobal::curExtMidiSyncTick;
    MusEGlobal::curExtMidiSyncTick += div;

    if(MusEGlobal::song->record() && MusEGlobal::curExtMidiSyncFrame > MusEGlobal::lastExtMidiSyncFrame)
    {
      double diff = double(MusEGlobal::curExtMidiSyncFrame - MusEGlobal::lastExtMidiSyncFrame) / double(MusEGlobal::sampleRate);
      if(diff != 0.0)
      {
        
        if(_clockAveragerPoles == 0)
        {
          double real_tempo = 60.0/(diff * 24.0);
          if(_tempoQuantizeAmount > 0.0)
          {
            double f_mod = fmod(real_tempo, _tempoQuantizeAmount);
            if(f_mod < _tempoQuantizeAmount/2.0)
              real_tempo -= f_mod;
            else
              real_tempo += _tempoQuantizeAmount - f_mod;
          }
          int new_tempo = ((1000000.0 * 60.0) / (real_tempo));
          if(new_tempo != lastTempo)
          {
            lastTempo = new_tempo;
            // Compute tick for this tempo - it is one step back in time.
            int add_tick = MusEGlobal::curExtMidiSyncTick - div;
            if(MusEGlobal::debugSync)
              fprintf(stderr, "adding new tempo tick:%d curExtMidiSyncTick:%d avg_diff:%f real_tempo:%f new_tempo:%d = %f\n", 
                      add_tick, MusEGlobal::curExtMidiSyncTick, diff, real_tempo, new_tempo, (double)((1000000.0 * 60.0)/new_tempo));
            MusEGlobal::song->addExternalTempo(TempoRecEvent(add_tick, new_tempo));
          }
        }
        else
        {
          double avg_diff = diff;
          for(int pole = 0; pole < _clockAveragerPoles; ++pole)
          {
            timediff[pole][_avgClkDiffCounter[pole]] = avg_diff;
            ++_avgClkDiffCounter[pole];
            if(_avgClkDiffCounter[pole] >= _clockAveragerStages[pole])
            {
              _avgClkDiffCounter[pole] = 0;
              _averagerFull[pole] = true;
            }

            // Each averager needs to be full before we can pass the data to
            //  the next averager or use the data if all averagers are full...
            if(!_averagerFull[pole])
              break;
            else
            {
              avg_diff = 0.0;
              for(int i = 0; i < _clockAveragerStages[pole]; ++i)
                avg_diff += timediff[pole][i];
              avg_diff /= _clockAveragerStages[pole];

              int fin_idx = _clockAveragerPoles - 1;

              // On the first pole? Check for large differences.
              if(_preDetect && pole == 0)
              {
                double real_tempo = 60.0/(avg_diff * 24.0);
                double real_tempo_diff = fabs(real_tempo - _lastRealTempo);

                // If the tempo changed a large amount, reset.
                if(real_tempo_diff >= 10.0)  // TODO: User-adjustable?
                {
                  if(_tempoQuantizeAmount > 0.0)
                  {
                    double f_mod = fmod(real_tempo, _tempoQuantizeAmount);
                    if(f_mod < _tempoQuantizeAmount/2.0)
                      real_tempo -= f_mod;
                    else
                      real_tempo += _tempoQuantizeAmount - f_mod;
                  }
                  _lastRealTempo = real_tempo;
                  int new_tempo = ((1000000.0 * 60.0) / (real_tempo));

                  if(new_tempo != lastTempo)
                  {
                    lastTempo = new_tempo;
                    // Compute tick for this tempo - it is way back in time.
                    int add_tick = MusEGlobal::curExtMidiSyncTick - _clockAveragerStages[0] * div;
                    if(add_tick < 0)
                    {
                      fprintf(stderr, "FIXME sync: adding restart tempo curExtMidiSyncTick:%d: add_tick:%d < 0 !\n", 
                              MusEGlobal::curExtMidiSyncTick, add_tick);
                      add_tick = 0;
                    }
                    if(MusEGlobal::debugSync)
                      fprintf(stderr, 
                       "adding restart tempo tick:%d curExtMidiSyncTick:%d tick_idx_sub:%d avg_diff:%f real_tempo:%f real_tempo_diff:%f new_tempo:%d = %f\n", 
                       add_tick, MusEGlobal::curExtMidiSyncTick, _clockAveragerStages[0], avg_diff, 
                       real_tempo, real_tempo_diff, new_tempo, (double)((1000000.0 * 60.0)/new_tempo));
                    MusEGlobal::song->addExternalTempo(TempoRecEvent(add_tick, new_tempo));
                  }

                  // Reset all the poles.
                  //for(int i = 0; i < clockAveragerPoles; ++i)
                  // We have a value for this pole, let's keep it but reset the other poles.
                  for(int i = 1; i < _clockAveragerPoles; ++i)
                  {
                    _avgClkDiffCounter[i] = 0;
                    _averagerFull[i] = false;
                  }
                  break;
                }
              }

              // On the last pole?
              // All averagers need to be full before we can use the data...
              if(pole == fin_idx)
              {
                double real_tempo = 60.0/(avg_diff * 24.0);
                double real_tempo_diff = fabs(real_tempo - _lastRealTempo);

                if(real_tempo_diff >= _tempoQuantizeAmount/2.0) // Anti-hysteresis
                {
                  if(_tempoQuantizeAmount > 0.0)
                  {
                    double f_mod = fmod(real_tempo, _tempoQuantizeAmount);
                    if(f_mod < _tempoQuantizeAmount/2.0)
                      real_tempo -= f_mod;
                    else
                      real_tempo += _tempoQuantizeAmount - f_mod;
                  }
                  _lastRealTempo = real_tempo;
                  int new_tempo = ((1000000.0 * 60.0) / (real_tempo));

                  if(new_tempo != lastTempo)
                  {
                    lastTempo = new_tempo;
                    // Compute tick for this tempo - it is way back in time.
                    int tick_idx_sub = 0;
                    for(int i = 0; i <= pole; ++i)
                      tick_idx_sub += _clockAveragerStages[i];
                    // Compensate: Each pole > 0 has a delay one less than its number of stages.
                    // For example three pole {8, 8, 8} has a delay of 22 not 24.
                    tick_idx_sub -= pole;
                    int add_tick = MusEGlobal::curExtMidiSyncTick - tick_idx_sub * div;
                    if(add_tick < 0)
                    {
                      fprintf(stderr, "FIXME sync: adding new tempo curExtMidiSyncTick:%d: add_tick:%d < 0 !\n", 
                             MusEGlobal::curExtMidiSyncTick, add_tick);
                      add_tick = 0;
                    }
                    if(MusEGlobal::debugSync)
                      fprintf(stderr, "adding new tempo tick:%d curExtMidiSyncTick:%d tick_idx_sub:%d avg_diff:%f real_tempo:%f new_tempo:%d = %f\n", 
                              add_tick, MusEGlobal::curExtMidiSyncTick, tick_idx_sub, avg_diff, 
                              real_tempo, new_tempo, (double)((1000000.0 * 60.0)/new_tempo));
                    MusEGlobal::song->addExternalTempo(TempoRecEvent(add_tick, new_tempo));
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return ExtMidiClock(frame, playStateExt, first_clock);
}

//---------------------------------------------------------
//   MusEGlobal::mtcSyncMsg
//    process received mtc Sync
//    seekFlag - first complete mtc frame received after
//                start
//---------------------------------------------------------

void MidiSyncContainer::mtcSyncMsg(const MTC& mtc, int type, bool seekFlag)
      {
      const uint64_t time = mtc.timeUS();
      const uint64_t stime = mtc.timeUS(type);
      if (MusEGlobal::debugSync)
            fprintf(stderr, "MidiSyncContainer::mtcSyncMsg timeUS:%lu stimeUS:%lu seekFlag:%d\n", 
                    (long unsigned)time, (long unsigned)stime, seekFlag);

      if (seekFlag && MusEGlobal::audio->isRunning() && !MusEGlobal::audio->isPlaying() && MusEGlobal::checkAudioDevice()) 
      {
        if (MusEGlobal::debugSync)
          fprintf(stderr, "MidiSyncContainer::mtcSyncMsg starting transport.\n");
        MusEGlobal::audioDevice->startTransport();
        return;
      }

      /*if (tempoSN != MusEGlobal::tempomap.tempoSN()) { DELETETHIS 13
            double cpos    = MusEGlobal::tempomap.tick2time(_midiTick, 0);
            samplePosStart = samplePos - lrint(cpos * MusEGlobal::sampleRate);
            rtcTickStart   = rtcTick - lrint(cpos * realRtcTicks);
            tempoSN        = MusEGlobal::tempomap.tempoSN();
            }*/

      //
      // diff is the time in sec MusE is out of sync
      //
      /*double diff = time - (double(samplePosStart)/double(MusEGlobal::sampleRate));
      if (MusEGlobal::debugSync)
            printf("   state %d diff %f\n", MusEGlobal::mtcState, diff);
      */
      }

//---------------------------------------------------------
//   setSyncRecFilterPresetArrays
//   To be called in realtime thread only.
//---------------------------------------------------------
void MidiSyncContainer::setSyncRecFilterPresetArrays()
{
  switch(_syncRecFilterPreset)
  {
    // NOTE: Max _clockAveragerPoles is 16 and maximum stages is 48 per pole !
    case MidiSyncInfo::NONE:
      _clockAveragerPoles = 0;
      _preDetect = false;
    break;
    case MidiSyncInfo::TINY:
      _clockAveragerPoles = 2;
      _clockAveragerStages[0] = 4;
      _clockAveragerStages[1] = 4;
      _preDetect = false;
    break;
    case MidiSyncInfo::SMALL:
      _clockAveragerPoles = 3;
      _clockAveragerStages[0] = 12;
      _clockAveragerStages[1] = 8;
      _clockAveragerStages[2] = 4;
      _preDetect = false;
    break;
    case MidiSyncInfo::MEDIUM:
      _clockAveragerPoles = 3;
      _clockAveragerStages[0] = 28;
      _clockAveragerStages[1] = 12;
      _clockAveragerStages[2] = 8;
      _preDetect = false;
    break;
    case MidiSyncInfo::LARGE:
      _clockAveragerPoles = 4;
      _clockAveragerStages[0] = 48;
      _clockAveragerStages[1] = 48;
      _clockAveragerStages[2] = 48;
      _clockAveragerStages[3] = 48;
      _preDetect = false;
    break;
    case MidiSyncInfo::LARGE_WITH_PRE_DETECT:
      _clockAveragerPoles = 4;
      _clockAveragerStages[0] = 8;
      _clockAveragerStages[1] = 48;
      _clockAveragerStages[2] = 48;
      _clockAveragerStages[3] = 48;
      _preDetect = true;
    break;

    default:
      fprintf(stderr, "MidiSyncContainer::setSyncRecFilterPresetArrays unknown preset type:%d\n", (int)_syncRecFilterPreset);
  }
}

//---------------------------------------------------------
//   setSyncRecFilterPreset
//   To be called in realtime thread only.
//---------------------------------------------------------
void MidiSyncContainer::setSyncRecFilterPreset(MidiSyncInfo::SyncRecFilterPresetType type)
{
  _syncRecFilterPreset = type;
  setSyncRecFilterPresetArrays();
  alignAllTicks();
}


} // namespace MusECore
