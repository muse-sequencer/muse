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

#include "drummap.h"
#include "al/xml.h"

static const int DEFAULT_QUANT = 16;
static const int DEFAULT_LEN   = 32;
static const int DEFAULT_CHANNEL = -1;
static const int DEFAULT_LV1   = 70;
static const int DEFAULT_LV2   = 90;
static const int DEFAULT_LV3   = 110;
static const int DEFAULT_LV4   = 127;

//---------------------------------------------------------
//    GM default drum map
//---------------------------------------------------------

DrumMap gmDrumMap("generic");
DrumMap noDrumMap("no-map");

//---------------------------------------------------------
//   DrumMap
//---------------------------------------------------------

DrumMap::DrumMap(const QString& s)
      {
      _name = s;
      for (int i = 0; i < DRUM_MAPSIZE; ++i) {
            map[i].name    = "?";
            map[i].enote   = i;
            map[i].anote   = i;
            map[i].quant   = DEFAULT_QUANT;
            map[i].len     = DEFAULT_LEN;
            map[i].channel = DEFAULT_CHANNEL;
            map[i].lv1     = DEFAULT_LV1;
            map[i].lv2     = DEFAULT_LV2;
            map[i].lv3     = DEFAULT_LV3;
            map[i].lv4     = DEFAULT_LV4;
            map[i].mute    = false;
            }
      init();
      }

//---------------------------------------------------------
//   initGm
//---------------------------------------------------------

void DrumMap::initGm()
      {
      static const char* gmNames[] = {
            "Acoustic Bass Drum", "Bass Drum 1", "Side Stick", "Acoustic Snare",
            "Hand Clap", "Electric Snare", "Low Floor Tom", "Closed Hi-Hat",
            "High Floor Tom", "Pedal Hi-Hat", "Low Tom", "Open Hi-Hat", "Low-Mid Tom",
            "Hi-Mid Tom", "Crash Cymbal 1", "High Tom", "Ride Cymbal 1",
            "Chinese Cymbal", "Ride Bell", "Tambourine", "Splash Cymbal",
            "Cowbell", "Crash Cymbal 2", "Vibraslap", "Ride Cymbal 2",
            "Hi Bongo", "Low Bongo", "Mute Hi Conga", "Open Hi Conga",
            "Low Conga", "High Timbale", "Low Timbale", "High Agogo",
            "Low Agogo", "Cabasa", "Maracas", "Short Whistle",
            "Long Whistle", "Short Guiro", "Long Guiro", "Claves",
            "Hi Wood Block", "Low Wood Block", "Mute Cuica",
            "Open Cuica", "Mute Triangle", "Open Triangle", 0
            };
      init();
      int idx = 0;
      const char** p = &gmNames[0];
      int val = 35;
      for (; *p; ++p, ++val, ++idx) {
            map[idx].name  = *p;
            map[idx].enote = val;
            map[idx].anote = val;
            _inmap[int(map[idx].enote)] = idx;
            _outmap[int(map[idx].anote)] = idx;
            }
      }

//---------------------------------------------------------
//   init
//    populate Inmap and Outmap
//---------------------------------------------------------

void DrumMap::init()
      {
      memset(_inmap, 0, sizeof(_inmap));
      memset(_outmap, 0, sizeof(_outmap));
      for (int i = 0; i < DRUM_MAPSIZE; ++i) {
            _inmap[int(map[i].enote)] = i;
            _outmap[int(map[i].anote)] = i;
            }
      }

//---------------------------------------------------------
//   writeDrumMap
//---------------------------------------------------------

void DrumMap::write(Xml& xml)
      {
      xml.tag("drummap");
      for (int i = 0; i < DRUM_MAPSIZE; ++i) {
            DrumMapEntry* dm = &map[i];
            xml.tag("entry");
            xml.strTag("name", dm->name);
            if (dm->quant != DEFAULT_QUANT)
                  xml.intTag("quant", dm->quant);
            if (dm->len != DEFAULT_LEN)
                  xml.intTag("len", dm->len);
            if (dm->channel != DEFAULT_CHANNEL)
                  xml.intTag("channel", dm->channel);
            if (dm->lv1 != DEFAULT_LV1)
                  xml.intTag("lv1", dm->lv1);
            if (dm->lv2 != DEFAULT_LV2)
                  xml.intTag("lv2", dm->lv2);
            if (dm->lv2 != DEFAULT_LV3)
                  xml.intTag("lv3", dm->lv3);
            if (dm->lv4 != DEFAULT_LV4)
                  xml.intTag("lv4", dm->lv4);
            xml.intTag("enote", dm->enote);
            xml.intTag("anote", dm->anote);
            xml.etag("entry");
            }
      xml.etag("drummap");
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void DrumMapEntry::read(QDomNode n)
      {
      for (QDomNode node = n.firstChild(); !node.isNull(); node = node.nextSibling()) {
            QDomElement e = node.toElement();
            QString tag(e.tagName());
            QString s(e.text());
            if (tag == "name")
                  name = s;
            else if (tag == "quant")
                  quant = s.toInt();
            else if (tag == "len")
                  len = s.toInt();
            else if (tag == "channel")
                  channel = s.toInt();
            else if (tag == "lv1")
                  lv1 = s.toInt();
            else if (tag == "lv2")
                  lv2 = s.toInt();
            else if (tag == "lv3")
                  lv3 = s.toInt();
            else if (tag == "lv4")
                  lv4 = s.toInt();
            else if (tag == "enote")
                  enote = s.toInt();
            else if (tag == "anote")
                  anote = s.toInt();
            else if (tag == "mute")
                  mute = s.toInt();
            else {
                  printf("read Drummap Entry: unknown tag %s\n", tag.toLatin1().data());
                  break;
                  }
            }
      }

//---------------------------------------------------------
//   readDrummap
//---------------------------------------------------------

void DrumMap::read(QDomNode node)
      {
      init();
      int idx = 0;
      for (; !node.isNull(); node = node.nextSibling()) {
            QDomElement e = node.toElement();
            QString tag(e.tagName());

            if (tag == "entry") {
                  QDomNode entryNode = node.firstChild();
                  map[idx].read(entryNode);
            	_inmap[int(map[idx].enote)]  = idx;
            	_outmap[int(map[idx].anote)] = idx;
                  ++idx;
                  }
            else {
                  printf("read Drummap: unknown tag %s\n", tag.toLatin1().data());
                  break;
                  }
            }
      }

