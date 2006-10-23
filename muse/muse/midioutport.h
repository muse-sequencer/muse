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

//---------------------------------------------------------
//   MidiOutPort
//---------------------------------------------------------

class MidiOutPort : public MidiTrackBase {
      Q_OBJECT

      MidiInstrument* _instrument;
      MidiChannel* _channel[MIDI_CHANNELS];

      bool _sendSync;   // this port sends mtc mmc events
      int _deviceId;    // 0-126; 127 == all

      MPEventList _playEvents;   // scheduled events to play
      iMPEvent _nextPlayEvent;

      // fifo for midi events send from gui
      // direct to midi port:

      MidiFifo eventFifo;

      void routeEvent(const MidiEvent&);

   signals:
      void instrumentChanged();
      void sendSyncChanged(bool);

   public:
      MidiOutPort();
      ~MidiOutPort();

      MidiChannel* channel(int n)         { return _channel[n]; }

      virtual void setName(const QString& s);
      virtual void write(Xml&) const;
      virtual void read(QDomNode);
      virtual Track* newTrack() const     { return new MidiOutPort(); }
      virtual bool isMute() const         { return _mute; }
      virtual Part* newPart(Part*, bool)  { return 0; }

      MidiInstrument* instrument() const        { return _instrument; }
      void setInstrument(MidiInstrument* i);

      bool guiVisible() const;
      bool hasGui() const;

      void putEvent(const MidiEvent&);

      MPEventList* playEvents()          { return &_playEvents;   }
      iMPEvent nextPlayEvent()           { return _nextPlayEvent; }

      void process(unsigned from, unsigned to, const AL::Pos&, unsigned frames);

      void setNextPlayEvent(iMPEvent i)  { _nextPlayEvent = i;    }
      void playFifo();
      void playMidiEvent(MidiEvent*);

      void sendSysex(const unsigned char*, int);
      void sendSongpos(int);
      void sendGmOn();
      void sendGsOn();
      void sendXgOn();
      void sendStart();
      void sendStop();
      void sendContinue();
      void sendClock();

      void playEventList();

      bool sendSync() const      { return _sendSync; }
      void setSendSync(bool val);

      int deviceId() const      { return _deviceId; }
      void setDeviceId(int val) { _deviceId = val; }
      };

#endif

