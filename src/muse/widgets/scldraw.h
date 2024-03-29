//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: scldraw.h,v 1.1.1.1 2003/10/27 18:55:08 wschweer Exp $
//
//    Copyright (C) 1997  Josef Wilgen
//    (C) Copyright 2000 Werner Schweer (ws@seh.de)
//    (C) Copyright 2016 Tim E. Real (terminator356 on sourceforge)
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
#include <QRect>
#include <QPoint>

#include "dimap.h"
#include "scldiv.h"

class QPalette;
class QFontMetrics;
class QPainter;

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

      void drawTick(QPainter *p, const QPalette& palette, double curValue, double val, int len) const;
      void drawBackbone(QPainter *p, const QPalette& palette, double curValue) const;
      void drawLabel(QPainter *p, const QPalette& palette, double curValue, double val, bool isSpecialText = false) const;

   public:

      ScaleDraw();

      void setScale(const ScaleDiv &s);
      void setScale(double vmin, double vmax, int maxMajIntv, int maxMinIntv,
        double step = 0.0, int logarithmic = 0);
      QPoint originOffsetHint(const QFontMetrics&, bool worst = false) const;
      void setGeometry(int xorigin, int yorigin, int length);
      void setAngleRange(double angle1, double angle2);
      // Special 'M' format (Metric suffix G, M, K, m, n, p) supported.
      void setLabelFormat(char f, int prec);
      void setBackBone(bool v);
      
      const ScaleDiv& scaleDiv() const;
      OrientationX orientation() const;
      void setOrientation(const OrientationX&);
      TextHighlightMode textHighlightMode() const;
      void setTextHighlightMode(TextHighlightMode mode);
      QString specialText() const;
      void setSpecialText(const QString& s);
      
      QRect maxBoundingRect(const QFontMetrics& fm) const;
      int maxWidth(const QFontMetrics& fm, bool worst = true, int penWidth = 1) const;
      int maxHeight(const QFontMetrics& fm, int penWidth = 1) const;
      int maxLabelWidth(const QFontMetrics& fm, bool worst = true) const;
      int scaleWidth(int penWidth = 1) const;
      
      void draw(QPainter *p, const QPalette& palette, double curValue = 0.0); // const;
      };

} // namespace MusEGui

#endif

