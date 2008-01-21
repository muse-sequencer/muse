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

#ifndef __MIDIFIFO_H__
#define __MIDIFIFO_H__

#include "midievent.h"
#include "driver/port.h"

#define MIDI_FIFO_SIZE    512

//---------------------------------------------------------
//   MidiFifo
//---------------------------------------------------------

class MidiFifo {
      MidiEvent fifo[MIDI_FIFO_SIZE];
      volatile int size;
      int wIndex;
      int rIndex;

   public:
      MidiFifo()  { clear(); }
      bool put(const MidiEvent& event);   // returns true on fifo overflow
      MidiEvent get();
      const MidiEvent& peek(int n = 0);
      void remove();
      bool isEmpty() const { return size == 0; }
      void clear()         { size = 0, wIndex = 0, rIndex = 0; }
      int getSize() const  { return size; }
      };

//---------------------------------------------------------
//   MidiOutEvent
//---------------------------------------------------------

struct MidiOutEvent {
      Port port;
      MidiEvent event;

      MidiOutEvent() {}
      MidiOutEvent(const Port& p, const MidiEvent& e)
         : port(p), event(e) {}
      bool operator<(const MidiOutEvent& e) const {
            if (port == e.port)
                  return event < e.event;
            return event < e.event;
            }
      };

typedef std::multiset<MidiOutEvent, std::less<MidiOutEvent> > MidiOutEventList;
typedef MidiOutEventList::iterator iMidiOutEvent;
typedef MidiOutEventList::const_iterator ciMidiOutEvent;

//---------------------------------------------------------
//   MidiOutFifo
//---------------------------------------------------------

class MidiOutFifo {
      MidiOutEvent fifo[MIDI_FIFO_SIZE];
      volatile int size;
      int wIndex;
      int rIndex;

   public:
      MidiOutFifo()  { clear(); }
      bool put(const MidiOutEvent& event);   // returns true on fifo overflow
      MidiOutEvent get();
      bool isEmpty() const { return size == 0; }
      void clear()         { size = 0, wIndex = 0, rIndex = 0; }
      };


#endif

