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

#include "splitter.h"
#include "utils.h"
#include "al/xml.h"

//---------------------------------------------------------
//   SplitterHandle
//---------------------------------------------------------

void SplitterHandle::paintEvent(QPaintEvent* ev)
      {
      QPainter p(this);
      QRect r(ev->rect());
      if (orientation() == Qt::Horizontal) {
            int x = 0;
            int y1 = r.y();
            int y2 = y1 + r.height();
            if (y1 == 0)
                  y1 = 1;
            if (y2 == height())
                  y2 = height() - 2;
            for (int i = 0; i < splitWidth; ++i) {
                  p.setPen(lineColor[i]);
                  p.drawLine(x, y1, x, y2);
                  ++x;
                  }
            p.setPen(lineColor[0]);
            p.drawLine(1, 0, splitWidth - 2, 0);
            p.drawLine(1, height()-1, splitWidth - 2, height()-1);
            }
      else {
            int y = 0;
            int x1 = r.x();
            int x2 = x1 + r.width();
            for (int i = 0; i < splitWidth; ++i) {
                  p.setPen(lineColor[i]);
                  p.drawLine(x1, y, x2, y);
                  ++y;
                  }
            }
      }

//---------------------------------------------------------
//   Splitter
//---------------------------------------------------------

Splitter::Splitter(Qt::Orientation o)
   : QSplitter(o)
      {
      setHandleWidth(splitWidth);
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void Splitter::writeStatus(const char* name, Xml& xml)
      {
      QList<int> sl = sizes();
      xml.stag(name);
      int n = sl.size();
      for (int i = 0; i < n; ++i) {
            int n = sl.at(i);
            xml.tag("size", n);
            }
      xml.etag(name);
      }

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void Splitter::readStatus(QDomNode node)
      {
      QList<int> sl;
      for (QDomNode n = node.firstChild(); !n.isNull(); n = n.nextSibling()) {
            QDomElement e = n.toElement();
            QString tag(e.tagName());
            int i = e.text().toInt();
            if (tag == "size") {
                  sl.push_back(i);
                  }
            else
                  printf("Splitter::unknown tag <%s>\n", tag.toLatin1().data());
            }
      setSizes(sl);
      }

