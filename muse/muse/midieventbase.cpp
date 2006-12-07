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

#include "event.h"
#include "midieventbase.h"
#include "al/xml.h"
#include "midievent.h"
#include "midictrl.h"
#include "muse.h"

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
      char* p;

      switch(type()) {
            case Note:       p = "Note   "; break;
            case Controller: p = "Ctrl   "; break;
            case Sysex:      p = "Sysex  "; break;
            case PAfter:     p = "PAfter "; break;
            case CAfter:     p = "CAfter "; break;
            case Meta:       p = "Meta   "; break;
            default:         p = "??     "; break;
            }
      for (int i = 0; i < (n+2); ++i)
            putchar(' ');
      printf("<%s> a:0x%x(%d) b:0x%x(%d) c:0x%x(%d)\n",
         p, a, a, b, b, c, c);
      }

//---------------------------------------------------------
//   MidiEventBase::write
//---------------------------------------------------------

void MidiEventBase::write(Xml& xml, const Pos& offset) const
      {
      QString s = QString("event tick=\"%1\"").arg(tick() + offset.tick());

      switch (type()) {
            case Note:
                  s += QString(" len=\"%1\"").arg(lenTick());
                  break;
            default:
                  s += QString(" type=\"%1\"").arg(type());
                  break;
            }
      if (edata.dataLen) {
            s += QString(" datalen=\"%1\"").arg(edata.dataLen);
            xml.stag(s);
            xml.dump(edata.dataLen, edata.data);
            xml.etag("event");
            }
      else {
            if (a)
                  s += QString(" a=\"%1\"").arg(a);
            if (b)
                  s += QString(" b=\"%1\"").arg(b);
            if (c)
                  s += QString(" c=\"%1\"").arg(c);
            xml.tagE(s);
            }
      }

//---------------------------------------------------------
//   MidiEventBase::read
//---------------------------------------------------------

void MidiEventBase::read(QDomNode node)
      {
      QDomElement e = node.toElement();
      setTick(e.attribute("tick","0").toInt());
      setType(EventType(e.attribute("type","0").toInt()));
      setLenTick(e.attribute("len","0").toInt());
      a = e.attribute("a","0").toInt();
      b = e.attribute("b","0").toInt();
      c = e.attribute("c","0").toInt();
      int dataLen = e.attribute("datalen","0").toInt();

      if (dataLen) {
      	QStringList l = e.text().simplified().split(" ", QString::SkipEmptyParts);
            if (dataLen != l.size()) {
			printf("error converting init string <%s>\n", e.text().toLatin1().data());
                  }
            edata.data    = new unsigned char[dataLen];
            edata.dataLen = dataLen;
            unsigned char* d = edata.data;
            int numberBase = 16;
            for (int i = 0; i < l.size(); ++i) {
            	bool ok;
                  *d++ = l.at(i).toInt(&ok, numberBase);
                  if (!ok)
                  	printf("error converting init val <%s>\n", l.at(i).toLatin1().data());
			}
            }
      }

