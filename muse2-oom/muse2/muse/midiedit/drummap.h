//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: drummap.h,v 1.3.2.3 2009/10/29 02:14:37 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __DRUMMAP_H__
#define __DRUMMAP_H__

class QString;

class Xml;

//---------------------------------------------------------
//   DrumMap
//---------------------------------------------------------

struct DrumMap {
      QString name;
      unsigned char vol;            // playback volume, percent.
      int quant;
      int len;                      // len of event in ticks
      int channel;                  // midi channel
      int port;                     // midi port
      char lv1, lv2, lv3, lv4;      // velocities
      char enote, anote;            // input note - output note
      bool mute;
//      bool selected;

      //bool const operator==(const DrumMap& map) const;
      bool operator==(const DrumMap& map) const;
      };

#define DRUM_MAPSIZE  128

extern char drumOutmap[DRUM_MAPSIZE];
extern char drumInmap[DRUM_MAPSIZE];
extern DrumMap drumMap[DRUM_MAPSIZE];
extern void initDrumMap();
extern void writeDrumMap(int level, Xml& xml, bool external);
extern void readDrumMap(Xml& xml, bool external);
extern void resetGMDrumMap();

#endif

