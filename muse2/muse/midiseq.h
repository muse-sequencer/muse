//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midiseq.h,v 1.6.2.11 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2003 Werner Schweer (ws@seh.de)
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

#ifndef __MIDISEQ_H__
#define __MIDISEQ_H__

#include "thread.h"
#include "mpevent.h"
#include "driver/alsatimer.h"
#include "driver/rtctimer.h"
#include "sync.h"

namespace MusECore {

class MidiDevice;
class MidiPort;
class MPEventList;
class MTC;
class SynthI;

//---------------------------------------------------------
//   MidiSeq
//---------------------------------------------------------

class MidiSeq : public Thread {
      int timerFd;
      int idle;
      int prio;   // realtime priority
      int midiClock;
      static int ticker;

/* Testing */
      bool playStateExt;       // used for keeping play state in sync functions
      int recTick;            // ext sync tick position
      double mclock1, mclock2;
      double songtick1, songtick2;
      int recTick1, recTick2;
      int lastTempo;
      double timediff[16][48];
      int storedtimediffs;
      int    _avgClkDiffCounter[16];
      double _lastRealTempo;
      bool _averagerFull[16];
      int _clockAveragerPoles;
      int* _clockAveragerStages; 
      bool _preDetect;
      double _tempoQuantizeAmount;
      MidiSyncInfo::SyncRecFilterPresetType _syncRecFilterPreset;
      
      void alignAllTicks(int frameOverride = 0);
/* Testing */

      Timer *timer;

      signed int selectTimer();
      int setRtcTicks();
      static void midiTick(void* p, void*);
      void processTimerTick();
      void processSeek();
      void processStop();
      void processMidiClock();
      virtual void processMsg(const ThreadMsg*);
      void updatePollFd();

      void mtcSyncMsg(const MTC&, int, bool);

   public:
      MidiSeq(const char* name);
      
      ~MidiSeq();
      
      virtual void start(int);
      
      virtual void threadStop();
      virtual void threadStart(void*);

      bool externalPlayState() const { return playStateExt; }
      void setExternalPlayState(bool v) { playStateExt = v; }
      void realtimeSystemInput(int port, int type, double time = 0.0);
      void mtcInputQuarter(int, unsigned char);
      void setSongPosition(int, int);
      void mmcInput(int, const unsigned char*, int);
      void mtcInputFull(int, const unsigned char*, int);
      void nonRealtimeSystemSysex(int, const unsigned char*, int);
      void checkAndReportTimingResolution();
      MidiSyncInfo::SyncRecFilterPresetType syncRecFilterPreset() const { return _syncRecFilterPreset; }
      void setSyncRecFilterPreset(MidiSyncInfo::SyncRecFilterPresetType type);
      double recTempoValQuant() const { return _tempoQuantizeAmount; }
      void setRecTempoValQuant(double q) { _tempoQuantizeAmount = q; }

      void msgMsg(int id);
      void msgSeek();
      void msgStop();
      void msgSetRtc();
      void msgUpdatePollFd();
      void msgAddSynthI(SynthI* synth);
      void msgRemoveSynthI(SynthI* synth);
      void msgSetMidiDevice(MidiPort*, MidiDevice*);
      };

} //namespace MusECore

namespace MusEGlobal {
extern MusECore::MidiSeq* midiSeq;
extern volatile bool midiBusy;
} // namespace MusEGlobal

#endif

