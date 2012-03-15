//=============================================================================
//  AL
//  Audio Utility Library
//  $Id:$
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License
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
//=============================================================================


///#include "al.h"
#include "gconfig.h"  // Tim
#include "sig.h"
///#include "xml.h"


namespace AL {

SigList sigmap;

//---------------------------------------------------------
//   isValid
//---------------------------------------------------------

bool TimeSignature::isValid() const
{
  if((z < 1) || (z > 63))
    return false;
            
  switch(n) 
  {
    case  1:
    case  2:
    case  3:
    case  4:
    case  8:
    case 16:
    case 32:
    case 64:
    case 128:
      return true;
    default:
      return false;
  }                
}

//---------------------------------------------------------
//   SigList
//---------------------------------------------------------

SigList::SigList()
      {
      insert(std::pair<const unsigned, SigEvent*> (MAX_TICK, new SigEvent(TimeSignature(4, 4), 0)));
      }

SigList::~SigList()
      {
      for (iSigEvent i = begin(); i != end(); ++i)
            delete i->second;
      }

//---------------------------------------------------------
//   add
//    signatures are only allowed at the beginning of
//    a bar
//---------------------------------------------------------

void SigList::add(unsigned tick, const TimeSignature& s)
      {
      if (s.z == 0 || s.n == 0) {
            printf("illegal signature %d/%d\n", s.z, s.n);
            return;
            }
      tick = raster1(tick, 0);
      iSigEvent e = upper_bound(tick);
      if(e == end())
      {
        printf("SigList::add Signal not found tick:%d\n", tick);
        return;
      }
      
      if (tick == e->second->tick) {
            e->second->sig = s;
            }
      else {
            SigEvent* ne = e->second;
            SigEvent* ev = new SigEvent(ne->sig, ne->tick);
            ne->sig = s;
            ne->tick = tick;
            insert(std::pair<const unsigned, SigEvent*> (tick, ev));
            }
      normalize();
      }

/*
void SigList::add(unsigned tick, int z, int n)
      {
      if (z == 0 || n == 0) {
            printf("SigList::add illegal signature %d/%d\n", z, n);
            // Added by Tim.
            return;
            }
      tick = raster1(tick, 0);
      iSigEvent e = upper_bound(tick);
      if(e == end())
      {
        printf("SigList::add Signal not found tick:%d\n", tick);
        return;
      }
      
      if (tick == e->second->tick) {
            e->second->sig.z = z;
            e->second->sig.n = n;
            }
      else {
            SigEvent* ne = e->second;
            SigEvent* ev = new SigEvent(ne->sig.z, ne->sig.n, ne->tick);
            ne->sig.z = z;
            ne->sig.n = n;
            ne->tick = tick;
            insert(std::pair<const unsigned, SigEvent*> (tick, ev));
            }
      normalize();
      }
*/

//---------------------------------------------------------
//   del
//---------------------------------------------------------

void SigList::del(unsigned tick)
      {
// printf("SigList::del(%d)\n", tick);
      iSigEvent e = find(tick);
      if (e == end()) {
            printf("SigList::del(%d): not found\n", tick);
            return;
            }
      iSigEvent ne = e;
      ++ne;
      if (ne == end()) {
            printf("SigList::del() next event not found!\n");
            return;
            }
      ne->second->sig = e->second->sig;
      ne->second->tick  = e->second->tick;
      erase(e);
      normalize();
      }

//---------------------------------------------------------
//   SigList::normalize
//---------------------------------------------------------

void SigList::normalize()
      {
      TimeSignature sig(0, 0);
      unsigned tick = 0;
      iSigEvent ee;

      for (iSigEvent e = begin(); e != end();) {
            if (sig.z == e->second->sig.z && sig.n == e->second->sig.n) {
                  e->second->tick = tick;
                  erase(ee);
                  }
            sig  = e->second->sig;
            ee   = e;
            tick = e->second->tick;
            ++e;
            }

      int bar = 0;
      for (iSigEvent e = begin(); e != end();) {
            e->second->bar = bar;
            int delta  = e->first - e->second->tick;
            int ticksB = ticks_beat(e->second->sig.n);
            int ticksM = ticksB * e->second->sig.z;
            bar += delta / ticksM;
            if (delta % ticksM)     // Teil eines Taktes
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
               i->second->bar, i->second->sig.z, i->second->sig.n);
            }
      }

void SigList::clear()
      {
      for (iSigEvent i = begin(); i != end(); ++i)
            delete i->second;
      SIGLIST::clear();
      insert(std::pair<const unsigned, SigEvent*> (MAX_TICK, new SigEvent(TimeSignature(4, 4), 0)));
      }

//---------------------------------------------------------
//   ticksMeasure
//---------------------------------------------------------

int SigList::ticksMeasure(const TimeSignature& sig) const
      {
      return ticks_beat(sig.n) * sig.z;
      }

int SigList::ticksMeasure(int Z, int N) const
      {
      return ticks_beat(N) * Z;
      }

int SigList::ticksMeasure(unsigned tick) const
      {
      ciSigEvent i = upper_bound(tick);
      if (i == end()) {
            printf("ticksMeasure: not found %d\n", tick);
            // abort();
            return 0;
            }
      return ticksMeasure(i->second->sig);
      }

//---------------------------------------------------------
//   ticksBeat
//---------------------------------------------------------

int SigList::ticksBeat(unsigned tick) const
      {
      ciSigEvent i = upper_bound(tick);
      if(i == end())
      {
        printf("SigList::ticksBeat event not found! tick:%d\n", tick);
        return 0;
      }
      return ticks_beat(i->second->sig.n);
      }

