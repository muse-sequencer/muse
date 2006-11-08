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

#include "song.h"
#include "midi.h"
#include "midisynti.h"
#include "midiplugin.h"
#include "midiplugins/libmidiplugin/mempi.h"
#include "al/tempo.h"

//---------------------------------------------------------
//   MidiSynti
//---------------------------------------------------------

MidiSynti::MidiSynti()
   : MidiTrackBase()
      {
      _synti = 0;
      init();
      }

MidiSynti::~MidiSynti()
      {
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

void MidiSynti::init()
      {
      }

//---------------------------------------------------------
//   hasGui
//---------------------------------------------------------

bool MidiSynti::hasGui() const
      {
      return _synti->hasGui();
      }

//---------------------------------------------------------
//   hasGui
//---------------------------------------------------------

void MidiSynti::showGui(bool val)
      {
      return _synti->showGui(val);
      }

//---------------------------------------------------------
//   guiVisible
//---------------------------------------------------------

bool MidiSynti::guiVisible() const
      {
      return _synti->guiVisible();
      }

//---------------------------------------------------------
//   initInstance
//    return true on error
//---------------------------------------------------------

bool MidiSynti::initInstance(MidiPlugin* plugin)
      {
      _synti = plugin->instantiate(this);
      return _synti == 0;
      }

//---------------------------------------------------------
//   MidiSynti::write
//---------------------------------------------------------

void MidiSynti::write(Xml& xml) const
      {
      xml.tag("MidiSynti");
      MidiTrackBase::writeProperties(xml);
      xml.strTag("class", _synti->plugin()->name());
      if (_synti->hasGui()) {
            xml.intTag("guiVisible", _synti->guiVisible());
            int x, y, w, h;
            w = 0;
            h = 0;
            _synti->getGeometry(&x, &y, &w, &h);
            if (h || w)
                  xml.qrectTag("geometry", QRect(x, y, w, h));
            }
      //---------------------------------------------
      // dump current state of plugin
      //---------------------------------------------

      int len = 0;
      const unsigned char* p;
      _synti->getInitData(&len, &p);
      if (len) {
            xml.tag("init len=\"%d\"", len);
            int col = 0;
            xml.putLevel();
            for (int i = 0; i < len; ++i, ++col) {
                  if (col >= 16) {
                        xml.put("");
                        col = 0;
                        xml.putLevel();
                        }
                  xml.nput("%02x ", p[i] & 0xff);
                  }
            if (col)
                  xml.put("");
            xml.etag("init");
            }
      xml.etag("MidiSynti");
      }

//---------------------------------------------------------
//   MidiSynti::read
//---------------------------------------------------------

void MidiSynti::read(QDomNode node)
      {
      bool startGui = false;
      QRect r;
      QString sclass;
      unsigned char* data = 0;
      int len = 0;
      for (; !node.isNull(); node = node.nextSibling()) {
            QDomElement e = node.toElement();
            QString tag(e.tagName());
            if (tag == "class")
                  sclass = e.text();
            else if (tag == "guiVisible")
                  startGui = e.text().toInt();
            else if (tag == "geometry")
                  r = AL::readGeometry(node);
            else if (tag == "init") {
                  len = e.attribute("len", "0").toInt();
                  if (len) {
                        QStringList l = e.text().simplified().split(" ", QString::SkipEmptyParts);
                        if (len != l.size()) {
                              printf("error converting init string <%s>\n", e.text().toLatin1().data());
                              }
                        data = new unsigned char[len];
                        unsigned char* d = data;
                        int numberBase = 16;
                        for (int i = 0; i < l.size(); ++i) {
                              bool ok;
                              *d++ = l.at(i).toInt(&ok, numberBase);
                              if (!ok)
                                    printf("error converting init val <%s>\n", l.at(i).toLatin1().data());
					}
                        }
                  }
            else if (MidiTrackBase::readProperties(node))
                  printf("MusE:MidiSynti: unknown tag %s\n", node.toElement().tagName().toLatin1().data());
            }
      iMidiPlugin i;
      for (i = midiPlugins.begin(); i != midiPlugins.end(); ++i) {
            if ((*i)->type() != MEMPI_GENERATOR)
                  continue;
            if ((*i)->name() == sclass)
                  break;
            }
      if (i == midiPlugins.end()) {
            fprintf(stderr, "MidiSynti::read: midi plugin not found\n");
            return;
            }
      MidiPlugin* mp = *i;
      if (initInstance(mp)) {
            fprintf(stderr, "MidiSynti::read: instantiate failed\n");
            return;
            }
      if (data) {
            _synti->setInitData(len, data);
            delete[] data;
            }
      song->insertTrack0(this, -1);
      _synti->showGui(startGui);
      _synti->setGeometry(r.x(), r.y(), r.width(), r.height());
      }

//---------------------------------------------------------
//   getEvents
//---------------------------------------------------------

void MidiSynti::getEvents(unsigned from, unsigned to, int, MidiEventList* dst)
      {
      MidiEventList il;
      foreach(const Route& r, *inRoutes()) {
            MidiTrackBase* track = (MidiTrackBase*)r.src.track;
            if (track->isMute())
                  continue;
            track->getEvents(from, to, r.src.channel, &il);
            }
      MidiEventList ol;
      _synti->apply(from, to, &il, &ol);
      for (iMidiEvent i = ol.begin(); i != ol.end(); ++i) {
            MidiEvent ev(*i);
            if (ev.type() == ME_NOTEON) {
                  _meter[0] += ev.dataB()/2;
                  if (_meter[0] > 127.0f)
                        _meter[0] = 127.0f;
                  }
            // convert tick-time to sample-time
            ev.setTime(AL::tempomap.tick2frame(ev.time()));
            dst->insert(ev);
            }
      }

bool MidiSynti::isMute() const
      {
      if (_solo)
            return false;
      if (song->solo())
            return true;
      return _mute;
      }


