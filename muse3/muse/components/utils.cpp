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
#include "muse_math.h"
#include <sys/time.h>

#include <QApplication>
#include <QClipboard>
#include <QLinearGradient>
#include <QMimeData>
#include <QPainter>
#include <QFileInfo>

#include "audio.h"
#include "audiodev.h"
#include "part.h"
#include "utils.h"
#include "xml.h"
#include "gconfig.h"

namespace MusECore {

//---------------------------------------------------------
//   curTimeUS
//---------------------------------------------------------

uint64_t curTimeUS()
      {
      // No audio device yet? Just get wall clock time.
      if(!MusEGlobal::audioDevice)  
      {
        struct timeval t;
        gettimeofday(&t, 0);
        //printf("%ld %ld\n", t.tv_sec, t.tv_usec);  // Note I observed values coming out of order! Causing some problems.
        return ((uint64_t)t.tv_sec * 1000000UL) + (uint64_t)t.tv_usec;
      }
      
      // Ask the driver for the system time. 
      // May depend on selected clock source. 
      // With Jack, may be based upon wallclock time, the   
      //  processor cycle counter or the HPET clock etc.
      return MusEGlobal::audioDevice->systemTimeUS();
      
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
            printf("%02x ", p[i]);
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
                                    ns = QString::number(first + 1);
                              else
                                    ns = QString("%1-%2").arg(first + 1).arg(i);
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
                                    ns = QString::number(first + 1);
                              else
                                    ns = QString("%1-%2").arg(first + 1).arg(i);
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

      if (ss.isEmpty())
            return 0;
      if (ss == QString("all"))
            return 0xffff;
      if (ss == QString("none"))
            return 0;
      
      QByteArray ba = ss.toLatin1();
      const char* s = ba.constData();
      
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

      if (ss.isEmpty())
            return 0;
      if (ss == QString("all"))
            return 0xffff;
      if (ss == QString("none"))
            return 0;
      
      QByteArray ba = ss.toLatin1();
      const char* s = ba.constData();
      
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

// //---------------------------------------------------------
// //   autoAdjustFontSize
// //   w: Widget to auto adjust font size
// //   s: String to fit
// //   ignoreWidth: Set if dealing with a vertically constrained widget - one which is free to resize horizontally.
// //   ignoreHeight: Set if dealing with a horizontally constrained widget - one which is free to resize vertically.
// //   Returns false if text would not fit even at min size.
// //   Caller should enable word wrap (if available) if false is returned, or disable word wrap if true is returned.
// //   Otherwise if word wrap is enabled all the time, there is the possibility it will break the line prematurely.
// //---------------------------------------------------------
//
// bool autoAdjustFontSize(QFrame* w, const QString& s, bool ignoreWidth, bool ignoreHeight, int max, int min)
// {
//   // In case the max or min was obtained from QFont::pointSize() which returns -1
//   //  if the font is a pixel font, or if min is greater than max...
// //   if(!w || (min < 0) || (max < 0) || (min > max))
//   if(!w)
//     return false;
//   if(min > max)
//     min = max;
//
//   // Make the minimum about 3/4 the maximum font size.
//   min = int(double(max) * 0.7);
//
//   // Limit the minimum and maximum sizes to something at least readable.
//   if(max < 6)
//     max = 6;
//   if(min < 6)
//     min = 6;
//
//   QRect cr = w->contentsRect();
//   QRect r;
//   QFont fnt = w->font();
//   // An extra amount just to be sure - I found it was still breaking up two words which would fit on one line.
// //   int extra = 4;
//   int extra = 0;
//
//   int fin_sz = max;
//   // Allow at least one loop. min can be equal to max.
// //   for(int i = max; i >= min; --i)
//   for( ; fin_sz >= min; --fin_sz)
//   {
//     fnt.setPointSize(fin_sz);
//     QFontMetrics fm(fnt);
//     r = fm.boundingRect(s);
//     // Would the text fit within the widget?
//     if((ignoreWidth || (r.width() <= (cr.width() - extra))) && (ignoreHeight || (r.height() <= cr.height())))
//       break;
//   }
//   //printf("autoAdjustFontSize: ptsz:%d widget:%s before setFont x:%d y:%d w:%d h:%d\n", fnt.pointSize(), w->name(), w->x(), w->y(), w->width(), w->height());
//
//   // Here we will always have a font ranging from min to max point size.
//   w->setFont(fnt);
// //   w->setStyleSheet(MusECore::font2StyleSheet(fnt));
//   //printf("autoAdjustFontSize: ptsz:%d widget:%s x:%d y:%d w:%d h:%d frame w:%d rw:%d rh:%d\n", fnt.pointSize(), w->name(), w->x(), w->y(), w->width(), w->height(), w->frameWidth(), cr.width(), cr.height());
//
// // -----------------------------------------------------------
// // This is an alternate faster method. But the one below is better.
// // -----------------------------------------------------------
//
// //   QFont fnt = w->font();
// //   //const int req_w = w->fontMetrics().width(s) + 4;
// //   const int req_w = w->fontMetrics().boundingRect(s).width() + 4;
// //   if(ignoreWidth || req_w == 0) // Also avoid divide by zero below.
// //   {
// //     if(fnt.pointSize() != max)
// //     {
// //       fnt.setPointSize(max);
// //       w->setFont(fnt);
// //     }
// //   }
// //   else
// //   {
// //     float factor = (float)w->rect().width() / (float)req_w;
// //     //if((factor < 1) || (factor > 1.25))
// //     if((factor < 1) || (factor > 1))
// //     {
// //       //qreal new_sz = fnt.pointSizeF() * factor;
// //       int new_sz = (float)fnt.pointSize() * factor;
// //       bool do_check = true;
// //       if(new_sz < min)
// //       {
// //         new_sz = min;
// //         do_check = false;
// //       }
// //       else if(new_sz > max)
// //       {
// //         new_sz = max;
// //         do_check = false;
// //       }
// //
// //       //if(fnt.pointSizeF() != new_sz)
// //       if(fnt.pointSize() != new_sz)
// //       {
// //         //fnt.setPointSizeF(new_sz);
// //         fnt.setPointSize(new_sz);
// //         if(do_check)
// //         {
// //           const QFontMetrics fm(fnt);
// //           const int check_w = fm.boundingRect(s).width() + 4;
// //           if(check_w > w->rect().width())
// //           {
// //             --new_sz;
// //             fnt.setPointSize(new_sz);
// //           }
// //         }
// //         w->setFont(fnt);
// //       }
// //     }
// //   }
//
//
// // -----------------------------------------------------------
// // This is an alternate faster method. The accuracy is poorer
// //  than the top method, and somewhat unreliable. Maybe with more tweaking...
// // -----------------------------------------------------------
//
// //   //qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());
// //   //QRectF r = boundingRect();
// //   QRectF r = w->rect();
// //   //QFont f = painter->font();
// //   QFont fnt = w->font();
//
// //   //if(ignoreWidth || req_w == 0) // Also avoid divide by zero below.
// //   if(ignoreWidth || s.isEmpty()) // Also avoid divide by zero below.
// //   {
// //     if(fnt.pointSize() != max)
// //     {
// //       fnt.setPointSize(max);
// //       w->setFont(fnt);
// //       w->setStyleSheet(MusECore::font2StyleSheet(fnt));
// //     }
// //   }
// //   else
// //   {
// //     //qreal aspectRatio = painter->fontMetrics().lineSpacing() / painter->fontMetrics().averageCharWidth();
// //     qreal aspectRatio = w->fontMetrics().lineSpacing() / w->fontMetrics().averageCharWidth();
// //     int pixelsize = sqrt(r.width() * r.height() / aspectRatio / (s.length() * 3)) * aspectRatio;
// //     fnt.setPixelSize(pixelsize);
// //     //int flags = Qt::AlignCenter|Qt::TextDontClip|Qt::TextWordWrap;
// //     int flags = Qt::AlignCenter;
// //     //if ((pixelsize * lod) < 13)
// //     //    flags |= Qt::TextWrapAnywhere;
// //     QFontMetricsF fm(fnt);
// //     QRectF tbr = fm.boundingRect(r,flags,s);
// //     pixelsize = fnt.pixelSize() * qMin(r.width() * 0.95 / tbr.width(), r.height() * 0.95 / tbr.height());
// // //     if(pixelsize < min)
// // //       pixelsize = min;
// // //     else if(pixelsize > max)
// // //       pixelsize = max;
// //     fnt.setPixelSize(pixelsize);
// //     const QFontInfo fi(fnt);
// //     const int pointsize = fi.pointSize();
// //     if(pointsize <= min)
// //       fnt.setPointSize(min);
// //     else if(pointsize >= max)
// //       fnt.setPointSize(max);
// //     w->setFont(fnt);
// //     w->setStyleSheet(MusECore::font2StyleSheet(fnt));
// //     //painter->drawText(r,flags,stitle);
// //   }
//
//
//
//   // Force minimum height. Use the expected height for the highest given point size.
//   // This way the mixer strips aren't all different label heights, but can be larger if necessary.
//   // Only if ignoreHeight is set (therefore the height is adjustable).
//   if(ignoreHeight)
//   {
//     fnt.setPointSize(max);
//     const QFontMetrics fm(fnt);
//     // Set the label's minimum height equal to the height of the font.
//     w->setMinimumHeight(fm.height() + 2 * w->frameWidth());
//   }
//
// //   return true;
//
//   // If the text still wouldn't fit at the min size, tell the caller to turn on word wrap.
//   return fin_sz >= min;
// }

//---------------------------------------------------------
//   autoAdjustFontSize
//   w: Widget to auto adjust font size
//   txt: String to fit
//   targetFont: Font input and output variable. Widget font is NOT automatically set due to stylesheets
//    completely overriding them, therefore stylesheet must be 'composed' from the resulting font along with OTHER settings.
//   ignoreWidth: Set if dealing with a vertically constrained widget - one which is free to resize horizontally.
//   ignoreHeight: Set if dealing with a horizontally constrained widget - one which is free to resize vertically.
//   Returns false if text would not fit even at min size.
//   Caller should enable word wrap (if available) if false is returned, or disable word wrap if true is returned.
//   Otherwise if word wrap is enabled all the time, there is the possibility it will break the line prematurely.
//---------------------------------------------------------

bool autoAdjustFontSize(QFrame* widget, const QString& txt, QFont& targetFont, bool ignoreWidth, bool ignoreHeight, int max, int min)
{
  // In case the max or min was obtained from QFont::pointSize() which returns -1
  //  if the font is a pixel font, or if min is greater than max...
//   if(!w || (min < 0) || (max < 0) || (min > max))
  if(!widget)
    return false;

  if(min > max)
    min = max;

  // Make the minimum about 3/4 the maximum font size.
  min = int(double(max) * 0.85);

  // Limit the minimum and maximum sizes to something at least readable.
  if(max < 7)
    max = 7;
  if(min < 7)
    min = 7;

  QRect cr = widget->contentsRect();
  QRect r;
//   QFont fnt = widget->font();
//   QFont fnt = targetFont; // Make a copy for later.

  // Force minimum height. Use the expected height for the highest given point size.
  // This way the mixer strips aren't all different label heights, but can be larger if necessary.
  // Only if ignoreHeight is set (therefore the height is adjustable).
  if(ignoreHeight)
  {
    targetFont.setPointSize(max);
    const QFontMetrics fm(targetFont);
    // Set the label's minimum height equal to the height of the font.
//     w->setMinimumHeight(fm.height() + 2 * w->frameWidth());
    widget->setMinimumHeight(fm.height() + 2 * widget->frameWidth());
  }

  // An extra amount just to be sure - I found it was still breaking up two words which would fit on one line.
//   int extra = 4;
  int extra = 0;

  int fin_sz = max;
  // Allow at least one loop. min can be equal to max.
//   for(int i = max; i >= min; --i)
  for( ; fin_sz >= min; --fin_sz)
  {
//     fnt.setPointSize(fin_sz);
    targetFont.setPointSize(fin_sz);
//     QFontMetrics fm(fnt);
    QFontMetrics fm(targetFont);
    r = fm.boundingRect(txt);
    // Would the text fit within the widget?
    if((ignoreWidth || (r.width() <= (cr.width() - extra))) && (ignoreHeight || (r.height() <= cr.height())))
//     if((ignoreWidth || (r.width() <= (sz.width() - extra))) && (ignoreHeight || (r.height() <= sz.height())))
      break;
  }
  //printf("autoAdjustFontSize: ptsz:%d widget:%s before setFont x:%d y:%d w:%d h:%d\n", fnt.pointSize(), w->name(), w->x(), w->y(), w->width(), w->height());

  // Here we will always have a font ranging from min to max point size.
//   w->setFont(fnt);
//   w->setStyleSheet(MusECore::font2StyleSheet(fnt));
  //printf("autoAdjustFontSize: ptsz:%d widget:%s x:%d y:%d w:%d h:%d frame w:%d rw:%d rh:%d\n", fnt.pointSize(), w->name(), w->x(), w->y(), w->width(), w->height(), w->frameWidth(), cr.width(), cr.height());

// -----------------------------------------------------------
// This is an alternate faster method. But the one below is better.
// -----------------------------------------------------------

//   QFont fnt = w->font();
//   //const int req_w = w->fontMetrics().width(s) + 4;
//   const int req_w = w->fontMetrics().boundingRect(s).width() + 4;
//   if(ignoreWidth || req_w == 0) // Also avoid divide by zero below.
//   {
//     if(fnt.pointSize() != max)
//     {
//       fnt.setPointSize(max);
//       w->setFont(fnt);
//     }
//   }
//   else
//   {
//     float factor = (float)w->rect().width() / (float)req_w;
//     //if((factor < 1) || (factor > 1.25))
//     if((factor < 1) || (factor > 1))
//     {
//       //qreal new_sz = fnt.pointSizeF() * factor;
//       int new_sz = (float)fnt.pointSize() * factor;
//       bool do_check = true;
//       if(new_sz < min)
//       {
//         new_sz = min;
//         do_check = false;
//       }
//       else if(new_sz > max)
//       {
//         new_sz = max;
//         do_check = false;
//       }
//
//       //if(fnt.pointSizeF() != new_sz)
//       if(fnt.pointSize() != new_sz)
//       {
//         //fnt.setPointSizeF(new_sz);
//         fnt.setPointSize(new_sz);
//         if(do_check)
//         {
//           const QFontMetrics fm(fnt);
//           const int check_w = fm.boundingRect(s).width() + 4;
//           if(check_w > w->rect().width())
//           {
//             --new_sz;
//             fnt.setPointSize(new_sz);
//           }
//         }
//         w->setFont(fnt);
//       }
//     }
//   }


// -----------------------------------------------------------
// This is an alternate faster method. The accuracy is poorer
//  than the top method, and somewhat unreliable. Maybe with more tweaking...
// -----------------------------------------------------------

//   //qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());
//   //QRectF r = boundingRect();
//   QRectF r = w->rect();
//   //QFont f = painter->font();
//   QFont fnt = w->font();

//   //if(ignoreWidth || req_w == 0) // Also avoid divide by zero below.
//   if(ignoreWidth || s.isEmpty()) // Also avoid divide by zero below.
//   {
//     if(fnt.pointSize() != max)
//     {
//       fnt.setPointSize(max);
//       w->setFont(fnt);
//       w->setStyleSheet(MusECore::font2StyleSheet(fnt));
//     }
//   }
//   else
//   {
//     //qreal aspectRatio = painter->fontMetrics().lineSpacing() / painter->fontMetrics().averageCharWidth();
//     qreal aspectRatio = w->fontMetrics().lineSpacing() / w->fontMetrics().averageCharWidth();
//     int pixelsize = sqrt(r.width() * r.height() / aspectRatio / (s.length() * 3)) * aspectRatio;
//     fnt.setPixelSize(pixelsize);
//     //int flags = Qt::AlignCenter|Qt::TextDontClip|Qt::TextWordWrap;
//     int flags = Qt::AlignCenter;
//     //if ((pixelsize * lod) < 13)
//     //    flags |= Qt::TextWrapAnywhere;
//     QFontMetricsF fm(fnt);
//     QRectF tbr = fm.boundingRect(r,flags,s);
//     pixelsize = fnt.pixelSize() * qMin(r.width() * 0.95 / tbr.width(), r.height() * 0.95 / tbr.height());
// //     if(pixelsize < min)
// //       pixelsize = min;
// //     else if(pixelsize > max)
// //       pixelsize = max;
//     fnt.setPixelSize(pixelsize);
//     const QFontInfo fi(fnt);
//     const int pointsize = fi.pointSize();
//     if(pointsize <= min)
//       fnt.setPointSize(min);
//     else if(pointsize >= max)
//       fnt.setPointSize(max);
//     w->setFont(fnt);
//     w->setStyleSheet(MusECore::font2StyleSheet(fnt));
//     //painter->drawText(r,flags,stitle);
//   }



//   // Force minimum height. Use the expected height for the highest given point size.
//   // This way the mixer strips aren't all different label heights, but can be larger if necessary.
//   // Only if ignoreHeight is set (therefore the height is adjustable).
//   if(ignoreHeight)
//   {
//     fnt.setPointSize(max);
//     const QFontMetrics fm(fnt);
//     // Set the label's minimum height equal to the height of the font.
// //     w->setMinimumHeight(fm.height() + 2 * w->frameWidth());
//     widget->setMinimumHeight(fm.height() + 2 * widget->frameWidth());
//   }

//   return true;

  // If the text still wouldn't fit at the min size, tell the caller to turn on word wrap.
  return fin_sz >= min;
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

QPainterPath roundedPath(const QRect& r, int xrad, int yrad, Corner roundCorner)
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

   if (roundCorner & CornerUpperLeft)
    {
      QPainterPath top_left_corner;
      top_left_corner.addRect(x, y, xrad, yrad);
      top_left_corner.moveTo(x + xrad, y + yrad);
      top_left_corner.arcTo(x, y, xrad*2, yrad*2, 180, -90);
      rounded_rect = rounded_rect.subtracted(top_left_corner);
    }

  if (roundCorner & CornerUpperRight)
    {
      QPainterPath top_right_corner;
      top_right_corner.addRect(x + w - xrad, y, xrad, yrad);
      top_right_corner.moveTo(x + w - xrad, y + yrad);
      top_right_corner.arcTo(x + w - xrad * 2, y, xrad*2, yrad*2, 90, -90);
      rounded_rect = rounded_rect.subtracted(top_right_corner);
    }

  if (roundCorner & CornerLowerLeft)
    {
      QPainterPath bottom_left_corner;
      bottom_left_corner.addRect(x, y + h - yrad, xrad, yrad);
      bottom_left_corner.moveTo(x + xrad, y + h - yrad);
      bottom_left_corner.arcTo(x, y + h - yrad*2, xrad*2, yrad*2, 180, 90);
      rounded_rect = rounded_rect.subtracted(bottom_left_corner);
    }

  if (roundCorner & CornerLowerRight)
    {
      QPainterPath bottom_right_corner;
      bottom_right_corner.addRect(x + w - xrad, y + h - yrad, xrad, yrad);
      bottom_right_corner.moveTo(x + w - xrad, y + h - yrad);
      bottom_right_corner.arcTo(x + w - xrad*2, y + h - yrad*2, xrad*2, yrad*2, 270, 90);
      rounded_rect = rounded_rect.subtracted(bottom_right_corner);
    }

  return rounded_rect;
}

void addRoundedPath(QPainterPath* path, const QRect& r, int xrad, int yrad, Corner roundCorner)
{
  addRoundedPath(path, r.x(), r.y(),
                 r.width(), r.height(),
                 xrad, yrad,
                 roundCorner);
}

void addRoundedPath(QPainterPath* path, int x, int y, int w, int h, int xrad, int yrad, Corner roundCorner)
{
  QPainterPath& pp = *path;
  pp.addRect(x, y, w, h);

  if (roundCorner & CornerUpperLeft)
  {
    QPainterPath top_left_corner;
    top_left_corner.addRect(x, y, xrad, yrad);
    top_left_corner.moveTo(x + xrad, y + yrad);
    top_left_corner.arcTo(x, y, xrad*2, yrad*2, 180, -90);
    pp -= top_left_corner;
  }

  if (roundCorner & CornerUpperRight)
  {
    QPainterPath top_right_corner;
    top_right_corner.addRect(x + w - xrad, y, xrad, yrad);
    top_right_corner.moveTo(x + w - xrad, y + yrad);
    top_right_corner.arcTo(x + w - xrad * 2, y, xrad*2, yrad*2, 90, -90);
    pp -= top_right_corner;
  }

  if (roundCorner & CornerLowerLeft)
  {
    QPainterPath bottom_left_corner;
    bottom_left_corner.addRect(x, y + h - yrad, xrad, yrad);
    bottom_left_corner.moveTo(x + xrad, y + h - yrad);
    bottom_left_corner.arcTo(x, y + h - yrad*2, xrad*2, yrad*2, 180, 90);
    pp -= bottom_left_corner;
  }

  if (roundCorner & CornerLowerRight)
  {
    QPainterPath bottom_right_corner;
    bottom_right_corner.addRect(x + w - xrad, y + h - yrad, xrad, yrad);
    bottom_right_corner.moveTo(x + w - xrad, y + h - yrad);
    bottom_right_corner.arcTo(x + w - xrad*2, y + h - yrad*2, xrad*2, yrad*2, 270, 90);
    pp -= bottom_right_corner;
  }
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
                              p = Part::readFromXml(xml, NULL, false, false);

                              if (p)
                                    {
                                    if (p->tick() < begin_tick)
                                          begin_tick=p->tick();

                                    if (p->endTick() > end_tick)
                                          end_tick=p->endTick();
                                    
                                    p->unchainClone(); // just for safety; shouldn't be chained anyway.
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

//---------------------------------------------------------
//   getUniqueFileName
//   Sets newAbsFilePath to origFilepath or a new version if found
//   Return true if success
//---------------------------------------------------------

bool getUniqueFileName(const QString& origFilepath, QString& newAbsFilePath)
      {
      QFileInfo fi(origFilepath);  
      if(!fi.exists())
      {
        newAbsFilePath = fi.absoluteFilePath();
        return true;
      }
      
      QString pre = fi.absolutePath() + QString('/') + fi.baseName() + QString('_');
      QString post = QString('.') + fi.completeSuffix();
      for(int i = 1; i < 100000; ++i)
      {
        fi.setFile(pre + QString::number(i) + post);
        if(!fi.exists())
        {
          newAbsFilePath = fi.absoluteFilePath();
          return true;
        }
      }
      
      printf("Could not find a suitable filename (more than 100000 files based on %s - clean up!\n", origFilepath.toLatin1().constData());
      return false;
       }

QString font2StyleSheetFull(const QFont& fnt)
{
//    QColor tt = qApp->palette().brush(QPalette::ToolTipBase).color();
    QString ss("* {" + MusECore::font2StyleSheet(fnt) + "}");
//    ss += "* {background-color: " + QColor(MusEGlobal::config.mixerBg).name() + "}";
    ss += "QToolTip {font-size:" + QString::number(qApp->font().pointSize()) + "pt}";
//    ss += "QToolTip {background-color: " + tt.name() + "}";
//    ss += "QToolTip {font-size:" + QString::number(qApp->font().pointSize()) + "pt}";
    return ss;
}

QString font2StyleSheet(const QFont& fnt)
{
  QString st;
  switch(fnt.style())
  {
    case QFont::StyleNormal:
      st = "normal";
    break;
    case QFont::StyleItalic:
      st = "italic";
    break;
    case QFont::StyleOblique:
      st = "oblique";
    break;
  }
  
  QString wt;
  switch(fnt.weight())
  {
    case QFont::Normal:
      wt = "normal";
    break;
    case QFont::Bold:
      wt = "bold";
    break;
    default:
      // QFont::weight() : "Qt uses a weighting scale from 0 to 99..."
      // Stylesheets : "The weight of a font:"
      // normal 
      // | bold 
      // | 100 
      // | 200 
      // ... 
      // | 900      
      wt = QString::number( (int)(((double)fnt.weight() / 99.0) * 8) * 100 + 100 );
    break;
  }
  
  QString sz;
  if(fnt.pointSize() > 0)
    sz = QString("%1pt").arg(fnt.pointSize());
  else if(fnt.pixelSize() > 0)
    sz = QString("%1px").arg(fnt.pixelSize());
  
  return QString("font: %1 %2 %3 \"%4\"; ").arg(wt).arg(st).arg(sz).arg(fnt.family());
}

} // namespace MusECore
