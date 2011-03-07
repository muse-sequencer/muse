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

#ifndef __MIDIRC_H__
#define __MIDIRC_H__

#include "event.h"
#include "midievent.h"

enum {
      RC_STOP, RC_PLAY, RC_RECORD, RC_GOTO_LEFT_MARK
      };

//---------------------------------------------------------
//   MidiRC
//---------------------------------------------------------

struct MidiRC {
      MidiEvent event;
      int action;

      MidiRC(const MidiEvent& e, int a)
         : event(e), action(a)
            {
            }
      };

//---------------------------------------------------------
//   MidiRCList
//---------------------------------------------------------

class MidiRCList : public std::list<MidiRC> {

      void emitAction(int) const;

   public:
      MidiRCList() {}
      void setAction(const MidiEvent& e, int action);
      bool isActive(int action);
      bool doAction(const MidiEvent&);
      void read(QDomNode);
      void write(Xml&);
      };

typedef MidiRCList::iterator iMidiRC;

extern MidiRCList midiRCList;
#endif
