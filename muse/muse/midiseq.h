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
#include "driver/timerdev.h"
#include "midififo.h"

//---------------------------------------------------------
//   MidiSeq
//---------------------------------------------------------

class MidiSeq : public Thread {
      int realRtcTicks;
      Timer* timer;

      MidiOutFifo fifo;
      MidiOutEventList playEvents;

      static void midiTick(void* p, void*);
      int getTimerTicks() { return timer->getTimerTicks(); }

   public:
      MidiSeq(const char* name);
      bool start(int);
      virtual void threadStop();
      virtual void threadStart(void*);
      void updatePollFd();
      bool initRealtimeTimer();
      void putEvent(const Port& p, const MidiEvent& e) { 
            fifo.put(MidiOutEvent(p,e)); 
            }
      };

extern MidiSeq* midiSeq;
#endif

