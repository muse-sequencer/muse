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

class MPEventList;
class SynthI;
class MTC;
class MidiPort;
class MidiDevice;

//---------------------------------------------------------
//   MidiSeq
//---------------------------------------------------------

class MidiSeq : public Thread {
      int realRtcTicks;
      int timerFd;
      int idle;
      int prio;   // realtime priority
      int midiClock;
      static int ticker;

/* Testing */
      bool playStateExt;       // used for keeping play state in sync functions
      int recTick;            // ext sync tick position
//      int lastTickPos;        // position of last sync tick
      // run values:
//      unsigned _midiTick;
      double mclock1, mclock2;
      double songtick1, songtick2;
      int recTick1, recTick2;
      int lastTempo;
      double timediff[24];
      int storedtimediffs;

      void alignAllTicks(int frameOverride = 0);
/* Testing */

      Timer *timer;

      signed int selectTimer();
      bool setRtcTicks();
      static void midiTick(void* p, void*);
      void processTimerTick();
      void processSeek();
      void processStop();
      void processMidiClock();
      virtual void processMsg(const ThreadMsg*);
      void updatePollFd();

      void mtcSyncMsg(const MTC&, int, bool);
      //void mtcInputFull(const unsigned char* p, int n);
      //void nonRealtimeSystemSysex(const unsigned char* p, int n);

   public:
      //MidiSeq(int prio, const char* name);
      MidiSeq(const char* name);
      
      ~MidiSeq();
      
      //bool start();
      virtual void start(int);
      
      virtual void threadStop();
      virtual void threadStart(void*);

      bool externalPlayState() const { return playStateExt; }
      void setExternalPlayState(bool v) { playStateExt = v; }
      void realtimeSystemInput(int, int);
      void mtcInputQuarter(int, unsigned char);
      void setSongPosition(int, int);
      // void eventReceived(MidiRecordEvent& event);
      //void mmcInput(const unsigned char* p, int n);
      void mmcInput(int, const unsigned char*, int);
      void mtcInputFull(int, const unsigned char*, int);
      void nonRealtimeSystemSysex(int, const unsigned char*, int);

      void msgMsg(int id);
      //void msgProcess();
      //void msgSeek();
      //void msgStop();
      void msgSetRtc();
      void msgUpdatePollFd();
      void msgAddSynthI(SynthI* synth);
      void msgRemoveSynthI(SynthI* synth);
      void msgSetMidiDevice(MidiPort*, MidiDevice*);
      };

extern MidiSeq* midiSeq;
extern volatile bool midiBusy;
#endif

