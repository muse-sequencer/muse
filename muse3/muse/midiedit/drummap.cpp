//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: drummap.cpp,v 1.3.2.6 2009/10/29 02:14:37 terminator356 Exp $
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

#include "audio.h"
#include "drummap.h"
#include "xml.h"
#include "song.h"

#include "helper.h"

#include <QSet>

namespace MusEGlobal {
char drumOutmap[DRUM_MAPSIZE];
char drumInmap[128];
MusECore::DrumMap drumMap[DRUM_MAPSIZE];
global_drum_ordering_t global_drum_ordering;
}

namespace MusECore {

//---------------------------------------------------------
//    GM default drum map
//---------------------------------------------------------

// Default to track port if -1 and track channel if -1.  (These used to say 9, 0 for chan, port).
const DrumMap blankdm = { QString(""), 100, 16, 32, -1, -1, 70, 90, 110, 127, 127, 127, false, false };

// this map should have 128 entries, as it's used for initialising iNewDrumMap as well.
// iNewDrumMap only has 128 entries. also, the every "out-note" ("anote") should be
// represented exactly once in idrumMap, and there shall be no duplicate or unused
// "out-notes".
// reason: iNewDrumMap is inited as follows: iterate through the full idrumMap[],
//         iNewDrumMap[ idrumMap[i].anote ] = idrumMap[i]
// if you ever want to change this, you will need to fix the initNewDrumMap() function.
const DrumMap idrumMap[DRUM_MAPSIZE] = {
      { QString("Acoustic Bass Drum"), 100, 16, 32, -1, -1, 70, 90, 110, 127, 35, 35, false, false },
      { QString("Bass Drum 1"),        100, 16, 32, -1, -1, 70, 90, 110, 127, 36, 36, false, false },
      { QString("Side Stick"),         100, 16, 32, -1, -1, 70, 90, 110, 127, 37, 37, false, false },
      { QString("Acoustic Snare"),     100, 16, 32, -1, -1, 70, 90, 110, 127, 38, 38, false, false },
      { QString("Hand Clap"),          100, 16, 32, -1, -1, 70, 90, 110, 127, 39, 39, false, false },
      { QString("Electric Snare"),     100, 16, 32, -1, -1, 70, 90, 110, 127, 40, 40, false, false },
      { QString("Low Floor Tom"),      100, 16, 32, -1, -1, 70, 90, 110, 127, 41, 41, false, false },
      { QString("Closed Hi-Hat"),      100, 16, 32, -1, -1, 70, 90, 110, 127, 42, 42, false, false },
      { QString("High Floor Tom"),     100, 16, 32, -1, -1, 70, 90, 110, 127, 43, 43, false, false },
      { QString("Pedal Hi-Hat"),       100, 16, 32, -1, -1, 70, 90, 110, 127, 44, 44, false, false },
      { QString("Low Tom"),            100, 16, 32, -1, -1, 70, 90, 110, 127, 45, 45, false, false },
      { QString("Open Hi-Hat"),        100, 16, 32, -1, -1, 70, 90, 110, 127, 46, 46, false, false },
      { QString("Low-Mid Tom"),        100, 16, 32, -1, -1, 70, 90, 110, 127, 47, 47, false, false },
      { QString("Hi-Mid Tom"),         100, 16, 32, -1, -1, 70, 90, 110, 127, 48, 48, false, false },
      { QString("Crash Cymbal 1"),     100, 16, 32, -1, -1, 70, 90, 110, 127, 49, 49, false, false },
      { QString("High Tom"),           100, 16, 32, -1, -1, 70, 90, 110, 127, 50, 50, false, false },

      { QString("Ride Cymbal 1"),      100, 16, 32, -1, -1, 70, 90, 110, 127, 51, 51, false, false },
      { QString("Chinese Cymbal"),     100, 16, 32, -1, -1, 70, 90, 110, 127, 52, 52, false, false },
      { QString("Ride Bell"),          100, 16, 32, -1, -1, 70, 90, 110, 127, 53, 53, false, false },
      { QString("Tambourine"),         100, 16, 32, -1, -1, 70, 90, 110, 127, 54, 54, false, false },
      { QString("Splash Cymbal"),      100, 16, 32, -1, -1, 70, 90, 110, 127, 55, 55, false, false },
      { QString("Cowbell"),            100, 16, 32, -1, -1, 70, 90, 110, 127, 56, 56, false, false },
      { QString("Crash Cymbal 2"),     100, 16, 32, -1, -1, 70, 90, 110, 127, 57, 57, false, false },
      { QString("Vibraslap"),          100, 16, 32, -1, -1, 70, 90, 110, 127, 58, 58, false, false },
      { QString("Ride Cymbal 2"),      100, 16, 32, -1, -1, 70, 90, 110, 127, 59, 59, false, false },
      { QString("Hi Bongo"),           100, 16, 32, -1, -1, 70, 90, 110, 127, 60, 60, false, false },
      { QString("Low Bongo"),          100, 16, 32, -1, -1, 70, 90, 110, 127, 61, 61, false, false },
      { QString("Mute Hi Conga"),      100, 16, 32, -1, -1, 70, 90, 110, 127, 62, 62, false, false },
      { QString("Open Hi Conga"),      100, 16, 32, -1, -1, 70, 90, 110, 127, 63, 63, false, false },
      { QString("Low Conga"),          100, 16, 32, -1, -1, 70, 90, 110, 127, 64, 64, false, false },
      { QString("High Timbale"),       100, 16, 32, -1, -1, 70, 90, 110, 127, 65, 65, false, false },
      { QString("Low Timbale"),        100, 16, 32, -1, -1, 70, 90, 110, 127, 66, 66, false, false },

      { QString("High Agogo"),         100, 16, 32, -1, -1, 70, 90, 110, 127, 67, 67, false, false },
      { QString("Low Agogo"),          100, 16, 32, -1, -1, 70, 90, 110, 127, 68, 68, false, false },
      { QString("Cabasa"),             100, 16, 32, -1, -1, 70, 90, 110, 127, 69, 69, false, false },
      { QString("Maracas"),            100, 16, 32, -1, -1, 70, 90, 110, 127, 70, 70, false, false },
      { QString("Short Whistle"),      100, 16, 32, -1, -1, 70, 90, 110, 127, 71, 71, false, false },
      { QString("Long Whistle"),       100, 16, 32, -1, -1, 70, 90, 110, 127, 72, 72, false, false },
      { QString("Short Guiro"),        100, 16, 32, -1, -1, 70, 90, 110, 127, 73, 73, false, false },
      { QString("Long Guiro"),         100, 16, 32, -1, -1, 70, 90, 110, 127, 74, 74, false, false },
      { QString("Claves"),             100, 16, 32, -1, -1, 70, 90, 110, 127, 75, 75, false, false },
      { QString("Hi Wood Block"),      100, 16, 32, -1, -1, 70, 90, 110, 127, 76, 76, false, false },
      { QString("Low Wood Block"),     100, 16, 32, -1, -1, 70, 90, 110, 127, 77, 77, false, false },
      { QString("Mute Cuica"),         100, 16, 32, -1, -1, 70, 90, 110, 127, 78, 78, false, false },
      { QString("Open Cuica"),         100, 16, 32, -1, -1, 70, 90, 110, 127, 79, 79, false, false },
      { QString("Mute Triangle"),      100, 16, 32, -1, -1, 70, 90, 110, 127, 80, 80, false, false },
      { QString("Open Triangle"),      100, 16, 32, -1, -1, 70, 90, 110, 127, 81, 81, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 82, 82, false, false },

      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 83, 83, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 84, 84, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 85, 85, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 86, 86, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 87, 87, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 88, 88, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 89, 89, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 90, 90, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 91, 91, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 92, 92, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 93, 93, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 94, 94, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 95, 95, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 96, 96, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 97, 97, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 98, 98, false, false },

      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 99, 99, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 100, 100, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 101, 101, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 102, 102, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 103, 103, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 104, 104, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 105, 105, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 106, 106, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 107, 107, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 108, 108, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 109, 109, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 110, 110, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 111, 111, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 112, 112, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 113, 113, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 114, 114, false, false },

      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 115, 115, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 116, 116, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 117, 117, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 118, 118, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 119, 119, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 120, 120, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 121, 121, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 122, 122, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 123, 123, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 124, 124, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 125, 125, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 126, 126, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 127, 127, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 0, 0, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 1, 1, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 2, 2, false, false },

      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 3, 3, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 4, 4, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 5, 5, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 6, 6, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 7, 7, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 8, 8, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 9, 9, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 10, 10, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 11, 11, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 12, 12, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 13, 13, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 14, 14, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 15, 15, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 16, 16, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 17, 17, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 18, 18, false, false },

      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 19, 19, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 20, 20, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 21, 21, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 22, 22, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 23, 23, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 24, 24, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 25, 25, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 26, 26, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 27, 27, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 28, 28, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 29, 29, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 30, 30, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 31, 31, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 32, 32, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 33, 33, false, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 34, 34, false, false }
      };
      
