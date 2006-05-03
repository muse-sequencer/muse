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

#include <sys/time.h>
#include "utils.h"
#include "icons.h"
#include "gconfig.h"
#include "greendotbutton.h"
#include "recordbutton.h"

static const int BFONT = 4;   // simple button font

//---------------------------------------------------------
//   RecordButton
//---------------------------------------------------------

RecordButton::RecordButton(QWidget* parent)
   : SimpleButton(record_on_Icon, record_off_Icon, parent)
      {
      setCheckable(true);
      setToolTip(tr("record"));
      }

//---------------------------------------------------------
//   GreendotButton
//---------------------------------------------------------

GreendotButton::GreendotButton(QWidget* parent)
   : SimpleButton(greendotIcon, darkgreendotIcon, parent)
      {
      setCheckable(true);
      setToolTip(tr("record"));
      }

//---------------------------------------------------------
//   curTime
//---------------------------------------------------------

double curTime()
      {
      struct timeval t;
      gettimeofday(&t, 0);
      return (double)((double)t.tv_sec + (t.tv_usec / 1000000.0));
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
//printf(" -> <%s>\n", s.toLatin1().data());
      return s;
      }

//---------------------------------------------------------
//   string2bitmap
//---------------------------------------------------------

int string2bitmap(const QString& str)
      {
      int val = 0;
      QString ss(str.simplified());
      const char* s = ss.toLatin1().data();
//printf("string2bitmap <%s>\n", s);

      if (s == 0)
            return 0;
      if (strcmp(s, "all") == 0)
            return 0xffff;
      if (strcmp(s, "none") == 0)
            return 0;
// printf("str2bitmap: <%s> ", str.toLatin1().data());
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
//   muteButton
//---------------------------------------------------------

SimpleButton* newMuteButton(QWidget* parent)
      {
      // SimpleButton* mute  = new SimpleButton(parent, muteIconOff, muteIconOn);
      SimpleButton* mute = new SimpleButton(QT_TR_NOOP("m"), parent);
      mute->setFont(*config.fonts[BFONT]);
      mute->setCheckable(true);
      mute->setToolTip(QT_TR_NOOP("mute"));
      return mute;
      }

//---------------------------------------------------------
//   soloButton
//---------------------------------------------------------

SimpleButton* newSoloButton(QWidget* parent)
      {
//      SimpleButton* solo  = new SimpleButton(parent, soloIconOn, soloIconOff);
      SimpleButton* solo = new SimpleButton(QT_TR_NOOP("s"), parent);
      solo->setFont(*config.fonts[BFONT]);
      solo->setCheckable(true);
      solo->setToolTip(QT_TR_NOOP("solo"));
      return solo;
      }

//---------------------------------------------------------
//   monitorButton
//---------------------------------------------------------

SimpleButton* newMonitorButton(QWidget* parent)
      {
      SimpleButton* monitor = new SimpleButton(QT_TR_NOOP("Mo"), parent);
      monitor->setFont(*config.fonts[BFONT]);
      monitor->setCheckable(true);
      monitor->setToolTip(QT_TR_NOOP("monitor"));
      return monitor;
      }

//---------------------------------------------------------
//   drumMapButton
//---------------------------------------------------------

SimpleButton* newDrumMapButton(QWidget* parent)
      {
      SimpleButton* dm = new SimpleButton(QT_TR_NOOP("Dr"), parent);
      dm->setFont(*config.fonts[BFONT]);
      dm->setCheckable(true);
      dm->setToolTip(QT_TR_NOOP("use drum map"));
      return dm;
      }

//---------------------------------------------------------
//   newOffButton
//---------------------------------------------------------

SimpleButton* newOffButton(QWidget* parent)
      {
      SimpleButton* off  = new SimpleButton(exit1Icon, exitIcon, parent);
      off->setCheckable(true);
      off->setToolTip(QT_TR_NOOP("off"));
      return off;
      }

//---------------------------------------------------------
//   newRecordButton
//---------------------------------------------------------

SimpleButton* newRecordButton(QWidget* parent)
      {
      return new RecordButton(parent);
      }

//---------------------------------------------------------
//   newAutoReadButton
//---------------------------------------------------------

SimpleButton* newAutoReadButton(QWidget* parent)
      {
      SimpleButton* ar = new SimpleButton(QT_TR_NOOP("aR"), parent);
      ar->setFont(*config.fonts[BFONT]);
      ar->setCheckable(true);
      ar->setToolTip(QT_TR_NOOP("automation read"));
      return ar;
      }

//---------------------------------------------------------
//   newAutoWriteButton
//---------------------------------------------------------

SimpleButton* newAutoWriteButton(QWidget* parent)
      {
      SimpleButton* aw = new SimpleButton(QT_TR_NOOP("aW"), parent);
      aw->setFont(*config.fonts[BFONT]);
      aw->setCheckable(true);
      aw->setToolTip(QT_TR_NOOP("automation write"));
      return aw;
      }

//---------------------------------------------------------
//   syncButton
//---------------------------------------------------------

SimpleButton* newSyncButton(QWidget* parent)
      {
      SimpleButton* sync = new SimpleButton(QT_TR_NOOP("sync"), parent);
      sync->setFont(*config.fonts[BFONT]);
      sync->setCheckable(true);
      sync->setToolTip(QT_TR_NOOP("send sync events"));
      return sync;
      }

//---------------------------------------------------------
//   newMinusButton
//---------------------------------------------------------

SimpleButton* newMinusButton(QWidget* parent)
      {
      return new SimpleButton(minusIcon, minusIcon, parent);
      }

//---------------------------------------------------------
//   newPlusButton
//---------------------------------------------------------

SimpleButton* newPlusButton(QWidget* parent)
      {
      SimpleButton* sync = new SimpleButton(plusIcon, plusIcon, parent);
      return sync;
      }

//---------------------------------------------------------
//   newStereoButton
//---------------------------------------------------------

SimpleButton* newStereoButton(QWidget* parent)
      {
      SimpleButton* stereo  = new SimpleButton(stereoIcon, monoIcon, parent);
      stereo->setCheckable(true);
      stereo->setToolTip(QT_TR_NOOP("1/2 channel"));
      return stereo;
      }

//---------------------------------------------------------
//   fatalError
//---------------------------------------------------------

void fatalError(const char* s)
      {
      fprintf(stderr, "%s\n", s);
      exit(-1);
      }

//---------------------------------------------------------
//   lineColor
//---------------------------------------------------------

QColor lineColor[splitWidth] = {
      QColor(0x55, 0x55, 0x52),
      QColor(0xc6, 0xc6, 0xbf),
      QColor(0xee, 0xee, 0xe6),
      QColor(0xff, 0xff, 0xfc),
      QColor(0xff, 0xff, 0xff),
      QColor(0x55, 0x55, 0x52),
      };

//---------------------------------------------------------
//   paintHLine
//---------------------------------------------------------

void paintHLine(QPainter& p, int x1, int x2, int y)
      {
      for (int i = 0; i < splitWidth; ++i) {
            p.setPen(lineColor[i]);
            p.drawLine(x1, y, x2, y);
            ++y;
            }
      }

