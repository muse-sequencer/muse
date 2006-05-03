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
      "Default",
      "Refrain",
      "Bridge",
      "Intro",
      "Coda",
      "Chorus",
      "Solo",
      "Brass",
      "Percussion",
      "Drums",
      "Guitar",
      "Bass",
      "Flute",
      "Strings",
      "Keyboard",
      "Piano",
      "Saxophon",
      };

CloneList cloneList;

//---------------------------------------------------------
//   init
//---------------------------------------------------------

void Part::init()
      {
      _raster     = -1;		// invalid
      _quant      = -1;
      _xmag       = -1.0;

      _selected   = false;
      _mute       = false;
      _colorIndex = 0;
      _fillLen    = 0;
      _events->incRef(1);
      _events->incARef(1);
      if (_track->type() == Track::WAVE)
            setType(AL::FRAMES);
      }

//---------------------------------------------------------
//   Part
//---------------------------------------------------------

Part::Part(Track* t)
      {
      _track      = t;
      _events     = new EventList;
      init();
      }

Part::Part(const Part& p)
   : PosLen(p)
      {
      _track      = p._track;
      _selected   = p._selected;
      _mute       = p._mute;
      _colorIndex = p._colorIndex;
      _name       = p._name;
      _events     = p._events;
      _quant      = p._quant;
      _raster     = p._raster;
      _xmag       = p._xmag;
      _fillLen    = p._fillLen;
      _events->incRef(1);
      }

Part::Part(const Part& p, EventList* el)
   : PosLen(p)
      {
      _track      = p._track;
      _selected   = p._selected;
      _mute       = p._mute;
      _colorIndex = p._colorIndex;
      _name       = p._name;
      _quant      = p._quant;
      _raster     = p._raster;
      _xmag       = p._xmag;
      _fillLen    = p._fillLen;
      _events     = el;
      _events->incRef(1);
      _events->incARef(1);
      }

//---------------------------------------------------------
//   Part
//---------------------------------------------------------

Part::Part(Track* t, EventList* el)
      {
      _track      = t;
      _events     = el;
      init();
      }

//---------------------------------------------------------
//   Part
//---------------------------------------------------------

Part::~Part()
      {
      _events->incRef(-1);
      if (_events->refCount() <= 0)
            delete _events;
      }

//---------------------------------------------------------
//   addEvent
//---------------------------------------------------------

