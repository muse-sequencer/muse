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
      MidiFifo();
      bool put(const MidiEvent& event);   // returns true on fifo overflow
      MidiEvent get();
      bool isEmpty() const { return size == 0; }
      void clear()         { size = 0, wIndex = 0, rIndex = 0; }
      };

#endif

