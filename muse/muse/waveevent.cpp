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
      ipos = -1;
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
      xml.tag("event");
      PosLen wpos(*this);
      wpos += offset;
//      if (offset)
//            wpos.setTick(wpos.tick() + offset);
      wpos.write(xml, "poslen");
      xml.intTag("frame", _spos);  // offset in wave file
      xml.strTag("file", f.finfo()->fileName());
      xml.etag("event");
      }

void WaveEventBase::read(unsigned offset, float** buffer, int channel, int n)
      {
      if (f.isNull())
            return;
/*      if (ipos != -1 && ipos != (offset + _spos))
            printf("Wave seek %d-%d = %d\n", 
               offset + _spos, ipos, offset + _spos - ipos);
  */
      f.seek(offset + _spos, 0);
      f.read(channel, buffer, n);

//      ipos = offset + _spos + n;
      }