int SigList::ticks_beat(int n) const
      {
      
      ///int m = AL::division;
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
            default: break;
            }
      return m;
      }

//---------------------------------------------------------
//   timesig
//---------------------------------------------------------

TimeSignature SigList::timesig(unsigned tick) const
      {
      ciSigEvent i = upper_bound(tick);
      if (i == end()) {
            printf("timesig(%d): not found\n", tick);
            // abort();
            return TimeSignature(4,4);
            }
      return i->second->sig;
      }

void SigList::timesig(unsigned tick, int& z, int& n) const
      {
      ciSigEvent i = upper_bound(tick);
      if (i == end()) {
            printf("timesig(%d): not found\n", tick);
            // abort();
            z = 4;
            n = 4;
            }
      else  {
            z = i->second->sig.z;
            n = i->second->sig.n;
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
      int ticksB = ticks_beat(e->second->sig.n);
      int ticksM = ticksB * e->second->sig.z;
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
      int ticksB = ticks_beat(e->second->sig.n);
      int ticksM = ticksB * e->second->sig.z;
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
            // abort();
            return t;
            }
      int delta  = t - e->second->tick;
      int ticksM = ticks_beat(e->second->sig.n) * e->second->sig.z;
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
      if(e == end())
      {
        printf("SigList::raster1 event not found tick:%d\n", t);
        //return 0;
        return t;
      }

      int delta  = t - e->second->tick;
      int ticksM = ticks_beat(e->second->sig.n) * e->second->sig.z;
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
      if(e == end())
      {
        printf("SigList::raster2 event not found tick:%d\n", t);
        //return 0;
        return t;
      }

      int delta  = t - e->second->tick;
      int ticksM = ticks_beat(e->second->sig.n) * e->second->sig.z;
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
            if(e == end())
            {
              printf("SigList::rasterStep event not found tick:%d\n", t);
              //return 0;
              return raster;
            }
      
            return ticks_beat(e->second->sig.n) * e->second->sig.z;
            }
      return raster;
      }

//---------------------------------------------------------
//   SigList::write
//---------------------------------------------------------

#if 0
void SigList::write(MusECore::Xml& xml) const
      {
      xml.stag("siglist");
      for (ciSigEvent i = begin(); i != end(); ++i)
            i->second->write(xml, i->first);
      xml.etag("siglist");
      }
#endif

void SigList::write(int level, MusECore::Xml& xml) const
      {
      xml.tag(level++, "siglist");
      for (ciSigEvent i = begin(); i != end(); ++i)
            i->second->write(level, xml, i->first);
      xml.tag(level, "/siglist");
      }

//---------------------------------------------------------
//   SigList::read
//---------------------------------------------------------

#if 0
void SigList::read(QDomNode node)
      {
      while (!node.isNull()) {
            QDomElement e = node.toElement();
            if (e.tagName() == "sig") {
                  SigEvent* t = new SigEvent();
                  unsigned tick = t->read(node);
                  iSigEvent pos = find(tick);
                  if (pos != end())
                        erase(pos);
                  insert(std::pair<const unsigned, SigEvent*> (tick, t));
                  }
            else
                  printf("MusE:SigList: unknown tag %s\n", e.tagName().toLatin1().constData());
            node = node.nextSibling();
            }
      normalize();
      }
#endif

void SigList::read(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
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
                  case MusECore::Xml::Attribut:
                        break;
                  case MusECore::Xml::TagEnd:
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

#if 0
void SigEvent::write(MusECore::Xml& xml, int at) const
      {
      xml.stag(QString("sig at=\"%1\"").arg(at));
      xml.tag("tick", tick);
      xml.tag("nom", sig.z);
      xml.tag("denom", sig.n);
      xml.etag("sig");
      }
#endif

void SigEvent::write(int level, MusECore::Xml& xml, int at) const
      {
      xml.tag(level++, "sig at=\"%d\"", at);
      xml.intTag(level, "tick", tick);
      xml.intTag(level, "nom", sig.z);
      xml.intTag(level, "denom", sig.n);
      xml.tag(level, "/sig");
      }

//---------------------------------------------------------
//   SigEvent::read
//---------------------------------------------------------

#if 0
int SigEvent::read(QDomNode node)
      {
      QDomElement e = node.toElement();
      int at = e.attribute("at", "0").toInt();
      node = node.firstChild();

      while (!node.isNull()) {
            QDomElement e = node.toElement();
            if (e.tagName() == "tick")
                  tick = e.text().toInt();
            else if (e.tagName() == "nom")
                  sig.z = e.text().toInt();
            else if (e.tagName() == "denom")
                  sig.n = e.text().toInt();
            else
                  printf("MusE:SigEvent: unknown tag %s\n", e.tagName().toLatin1().constData());
            node = node.nextSibling();
            }
      return at;
      }

}
#endif

int SigEvent::read(MusECore::Xml& xml)
      {
      int at = 0;
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return 0;
                  case MusECore::Xml::TagStart:
                        if (tag == "tick")
                              tick = xml.parseInt();
                        else if (tag == "nom")
                              sig.z = xml.parseInt();
                        else if (tag == "denom")
                              sig.n = xml.parseInt();
                        else
                              xml.unknown("SigEvent");
                        break;
                  case MusECore::Xml::Attribut:
                        if (tag == "at")
                              at = xml.s2().toInt();
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "sig")
                              return at;
                  default:
                        break;
                  }
            }
      return 0;
      }


} // namespace AL

