//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: drummap.cpp,v 1.3.2.6 2009/10/29 02:14:37 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
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
const DrumMap blankdm = { QString(""), 100, 16, 32, -1, -1, 70, 90, 110, 127, 127, 127, false };

// this map should have 128 entries, as it's used for initalising iNewDrumMap as well.
// iNewDrumMap only has 128 entries. also, the every "out-note" ("anote") should be
// represented exactly once in idrumMap, and there shall be no duplicate or unused
// "out-notes".
// reason: iNewDrumMap is inited as follows: iterate through the full idrumMap[],
//         iNewDrumMap[ idrumMap[i].anote ] = idrumMap[i]
// if you ever want to change this, you will need to fix the initNewDrumMap() function.
const DrumMap idrumMap[DRUM_MAPSIZE] = {
      { QString("Acoustic Bass Drum"), 100, 16, 32, -1, -1, 70, 90, 110, 127, 35, 35, false },
      { QString("Bass Drum 1"),        100, 16, 32, -1, -1, 70, 90, 110, 127, 36, 36, false },
      { QString("Side Stick"),         100, 16, 32, -1, -1, 70, 90, 110, 127, 37, 37, false },
      { QString("Acoustic Snare"),     100, 16, 32, -1, -1, 70, 90, 110, 127, 38, 38, false },
      { QString("Hand Clap"),          100, 16, 32, -1, -1, 70, 90, 110, 127, 39, 39, false },
      { QString("Electric Snare"),     100, 16, 32, -1, -1, 70, 90, 110, 127, 40, 40, false },
      { QString("Low Floor Tom"),      100, 16, 32, -1, -1, 70, 90, 110, 127, 41, 41, false },
      { QString("Closed Hi-Hat"),      100, 16, 32, -1, -1, 70, 90, 110, 127, 42, 42, false },
      { QString("High Floor Tom"),     100, 16, 32, -1, -1, 70, 90, 110, 127, 43, 43, false },
      { QString("Pedal Hi-Hat"),       100, 16, 32, -1, -1, 70, 90, 110, 127, 44, 44, false },
      { QString("Low Tom"),            100, 16, 32, -1, -1, 70, 90, 110, 127, 45, 45, false },
      { QString("Open Hi-Hat"),        100, 16, 32, -1, -1, 70, 90, 110, 127, 46, 46, false },
      { QString("Low-Mid Tom"),        100, 16, 32, -1, -1, 70, 90, 110, 127, 47, 47, false },
      { QString("Hi-Mid Tom"),         100, 16, 32, -1, -1, 70, 90, 110, 127, 48, 48, false },
      { QString("Crash Cymbal 1"),     100, 16, 32, -1, -1, 70, 90, 110, 127, 49, 49, false },
      { QString("High Tom"),           100, 16, 32, -1, -1, 70, 90, 110, 127, 50, 50, false },

      { QString("Ride Cymbal 1"),      100, 16, 32, -1, -1, 70, 90, 110, 127, 51, 51, false },
      { QString("Chinese Cymbal"),     100, 16, 32, -1, -1, 70, 90, 110, 127, 52, 52, false },
      { QString("Ride Bell"),          100, 16, 32, -1, -1, 70, 90, 110, 127, 53, 53, false },
      { QString("Tambourine"),         100, 16, 32, -1, -1, 70, 90, 110, 127, 54, 54, false },
      { QString("Splash Cymbal"),      100, 16, 32, -1, -1, 70, 90, 110, 127, 55, 55, false },
      { QString("Cowbell"),            100, 16, 32, -1, -1, 70, 90, 110, 127, 56, 56, false },
      { QString("Crash Cymbal 2"),     100, 16, 32, -1, -1, 70, 90, 110, 127, 57, 57, false },
      { QString("Vibraslap"),          100, 16, 32, -1, -1, 70, 90, 110, 127, 58, 58, false },
      { QString("Ride Cymbal 2"),      100, 16, 32, -1, -1, 70, 90, 110, 127, 59, 59, false },
      { QString("Hi Bongo"),           100, 16, 32, -1, -1, 70, 90, 110, 127, 60, 60, false },
      { QString("Low Bongo"),          100, 16, 32, -1, -1, 70, 90, 110, 127, 61, 61, false },
      { QString("Mute Hi Conga"),      100, 16, 32, -1, -1, 70, 90, 110, 127, 62, 62, false },
      { QString("Open Hi Conga"),      100, 16, 32, -1, -1, 70, 90, 110, 127, 63, 63, false },
      { QString("Low Conga"),          100, 16, 32, -1, -1, 70, 90, 110, 127, 64, 64, false },
      { QString("High Timbale"),       100, 16, 32, -1, -1, 70, 90, 110, 127, 65, 65, false },
      { QString("Low Timbale"),        100, 16, 32, -1, -1, 70, 90, 110, 127, 66, 66, false },

      { QString("High Agogo"),         100, 16, 32, -1, -1, 70, 90, 110, 127, 67, 67, false },
      { QString("Low Agogo"),          100, 16, 32, -1, -1, 70, 90, 110, 127, 68, 68, false },
      { QString("Cabasa"),             100, 16, 32, -1, -1, 70, 90, 110, 127, 69, 69, false },
      { QString("Maracas"),            100, 16, 32, -1, -1, 70, 90, 110, 127, 70, 70, false },
      { QString("Short Whistle"),      100, 16, 32, -1, -1, 70, 90, 110, 127, 71, 71, false },
      { QString("Long Whistle"),       100, 16, 32, -1, -1, 70, 90, 110, 127, 72, 72, false },
      { QString("Short Guiro"),        100, 16, 32, -1, -1, 70, 90, 110, 127, 73, 73, false },
      { QString("Long Guiro"),         100, 16, 32, -1, -1, 70, 90, 110, 127, 74, 74, false },
      { QString("Claves"),             100, 16, 32, -1, -1, 70, 90, 110, 127, 75, 75, false },
      { QString("Hi Wood Block"),      100, 16, 32, -1, -1, 70, 90, 110, 127, 76, 76, false },
      { QString("Low Wood Block"),     100, 16, 32, -1, -1, 70, 90, 110, 127, 77, 77, false },
      { QString("Mute Cuica"),         100, 16, 32, -1, -1, 70, 90, 110, 127, 78, 78, false },
      { QString("Open Cuica"),         100, 16, 32, -1, -1, 70, 90, 110, 127, 79, 79, false },
      { QString("Mute Triangle"),      100, 16, 32, -1, -1, 70, 90, 110, 127, 80, 80, false },
      { QString("Open Triangle"),      100, 16, 32, -1, -1, 70, 90, 110, 127, 81, 81, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 82, 82, false },

      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 83, 83, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 84, 84, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 85, 85, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 86, 86, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 87, 87, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 88, 88, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 89, 89, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 90, 90, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 91, 91, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 92, 92, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 93, 93, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 94, 94, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 95, 95, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 96, 96, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 97, 97, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 98, 98, false },

      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 99, 99, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 100, 100, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 101, 101, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 102, 102, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 103, 103, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 104, 104, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 105, 105, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 106, 106, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 107, 107, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 108, 108, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 109, 109, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 110, 110, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 111, 111, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 112, 112, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 113, 113, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 114, 114, false },

      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 115, 115, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 116, 116, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 117, 117, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 118, 118, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 119, 119, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 120, 120, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 121, 121, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 122, 122, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 123, 123, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 124, 124, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 125, 125, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 126, 126, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 127, 127, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 0, 0, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 1, 1, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 2, 2, false },

      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 3, 3, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 4, 4, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 5, 5, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 6, 6, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 7, 7, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 8, 8, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 9, 9, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 10, 10, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 11, 11, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 12, 12, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 13, 13, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 14, 14, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 15, 15, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 16, 16, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 17, 17, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 18, 18, false },

      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 19, 19, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 20, 20, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 21, 21, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 22, 22, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 23, 23, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 24, 24, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 25, 25, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 26, 26, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 27, 27, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 28, 28, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 29, 29, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 30, 30, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 31, 31, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 32, 32, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 33, 33, false },
      { QString(""),                   100, 16, 32, -1, -1, 70, 90, 110, 127, 34, 34, false }
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
      printf("ERROR: THIS SHOULD NEVER HAPPEN: idrumMap[%i].anote is not within 0..127!\n", idx);
    else
    {
      if (done[idx]==true)
      {
        printf("ERROR: iNewDrumMap[%i] is already initalized!\n"
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
      printf("ERROR: iNewDrumMap[%i] is uninitalized!\n"
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
            d.vol = d.len = d.channel = d.port = d.lv1 = d.lv2 = d.lv3 = d.lv4 = d.enote = d.anote = d.mute = 0;
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
            if (!(d.vol || d.len || d.channel || d.port || d.lv1 || d.lv2 || d.lv3 || d.lv4 || d.enote || d.anote || d.mute))
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
      MusEGlobal::audio->msgIdle(true);
      MusEGlobal::song->changeAllPortDrumCtrlEvents(false); // Delete all port controller events.
      
      for(int i = 0; i < DRUM_MAPSIZE; ++i) 
        MusEGlobal::drumMap[i] = idrumMap[i];
      memset(MusEGlobal::drumInmap, 0, sizeof(MusEGlobal::drumInmap));
      memset(MusEGlobal::drumOutmap, 0, sizeof(MusEGlobal::drumOutmap));
      for (int i = 0; i < DRUM_MAPSIZE; ++i) {
            MusEGlobal::drumInmap[(unsigned int)(MusEGlobal::drumMap[i].enote)] = i;
            MusEGlobal::drumOutmap[(unsigned int)(MusEGlobal::drumMap[i].anote)] = i;
            }
      
      MusEGlobal::song->changeAllPortDrumCtrlEvents(true); // Add all port controller events.
      MusEGlobal::audio->msgIdle(false);
      }

//---------------------------------------------------------
//   operator ==
//---------------------------------------------------------

bool DrumMap::operator==(const DrumMap& map) const
      {
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
         && anote == map.anote
         && mute == map.mute;
      }

bool DrumMap::almost_equals(const DrumMap& map) const
{
  DrumMap tmp=map;
  tmp.mute=this->mute;
  return tmp==*this;
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
                        else if (tag == "selected")
                              //; // dm->selected = xml.parseInt();
                              xml.skip(tag);
                        else
                              xml.unknown("DrumMapEntry");
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
      MusEGlobal::audio->msgIdle(true);
      MusEGlobal::song->changeAllPortDrumCtrlEvents(false); // Delete all port controller events.
      
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
                        MusEGlobal::audio->msgIdle(false);
                        return;
                  case Xml::TagStart:
                        if (tag == "entry") {
                              if(i >= DRUM_MAPSIZE)
                              {
                                MusEGlobal::audio->msgIdle(false);
                                return;
                              }
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
                              
                              MusEGlobal::song->changeAllPortDrumCtrlEvents(true); // Add all port controller events.
                              
                              MusEGlobal::audio->msgIdle(false);
                              return;
                              }
                  default:
                        break;
                  }
            }
            
            MusEGlobal::song->changeAllPortDrumCtrlEvents(true); // Add all port controller events.
            MusEGlobal::audio->msgIdle(false);
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
  xml.tag(level++, "entry");
  xml.strTag(level, "track", entry.first->name());
  xml.intTag(level, "instrument", entry.second);
  xml.etag(level, "entry");
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
				if (tag == "entry")
					append(read_single(xml));
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
    printf("ERROR: global_drum_ordering_t::read_single() couldn't find the specified track!\n");
  
  if (entry.second < 0 || entry.second > 127)
    printf("ERROR: global_drum_ordering_t::read_single(): instrument number is out of bounds (%i)!\n", entry.second);
  
  return entry;
}
  
} // namespace MusEGlobal
