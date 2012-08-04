//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: utils.h,v 1.1.1.1.2.3 2009/11/14 03:37:48 terminator356 Exp $
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

#ifndef __UTILS_H__
#define __UTILS_H__

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

class QFrame;
class QString;
class QWidget;
class QGradient;
class QCanvas;
class QPointF;
class QColor;
class QPainterPath;

namespace MusECore {

enum Corner { UpperLeft = 0x1, UpperRight = 0x2, LowerLeft = 0x4, LowerRight = 0x8 };

extern QString bitmap2String(int bm);
extern int string2bitmap(const QString& str);
extern QString u32bitmap2String(unsigned int bm);
extern unsigned int string2u32bitmap(const QString& str);
extern bool autoAdjustFontSize(QFrame* w, const QString& s, bool ignoreWidth = false, bool ignoreHeight = false, int max = 10, int min = 4);
extern QGradient gGradientFromQColor(const QColor& c, const QPointF& start, const QPointF& finalStop);

extern int num2cols(int min, int max);
extern QFrame* hLine(QWidget* parent);
extern QFrame* vLine(QWidget* parent);
extern void dump(const unsigned char* p, int n);
extern double curTime();

extern QPainterPath roundedPath(QRect r, int xrad, int yrad, Corner roundCorner);
extern QPainterPath roundedPath(int x, int y, int w, int h, int xrad, int yrad, Corner roundCorner);

extern QIcon colorRect(const QColor& color, int width, int height);
extern int get_paste_len();

} // namespace MusECores

#endif

