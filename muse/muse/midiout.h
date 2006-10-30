//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2006 by Werner Schweer and others
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

#ifndef __MIDIOUT_H__
#define __MIDIOUT_H__

#include "al/al.h"
#include "globaldefs.h"
#include "midievent.h"
#include "midififo.h"

class Track;
class MidiInstrument;
class MidiChannel;

//---------------------------------------------------------
//    MidiOut
//---------------------------------------------------------

class MidiOut
      {
   public:
      Track* track;
      MidiInstrument* _instrument;
      MidiChannel* _channel[MIDI_CHANNELS];
      MPEventList _schedEvents;  // scheduled events by process()

      // fifo for midi events send from gui
      // direct to midi port:

      MidiFifo eventFifo;

      void processMidi(MPEventList& el, unsigned fromTick, unsigned toTick, 
         unsigned fromFrame, unsigned toFrame);
      MidiChannel* channel(int n)           { return _channel[n]; }
      MidiInstrument* instrument()          { return _instrument; }
      void setInstrument(MidiInstrument* i) { _instrument = i; }

      void seek(unsigned, unsigned);
      void stop();
      void start();
      void reset();

      void sendSysex(const unsigned char*, int);
      void sendSongpos(int);
      void sendGmOn();
      void sendGsOn();
      void sendXgOn();
      void sendStart();
      void sendStop();
      void sendContinue();
      void sendClock();
      void playMidiEvent(MidiEvent* ev);
      };

#endif

