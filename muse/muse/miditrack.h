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

#ifndef __MIDITRACK_H__
#define __MIDITRACK_H__

#include "track.h"
#include "midififo.h"

class Part;
class EventList;

//---------------------------------------------------------
//   MidiTrack
//---------------------------------------------------------

class MidiTrack : public MidiTrackBase {
      Q_OBJECT

      EventList* _events;     // tmp Events during midi import

      // recording:
      MidiFifo recordFifo;       // for event transfer from RT-thread to gui thread
      std::list<Event> keyDown;  // keep track of "note on" events
      Part* recordPart;      // part we are recording into
      int recordedEvents;
      bool partCreated;
      int hbank, lbank;
      int datah, datal;
      int rpnh, rpnl;
      int dataType;

   signals:
      void drumMapChanged() const;

   public:
      MidiTrack();
      virtual ~MidiTrack();
      void init();
      void clone(MidiTrack*);
      MidiChannel* channel() const;

      void changeDrumMap() const;

      // play parameter
      int transposition;
      int velocity;
      int delay;
      int len;
      int compression;

      void startRecording();
      void recordBeat();
      void stopRecording();

      EventList* events() const           { return _events;   }

      virtual void read(QDomNode);
      virtual void write(Xml&) const;

      virtual MidiTrack* newTrack() const { return new MidiTrack(); }
      virtual Part* newPart(Part*p=0, bool clone=false);

      virtual bool isMute() const;
      virtual bool canRecord() const      { return true; }
      void playMidiEvent(MidiEvent*);

      virtual void getEvents(unsigned from, unsigned to, int channel, MPEventList* dst);
      bool useDrumMap() const;
      DrumMap* drumMap() const;
      };

typedef tracklist<MidiTrack*>::iterator iMidiTrack;
typedef tracklist<MidiTrack*>::const_iterator ciMidiTrack;
typedef tracklist<MidiTrack*> MidiTrackList;

#endif

