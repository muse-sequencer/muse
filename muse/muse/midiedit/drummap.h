//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: drummap.h,v 1.10 2006/01/27 21:12:10 wschweer Exp $
//
//  (C) Copyright 1999-2006 Werner Schweer (ws@seh.de)
//=========================================================

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