DrumMap iNewDrumMap[128];

void initNewDrumMap()
{
  bool done[128];
  for (int i=0;i<128;i++) done[i]=false;
  
  for (int i=0;i<DRUM_MAPSIZE;i++)
  {
    int idx=idrumMap[i].anote;
    if (idx < 0 || idx >= 128)
      fprintf(stderr, "ERROR: THIS SHOULD NEVER HAPPEN: idrumMap[%i].anote is not within 0..127!\n", idx);
    else
    {
      if (done[idx]==true)
      {
        fprintf(stderr, "ERROR: iNewDrumMap[%i] is already initialized!\n"
               "       this will be probably not a problem, but some programmer didn't read\n"
               "       flo's comment at drummap.cpp, above idrumMap[].\n", idx);
      }
      else
      {
        iNewDrumMap[idx]=idrumMap[i];
        done[idx]=true;
      }
    }
  }
  
  for (int i=0;i<128;i++)
  {
    if (done[i]==false)
    {
      fprintf(stderr, "ERROR: iNewDrumMap[%i] is uninitialized!\n"
             "       this will be probably not a problem, but some programmer didn't read\n"
             "       flo's comment at drummap.cpp, above idrumMap[].\n", i);
      iNewDrumMap[i].name="";
      iNewDrumMap[i].vol=100;
      iNewDrumMap[i].quant=16;
      iNewDrumMap[i].len=32;
      iNewDrumMap[i].lv1=70;
      iNewDrumMap[i].lv2=90;
      iNewDrumMap[i].lv3=127;
      iNewDrumMap[i].lv4=110;
      iNewDrumMap[i].enote=i;
      iNewDrumMap[i].anote=i;
      iNewDrumMap[i].mute=false;
      iNewDrumMap[i].hide=false;
    }
  }
}

