//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: drummap.h,v 1.3.2.3 2009/10/29 02:14:37 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//  (C) Copyright 2016 Tim E. Real (terminator356 on users dot sourceforge dot net)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#ifndef __DRUMMAP_H__
#define __DRUMMAP_H__

#include <QString>
#include <QList>
#include <map>

namespace MusECore {

class Xml;

//---------------------------------------------------------
//   DrumMap
//---------------------------------------------------------

struct DrumMap {
      QString name;
      unsigned char vol;            // playback volume, percent.
      int quant;
      int len;                      // len of event in ticks

      // Default to track port if -1 and track channel if -1.
      int channel;                  // midi channel
      int port;                     // midi port

      char lv1, lv2, lv3, lv4;      // velocities
      char enote, anote;            // input note - output note
      bool mute;
      bool hide;

      // Initializes the structure. NOTE: Be sure the enote value
      //  does not conflict with another map item's enote value.
      void init() {
        vol = 100;
        quant = 16;
        len = 32;
        channel = -1;
        port = -1;
        lv1 = 70;
        lv2 = 90;
        lv3 = 110;
        lv4 = 127;
        enote = 0;
        anote = 0;
        mute = false;
      };

      bool operator==(const DrumMap& map) const;
      bool operator!=(const DrumMap& map) const { return !operator==(map); }
      bool almost_equals(const DrumMap& map) const;

      void dump();
      };

// please let this at "128". idrumMap should have length 128 (see drummap.cpp for details)
#define DRUM_MAPSIZE  128

extern DrumMap iNewDrumMap[128];
extern void initNewDrumMap();

extern void clearDrumMap();  // One-time only early init
extern void initDrumMap();
extern void writeDrumMap(int level, Xml& xml, bool external);
extern void readDrumMap(Xml& xml, bool external);
extern void resetGMDrumMap();

class MidiTrack;
} // namespace MusECore

namespace MusEGlobal {
extern char drumOutmap[DRUM_MAPSIZE];
extern char drumInmap[DRUM_MAPSIZE];
extern MusECore::DrumMap drumMap[DRUM_MAPSIZE];


class global_drum_ordering_t : public QList< std::pair<MusECore::MidiTrack*,int> >
{
  public:
    void cleanup();
    void write(int level, MusECore::Xml& xml);
    void read(MusECore::Xml& xml);
  
  private:
    typedef std::pair<MusECore::MidiTrack*,int> entry_t;
    
    void write_single(int level, MusECore::Xml& xml, const entry_t& entry);
    entry_t read_single(MusECore::Xml& xml);
};

extern global_drum_ordering_t global_drum_ordering;

}

#endif

