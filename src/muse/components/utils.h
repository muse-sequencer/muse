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

#include <stdint.h>

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

#include <QFont>
#include <QString>
#include <QGradient>
#include <QPointF>
#include <QColor>
#include <QPainterPath>
#include <QIcon>

// Forward declarations:
class QFrame;
class QWidget;
class QPainter;

namespace MusECore {

enum Corner { CornerUpperLeft = 0x1, CornerUpperRight = 0x2, CornerLowerLeft = 0x4, CornerLowerRight = 0x8, CornerAll = 0xF };

extern QString bitmap2String(int bm);
extern int string2bitmap(const QString& str);
extern QString u32bitmap2String(unsigned int bm);
extern unsigned int string2u32bitmap(const QString& str);
extern bool autoAdjustFontSize(QFrame* w, const QString& s, QFont& targetFont, bool ignoreWidth = false, bool ignoreHeight = false, int max = 10, int min = 4);
extern QGradient gGradientFromQColor(const QColor& c, const QPointF& start, const QPointF& finalStop);
extern QGradient getGradientFromColor(const QColor& c, const QPoint& start, const QPoint& stop, const int strength);
extern bool isColorBright(const QColor& c);
extern int getPerceivedLuminance(const QColor& c);

extern int num2cols(int min, int max);
extern QFrame* hLine(QWidget* parent);
extern QFrame* vLine(QWidget* parent);
extern void dump(const unsigned char* p, int n);
extern uint64_t curTimeUS();

extern QPainterPath roundedPath(const QRect& r, int xrad, int yrad, Corner roundCorner);
extern QPainterPath roundedPath(int x, int y, int w, int h, int xrad, int yrad, Corner roundCorner);

extern void addRoundedPath(QPainterPath* path, const QRect& r, int xrad, int yrad, Corner roundCorner);
extern void addRoundedPath(QPainterPath* path, int x, int y, int w, int h, int xrad, int yrad, Corner roundCorner);

extern QIcon colorRect(const QColor& color, int width, int height);
extern int get_paste_len();

extern bool getUniqueFileName(const QString& filename, QString& newAbsFilePath);

extern QString font2StyleSheet(const QFont& fnt);
extern QString font2StyleSheetFull(const QFont& fnt);

// These two functions are intended to solve an apparent BUG in Qt's patterned line drawing.
// Artifacts are left over at the end of the line, causing smearing when scrolled.
// It only happens with long 1 pixel wide patterned lines. 2 or more pixels wide, or solid lines, or short lines, are OK.
// The solution is to break the line up into segments.
// segLength should be a multiple of the pattern length so that the segments are drawn cleanly and contiguous.
// For example with a pattern of one dot and two dashes, 21 is a good value.
// segLength should be a fairly low value, say in the 20's, because the bug only occurs with longer lines.
// Thus by segmenting the line into say 20's, it never gets a chance to exhibit the artifacts seen with longer lines.
extern void drawSegmentedHLine(QPainter* p, int x1, int x2, int y, int segLength, int offset = 0);
extern void drawSegmentedVLine(QPainter* p, int x, int y1, int y2, int segLength, int offset = 0);

} // namespace MusECore

#endif