//---------------------------------------------------------
//   clearDrumMap
//    One-time only early init
//---------------------------------------------------------

void clearDrumMap()
      {
      for (int i = 0; i < DRUM_MAPSIZE; ++i) {
            DrumMap& d = MusEGlobal::drumMap[i];
            d.vol = d.len = d.channel = d.port = d.lv1 = d.lv2 = d.lv3 = d.lv4 = d.enote = d.anote = 0;
            d.mute = d.hide = false;
         }
      }
//---------------------------------------------------------
//   initDrumMap
//    populate Inmap and Outmap
//---------------------------------------------------------

void initDrumMap()
      {
      for (int i = 0; i < DRUM_MAPSIZE; ++i) {
            DrumMap& d = MusEGlobal::drumMap[i];
            //Make sure we're not overwriting any values loaded
            //On init, all these values are zero. If so, just set the drummap entry to the initial drummap entry.
            if (!(d.vol || d.len || d.channel || d.port || d.lv1 || d.lv2 || d.lv3 || d.lv4 || d.enote || d.anote || d.mute || d.hide))
                  MusEGlobal::drumMap[i] = idrumMap[i];
         }
      //Finally, setup the inMap, outMap-values
      memset(MusEGlobal::drumInmap, 0, sizeof(MusEGlobal::drumInmap));
      memset(MusEGlobal::drumOutmap, 0, sizeof(MusEGlobal::drumOutmap));
      for (int i = 0; i < DRUM_MAPSIZE; ++i) {
            MusEGlobal::drumInmap[(unsigned int)(MusEGlobal::drumMap[i].enote)] = i;
            MusEGlobal::drumOutmap[(unsigned int)(MusEGlobal::drumMap[i].anote)] = i;
            }
      }

//---------------------------------------------------------
//   resetGMDrumMap
//---------------------------------------------------------

void resetGMDrumMap()
      {
      for(int i = 0; i < DRUM_MAPSIZE; ++i) 
        MusEGlobal::drumMap[i] = idrumMap[i];
      memset(MusEGlobal::drumInmap, 0, sizeof(MusEGlobal::drumInmap));
      memset(MusEGlobal::drumOutmap, 0, sizeof(MusEGlobal::drumOutmap));
      for (int i = 0; i < DRUM_MAPSIZE; ++i) {
            MusEGlobal::drumInmap[(unsigned int)(MusEGlobal::drumMap[i].enote)] = i;
            MusEGlobal::drumOutmap[(unsigned int)(MusEGlobal::drumMap[i].anote)] = i;
            }
      }

