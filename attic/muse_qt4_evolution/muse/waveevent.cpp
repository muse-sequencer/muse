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

#include "globals.h"
#include "event.h"
#include "waveevent.h"
#include "al/xml.h"
#include "wave.h"

//---------------------------------------------------------
//   WaveEvent
//---------------------------------------------------------

WaveEventBase::WaveEventBase(EventType t)
   : EventBase(t)
      {
      deleted = false;
      }

//---------------------------------------------------------
//   WaveEvent::mid
//---------------------------------------------------------

EventBase* WaveEventBase::mid(unsigned b, unsigned e)
      {
      WaveEventBase* ev = new WaveEventBase(*this);

      int offset   = b - frame();
      unsigned end = endFrame();
      if (e < end)
            end = e;

      int len = end - b;
      ev->setFrame(b);
      ev->setLenFrame(len);
      ev->setSpos(spos() + offset);
      return ev;
      }

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void WaveEventBase::dump(int n) const
      {
      EventBase::dump(n);
      }

//---------------------------------------------------------
//   WaveEventBase::read
//---------------------------------------------------------

void WaveEventBase::read(QDomNode node)
      {
      node = node.firstChild();
      while (!node.isNull()) {
            QDomElement e = node.toElement();
            if (e.tagName() == "poslen")
                  PosLen::read(node);
            else if (e.tagName() == "frame")
                  _spos = e.text().toInt();
            else if (e.tagName() == "file") {
                  SndFile* wf = SndFile::getWave(e.text(), false);
                  if (wf)
                        f = SndFileR(wf);
                  }
            else
                  printf("MusE:WaveEventBase: unknown tag %s\n", e.tagName().toLatin1().data());
            node = node.nextSibling();
            }
      Pos::setType(AL::FRAMES);
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void WaveEventBase::write(Xml& xml, const Pos& offset) const
      {
      if (f.isNull())
            return;
      xml.stag("event");
      PosLen wpos(*this);
      wpos += offset;
//      if (offset)
//            wpos.setTick(wpos.tick() + offset);
      wpos.write(xml, "poslen");
      xml.tag("frame", _spos);  // offset in wave file
      xml.tag("file", f.finfo()->fileName());
      xml.etag("event");
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void WaveEventBase::read(unsigned offset, float** buffer, int channel, int n)
      {
      if (f.isNull())
            return;
      f.seek(offset + _spos);
      f.read(channel, buffer, n);
      }

//---------------------------------------------------------
//   WaveEventBase::operator==
//---------------------------------------------------------

bool WaveEventBase::operator==(const EventBase& ev) const {

  const WaveEventBase* pev = dynamic_cast<const WaveEventBase*>(&ev);

  if(pev) return operator==(*pev);
  else return false;
}

bool WaveEventBase::operator==(const WaveEventBase& /*ev*/) const {
  //TODO
  return false;
}
