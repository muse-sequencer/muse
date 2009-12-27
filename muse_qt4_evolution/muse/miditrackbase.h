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

#ifndef __MIDITRACKBASE_H__
#define __MIDITRACKBASE_H__

#include "track.h"

//---------------------------------------------------------
//   MidiTrackBase
//---------------------------------------------------------

class MidiTrackBase : public Track {
      Q_OBJECT

      MidiPipeline* _pipeline;

   public:
      MidiTrackBase();
      virtual ~MidiTrackBase();

      bool readProperties(QDomNode);
      void writeProperties(Xml&) const;

      MidiPipeline* pipeline()      { return _pipeline;  }
      void addPlugin(MidiPluginI* plugin, int idx);
      MidiPluginI* plugin(int idx) const;

      virtual void processMidi(unsigned, unsigned, unsigned, unsigned) {}
      virtual void getEvents(unsigned /*from*/, unsigned /*to*/, int /*channel*/, MidiEventList* /*dst*/) {}
      };

#endif



