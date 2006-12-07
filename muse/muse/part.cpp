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
#include "part.h"
#include "track.h"
#include "globals.h"
#include "event.h"
#include "audio.h"
#include "al/tempo.h"

const char* partColorNames[] = {
      QT_TR_NOOP("Default"),
      QT_TR_NOOP("Refrain"),
      QT_TR_NOOP("Bridge"),
      QT_TR_NOOP("Intro"),
      QT_TR_NOOP("Coda"),
      QT_TR_NOOP("Chorus"),
      QT_TR_NOOP("Solo"),
      QT_TR_NOOP("Brass"),
      QT_TR_NOOP("Percussion"),
      QT_TR_NOOP("Drums"),
      QT_TR_NOOP("Guitar"),
      QT_TR_NOOP("Bass"),
      QT_TR_NOOP("Flute"),
      QT_TR_NOOP("Strings"),
      QT_TR_NOOP("Keyboard"),
      QT_TR_NOOP("Piano"),
      QT_TR_NOOP("Saxophon"),
      };

CloneList cloneList;

//---------------------------------------------------------
//   Part
//---------------------------------------------------------

Part::Part(Track* t)
      {
      _selected   = false;
      _mute       = false;
      _colorIndex = 0;
      _raster     = -1;		// invalid
      _quant      = -1;
      _xmag       = -1.0;
      _fillLen    = 0;
      _track      = t;
      _events     = 0;
      if (_track && _track->type() == Track::WAVE)
            setType(AL::FRAMES);
      }

//---------------------------------------------------------
//   clone
//---------------------------------------------------------

void Part::clone(EventList* e)  
      {
      _events = e;
      ref();
      }

//---------------------------------------------------------
//   addEvent
//---------------------------------------------------------

iEvent Part::addEvent(Event& p)
      {
      return _events->add(p);
      }

iEvent Part::addEvent(Event& p, unsigned t)
      {
      return _events->add(p, t);
      }

//---------------------------------------------------------
//   index
//---------------------------------------------------------

int PartList::index(Part* part)
      {
      int index = 0;
      for (iPart i = begin(); i != end(); ++i, ++index)
            if (i->second == part) {
                  return index;
                  }
      printf("PartList::index(): not found!\n");
      abort();
//      return 0;
      }

//---------------------------------------------------------
//   find
//---------------------------------------------------------

Part* PartList::find(int idx)
      {
      int index = 0;
      for (iPart i = begin(); i != end(); ++i, ++index)
            if (index == idx)
                  return i->second;
      return 0;
      }

//---------------------------------------------------------
//   findPart
//    search for part which contains tick
//---------------------------------------------------------

Part* PartList::findPart(unsigned tick)
      {
      iPart i = lower_bound(tick);
      if (i != begin())
            --i;
      for (; i != end(); ++i) {
            unsigned tick1 = i->second->tick();
            unsigned tick2 = tick1 + i->second->lenTick();

            if (tick >= tick2)
                  continue;
            if (tick >= tick1)
                  return i->second;
            if (tick < tick1)
                  break;
            }
      return 0;
      }

//---------------------------------------------------------
//   add
//---------------------------------------------------------

iPart PartList::add(Part* part)
      {
      return insert(std::pair<const unsigned, Part*> (part->tick(), part));
      }

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void PartList::remove(Part* part)
      {
      iPart i;
      for (i = begin(); i != end(); ++i) {
            if (i->second == part) {
                  erase(i);
                  return;
                  }
            }
printf("remove part: not found\n");
      assert(i != end());
      }

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void Part::dump(int n) const
      {
      for (int i = 0; i < n; ++i)
            putchar(' ');
      printf("Part: <%s> ", _name.toLatin1().data());
      for (int i = 0; i < n; ++i)
            putchar(' ');
      PosLen::dump();
      }

//---------------------------------------------------------
//   Part::write
//---------------------------------------------------------

