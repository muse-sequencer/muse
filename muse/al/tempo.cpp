//=============================================================================
//  AL
//  Audio Utility Library
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

#include "al.h"
#include "tempo.h"
#include "xml.h"

namespace AL {

TempoList tempomap;

//---------------------------------------------------------
//   TempoList
//---------------------------------------------------------

TempoList::TempoList()
      {
      _tempo   = 500000;
      insert(std::pair<const unsigned, TEvent*> (MAX_TICK+1, new TEvent(_tempo, 0)));
      _tempoSN     = 1;
      _globalTempo = 100;
      useList      = true;
      }

//---------------------------------------------------------
//   add
//---------------------------------------------------------

void TempoList::add(unsigned tick, int tempo)
      {
      if (tick > MAX_TICK)
            tick = MAX_TICK;

      iTEvent e = upper_bound(tick);

      if (tick == e->second->tick)
            e->second->tempo = tempo;
      else {
            TEvent* ne = e->second;
            TEvent* ev = new TEvent(ne->tempo, ne->tick);
            ne->tempo  = tempo;
            ne->tick   = tick;
            insert(std::pair<const unsigned, TEvent*> (tick, ev));
            }
      normalize();
      }

//---------------------------------------------------------
//   TempoList::normalize
//---------------------------------------------------------

void TempoList::normalize()
      {
      int frame = 0;
      for (iTEvent e = begin(); e != end(); ++e) {
            e->second->frame = frame;
            unsigned dtick = e->first - e->second->tick;
            double dtime = double(dtick) / (division * _globalTempo * 10000.0/e->second->tempo);
            frame += lrint(dtime * sampleRate);
            }
      }

//---------------------------------------------------------
//   TempoList::dump
//---------------------------------------------------------

void TempoList::dump() const
      {
      printf("\nTempoList:\n");
      for (ciTEvent i = begin(); i != end(); ++i) {
            printf("%6d %06d Tempo %6d Frame %d\n",
               i->first, i->second->tick, i->second->tempo,
               i->second->frame);
            }
      }

//---------------------------------------------------------
//   clear
//---------------------------------------------------------

void TempoList::clear()
      {
      for (iTEvent i = begin(); i != end(); ++i)
            delete i->second;
      TEMPOLIST::clear();
      insert(std::pair<const unsigned, TEvent*> (MAX_TICK+1, new TEvent(500000, 0)));
      ++_tempoSN;
      }

//---------------------------------------------------------
//   tempo
//---------------------------------------------------------

int TempoList::tempo(unsigned tick) const
      {
      if (useList) {
            ciTEvent i = upper_bound(tick);
            if (i == end()) {
                  printf("no TEMPO at tick %d,0x%x\n", tick, tick);
                  return 1000;
                  }
            return i->second->tempo;
            }
      else
            return _tempo;
      }

//---------------------------------------------------------
//   del
//---------------------------------------------------------

void TempoList::del(unsigned tick)
      {
// printf("TempoList::del(%d)\n", tick);
      iTEvent e = find(tick);
      if (e == end()) {
            printf("TempoList::del(%d): not found\n", tick);
            return;
            }
      del(e);
      ++_tempoSN;
      }

void TempoList::del(iTEvent e)
      {
      iTEvent ne = e;
      ++ne;
      if (ne == end()) {
            printf("TempoList::del(): not found\n");
            return;
            }
      ne->second->tempo = e->second->tempo;
      ne->second->tick  = e->second->tick;
      erase(e);
      normalize();
      ++_tempoSN;
      }

//---------------------------------------------------------
//   change
//---------------------------------------------------------

void TempoList::change(unsigned tick, int newTempo)
      {
      iTEvent e = find(tick);
      e->second->tempo = newTempo;
      normalize();
      ++_tempoSN;
      }

//---------------------------------------------------------
//   setTempo
//    called from transport window
//    & slave mode tempo changes
//---------------------------------------------------------

void TempoList::setTempo(unsigned tick, int newTempo)
      {
      if (useList)
            add(tick, newTempo);
      else
            _tempo = newTempo;
      ++_tempoSN;
      }

//---------------------------------------------------------
//   setGlobalTempo
//---------------------------------------------------------

void TempoList::setGlobalTempo(int val)
      {
      _globalTempo = val;
      ++_tempoSN;
      normalize();
      }

//---------------------------------------------------------
//   addTempo
//---------------------------------------------------------

void TempoList::addTempo(unsigned t, int tempo)
      {
      add(t, tempo);
      ++_tempoSN;
      }

//---------------------------------------------------------
//   delTempo
//---------------------------------------------------------

void TempoList::delTempo(unsigned tick)
      {
      del(tick);
      ++_tempoSN;
      }

//---------------------------------------------------------
//   changeTempo
//---------------------------------------------------------

void TempoList::changeTempo(unsigned tick, int newTempo)
      {
      change(tick, newTempo);
      ++_tempoSN;
      }

//---------------------------------------------------------
//   setMasterFlag
//---------------------------------------------------------

bool TempoList::setMasterFlag(unsigned /*tick*/, bool val)
      {
      if (useList != val) {
            useList = val;
            ++_tempoSN;
            return true;
            }
      return false;
      }

//---------------------------------------------------------
//   tick2frame
//---------------------------------------------------------

unsigned TempoList::tick2frame(unsigned tick, unsigned frame, int* sn) const
      {
      return (*sn == _tempoSN) ? frame : tick2frame(tick, sn);
      }

//---------------------------------------------------------
//   tick2frame
//---------------------------------------------------------

unsigned TempoList::tick2frame(unsigned tick, int* sn) const
      {
      int f;
      if (useList) {
            ciTEvent i = upper_bound(tick);
            if (i == end()) {
                  printf("tick2frame(%d,0x%x): not found\n", tick, tick);
                  abort();  // debug
                  return 0;
                  }
            unsigned dtick = tick - i->second->tick;
            double dtime   = double(dtick) / (division * _globalTempo * 10000.0/ i->second->tempo);
            unsigned dframe   = lrint(dtime * sampleRate);
            f = i->second->frame + dframe;
            }
      else {
            double t = (double(tick) * double(_tempo)) / (double(division) * _globalTempo * 10000.0);
            f = lrint(t * sampleRate);
            }
      if (sn)
            *sn = _tempoSN;
      return f;
      }

//---------------------------------------------------------
//   frame2tick
//    return cached value t if list did not change
//---------------------------------------------------------

unsigned TempoList::frame2tick(unsigned frame, unsigned t, int* sn) const
      {
      return (*sn == _tempoSN) ? t : frame2tick(frame, sn);
      }

//---------------------------------------------------------
//   frame2tick
//---------------------------------------------------------

unsigned TempoList::frame2tick(unsigned frame, int* sn) const
      {
      unsigned tick;
      if (useList) {
            ciTEvent e;
            for (e = begin(); e != end();) {
                  ciTEvent ee = e;
                  ++ee;
                  if (ee == end())
                        break;
                  if (frame < ee->second->frame)
                        break;
                  e = ee;
                  }
            unsigned te  = e->second->tempo;
            int dframe   = frame - e->second->frame;
            double dtime = double(dframe) / double(sampleRate);
            tick         = e->second->tick + lrint(dtime * _globalTempo * division * 10000.0 / te);
            }
      else
            tick = lrint((double(frame)/double(sampleRate)) * _globalTempo * division * 10000.0 / double(_tempo));
      if (sn)
            *sn = _tempoSN;
      return tick;
      }

//---------------------------------------------------------
//   TempoList::write
//---------------------------------------------------------

void TempoList::write(Xml& xml) const
      {
      xml.tag("tempolist fix=\"%d\"", _tempo);
      if (_globalTempo != 100)
            xml.intTag("globalTempo", _globalTempo);
      for (ciTEvent i = begin(); i != end(); ++i)
            i->second->write(xml, i->first);
      xml.etag("tempolist");
      }

//---------------------------------------------------------
//   TempoList::read
//---------------------------------------------------------

void TempoList::read(QDomNode node)
      {
      QDomElement e = node.toElement();
      _tempo = e.attribute("fix","500000").toInt();

      node = node.firstChild();
      while (!node.isNull()) {
            e = node.toElement();
            if (e.tagName() == "tempo") {
                  TEvent* t = new TEvent();
                  unsigned tick = t->read(node);
                  iTEvent pos = find(tick);
                  if (pos != end())
                        erase(pos);
                  insert(std::pair<const int, TEvent*> (tick, t));
                  }
            else if (e.tagName() == "globalTempo")
                  _globalTempo = e.text().toInt();
            else
                  printf("MusE:Tempolist: unknown tag %s\n", e.tagName().toLatin1().data());
            node = node.nextSibling();
            }
      normalize();
      ++_tempoSN;
      }

//---------------------------------------------------------
//   TEvent::write
//---------------------------------------------------------

void TEvent::write(Xml& xml, int at) const
      {
      xml.tag("tempo at=\"%d\"", at);
      xml.intTag("tick", tick);
      xml.intTag("val", tempo);
      xml.etag("tempo");
      }

//---------------------------------------------------------
//   TEvent::read
//---------------------------------------------------------

int TEvent::read(QDomNode node)
      {
      QDomElement e = node.toElement();
      int at = e.attribute("at","0").toInt();

      node = node.firstChild();
      while (!node.isNull()) {
            e = node.toElement();
            if (e.tagName() == "tick")
                  tick = e.text().toInt();
            else if (e.tagName() == "val")
                  tempo = e.text().toInt();
            else
                  printf("MusE:TEvent: unknown tag %s\n", e.tagName().toLatin1().data());
            node = node.nextSibling();
            }
      return at;
      }
}