//---------------------------------------------------------
//   operator ==
//---------------------------------------------------------

bool DrumMap::operator==(const DrumMap& map) const
      {
        // Everything equal, including mute and hide settings?
        return almost_equals(map) &&
               mute == map.mute &&
               hide == map.hide;
      }

bool DrumMap::almost_equals(const DrumMap& map) const
{
      // Disregarding mute and hide settings.
      return
         name == map.name
         && vol == map.vol
         && quant == map.quant
         && len == map.len
         && channel == map.channel
         && port == map.port
         && lv1 == map.lv1
         && lv2 == map.lv2
         && lv3 == map.lv3
         && lv4 == map.lv4
         && enote == map.enote
         && anote == map.anote;
}

void DrumMap::dump()
{
  fprintf(stderr, "%s\t\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d(%s)\t\t%d(%s)\t\t%d\t%d\t\n",
      name.toLatin1().constData(),
      vol,
      quant,
      len,
      channel,
      port,
      lv1, lv2, lv3, lv4,
      enote, pitch2string(enote).toLatin1().constData(),
      anote, pitch2string(anote).toLatin1().constData(),
      mute,
      hide);
}

//---------------------------------------------------------
//   writeDrumMap
//---------------------------------------------------------

void writeDrumMap(int level, Xml& xml, bool external)
      {
      xml.tag(level++, "drummap");
      for (int i = 0; i < DRUM_MAPSIZE; ++i) {
            DrumMap* dm = &MusEGlobal::drumMap[i];
            const DrumMap* idm = &idrumMap[i];

            if (external) {
                  xml.tag(level++, "entry");
                  xml.strTag(level, "name", dm->name);
                  xml.intTag(level, "vol", dm->vol);
                  xml.intTag(level, "quant", dm->quant);
                  xml.intTag(level, "len", dm->len);
                  xml.intTag(level, "channel", dm->channel);
                  xml.intTag(level, "port", dm->port);
                  xml.intTag(level, "lv1", dm->lv1);
                  xml.intTag(level, "lv2", dm->lv2);
                  xml.intTag(level, "lv3", dm->lv3);
                  xml.intTag(level, "lv4", dm->lv4);
                  xml.intTag(level, "enote", dm->enote);
                  xml.intTag(level, "anote", dm->anote);
                  xml.intTag(level, "hide", dm->hide);
                  }
            else {
                  // write only, if entry is different from initial entry
                  if (!external && *dm == *idm)
                        continue;
                  xml.tag(level++, "entry idx=\"%d\"", i);
                  if (dm->name != idm->name)
                        xml.strTag(level, "name", dm->name);
                  if (dm->vol != idm->vol)
                        xml.intTag(level, "vol", dm->vol);
                  if (dm->quant != idm->quant)
                        xml.intTag(level, "quant", dm->quant);
                  if (dm->len != idm->len)
                        xml.intTag(level, "len", dm->len);
                  if (dm->channel != idm->channel)
                        xml.intTag(level, "channel", dm->channel);
                  if (dm->port != idm->port)
                        xml.intTag(level, "port", dm->port);
                  if (dm->lv1 != idm->lv1)
                        xml.intTag(level, "lv1", dm->lv1);
                  if (dm->lv2 != idm->lv2)
                        xml.intTag(level, "lv2", dm->lv2);
                  if (dm->lv3 != idm->lv3)
                        xml.intTag(level, "lv3", dm->lv3);
                  if (dm->lv4 != idm->lv4)
                        xml.intTag(level, "lv4", dm->lv4);
                  if (dm->enote != idm->enote)
                        xml.intTag(level, "enote", dm->enote);
                  if (dm->anote != idm->anote)
                        xml.intTag(level, "anote", dm->anote);
                  if (dm->mute != idm->mute)
                        xml.intTag(level, "mute", dm->mute);
                  if (dm->hide != idm->hide)
                        xml.intTag(level, "hide", dm->hide);
                  }
            xml.tag(level--, "/entry");
            }
      xml.tag(level--, "/drummap");
      }

//---------------------------------------------------------
//   readDrummapEntry
//---------------------------------------------------------

