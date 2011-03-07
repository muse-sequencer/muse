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

#include "miditrackbase.h"
#include "midififo.h"
#include "event.h"

class Part;
class EventList;
class MidiOut;
class SeqTime;

//---------------------------------------------------------
//   MidiTrack
//---------------------------------------------------------

class MidiTrack : public MidiTrackBase {
      Q_OBJECT

      EventList _events;         // tmp Events during midi import

      // recording:
      MidiFifo recordFifo;       // for event transfer from RT-thread to gui thread
      std::list<Event> keyDown;  // keep track of "note on" events
      Part* recordPart;          // part we are recording into
      int recordedEvents;
      bool partCreated;
      int hbank, lbank;
      int datah, datal;
      int rpnh, rpnl;
      int dataType;

      // channel data:
      DrumMap* _drumMap;
      bool _useDrumMap;

      // play parameter
      int _transposition;
      int _velocity;
      int _delay;
      int _len;
      int _compression;

      MidiEventList schedEvents;  // scheduled events by process()

   signals:
      void drumMapChanged() const;
      void useDrumMapChanged(bool);
      void channelChanged(int);

   public slots:
      void setChannel(int);

   public:
      MidiTrack();
      virtual ~MidiTrack();
      virtual TrackType type() const { return MIDI; }
      void clone(MidiTrack*);

      int transposition() const      { return _transposition; }
      int velocity() const           { return _velocity;     }
      int delay() const              { return _delay;        }
      int len() const                { return _len;          }
      int compression() const        { return  _compression; }
      void setTransposition(int val) { _transposition = val; }
      void setVelocity(int val)      { _velocity      = val; }
      void setDelay(int val)         { _delay         = val; }
      void setLen(int val)           { _len           = val; }
      void setCompression(int val)   { _compression   = val; }

      void startRecording();
      void recordBeat();
      void stopRecording();

      EventList* events()             { return &_events;   }

      virtual void read(QDomNode);
      virtual void write(Xml&) const;

      virtual Part* newPart(Part*p=0, bool clone=false);

      virtual bool isMute() const;
      virtual bool canRecord() const      { return true; }
      void playMidiEvent(MidiEvent*);

      virtual void processMidi(SeqTime*);
      virtual void getEvents(unsigned from, unsigned to, int channel, MidiEventList* dst);

      bool useDrumMap() const             { return _useDrumMap;    }
      DrumMap* drumMap() const            { return _drumMap; }
      void setUseDrumMap(bool val);

      int channelNo() const;
      virtual void emitControllerChanged(int id);

      virtual MidiOut* midiOut();
      virtual MidiInstrument* instrument();
      };

typedef QList<MidiTrack*> MidiTrackList;
typedef MidiTrackList::iterator iMidiTrack;
typedef MidiTrackList::const_iterator ciMidiTrack;

Q_DECLARE_METATYPE(class MidiTrack*);

#endif

