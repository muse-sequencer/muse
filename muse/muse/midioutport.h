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

#ifndef __MIDIOUTPORT_H__
#define __MIDIOUTPORT_H__

#include "track.h"
#include "midiout.h"

//---------------------------------------------------------
//   MidiOutPort
//---------------------------------------------------------

class MidiOutPort : public MidiTrackBase, public MidiOut {
      Q_OBJECT

      MPEventList _playEvents;   // event queue for MidiSeq

      void routeEvent(const MidiEvent&);
      void queueAlsaEvent(const MidiEvent& event);
      void queueJackEvent(const MidiEvent& event);

   signals:
      void instrumentChanged();

   public:
      MidiOutPort();
      ~MidiOutPort();
      virtual TrackType type() const { return MIDI_OUT; }

      MidiChannel* channel(int n)         { return _channel[n]; }

      virtual void setName(const QString& s);
      virtual void write(Xml&) const;
      virtual void read(QDomNode);
      virtual bool isMute() const         { return _mute; }
      virtual Part* newPart(Part*, bool)  { return 0; }

      MidiInstrument* instrument() const  { return _instrument; }
      void setInstrument(MidiInstrument* i);

      bool guiVisible() const;
      bool hasGui() const;

      MPEventList* playEvents()          { return &_playEvents;   }

      void playAlsaEvent(const MidiEvent& event) const;
      void processMidi(unsigned fromTick, unsigned toTick, unsigned fromFrame, unsigned toFrame);
      };

#endif

