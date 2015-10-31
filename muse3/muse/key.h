//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: key.h,v 1.1.1.1 2003/10/27 18:51:25 wschweer Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
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

#ifndef __KEY_H__
#define __KEY_H__

#include <stdio.h>
class QPainter;
class QPoint;

namespace MusECore {

class Xml;

//---------------------------------------------------------
//   NKey
//---------------------------------------------------------

class NKey {
      static int offsets[14];
      int val;
   public:
      NKey() { val = 7; }
      NKey(int k) { val = k; }
      void draw(QPainter& p, const QPoint& pt) const;
      int idx() const  { return val; }
      int offset() const { return offsets[val]; }
      void read(Xml&);
      void write(int, Xml&) const;
      void set(int n) { val = n; }
      int width() const;
      };

//---------------------------------------------------------
//   Scale
//---------------------------------------------------------

class Scale {
      int val;                // 1 = 1 sharp,  -1 1 flat
      bool minor;
   public:
      Scale() { val = 0; minor = false; }
      Scale(int s, bool m = false) { val = s; minor = m; }
      int idx() const             { return val; }
      void read(Xml&);
      void write(int, Xml&) const;
      void set(int n)             { val = n; }
      void setMajorMinor(bool f)  { minor = f; }      // true == minor
      int width() const;
      };

} // namespace MusECore

#endif

