//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: scldraw.h,v 1.1.1.1 2003/10/27 18:55:08 wschweer Exp $
//
//    Copyright (C) 1997  Josef Wilgen
//    (C) Copyright 2000 Werner Schweer (ws@seh.de)
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

#ifndef __SCLDRAW_H__
#define __SCLDRAW_H__

#include <QString>
#include "dimap.h"
#include "scldiv.h"

class QPalette;
class QFontMetrics;
class QPainter;
class QRect;
class QString;

namespace MusEGui {

class ScaleDraw : public DiMap {
   public:
      enum OrientationX { Bottom, Top, Left, Right, InsideHorizontal, InsideVertical, Round };
      enum TextHighlightMode { TextHighlightNone, 
                               TextHighlightAlways, 
                               TextHighlightSplit, 
                               TextHighlightShadow,
                               TextHighlightSplitAndShadow };

   private:
      ScaleDiv d_scldiv;
      static const int minLen;
      OrientationX d_orient;
      TextHighlightMode d_textHighlightMode;
      QString _specialText;   // Text to show if value = min
      
      int d_xorg;
      int d_yorg;
      int d_len;
	
      int d_hpad;
      int d_vpad;
	
      int d_medLen;
      int d_majLen;
      int d_minLen;

      int d_minAngle;
      int d_maxAngle;

      double d_xCenter;
      double d_yCenter;
      double d_radius;

      char d_fmt;
      int d_prec;
      
      bool d_drawBackBone;

      // Like QString::number except it allows special 'M' format (Metric suffix G, M, K).
      QString composeLabelText(double val, char fmt, int prec) const;
      
      void drawTick(QPainter *p, const QPalette& palette, double curValue, double val, int len) const;
      void drawBackbone(QPainter *p, const QPalette& palette, double curValue) const;
      void drawLabel(QPainter *p, const QPalette& palette, double curValue, double val, bool isSpecialText = false) const;
	
   public:

      ScaleDraw();

      void setScale(const ScaleDiv &s);
      void setScale(double vmin, double vmax, int maxMajIntv, int maxMinIntv,
	   double step = 0.0, int logarithmic = 0);
      void setGeometry(int xorigin, int yorigin, int length, OrientationX o);
      void setAngleRange(double angle1, double angle2);
      // Special 'M' format (Metric suffix G, M, K) supported.
      void setLabelFormat(char f, int prec);
      void setBackBone(bool v) { d_drawBackBone = v; }
      
      const ScaleDiv& scaleDiv() const { return d_scldiv; }
      OrientationX orientation() const { return d_orient; }
      TextHighlightMode textHighlightMode() const { return d_textHighlightMode; }
      void setTextHighlightMode(TextHighlightMode mode) { d_textHighlightMode = mode; }
      QString specialText() const           { return _specialText; }
      void setSpecialText(const QString& s) { _specialText = s; }
      
// REMOVE Tim. Trackinfo. Changed.
//       QRect maxBoundingRect(QPainter *p) const;
//       int maxWidth(QPainter *p, bool worst = true) const;
//       int maxHeight(QPainter *p) const;
//       int maxLabelWidth(QPainter *p, bool worst = true) const;
      QRect maxBoundingRect(const QFontMetrics& fm) const;
      int maxWidth(const QFontMetrics& fm, bool worst = true, int penWidth = 1) const;
      int maxHeight(const QFontMetrics& fm, int penWidth = 1) const;
      int maxLabelWidth(const QFontMetrics& fm, bool worst = true) const;
      int scaleWidth(int penWidth = 1) const;
      
      void draw(QPainter *p, const QPalette& palette, double curValue = 0.0); // const;
      };

} // namespace MusEGui

#endif







