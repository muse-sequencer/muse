//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __MIDISEQ_H__
#define __MIDISEQ_H__

#include "thread.h"
#include "mpevent.h"

#include "driver/timerdev.h"
namespace AL {
      class Pos;
      };
class AL::Pos;

class MPEventList;
class SynthI;
class MTC;

//---------------------------------------------------------
//   MidiSeq
//---------------------------------------------------------

class MidiSeq : public Thread {
      int realRtcTicks;
      Timer* timer;
      int idle;
      int midiClock;

/* Testing */
      int recTick;            // ext sync tick position
      int lastTickPos;        // position of last sync tick
      // run values:
      unsigned _midiTick;
      double mclock1, mclock2;
      double songtick1, songtick2;
      int recTick1, recTick2;
      int lastTempo;
      double timediff[24];
      int storedtimediffs;
/* Testing */

      bool initRealtimeTimer();
      static void midiTick(void* p, void*);
      void processTimerTick();
      void processSeek();
      void processStart();
      void processStop();
      void resetDevices();
      void processMidiClock();
      virtual void processMsg(const ThreadMsg*);
      void updatePollFd();

      void mtcSyncMsg(const MTC& mtc, bool seekFlag);
      void mtcInputFull(const unsigned char* p, int n);
      void nonRealtimeSystemSysex(const unsigned char* p, int n);

   public:
      MidiSeq(const char* name);
      bool start(int);
      virtual void threadStop();
      virtual void threadStart(void*);

      void realtimeSystemInput(int, int);
      void mtcInputQuarter(int, unsigned char);
      void setSongPosition(int, int);
      void mmcInput(int id, int cmd, const AL::Pos&);

      void msgMsg(int id);
      void msgProcess(unsigned frames);
      void msgSeek();
      void msgStart();
      void msgStop();
      void msgSetRtc();
      void msgAddSynthI(SynthI* synth);
      void msgRemoveSynthI(SynthI* synth);
      };

extern MidiSeq* midiSeq;
#endif

