//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: dimap.h,v 1.1.1.1 2003/10/27 18:54:28 wschweer Exp $
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

#ifndef __DIMAP_H__
#define __DIMAP_H__

#include <QtGlobal>

namespace MusEGui {

class DiMap
      {
      double d_x1, d_x2;  // double interval boundaries
      int d_y1,d_y2;      // integer interval boundaries
      double d_cnv;       // conversion factor
      bool d_log;		// logarithmic scale?
	
      void newFactor();	

  public:
      static const double LogMin;
      static const double LogMax;

      DiMap();
      DiMap(int, int, double, double, bool lg = FALSE);
      ~DiMap();
	

      bool contains(double x) const;
      bool contains(int x) const;

      void setIntRange(int i1, int i2);
      void setDblRange(double d1, double d2, bool lg = FALSE);

      int transform(double x) const;
      double invTransform(int i) const;
      int limTransform(double x) const;
      double xTransform(double x) const;
	
      double d1() const        { return d_x1;}
      double d2() const        { return d_x2;}
      int i1() const           { return d_y1;}
      int i2() const           { return d_y2;}
      bool logarithmic() const { return d_log;}
      };

} // namespace MusEGui

#endif
