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

#include "event.h"
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

//---------------------------------------------------------
//   MidiEventBase::mid
//---------------------------------------------------------

EventBase* MidiEventBase::mid(unsigned b, unsigned e)
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

bool MidiEventBase::isNoteOff(const Event& e) const
      {
      return (e.isNoteOff() && (e.pitch() == a));
      }

void MidiEventBase::dump(int n) const
      {
      EventBase::dump(n);
      const char* p;

      switch(type()) {
            case Note:      p = "Note    "; break;
            case Controller: p = "Ctrl    "; break;
            case Sysex:     p = "Sysex   "; break;
            case PAfter:    p = "PAfter  "; break;
            case CAfter:    p = "CAfter  "; break;
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
      
      if (edata.dataLen) {
            xml.nput(" datalen=\"%d\">\n", edata.dataLen);
            xml.nput(level, "");
            for (int i = 0; i < edata.dataLen; ++i)
                  xml.nput("%02x ", edata.data[i] & 0xff);
            xml.nput("\n");
            xml.tag(level, "/event");
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
      setType(Note);
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
                        edata.data       = new unsigned char[dataLen];
                        edata.dataLen    = dataLen;
                        unsigned char* d = edata.data;
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
                              setType(EventType(xml.s2().toInt()));
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
                        if (tag == "event")
                              return;
                  default:
                        break;
                  }
            }
      }

} // namespace MusECore
