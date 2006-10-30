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

#ifndef __MIDISYNTH_H__
#define __MIDISYNTH_H__

#include "track.h"

//---------------------------------------------------------
//   MidiSynti
//---------------------------------------------------------

class MidiSynti : public MidiTrackBase {
      Q_OBJECT

      MidiPluginI* _synti;

   public:
      MidiSynti();
      virtual ~MidiSynti();
      virtual TrackType type() const { return MIDI_SYNTI; }

      void init();

      virtual void read(QDomNode);
      virtual void write(Xml&) const;
      virtual bool isMute() const;

      virtual Part* newPart(Part*, bool) { return 0; }

      bool initInstance(MidiPlugin*);
      virtual void getEvents(unsigned from, unsigned to, int channel, MPEventList* dst);

      bool hasGui() const;
      bool guiVisible() const;
      void showGui(bool);
      };


typedef tracklist<MidiSynti*>::iterator iMidiSynti;
typedef tracklist<MidiSynti*>::const_iterator ciMidiSynti;
typedef tracklist<MidiSynti*> MidiSyntiList;

#endif