void Part::write(Xml& xml)
      {
      int id              = -1;
      bool dumpEvents     = true;


      if (isClone()) {
            // we have to dump the event list only on first
            // incarnation of clone

            for (iClone i = cloneList.begin(); i != cloneList.end(); ++i) {
                  if (i->el == _events) {
                        id = i->id;
                        dumpEvents = false;
                        break;
                        }
                  }
            if (id == -1)
                  cloneList.push_back(ClonePart(_events, cloneList.size()));
            }

      if (id != -1)
            xml.stag(QString("part cloneId=\"%1\"").arg(id));
      else
            xml.stag("part");
      if (!_name.isEmpty())
            xml.tag("name", _name);

      PosLen::write(xml, "poslen");
      if (_selected)
            xml.tag("selected", _selected);
      xml.tag("color", _colorIndex);
      if (_raster != -1)
      	xml.tag("raster", _raster);
      if (_quant != -1)
      	xml.tag("quant", _quant);
      if (_xmag != -1.0)
            xml.tag("xmag", _xmag);
      for (ciCtrlCanvas i = ctrlCanvasList.begin(); i != ctrlCanvasList.end(); ++i)
            xml.tagE(QString("CtrlCanvas h=\"%1\" id=\"%2\"").arg(i->height).arg(i->ctrlId));
      if (_fillLen)
            xml.tag("fillLen", _fillLen);
      if (_mute)
            xml.tag("mute", _mute);
      if (dumpEvents) {
            for (ciEvent e = _events->begin(); e != _events->end(); ++e)
                  e->second.write(xml, *this);
            }
      xml.etag("part");
      }

//---------------------------------------------------------
//   Part::read
//---------------------------------------------------------

void Part::read(QDomNode node, bool isMidiPart)
      {
      QDomElement e = node.toElement();
      int id = e.attribute("cloneId", "-1").toInt();

	ctrlCanvasList.clear();
      for (node = node.firstChild(); !node.isNull(); node = node.nextSibling()) {
            QDomElement e = node.toElement();
            QString tag(e.tagName());
            QString s(e.text());
            int i = s.toInt();
            if (tag == "name")
                  _name = s;
            else if (tag == "poslen")
                  PosLen::read(node);
            else if (tag == "selected")
                  _selected = i;
            else if (tag == "color")
                  _colorIndex = i;
            else if (tag == "raster")
                  _raster = i;
            else if (tag == "quant")
                  _quant = i;
            else if (tag == "xmag")
                  _xmag = s.toDouble();
            else if (tag == "CtrlCanvas") {
                  CtrlCanvas c;
                  c.ctrlId = e.attribute("id","0").toInt();
                  c.height = e.attribute("h","50").toInt();
                  ctrlCanvasList.push_back(c);
                  }
            else if (tag == "mute")
                  _mute = i;
            else if (tag == "fillLen")
                  _fillLen = i;
            else if (tag == "event") {
                  EventType type = isMidiPart ? Note : Wave;
                  Event e(type);
                  e.read(node);
                  // tickpos is relative to start of part
                  // TODO: better handling for wave event
                  e.move(-tick());
                  int tick = e.tick();
                  if ((tick < 0) || (tick >= int(lenTick()))) {
                        printf("ReadEvent: warning: event not in part: %d - %d - %d, discarded\n",
                           0, tick, lenTick());
                        }
                  else {
#if 0
                        if (e.type() == Controller) {
                              MidiChannel* mc = ((MidiTrack*)_track)->channel();
                              if (mc) {
                                    CVal v;
                                    v.i = e.dataB();
                                    mc->addControllerVal(e.dataA(), tick, v);
                                    }
                              else
                                    _events.add(e);
                              }
                        else
#endif
                              _events->add(e);
                        }
                  }
            else
                  printf("MusE:read: unknown tag %s\n", e.tagName().toLatin1().data());
            }

      if (id != -1) {
            bool found = false;
            for (iClone i = cloneList.begin(); i != cloneList.end(); ++i) {
                  if (i->id == id) {
                        if (_events->size())
                              printf("MusE::internal error: clone part contains events\n");
                        clone(i->el);
                        found = true;
                        break;
                        }
                  }
            if (!found) {
                  // add to cloneList:
                  ClonePart cp(_events, id);
                  cloneList.push_back(cp);
                  }
            }
      }

//---------------------------------------------------------
//   isClone
//---------------------------------------------------------

bool Part::isClone() const
      { 
      return _events->cloneCount > 1; 
      }

//---------------------------------------------------------
//   ref
//---------------------------------------------------------

void Part::ref()
      {
      if (_events == 0)
            _events = new EventList;
      ++(_events->cloneCount);
      }

//---------------------------------------------------------
//   deref
//---------------------------------------------------------

void Part::deref()
      {
      --(_events->cloneCount);
      }

