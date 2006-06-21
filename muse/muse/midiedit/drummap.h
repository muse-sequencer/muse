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

#ifndef __DRUMMAP_H__
#define __DRUMMAP_H__

namespace AL {
      class Xml;
      };
using AL::Xml;

const int DRUM_MAPSIZE = 128;

//---------------------------------------------------------
//   DrumMapEntry
//---------------------------------------------------------

struct DrumMapEntry {
      QString name;
      int quant;
      int len;                      // len of event in ticks
      int channel;                  // midi channel
      char lv1, lv2, lv3, lv4;      // velocities
      char enote, anote;            // input note - output note
      bool mute;

      void read(QDomNode node);
      };

//---------------------------------------------------------
//   DrumMap
//---------------------------------------------------------

class DrumMap {
      QString _name;
      DrumMapEntry map[DRUM_MAPSIZE];
      char _outmap[DRUM_MAPSIZE];
      char _inmap[DRUM_MAPSIZE];

   public:
      DrumMap(const QString& name);
      void init();
      void write(Xml& xml);
      void read(QDomNode);
      void initGm();
      DrumMapEntry* entry(int idx) { return &map[idx];      }
      QString name() const         { return _name;           }
      QString name(int i) const    { return map[i].name;    }
      int quant(int i) const       { return map[i].quant;   }
      int len(int i) const         { return map[i].len;     }
      int channel(int i) const     { return map[i].channel; }
      int enote(int i) const       { return map[i].enote;   }
      int anote(int i) const       { return map[i].anote;   }
      bool mute(int i) const       { return map[i].mute;    }
      int inmap(int i) const       { return _inmap[i];      }
      int outmap(int i) const      { return _outmap[i];     }
      };

extern DrumMap gmDrumMap;
extern DrumMap noDrumMap;

#endif

