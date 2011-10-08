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

#include "dimap.h"
#include "scldiv.h"

class QPainter;
class QRect;

namespace MusEGui {

class ScaleDraw : public DiMap {
   public:
      enum OrientationX { Bottom, Top, Left, Right, Round };

   private:
      ScaleDiv d_scldiv;
      static const int minLen;
      OrientationX d_orient;
	
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
	
      void drawTick(QPainter *p, double val, int len) const;
      void drawBackbone(QPainter *p) const;
      void drawLabel(QPainter *p, double val) const;
	
   public:

      ScaleDraw();

      void setScale(const ScaleDiv &s);
      void setScale(double vmin, double vmax, int maxMajIntv, int maxMinIntv,
	   double step = 0.0, int logarithmic = 0);
      void setGeometry(int xorigin, int yorigin, int length, OrientationX o);
      void setAngleRange(double angle1, double angle2);
      void setLabelFormat(char f, int prec);

      const ScaleDiv& scaleDiv() const { return d_scldiv; }
      OrientationX orientation() const { return d_orient; }
      QRect maxBoundingRect(QPainter *p) const;
      int maxWidth(QPainter *p, bool worst = TRUE) const;
      int maxHeight(QPainter *p) const;
      int maxLabelWidth(QPainter *p, int worst = TRUE) const;
      void draw(QPainter *p) const;
      };

} // namespace MusEGui

#endif







