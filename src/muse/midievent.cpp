//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midievent.cpp,v 1.7.2.2 2009/05/24 21:43:44 terminator356 Exp $
//
//  (C) Copyright 2000-2003 Werner Schweer (ws@seh.de)
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

#include "midievent.h"
#include "xml.h"
#include "mpevent.h"
#include "midictrl.h"

namespace MusECore {

//---------------------------------------------------------
//   MidiEventBase
//---------------------------------------------------------

MidiEventBase::MidiEventBase(EventType t)
   : EventBase(t)
      {
      a = 0;
      b = 0;
      c = 0;
      }

MidiEventBase::MidiEventBase(const MidiEventBase& ev, bool duplicate_not_clone)
   : EventBase(ev, duplicate_not_clone)
{
      a = ev.a;
      b = ev.b;
      c = ev.c;
      if(duplicate_not_clone)
        edata.setData(ev.constData(), ev.dataLen()); // Makes a copy.
      else
        // NOTE: Even non-shared clone events ALWAYS share edata. edata does NOT currently require 
        //        separate instances, unlike wave events which absolutely do.
        //       Be aware when iterating or modifying clones for example. (It can save time.)
        edata = ev.edata;
}

//---------------------------------------------------------
//   assign
//---------------------------------------------------------

void MidiEventBase::assign(const EventBase& ev)  
{
  if(ev.type() != type())
    return;
  EventBase::assign(ev);
  a = ev.dataA();
  b = ev.dataB();
  c = ev.dataC();
  // NOTE: Even non-shared clone events ALWAYS share edata. edata does NOT currently require 
  //        separate instances, unlike wave events which absolutely do.
  //       Be aware when iterating or modifying clones for example. (It can save time.)
  if(edata.constData() != ev.constData())
    edata.setData(ev.constData(), ev.dataLen()); // Makes a copy.
}

bool MidiEventBase::isSimilarTo(const EventBase& other_) const
{
	const MidiEventBase* other = dynamic_cast<const MidiEventBase*>(&other_);
	if (other==nullptr) // dynamic cast hsa failed: "other_" is not of type MidiEventBase.
		return false;
	
	if ((a==other->a && b==other->b && c==other->c && edata.dataLen()==other->edata.dataLen() && this->PosLen::operator==(*other)) == false)
		return false;
	
	if (edata.dataLen() > 0)
		return (memcmp(edata.constData(), other->edata.constData(), edata.dataLen() ) == 0);
	else
		return true; // no data equals no data.
}

//---------------------------------------------------------
//   MidiEventBase::mid
//---------------------------------------------------------

EventBase* MidiEventBase::mid(unsigned b, unsigned e) const
      {
      if (tick() < b || tick() >= e)
            return 0;
      return new MidiEventBase(*this);
      }

//---------------------------------------------------------
//   isNoteOff
//---------------------------------------------------------

bool MidiEventBase::isNoteOff() const
      {
      return (type() == Note && (velo() == 0));
      }

void MidiEventBase::dump(int n) const
      {
      EventBase::dump(n);
      const char* p;

      switch(type()) {
            case Note:      p = "Note    "; break;
            case Controller: p = "Ctrl    "; break;
            case Sysex:     p = "Sysex   "; break;
            case Meta:      p = "Meta    "; break;
            default:        p = "??      "; break;
            }
      for (int i = 0; i < (n+2); ++i)
            putchar(' ');
      printf("<%s> a:0x%x(%d) b:0x%x(%d) c:0x%x(%d)\n",
         p, a, a, b, b, c, c);
      }

//---------------------------------------------------------
//   MidiEventBase::write
//---------------------------------------------------------

void MidiEventBase::write(int level, Xml& xml, const Pos& offset, bool /*forcePath*/) const
      {
      xml.nput(level++, "<event tick=\"%d\"", tick() + offset.tick());
      switch (type()) {
            case Note:
                  xml.nput(" len=\"%d\"", lenTick());
                  break;
            case Controller:
                  xml.nput(" type=\"%d\"", type());
                  // HACK We abuse the length member as a visual length for certain temporary
                  //       operations such as copy/paste. Write it here but make sure
                  //       to DISCARD it later, never store it into an actual file,
                  //       the length is normally ZERO for all controllers.
                  if(lenTick() != 0)
                    xml.nput(" len=\"%d\"", lenTick());
                  break;
            default:
                  xml.nput(" type=\"%d\"", type());
                  break;
            }
      
      // Changed by T356. BUG: *.med does not save meta event types - ID: 2879426
      if (a)
            xml.nput(" a=\"%d\"", a);
      if (b)
            xml.nput(" b=\"%d\"", b);
      if (c)
            xml.nput(" c=\"%d\"", c);

      if (edata.dataLen() ) {
            xml.nput(" datalen=\"%d\">\n", edata.dataLen() );
            xml.nput(level, "");
            for (int i = 0; i < edata.dataLen(); ++i) {
                  if (i && ((i % 16) == 0)) {
                        xml.nput("\n");
                        xml.nput(level, "");
                        }
                  xml.nput("%02x ", edata.constData()[i] & 0xff);
                  }
            xml.nput("\n");
            xml.etag(--level, "event");
            }
      else {
            xml.nput(" />\n");
            }
      }

//---------------------------------------------------------
//   MidiEventBase::read
//---------------------------------------------------------

void MidiEventBase::read(Xml& xml)
      {
      int ev_type = Note;
      a      = 0;
      b      = 0;
      c      = 0;

      int dataLen = 0;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        xml.unknown("Event");
                        break;
                  case Xml::Text:
                        {
                        QByteArray ba    = tag.toLatin1();
                        const char*s     = ba.constData();
                        edata.resize(dataLen);
                        unsigned char* d = edata.data();
                        for (int i = 0; i < dataLen; ++i) {
                              char* endp;
                              *d++ = strtol(s, &endp, 16);
                              s = endp;
                              }
                        }
                        break;
                  case Xml::Attribut:
                        if (tag == "tick")
                              setTick(xml.s2().toInt());
                        else if (tag == "type")
                              //setType(EventType(xml.s2().toInt()));
                              ev_type = xml.s2().toInt();
                        else if (tag == "len")
                              setLenTick(xml.s2().toInt());
                        else if (tag == "a")
                              a = xml.s2().toInt();
                        else if (tag == "b")
                              b = xml.s2().toInt();
                        else if (tag == "c")
                              c = xml.s2().toInt();
                        else if (tag == "datalen")
                              dataLen = xml.s2().toInt();
                        break;
                  case Xml::TagEnd:
                        if (tag == "event") {
                              // Convert obsolete PAfter and CAfter events to newer controllers
                              if(ev_type == 3) // PAfter
                              {
                                ev_type = Controller;
                                a = (a & 0x7f) | (CTRL_POLYAFTER & ~0xff); // Controller number + pitch. B is the value.
                              }
                              else
                              if(ev_type == 4) // CAfter
                              {
                                ev_type = Controller;
                                b = a;               // Value
                                a = CTRL_AFTERTOUCH; // Controller number
                              }

                              setType(EventType(ev_type));
                              
                              // HACK: Repair controllers saved with lo byte 0xff. 
                              // No such control. It was supposed to be 0x00.
                              // It's an error caused by a previous bug, fixed now.
                              if((ev_type == Controller) && ((a & 0xff) == 0xff))
                                a &= ~0xff;
                              return;
                        }
                  default:
                        break;
                  }
            }
      }

} // namespace MusECore
