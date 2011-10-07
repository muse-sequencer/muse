//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: scldiv.h,v 1.1.1.1 2003/10/27 18:54:43 wschweer Exp $
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

#ifndef __SCLDIV_H__
#define __SCLDIV_H__

#include <QVector>

namespace MusEGui {

class ScaleDiv
      {
      double d_lBound;
      double d_hBound;
      double d_majStep;
      bool d_log;

      QVector<double> d_majMarks;
      QVector<double> d_minMarks;

      void copy(const ScaleDiv &s);

      bool buildLinDiv(int maxMajMark, int maxMinMark, double step = 0.0);
      bool buildLogDiv(int maxMajMark, int maxMinMark, double step = 0.0);

   public:
      ScaleDiv ();
      virtual ~ScaleDiv();
      ScaleDiv(const ScaleDiv& s);

      ScaleDiv& operator= (const ScaleDiv &s);
      int operator== (const ScaleDiv &s) const;
      int operator!= (const ScaleDiv &s) const;

      double lBound() const { return d_lBound; }
      double hBound() const { return d_hBound; }
      int minCnt() const { return d_minMarks.size(); }
      int majCnt() const { return d_majMarks.size(); }
      bool logScale() const { return d_log; }
      double majMark(int i) const { return d_majMarks[i]; }
      double minMark(int i) const { return d_minMarks[i]; }
      double majStep() const { return d_majStep; }
      void reset();
      bool rebuild(double lBound, double hBound, int maxMaj, int maxMin,
         bool log, double step = 0.0, bool ascend = TRUE);
      };

} // namespace MusEGui

#endif
