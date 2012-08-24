//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: meter.h,v 1.1.1.1.2.2 2009/05/03 04:14:00 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
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

#ifndef __VERTICALMETER_H__
#define __VERTICALMETER_H__

#include "meter.h"

class QPaintEvent;
class QResizeEvent;
class QMouseEvent;
class QPainter;

namespace MusEGui {

class VerticalMeter : public Meter {
      Q_OBJECT
    
   private:
      MeterType mtype;
      bool overflow;
      double val;
      double maxVal;
      double minScale, maxScale;
      int yellowScale, redScale;

      void drawVU(QPainter& p, int, int, int);

      
      virtual void paintEvent(QPaintEvent*);
      virtual void resizeEvent(QResizeEvent*);

   public slots:
      void resetPeaks();
      void setVal(double);

   public:
      VerticalMeter(QWidget* parent, MeterType type = DBMeter);
      void setRange(double min, double max);
      };

} // namespace MusEGui

#endif