static void readDrummapEntry(Xml& xml, DrumMap* dm)
      {
      
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "name")
                              dm->name = xml.parse(QString("name"));
                        else if (tag == "vol")
                              dm->vol = (unsigned char)xml.parseInt();
                        else if (tag == "quant")
                              dm->quant = xml.parseInt();
                        else if (tag == "len")
                              dm->len = xml.parseInt();
                        else if (tag == "channel")
                              dm->channel = xml.parseInt();
                        else if (tag == "port")
                              dm->port = xml.parseInt();
                        else if (tag == "lv1")
                              dm->lv1 = xml.parseInt();
                        else if (tag == "lv2")
                              dm->lv2 = xml.parseInt();
                        else if (tag == "lv3")
                              dm->lv3 = xml.parseInt();
                        else if (tag == "lv4")
                              dm->lv4 = xml.parseInt();
                        else if (tag == "enote")
                              dm->enote = xml.parseInt();
                        else if (tag == "anote")
                              dm->anote = xml.parseInt();
                        else if (tag == "mute")
                              dm->mute = xml.parseInt();
                        else if (tag == "hide")
                              dm->hide = xml.parseInt();
                        else if (tag == "selected")
                              //; // dm->selected = xml.parseInt();
                              xml.skip(tag);
                        else
                              xml.unknown("entry");
                        break;
                  case Xml::Attribut:
                        if (tag == "idx") {
                              int idx = xml.s2().toInt() & 0x7f;
                              dm = &MusEGlobal::drumMap[idx];
                              
                              }
                        break;
                  case Xml::TagEnd:
                        if (tag == "entry")
                        {
                              return;
                        }      
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   readDrummap
//---------------------------------------------------------

