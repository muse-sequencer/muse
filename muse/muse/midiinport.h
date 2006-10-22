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

#ifndef __MIDIINPORT_H__
#define __MIDIINPORT_H__

#include "track.h"

//---------------------------------------------------------
//   MidiInPort
//---------------------------------------------------------

class MidiInPort : public MidiTrackBase {
      Q_OBJECT

      MPEventList _recordEvents;

   public:
      MidiInPort();
      ~MidiInPort();

      virtual void setName(const QString& s);
      virtual void write(Xml&) const;
      virtual void read(QDomNode);
      virtual Track* newTrack() const     { return new MidiInPort(); }
      virtual bool isMute() const         { return _mute; }
      virtual Part* newPart(Part*, bool)  { return 0; }

#ifndef __APPLE__      
      void eventReceived(snd_seq_event_t*);
#endif
      virtual void getEvents(unsigned from, unsigned to, int channel, MPEventList* dst);
      void afterProcess();
      };

#endif