iEvent Part::addEvent(Event& p)
      {
      return _events->add(p);
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
//   splitPart
//    split part "part" at "tick" position
//    create two new parts p1 and p2
//---------------------------------------------------------

void Track::splitPart(Part* part, int tickpos, Part*& p1, Part*& p2)
      {
      int l1 = 0;       // len of first new part (ticks or samples)
      int l2 = 0;       // len of second new part

      int samplepos = AL::tempomap.tick2frame(tickpos);

      switch (type()) {
            case WAVE:
                  l1 = samplepos - part->frame();
                  l2 = part->lenFrame() - l1;
                  break;
            case MIDI:
                  l1 = tickpos - part->tick();
                  l2 = part->lenTick() - l1;
                  break;
            default:
                  return;
            }

      if (l1 <= 0 || l2 <= 0)
            return;

      p1 = newPart(part);     // new left part
      p2 = newPart(part);     // new right part

      switch (type()) {
            case WAVE:
                  p1->setLenFrame(l1);
                  p2->setFrame(samplepos);
                  p2->setLenFrame(l2);
                  break;
            case MIDI:
                  p1->setLenTick(l1);
                  p2->setTick(tickpos);
                  p2->setLenTick(l2);
                  break;
            default:
                  break;
            }

      EventList* se  = part->events();
      EventList* de1 = p1->events();
      EventList* de2 = p2->events();

      if (type() == WAVE) {
            int ps   = part->frame();
            int d1p1 = p1->frame();
            int d2p1 = p1->endFrame();
            int d1p2 = p2->frame();
            int d2p2 = p2->endFrame();
            for (iEvent ie = se->begin(); ie != se->end(); ++ie) {
                  Event event = ie->second;
                  int s1 = event.frame() + ps;
                  int s2 = event.endFrame() + ps;

                  if ((s2 > d1p1) && (s1 < d2p1)) {
                        Event si = event.mid(d1p1 - ps, d2p1 - ps);
                        de1->add(si);
                        }
                  if ((s2 > d1p2) && (s1 < d2p2)) {
                        Event si = event.mid(d1p2 - ps, d2p2 - ps);
                        si.setFrame(si.frame() - l1);       //??
                        si.setFrame(0);                     //??
                        de2->add(si);
                        }
                  }
            }
      else {
            for (iEvent ie = se->begin(); ie != se->end(); ++ie) {
                  Event event = ie->second.clone();
                  int t = event.tick();
                  if (t >= l1) {
                        event.move(-l1);
                        de2->add(event);
                        }
                  else
                        de1->add(event);
                  }
            }
      }

//---------------------------------------------------------
//   cmdSplitPart
//---------------------------------------------------------

void Song::cmdSplitPart(Part* part, const Pos& pos)
      {
      int tick = pos.tick();
      int l1 = tick - part->tick();
      int l2 = part->lenTick() - l1;
      if (l1 <= 0 || l2 <= 0)
            return;
      Part* p1;
      Part* p2;
      part->track()->splitPart(part, tick, p1, p2);

      startUndo();
      audio->msgChangePart(part, p1, false);
      audio->msgAddPart(p2, false);
      endUndo(SC_TRACK_MODIFIED | SC_PART_MODIFIED | SC_PART_INSERTED);
      part->track()->partListChanged();
      }

//---------------------------------------------------------
//   cmdGluePart
//---------------------------------------------------------

void Song::cmdGluePart(Part* oPart)
      {
      Track* track   = oPart->track();
      PartList* pl   = track->parts();
      Part* nextPart = 0;

      for (iPart ip = pl->begin(); ip != pl->end(); ++ip) {
            if (ip->second == oPart) {
                  ++ip;
                  if (ip == pl->end())
                        return;
                  nextPart = ip->second;
                  break;
                  }
            }

      Part* nPart = track->newPart(oPart);
      nPart->setLenTick(nextPart->tick() + nextPart->lenTick() - oPart->tick());

      // populate nPart with Events from oPart and nextPart

      EventList* sl1 = oPart->events();
      EventList* dl  = nPart->events();

      for (iEvent ie = sl1->begin(); ie != sl1->end(); ++ie)
            dl->add(ie->second);

      EventList* sl2 = nextPart->events();
      int tickOffset = nextPart->tick() - oPart->tick();

      for (iEvent ie = sl2->begin(); ie != sl2->end(); ++ie) {
            Event event = ie->second.clone();
            event.move(tickOffset);
            dl->add(event);
            }
      startUndo();
      audio->msgRemovePart(nextPart, false);
      audio->msgChangePart(oPart, nPart, false);
      endUndo(SC_PART_MODIFIED | SC_PART_REMOVED);
      track->partListChanged();
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

void Part::write(Xml& xml) const
      {
      const EventList* el = events();
      int id              = -1;
      bool dumpEvents     = true;

      if (el->arefCount() > 1) {
            for (iClone i = cloneList.begin(); i != cloneList.end(); ++i) {
                  if (i->el == el) {
                        id = i->id;
                        dumpEvents = false;
                        break;
                        }
                  }
            if (id == -1) {
                  id = cloneList.size();
                  ClonePart cp(el, id);
                  cloneList.push_back(cp);
                  }
            }

      if (id != -1)
            xml.tag("part cloneId=\"%d\"", id);
      else
            xml.tag("part");
      xml.strTag("name", _name);

      PosLen::write(xml, "poslen");
      xml.intTag("selected", _selected);
      xml.intTag("color", _colorIndex);
      if (_raster != -1)
      	xml.intTag("raster", _raster);
      if (_quant != -1)
      	xml.intTag("quant", _quant);
      if (_xmag != -1.0)
            xml.doubleTag("xmag", _xmag);
      for (ciCtrlCanvas i = ctrlCanvasList.begin(); i != ctrlCanvasList.end(); ++i)
            xml.tagE("CtrlCanvas h=\"%d\" id=\"%d\"",
               i->height, i->ctrlId);
      if (_fillLen)
            xml.intTag("fillLen", _fillLen);
      if (_mute)
            xml.intTag("mute", _mute);
      if (dumpEvents) {
            for (ciEvent e = el->begin(); e != el->end(); ++e)
                  e->second.write(xml, *this);
            }
      xml.etag("part");
      }

//---------------------------------------------------------
//   Part::read
//---------------------------------------------------------

void Part::read(QDomNode node)
      {
      int id = -1;
      bool containsEvents = false;

	ctrlCanvasList.clear();
      while (!node.isNull()) {
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
                  containsEvents = true;
                  EventType type = Wave;
                  if (_track->isMidiTrack())
                        type = Note;
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
                                    _events->add(e);
                              }
                        else
#endif
                              _events->add(e);
                        }
                  }
            else if (tag == "cloneId")
                  id = i;
            else
                  printf("MusE:read: unknown tag %s\n", e.tagName().toLatin1().data());
            node = node.nextSibling();
            }

      if (id != -1) {
            // clone part
            if (containsEvents) {
                  // add to cloneList:
                  ClonePart cp(_events, id);
                  cloneList.push_back(cp);
                  }
            else {
                  // replace event list with clone event
                  // list
                  for (iClone i = cloneList.begin();
                     i != cloneList.end(); ++i) {
                        if (i->id == id) {
                              delete _events;
                              _events = (EventList*)(i->el);
                              _events->incRef(1);
                              _events->incARef(1);
                              break;
                              }
                        }
                  }
            }
      }


