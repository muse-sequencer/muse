//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: splitter.cpp,v 1.9 2005/11/23 13:55:32 wschweer Exp $
//  (C) Copyright 2005 Werner Schweer (ws@seh.de)
//=========================================================

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
      xml.tag(name);
      int n = sl.size();
      for (int i = 0; i < n; ++i) {
            int n = sl.at(i);
            xml.intTag("size", n);
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

