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

namespace MusEGlobal {
char drumOutmap[DRUM_MAPSIZE];
char drumInmap[128];
MusECore::DrumMap drumMap[DRUM_MAPSIZE];
}

namespace MusECore {

//---------------------------------------------------------
//    GM default drum map
//---------------------------------------------------------

const DrumMap blankdm = { QString(""), 100, 16, 32, 9, 0, 70, 90, 127, 110, 127, 127, false };

const DrumMap idrumMap[DRUM_MAPSIZE] = {
      { QString("Acoustic Bass Drum"), 100, 16, 32, 9, 0, 70, 90, 127, 110, 35, 35, false },
      { QString("Bass Drum 1"),        100, 16, 32, 9, 0, 70, 90, 127, 110, 36, 36, false },
      { QString("Side Stick"),         100, 16, 32, 9, 0, 70, 90, 127, 110, 37, 37, false },
      { QString("Acoustic Snare"),     100, 16, 32, 9, 0, 70, 90, 127, 110, 38, 38, false },
      { QString("Hand Clap"),          100, 16, 32, 9, 0, 70, 90, 127, 110, 39, 39, false },
      { QString("Electric Snare"),     100, 16, 32, 9, 0, 70, 90, 127, 110, 40, 40, false },
      { QString("Low Floor Tom"),      100, 16, 32, 9, 0, 70, 90, 127, 110, 41, 41, false },
      { QString("Closed Hi-Hat"),      100, 16, 32, 9, 0, 70, 90, 127, 110, 42, 42, false },
      { QString("High Floor Tom"),     100, 16, 32, 9, 0, 70, 90, 127, 110, 43, 43, false },
      { QString("Pedal Hi-Hat"),       100, 16, 32, 9, 0, 70, 90, 127, 110, 44, 44, false },
      { QString("Low Tom"),            100, 16, 32, 9, 0, 70, 90, 127, 110, 45, 45, false },
      { QString("Open Hi-Hat"),        100, 16, 32, 9, 0, 70, 90, 127, 110, 46, 46, false },
      { QString("Low-Mid Tom"),        100, 16, 32, 9, 0, 70, 90, 127, 110, 47, 47, false },
      { QString("Hi-Mid Tom"),         100, 16, 32, 9, 0, 70, 90, 127, 110, 48, 48, false },
      { QString("Crash Cymbal 1"),     100, 16, 32, 9, 0, 70, 90, 127, 110, 49, 49, false },
      { QString("High Tom"),           100, 16, 32, 9, 0, 70, 90, 127, 110, 50, 50, false },

      { QString("Ride Cymbal 1"),      100, 16, 32, 9, 0, 70, 90, 127, 110, 51, 51, false },
      { QString("Chinese Cymbal"),     100, 16, 32, 9, 0, 70, 90, 127, 110, 52, 52, false },
      { QString("Ride Bell"),          100, 16, 32, 9, 0, 70, 90, 127, 110, 53, 53, false },
      { QString("Tambourine"),         100, 16, 32, 9, 0, 70, 90, 127, 110, 54, 54, false },
      { QString("Splash Cymbal"),      100, 16, 32, 9, 0, 70, 90, 127, 110, 55, 55, false },
      { QString("Cowbell"),            100, 16, 32, 9, 0, 70, 90, 127, 110, 56, 56, false },
      { QString("Crash Cymbal 2"),     100, 16, 32, 9, 0, 70, 90, 127, 110, 57, 57, false },
      { QString("Vibraslap"),          100, 16, 32, 9, 0, 70, 90, 127, 110, 58, 58, false },
      { QString("Ride Cymbal 2"),      100, 16, 32, 9, 0, 70, 90, 127, 110, 59, 59, false },
      { QString("Hi Bongo"),           100, 16, 32, 9, 0, 70, 90, 127, 110, 60, 60, false },
      { QString("Low Bongo"),          100, 16, 32, 9, 0, 70, 90, 127, 110, 61, 61, false },
      { QString("Mute Hi Conga"),      100, 16, 32, 9, 0, 70, 90, 127, 110, 62, 62, false },
      { QString("Open Hi Conga"),      100, 16, 32, 9, 0, 70, 90, 127, 110, 63, 63, false },
      { QString("Low Conga"),          100, 16, 32, 9, 0, 70, 90, 127, 110, 64, 64, false },
      { QString("High Timbale"),       100, 16, 32, 9, 0, 70, 90, 127, 110, 65, 65, false },
      { QString("Low Timbale"),        100, 16, 32, 9, 0, 70, 90, 127, 110, 66, 66, false },

      { QString("High Agogo"),         100, 16, 32, 9, 0, 70, 90, 127, 110, 67, 67, false },
      { QString("Low Agogo"),          100, 16, 32, 9, 0, 70, 90, 127, 110, 68, 68, false },
      { QString("Cabasa"),             100, 16, 32, 9, 0, 70, 90, 127, 110, 69, 69, false },
      { QString("Maracas"),            100, 16, 32, 9, 0, 70, 90, 127, 110, 70, 70, false },
      { QString("Short Whistle"),      100, 16, 32, 9, 0, 70, 90, 127, 110, 71, 71, false },
      { QString("Long Whistle"),       100, 16, 32, 9, 0, 70, 90, 127, 110, 72, 72, false },
      { QString("Short Guiro"),        100, 16, 32, 9, 0, 70, 90, 127, 110, 73, 73, false },
      { QString("Long Guiro"),         100, 16, 32, 9, 0, 70, 90, 127, 110, 74, 74, false },
      { QString("Claves"),             100, 16, 32, 9, 0, 70, 90, 127, 110, 75, 75, false },
      { QString("Hi Wood Block"),      100, 16, 32, 9, 0, 70, 90, 127, 110, 76, 76, false },
      { QString("Low Wood Block"),     100, 16, 32, 9, 0, 70, 90, 127, 110, 77, 77, false },
      { QString("Mute Cuica"),         100, 16, 32, 9, 0, 70, 90, 127, 110, 78, 78, false },
      { QString("Open Cuica"),         100, 16, 32, 9, 0, 70, 90, 127, 110, 79, 79, false },
      { QString("Mute Triangle"),      100, 16, 32, 9, 0, 70, 90, 127, 110, 80, 80, false },
      { QString("Open Triangle"),      100, 16, 32, 9, 0, 70, 90, 127, 110, 81, 81, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 82, 82, false },

      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 83, 83, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 84, 84, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 85, 85, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 86, 86, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 87, 87, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 88, 88, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 89, 89, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 90, 90, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 91, 91, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 92, 92, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 93, 93, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 94, 94, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 95, 95, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 96, 96, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 97, 97, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 98, 98, false },

      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 99, 99, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 100, 100, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 101, 101, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 102, 102, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 103, 103, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 104, 104, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 105, 105, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 106, 106, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 107, 107, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 108, 108, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 109, 109, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 110, 110, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 111, 111, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 112, 112, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 113, 113, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 114, 114, false },

      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 115, 115, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 116, 116, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 117, 117, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 118, 118, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 119, 119, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 120, 120, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 121, 121, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 122, 122, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 123, 123, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 124, 124, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 125, 125, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 126, 126, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 127, 127, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 0, 0, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 1, 1, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 2, 2, false },

      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 3, 3, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 4, 4, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 5, 5, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 6, 6, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 7, 7, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 8, 8, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 9, 9, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 10, 10, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 11, 11, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 12, 12, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 13, 13, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 14, 14, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 15, 15, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 16, 16, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 17, 17, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 18, 18, false },

      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 19, 19, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 20, 20, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 21, 21, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 22, 22, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 23, 23, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 24, 24, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 25, 25, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 26, 26, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 27, 27, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 28, 28, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 29, 29, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 30, 30, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 31, 31, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 32, 32, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 33, 33, false },
      { QString(""),                   100, 16, 32, 9, 0, 70, 90, 127, 110, 34, 34, false }
      };
      

//---------------------------------------------------------
//   initDrumMap
//    populate Inmap and Outmap
//---------------------------------------------------------

void initDrumMap()
      {
      for (int i = 0; i < DRUM_MAPSIZE; ++i) {
            DrumMap d = MusEGlobal::drumMap[i];
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
         (name == map.name)
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
