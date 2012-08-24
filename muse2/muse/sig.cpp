// THIS FILE IS ORPHANED: nothing uses its functions

//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: sig.cpp,v 1.5.2.2 2009/03/09 02:05:17 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
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

#include <stdio.h>
#include "sig.h"
#include "gconfig.h"
#include "xml.h"

namespace MusEGlobal {
MusECore::SigList sigmap;
}

namespace MusECore {

//---------------------------------------------------------
//   SigList
//---------------------------------------------------------

SigList::SigList()
      {
      insert(std::pair<const unsigned, SigEvent*> (MAX_TICK, new SigEvent(4, 4, 0)));
      }

//---------------------------------------------------------
//   add
//    signatures are only allowed at the beginning of
//    a bar
//---------------------------------------------------------

void SigList::add(unsigned tick, int z, int n)
      {
      if (z == 0 || n == 0) {
            printf("THIS SHOULD NEVER HAPPEN: SigList::add() illegal signature %d/%d\n", z, n);
            
            // Added p3.3.43
            return;
            }
      tick = raster1(tick, 0);
      iSigEvent e = upper_bound(tick);
      if (e == end())
      {
        printf("THIS SHOULD NEVER HAPPEN: could not find upper_bound(%i) in SigList::add()!\n", tick);
        return;
      }

      if (tick == e->second->tick) {
            e->second->z = z;
            e->second->n = n;
            }
      else {
            SigEvent* ne = e->second;
            SigEvent* ev = new SigEvent(ne->z, ne->n, ne->tick);
            ne->z = z;
            ne->n = n;
            ne->tick = tick;
            insert(std::pair<const unsigned, SigEvent*> (tick, ev));
            }
      normalize();
      }

//---------------------------------------------------------
//   del
//---------------------------------------------------------

void SigList::del(unsigned tick)
      {
      iSigEvent e = find(tick);
      if (e == end()) {
            printf("SigList::del(%d): not found\n", tick);
            return;
            }
      iSigEvent ne = e;
      ++ne;
      if (ne == end()) {
            printf("SigList::del() HALLO\n");
            return;
            }
      ne->second->z = e->second->z;
      ne->second->n = e->second->n;
      ne->second->tick = e->second->tick;
      erase(e);
      normalize();
      }

//---------------------------------------------------------
//   SigList::normalize
//---------------------------------------------------------

void SigList::normalize()
      {
      int z = 0;
      int n = 0;
      unsigned tick = 0;
      iSigEvent ee;

      for (iSigEvent e = begin(); e != end();) {
            if (z == e->second->z && n == e->second->n) {
                  e->second->tick = tick;
                  erase(ee);
                  }
            z    = e->second->z;
            n    = e->second->n;
            ee   = e;
            tick = e->second->tick;
            ++e;
            }

      int bar = 0;
      for (iSigEvent e = begin(); e != end();) {
            e->second->bar = bar;
            int delta  = e->first - e->second->tick;
            int ticksB = ticks_beat(e->second->n);
            int ticksM = ticksB * e->second->z;
            bar += delta / ticksM;
            if (delta % ticksM)     // Part of a measure
                  ++bar;
            ++e;
            }
      }

//---------------------------------------------------------
//   SigList::dump
//---------------------------------------------------------

void SigList::dump() const
      {
      printf("\nSigList:\n");
      for (ciSigEvent i = begin(); i != end(); ++i) {
            printf("%6d %06d Bar %3d %02d/%d\n",
               i->first, i->second->tick,
               i->second->bar, i->second->z, i->second->n);
            }
      }

void SigList::clear()
      {
      for (iSigEvent i = begin(); i != end(); ++i)
            delete i->second;
      SIGLIST::clear();
      insert(std::pair<const unsigned, SigEvent*> (MAX_TICK, new SigEvent(4, 4, 0)));
      }

//---------------------------------------------------------
//   ticksMeasure
//---------------------------------------------------------

int SigList::ticksMeasure(int Z, int N) const
      {
      return ticks_beat(N) * Z;
      }

int SigList::ticksMeasure(unsigned tick) const
      {
      ciSigEvent i = upper_bound(tick);
      if (i == end()) {
            printf("ticksMeasure: not found %d\n", tick);
            return 0;
            }
      return ticksMeasure(i->second->z, i->second->n);
      }

//---------------------------------------------------------
//   ticksBeat
//---------------------------------------------------------

int SigList::ticksBeat(unsigned tick) const
      {
      ciSigEvent i = upper_bound(tick);
      if (i == end())
      {
        printf("THIS SHOULD NEVER HAPPEN: couldn't find sig event for tick=%i in SigList::ticksBeat()!\n",tick);
        return 0;
      }
      return ticks_beat(i->second->n);
      }

int SigList::ticks_beat(int n) const
      {
      int m = MusEGlobal::config.division;
      switch (n) {
            case  1:  m <<= 2; break;           // 1536
            case  2:  m <<= 1; break;           // 768
            case  3:  m += m >> 1; break;       // 384+192
            case  4:  break;                    // 384
            case  8:  m >>= 1; break;           // 192
            case 16:  m >>= 2; break;           // 96
            case 32:  m >>= 3; break;           // 48
            case 64:  m >>= 4; break;           // 24
            case 128: m >>= 5; break;           // 12
            default: printf("THIS SHOULD NEVER HAPPEN: invalid function call in SigList::ticks_beat(): n=%i\n",n); break;
            }
      return m;
      }

//---------------------------------------------------------
//   timesig
//---------------------------------------------------------

void SigList::timesig(unsigned tick, int& z, int& n) const
      {
      ciSigEvent i = upper_bound(tick);
      if (i == end()) {
            printf("timesig(%d): not found\n", tick);
            z = 4;
            n = 4;
            }
      else  {
            z = i->second->z;
            n = i->second->n;
            }
      }

//---------------------------------------------------------
//   tickValues
//---------------------------------------------------------

void SigList::tickValues(unsigned t, int* bar, int* beat, unsigned* tick) const
      {
      ciSigEvent e = upper_bound(t);
      if (e == end()) {
            fprintf(stderr, "tickValues(0x%x) not found(%zd)\n", t, size());
            *bar = 0;
            *beat = 0;
            *tick = 0;
            return;
            }

      int delta  = t - e->second->tick;
      int ticksB = ticks_beat(e->second->n);
      int ticksM = ticksB * e->second->z;
      *bar       = e->second->bar + delta / ticksM;
      int rest   = delta % ticksM;
      *beat      = rest / ticksB;
      *tick      = rest % ticksB;
      }

//---------------------------------------------------------
//   bar2tick
//---------------------------------------------------------

unsigned SigList::bar2tick(int bar, int beat, unsigned tick) const
      {
      ciSigEvent e;

      if (bar < 0)
            bar = 0;
      for (e = begin(); e != end();) {
            ciSigEvent ee = e;
            ++ee;
            if (ee == end())
                  break;
            if (bar < ee->second->bar)
                  break;
            e = ee;
            }
      int ticksB = ticks_beat(e->second->n);
      int ticksM = ticksB * e->second->z;
      return e->second->tick + (bar-e->second->bar)*ticksM + ticksB*beat + tick;
      }

//---------------------------------------------------------
//   raster
//---------------------------------------------------------

unsigned SigList::raster(unsigned t, int raster) const
      {
      if (raster == 1)
            return t;
      ciSigEvent e = upper_bound(t);
      if (e == end()) {
            printf("SigList::raster(%x,)\n", t);
            return t;
            }
      int delta  = t - e->second->tick;
      int ticksM = ticks_beat(e->second->n) * e->second->z;
      if (raster == 0)
            raster = ticksM;
      int rest   = delta % ticksM;
      int bb     = (delta/ticksM)*ticksM;
      return  e->second->tick + bb + ((rest + raster/2)/raster)*raster;
      }

//---------------------------------------------------------
//   raster1
//    round down
//---------------------------------------------------------

unsigned SigList::raster1(unsigned t, int raster) const
      {
      if (raster == 1)
            return t;
      ciSigEvent e = upper_bound(t);
      if (e == end())
      {
        printf("THIS SHOULD NEVER HAPPEN: couldn't find sig event for tick=%i in SigList::raster1()!\n", t);
        return 0;
      }

      int delta  = t - e->second->tick;
      int ticksM = ticks_beat(e->second->n) * e->second->z;
      if (raster == 0)
            raster = ticksM;
      int rest   = delta % ticksM;
      int bb     = (delta/ticksM)*ticksM;
      return  e->second->tick + bb + (rest/raster)*raster;
      }

//---------------------------------------------------------
//   raster2
//    round up
//---------------------------------------------------------

unsigned SigList::raster2(unsigned t, int raster) const
      {
      if (raster == 1)
            return t;
      ciSigEvent e = upper_bound(t);
      if (e == end())
      {
        printf("THIS SHOULD NEVER HAPPEN: couldn't find sig event for tick=%i in SigList::raster2()!\n", t);
        return 0;
      }

      int delta  = t - e->second->tick;
      int ticksM = ticks_beat(e->second->n) * e->second->z;
      if (raster == 0)
            raster = ticksM;
      int rest   = delta % ticksM;
      int bb     = (delta/ticksM)*ticksM;
      return  e->second->tick + bb + ((rest+raster-1)/raster)*raster;
      }

//---------------------------------------------------------
//   rasterStep
//---------------------------------------------------------

int SigList::rasterStep(unsigned t, int raster) const
      {
      if (raster == 0) {
            ciSigEvent e = upper_bound(t);
            if (e == end())
            {
              printf("THIS SHOULD NEVER HAPPEN: couldn't find sig event for tick=%i in SigList::rasterStep()!\n", t);
              return 0;
            }
            return ticks_beat(e->second->n) * e->second->z;
            }
      return raster;
      }

//---------------------------------------------------------
//   SigList::write
//---------------------------------------------------------

void SigList::write(int level, Xml& xml) const
      {
      xml.tag(level++, "siglist");
      for (ciSigEvent i = begin(); i != end(); ++i)
            i->second->write(level, xml, i->first);
      xml.tag(level, "/siglist");
      }

//---------------------------------------------------------
//   SigList::read
//---------------------------------------------------------

void SigList::read(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "sig") {
                              SigEvent* t = new SigEvent();
                              unsigned tick = t->read(xml);
                              iSigEvent pos = find(tick);
                              if (pos != end())
                                    erase(pos);
                              insert(std::pair<const unsigned, SigEvent*> (tick, t));
                              }
                        else
                              xml.unknown("SigList");
                        break;
                  case Xml::Attribut:
                        break;
                  case Xml::TagEnd:
                        if (tag == "siglist") {
                              normalize();
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   SigEvent::write
//---------------------------------------------------------

void SigEvent::write(int level, Xml& xml, int at) const
      {
      xml.tag(level++, "sig at=\"%d\"", at);
      xml.intTag(level, "tick", tick);
      xml.intTag(level, "nom", z);
      xml.intTag(level, "denom", n);
      xml.tag(level, "/sig");
      }

//---------------------------------------------------------
//   SigEvent::read
//---------------------------------------------------------

int SigEvent::read(Xml& xml)
      {
      int at = 0;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return 0;
                  case Xml::TagStart:
                        if (tag == "tick")
                              tick = xml.parseInt();
                        else if (tag == "nom")
                              z = xml.parseInt();
                        else if (tag == "denom")
                              n = xml.parseInt();
                        else
                              xml.unknown("SigEvent");
                        break;
                  case Xml::Attribut:
                        if (tag == "at")
                              at = xml.s2().toInt();
                        break;
                  case Xml::TagEnd:
                        if (tag == "sig")
                              return at;
                  default:
                        break;
                  }
            }
      return 0;
      }

} // namespace MusECore
