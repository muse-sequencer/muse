//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: key.h,v 1.1.1.1 2003/10/27 18:51:25 wschweer Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __KEY_H__
#define __KEY_H__

#include <stdio.h>
class QPainter;
class QPoint;
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
#endif

