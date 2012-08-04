//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: utils.cpp,v 1.1.1.1.2.3 2009/11/14 03:37:48 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
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
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
//#include <time.h>

#include <QApplication>
#include <QFrame>
#include <QClipboard>
#include <QColor>
#include <QGradient>
#include <QIcon>
#include <QLinearGradient>
#include <QMimeData>
#include <QPainter>
#include <QPointF>

#include "audio.h"
#include "audiodev.h"
#include "part.h"
#include "utils.h"
#include "xml.h"

namespace MusECore {

//---------------------------------------------------------
//   curTime
//---------------------------------------------------------

double curTime()
      {
      // No audio device yet? Just get wall clock time.
      if(!MusEGlobal::audioDevice)  
      {
        struct timeval t;
        gettimeofday(&t, 0);
        //printf("%ld %ld\n", t.tv_sec, t.tv_usec);  // Note I observed values coming out of order! Causing some problems.
        return (double)((double)t.tv_sec + (t.tv_usec / 1000000.0));
      }
      
      // Ask the driver for the system time. 
      // May depend on selected clock source. 
      // With Jack, may be based upon wallclock time, the   
      //  processor cycle counter or the HPET clock etc.
      return MusEGlobal::audioDevice->systemTime();
      
      /*
      struct timespec t;
      //clock_gettime(CLOCK_MONOTONIC, &t);
      //clock_gettime(CLOCK_MONOTONIC_RAW, &t);
      //clock_gettime(CLOCK_REALTIME, &t);
      clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);  // Only this one works for me. Could be my older kernel...
      printf("%ld %ld\n", t.tv_sec, t.tv_nsec);  
      return (double)((double)t.tv_sec + (t.tv_nsec / 1000000000.0));
      */
      }

//---------------------------------------------------------
//   dump
//    simple debug output
//---------------------------------------------------------

void dump(const unsigned char* p, int n)
      {
      printf("dump %d\n", n);
      for (int i = 0; i < n; ++i) {
            printf("%02x ", *p++);
            if ((i > 0) && (i % 16 == 0) && (i+1 < n))
                  printf("\n");
            }
      printf("\n");
      }

//---------------------------------------------------------
//   num2cols
//---------------------------------------------------------

int num2cols(int min, int max)
      {
      int amin = abs(min);
      int amax = abs(max);
      int l = amin > amax ? amin : amax;
      return int(log10(l)) + 1;
      }

//---------------------------------------------------------
//   hLine
//---------------------------------------------------------

QFrame* hLine(QWidget* w)
      {
      QFrame* delim = new QFrame(w);
      delim->setFrameStyle(QFrame::HLine | QFrame::Sunken);
      return delim;
      }

//---------------------------------------------------------
//   vLine
//---------------------------------------------------------

QFrame* vLine(QWidget* w)
      {
      QFrame* delim = new QFrame(w);
      delim->setFrameStyle(QFrame::VLine | QFrame::Sunken);
      return delim;
      }

//---------------------------------------------------------
//   bitmap2String
//    5c -> 1-4 1-6
//
//    01011100
//
//---------------------------------------------------------

QString bitmap2String(int bm)
      {
      QString s;
//printf("bitmap2string: bm %04x", bm);
      if (bm == 0xffff)
            s = "all";
      else if (bm == 0)
            s = "none";
      else {
            bool range = false;
            int first = 0;
            bool needSpace = false;
            bm &= 0xffff;
            for (int i = 0; i < 17; ++i) {
            //for (int i = 0; i < 16; ++i) {
                  if ((1 << i) & bm) {
                        if (!range) {
                              range = true;
                              first = i;
                              }
                        }
                  else {
                        if (range) {
                              if (needSpace)
                                    s += " ";
                              QString ns;
                              if (first == i-1)
                                    ns.sprintf("%d", first+1);
                              else
                                    ns.sprintf("%d-%d", first+1, i);
                              s += ns;
                              needSpace = true;
                              }
                        range = false;
                        }
                  }
            }
//printf(" -> <%s>\n", s.toLatin1());
      return s;
      }

//---------------------------------------------------------
//   u32bitmap2String
//---------------------------------------------------------

QString u32bitmap2String(unsigned int bm)
      {
      QString s;
//printf("bitmap2string: bm %04x", bm);
      //if (bm == 0xffff)
      if (bm == 0xffffffff)
            s = "all";
      else if (bm == 0)
            s = "none";
      else {
            bool range = false;
            int first = 0;
            //unsigned int first = 0;
            bool needSpace = false;
            //bm &= 0xffff;
            //for (int i = 0; i < 17; ++i) {
            for (int i = 0; i < 33; ++i) {
                  if ((i < 32) && ((1U << i) & bm)) {
                        if (!range) {
                              range = true;
                              first = i;
                              }
                        }
                  else {
                        if (range) {
                              if (needSpace)
                                    s += " ";
                              QString ns;
                              if (first == i-1)
                                    ns.sprintf("%d", first+1);
                                    //ns.sprintf("%u", first+1);
                              else
                                    ns.sprintf("%d-%d", first+1, i);
                                    //ns.sprintf("%u-%u", first+1, i);
                              s += ns;
                              needSpace = true;
                              }
                        range = false;
                        }
                  }
            }
//printf(" -> <%s>\n", s.toLatin1());
      return s;
      }

//---------------------------------------------------------
//   string2bitmap
//---------------------------------------------------------

int string2bitmap(const QString& str)
      {
      int val = 0;
      QString ss = str.simplified();
      QByteArray ba = ss.toLatin1();
      const char* s = ba.constData();
//printf("string2bitmap <%s>\n", s);

      if (s == 0)
            return 0;
      if (strcmp(s, "all") == 0)
            return 0xffff;
      if (strcmp(s, "none") == 0)
            return 0;
// printf("str2bitmap: <%s> ", str.toLatin1);
      int tval   = 0;
      bool range = false;
      int sval   = 0;
      while (*s == ' ')
            ++s;
      while (*s) {
            if (*s >= '0'  && *s <= '9') {
                  tval *= 10;
                  tval += *s - '0';
                  }
            else if (*s == ' ' || *s == ',') {
                  if (range) {
                        for (int i = sval-1; i < tval; ++i)
                              val |= (1 << i);
                        range = false;
                        }
                  else {
                        val |= (1 << (tval-1));
                        }
                  tval = 0;
                  }
            else if (*s == '-') {
                  range = true;
                  sval  = tval;
                  tval  = 0;
                  }
            ++s;
            }
      if (range && tval) {
            for (int i = sval-1; i < tval; ++i)
                  val |= (1 << i);
            }
      else if (tval) {
            val |= (1 << (tval-1));
            }
      return val & 0xffff;
      }

//---------------------------------------------------------
//   string2u32bitmap
//---------------------------------------------------------

unsigned int string2u32bitmap(const QString& str)
      {
      //int val = 0;
      unsigned int val = 0;
      QString ss = str.simplified();
      QByteArray ba = ss.toLatin1();
      const char* s = ba.constData();
//printf("string2bitmap <%s>\n", s);

      if (s == 0)
            return 0;
      if (strcmp(s, "all") == 0)
            //return 0xffff;
            return 0xffffffff;
      if (strcmp(s, "none") == 0)
            return 0;
// printf("str2bitmap: <%s> ", str.toLatin1);
      int tval   = 0;
      //unsigned int tval   = 0;
      bool range = false;
      int sval   = 0;
      //unsigned int sval   = 0;
      while (*s == ' ')
            ++s;
      while (*s) {
            if (*s >= '0'  && *s <= '9') {
                  tval *= 10;
                  tval += *s - '0';
                  }
            else if (*s == ' ' || *s == ',') {
                  if (range) {
                        for (int i = sval-1; i < tval; ++i)
                        //for (unsigned int i = sval-1; i < tval; ++i)
                              val |= (1U << i);
                        range = false;
                        }
                  else {
                        val |= (1U << (tval-1));
                        }
                  tval = 0;
                  }
            else if (*s == '-') {
                  range = true;
                  sval  = tval;
                  tval  = 0;
                  }
            ++s;
            }
      if (range && tval) {
            for (int i = sval-1; i < tval; ++i)
            //for (unsigned int i = sval-1; i < tval; ++i)
                  val |= (1U << i);
            }
      else if (tval) {
            val |= (1U << (tval-1));
            }
      //return val & 0xffff;
      return val;
      }

//---------------------------------------------------------
//   autoAdjustFontSize
//   w: Widget to auto adjust font size
//   s: String to fit
//   ignoreWidth: Set if dealing with a vertically constrained widget - one which is free to resize horizontally.
//   ignoreHeight: Set if dealing with a horizontally constrained widget - one which is free to resize vertically. 
//---------------------------------------------------------

bool autoAdjustFontSize(QFrame* w, const QString& s, bool ignoreWidth, bool ignoreHeight, int max, int min)
{
  // In case the max or min was obtained from QFont::pointSize() which returns -1 
  //  if the font is a pixel font, or if min is greater than max...
  if(!w || (min < 0) || (max < 0) || (min > max))
    return false;
    
  // Limit the minimum and maximum sizes to something at least readable.
  if(max < 4)
    max = 4;
  if(min < 4)
    min = 4;
    
  QRect cr = w->contentsRect();
  QRect r;
  QFont fnt = w->font();
  // An extra amount just to be sure - I found it was still breaking up two words which would fit on one line.
  int extra = 4;
  // Allow at least one loop. min can be equal to max.
  for(int i = max; i >= min; --i)
  {
    fnt.setPointSize(i);
    QFontMetrics fm(fnt);
    r = fm.boundingRect(s);
    // Would the text fit within the widget?
    if((ignoreWidth || (r.width() <= (cr.width() - extra))) && (ignoreHeight || (r.height() <= cr.height())))
      break;
  }
  //printf("autoAdjustFontSize: ptsz:%d widget:%s before setFont x:%d y:%d w:%d h:%d\n", fnt.pointSize(), w->name(), w->x(), w->y(), w->width(), w->height());
  
  // Here we will always have a font ranging from min to max point size.
  w->setFont(fnt);
  //printf("autoAdjustFontSize: ptsz:%d widget:%s x:%d y:%d w:%d h:%d frame w:%d rw:%d rh:%d\n", fnt.pointSize(), w->name(), w->x(), w->y(), w->width(), w->height(), w->frameWidth(), cr.width(), cr.height());
  
  // Force minimum height. Use the expected height for the highest given point size.
  // This way the mixer strips aren't all different label heights, but can be larger if necessary.
  // Only if ignoreHeight is set (therefore the height is adjustable).
  if(ignoreHeight)
  {
    fnt.setPointSize(max);
    QFontMetrics fm(fnt);
    // Set the label's minimum height equal to the height of the font.
    w->setMinimumHeight(fm.height() + 2 * w->frameWidth());
  }
  
  return true;  
}

QGradient gGradientFromQColor(const QColor& c, const QPointF& start, const QPointF& finalStop)
{
  int h, s, v, a;
  c.getHsv(&h, &s, &v, &a);
  const int v0 = v + (255 - v)/2;
  const int v1 = v - v/2;
  const QColor c0 = QColor::fromHsv(h, s, v0, a); 
  const QColor c1 = QColor::fromHsv(h, s, v1, a); 
  
  QLinearGradient gradient(start, finalStop);
  gradient.setColorAt(0, c0);
  gradient.setColorAt(1, c1);
  
  return gradient;
}

QPainterPath roundedPath(QRect r, int xrad, int yrad, Corner roundCorner)
{
  return roundedPath(r.x(), r.y(),
                       r.width(), r.height(),
                       xrad, yrad,
                       roundCorner);
}

QPainterPath roundedPath(int x, int y, int w, int h, int xrad, int yrad, Corner roundCorner)
{
  QPainterPath rounded_rect;
  rounded_rect.addRect(x, y, w, h);

   if (roundCorner & UpperLeft)
    {
      QPainterPath top_left_corner;
      top_left_corner.addRect(x, y, xrad, yrad);
      top_left_corner.moveTo(x + xrad, y + yrad);
      top_left_corner.arcTo(x, y, xrad*2, yrad*2, 180, -90);
      rounded_rect = rounded_rect.subtracted(top_left_corner);
    }

  if (roundCorner & UpperRight)
    {
      QPainterPath top_right_corner;
      top_right_corner.addRect(x + w - xrad, y, xrad, yrad);
      top_right_corner.moveTo(x + w - xrad, y + yrad);
      top_right_corner.arcTo(x + w - xrad * 2, y, xrad*2, yrad*2, 90, -90);
      rounded_rect = rounded_rect.subtracted(top_right_corner);
    }

  if (roundCorner & LowerLeft)
    {
      QPainterPath bottom_left_corner;
      bottom_left_corner.addRect(x, y + h - yrad, xrad, yrad);
      bottom_left_corner.moveTo(x + xrad, y + h - yrad);
      bottom_left_corner.arcTo(x, y + h - yrad*2, xrad*2, yrad*2, 180, 90);
      rounded_rect = rounded_rect.subtracted(bottom_left_corner);
    }

  if (roundCorner & LowerRight)
    {
      QPainterPath bottom_right_corner;
      bottom_right_corner.addRect(x + w - xrad, y + h - yrad, xrad, yrad);
      bottom_right_corner.moveTo(x + w - xrad, y + h - yrad);
      bottom_right_corner.arcTo(x + w - xrad*2, y + h - yrad*2, xrad*2, yrad*2, 270, 90);
      rounded_rect = rounded_rect.subtracted(bottom_right_corner);
    }

  return rounded_rect;
}

//---------------------------------------------------------
//   colorRect
//   paints a rectangular icon with a given color
//---------------------------------------------------------

QIcon colorRect(const QColor& color, int width, int height) {
      QPainter painter;
      QPixmap image(width, height);
      painter.begin(&image);
      painter.setBrush(color);
      QRect rectangle(0, 0, width, height);
      painter.drawRect(rectangle);
      painter.end();
      QIcon icon(image);
      return icon;
}

int get_paste_len()
      {
      QClipboard* cb  = QApplication::clipboard();
      const QMimeData* md = cb->mimeData(QClipboard::Clipboard);

      QString pfx("text/");
      QString mdpl("x-muse-midipartlist");
      QString wvpl("x-muse-wavepartlist");
      QString mxpl("x-muse-mixedpartlist");
      QString txt;

      if(md->hasFormat(pfx + mdpl))
            txt = cb->text(mdpl, QClipboard::Clipboard);
      else if(md->hasFormat(pfx + wvpl))
            txt = cb->text(wvpl, QClipboard::Clipboard);
      else if(md->hasFormat(pfx + mxpl))
            txt = cb->text(mxpl, QClipboard::Clipboard);
      else
            return 0;


      QByteArray ba = txt.toLatin1();
      const char* ptxt = ba.constData();
      Xml xml(ptxt);
      bool end = false;

      unsigned begin_tick=-1; //this uses the greatest possible begin_tick
      unsigned end_tick=0;

      for (;;)
            {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token)
                  {
                  case Xml::Error:
                  case Xml::End:
                        end = true;
                        break;

                  case Xml::TagStart:
                        if (tag == "part")
                              {
                              Part* p = 0;
                              p = readXmlPart(xml, NULL, false, false);

                              if (p)
                                    {
                                    if (p->tick() < begin_tick)
                                          begin_tick=p->tick();

                                    if (p->endTick() > end_tick)
                                          end_tick=p->endTick();
                                    
                                    unchainClone(p);
                                    delete p;
                                    }
                              }
                        else
                              xml.unknown("PartCanvas::get_paste_len");
                        break;

                  case Xml::TagEnd:
                        break;

                  default:
                        end = true;
                        break;
                  }
            if(end)
                  break;
            }

      if (begin_tick > end_tick)
            return 0;
      else
            return end_tick - begin_tick;
}

} // namespace MusECore