void readDrumMap(Xml& xml, bool external)
      {
      if (external) {
            for (int i = 0; i < DRUM_MAPSIZE; ++i)
                  MusEGlobal::drumMap[i] = blankdm;
            }
      else {
            for (int i = 0; i < DRUM_MAPSIZE; ++i)
                  MusEGlobal::drumMap[i] = idrumMap[i];
            }
      int i = 0;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
//                             goto read_drummap_end;
                            return;
                  case Xml::TagStart:
                        if (tag == "entry") {
                              if(i >= DRUM_MAPSIZE)
//                                 goto read_drummap_end;
                                return;
                              readDrummapEntry(xml, external ? &MusEGlobal::drumMap[i] : 0);
                              ++i;
                              }
                        else if (tag == "comment")
                              xml.parse();
                        else
                              xml.unknown("DrumMap");
                        break;
                  case Xml::Attribut:
                        break;
                  case Xml::TagEnd:
                        if (tag == "drummap") {
                              memset(MusEGlobal::drumInmap, 0, sizeof(MusEGlobal::drumInmap));
                              memset(MusEGlobal::drumOutmap, 0, sizeof(MusEGlobal::drumOutmap));
                              for (int i = 0; i < DRUM_MAPSIZE; ++i) {
                                    MusEGlobal::drumInmap[(unsigned int)(MusEGlobal::drumMap[i].enote)] = i;
                                    MusEGlobal::drumOutmap[(unsigned int)(MusEGlobal::drumMap[i].anote)] = i;
                                    }
//                               goto read_drummap_end;
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

} // namespace MusECore


namespace MusEGlobal {

void global_drum_ordering_t::cleanup()
{
  using MusEGlobal::song;
  using MusECore::MidiTrack;
  using MusECore::ciTrack;
  
  QSet<MidiTrack*> tracks;
  for (ciTrack it = song->tracks()->begin(); it != song->tracks()->end(); it++)
    tracks.insert( dynamic_cast<MidiTrack*>(*it) );
  
  for (iterator it = begin(); it != end();)
  {
    if (!tracks.contains(it->first))
      it=erase(it);
    else
      it++;
  }
}

void global_drum_ordering_t::write(int level, MusECore::Xml& xml)
{
  cleanup();
  
  xml.tag(level++, "drum_ordering");

  for (iterator it = begin(); it != end(); it++)
    write_single(level, xml, *it);

  xml.etag(level, "drum_ordering");
}

void global_drum_ordering_t::write_single(int level, MusECore::Xml& xml, const entry_t& entry)
{
  const int trk_idx = MusEGlobal::song->tracks()->index(entry.first);
  if(trk_idx >= 0)
  {
    const QString s= QString("<item track=\"%1\" instr=\"%2\" />").arg(trk_idx).arg(entry.second);
    xml.put(level, "%s", s.toLatin1().constData());
  }
}

void global_drum_ordering_t::read(MusECore::Xml& xml)
{
  using MusECore::Xml;
  
  clear();
  
  for (;;)
  {
    Xml::Token token = xml.parse();
    if (token == Xml::Error || token == Xml::End)
      break;
      
    const QString& tag = xml.s1();
    switch (token)
    {
      case Xml::TagStart:
        // OBSOLETE. Keep for backwards compatibility.
        if (tag == "entry")
          append(read_single(xml));
        else if (tag == "item")
          append(read_item(xml));
        else
          xml.unknown("global_drum_ordering_t");
        break;
        
      case Xml::TagEnd:
        if (tag == "drum_ordering")
          return;
        
      default:
        break;
    }
  }
}
  
global_drum_ordering_t::entry_t global_drum_ordering_t::read_single(MusECore::Xml& xml)
{
  using MusECore::Xml;
  using MusEGlobal::song;
  using MusECore::ciTrack;
  
  entry_t entry;
  entry.first=NULL;
  entry.second=-1;
  
	for (;;)
	{
		Xml::Token token = xml.parse();
		if (token == Xml::Error || token == Xml::End)
			break;
			
		const QString& tag = xml.s1();
		switch (token)
		{
			case Xml::TagStart:
				if (tag == "track")
        {
					QString track_name=xml.parse1();
          
          ciTrack it;
          for (it = song->tracks()->begin(); it != song->tracks()->end(); it++)
            if (track_name == (*it)->name())
              break;
          
          if (it != song->tracks()->end())
            entry.first=dynamic_cast<MusECore::MidiTrack*>(*it);
        }
				else if (tag == "instrument")
					entry.second=xml.parseInt();
				else
					xml.unknown("global_drum_ordering_t (single entry)");
				break;
				
			case Xml::TagEnd:
				if (tag == "entry")
					goto end_of_read_single;
				
			default:
				break;
		}
	}

  end_of_read_single:
  
  if (entry.first == NULL)
    fprintf(stderr, "ERROR: global_drum_ordering_t::read_single() couldn't find the specified track!\n");
  
  if (entry.second < 0 || entry.second > 127)
    fprintf(stderr, "ERROR: global_drum_ordering_t::read_single(): instrument number is out of bounds (%i)!\n", entry.second);
  
  return entry;
}

global_drum_ordering_t::entry_t global_drum_ordering_t::read_item(MusECore::Xml& xml)
{
  using MusECore::Xml;
  using MusEGlobal::song;
  using MusECore::ciTrack;

  entry_t entry;
  entry.first=NULL;
  entry.second=-1;

  int trk_idx = -1;
  int instr = -1;

  for (;;)
  {
    Xml::Token token = xml.parse();
    if (token == Xml::Error || token == Xml::End)
      break;
      
    const QString& tag = xml.s1();
    switch (token)
    {
      case Xml::TagStart:
          xml.unknown("global_drum_ordering_t (single item)");
        break;
        
      case Xml::Attribut:
            if (tag == "track")
              trk_idx = xml.s2().toInt();
            else if (tag == "instr")
              instr = xml.s2().toInt();
            else
              fprintf(stderr, "unknown tag %s\n", tag.toLatin1().constData());
        break;

      case Xml::TagEnd:
        if (tag == "item")
          goto end_of_read_item;
        break;
        
      default:
        break;
    }
  }

  end_of_read_item:
  
  if(trk_idx < 0)
  {
    fprintf(stderr, "ERROR: global_drum_ordering_t::read_single() invalid track index (%i)!\n", trk_idx);
    return entry;
  }
  if(instr < 0 || instr > 127)
  {
    fprintf(stderr, "ERROR: global_drum_ordering_t::read_single(): instrument number is out of bounds (%i)!\n", instr);
    return entry;
  }

  MusECore::Track* trk = MusEGlobal::song->tracks()->index(trk_idx);
  if(!trk)
  {
    fprintf(stderr, "ERROR: global_drum_ordering_t::read_single() couldn't find the specified track at idx %i !\n", trk_idx);
    return entry;
  }
  if(!trk->isMidiTrack())
  {
    fprintf(stderr, "ERROR: global_drum_ordering_t::read_single() track is not a midi track at idx %i !\n", trk_idx);
    return entry;
  }
  
  entry.first = static_cast<MusECore::MidiTrack*>(trk);
  entry.second = instr;
  
  return entry;
}

} // namespace MusEGlobal
