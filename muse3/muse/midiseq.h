//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midiseq.h,v 1.6.2.11 2009/12/20 05:00:35 terminator356 Exp $
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

#ifndef __MIDISEQ_H__
#define __MIDISEQ_H__

#include "thread.h"

namespace MusECore {

class MidiDevice;
class MidiPort;
class MPEventList;
class MTC;
class SynthI;
class Timer;

//---------------------------------------------------------
//   MidiSeq
//---------------------------------------------------------

class MidiSeq : public Thread {
      int timerFd;
// REMOVE Tim. autoconnect. Added.      
      int toThreadFdwNonWait; // Non-waiting message to thread (app write)
      int toThreadFdrNonWait; // Non-waiting message to thread (seq read)
      
      int idle;
      int prio;   // realtime priority
      static int ticker;
      Timer *timer;

      int setRtcTicks();
      static void midiTick(void* p, void*);
      void processTimerTick();
      void processSeek();
      void processStop();
      virtual void processMsg(const ThreadMsg*);
// REMOVE Tim. autoconnect. Added.      
      virtual void processMsg1(const void*);
      void updatePollFd();

   public:
      MidiSeq(const char* name);
      
      ~MidiSeq();
      
      virtual void start(int, void* ptr=0);
      
      virtual void threadStop();
      virtual void threadStart(void*);
      signed int selectTimer();
      // Destroy timer if valid. Returns true if successful.
      bool deleteTimer();
      void addAlsaPollFd();
      void removeAlsaPollFd();
      bool isIdle() const { return idle; }

      void checkAndReportTimingResolution();

      void msgMsg(int id);
      void msgSeek();
      void msgStop();
      void msgSetRtc();
      void msgUpdatePollFd();
      };


extern void initMidiSequencer();
extern void exitMidiSequencer();

} //namespace MusECore

namespace MusEGlobal {
extern MusECore::MidiSeq* midiSeq;
// REMOVE Tim. autoconnect. Removed.
// extern volatile bool midiBusy;
} // namespace MusEGlobal

#endif

