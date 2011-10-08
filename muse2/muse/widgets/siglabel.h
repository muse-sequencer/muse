//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: siglabel.h,v 1.1.1.1 2003/10/27 18:54:56 wschweer Exp $
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

#ifndef __SIGLABEL_H__
#define __SIGLABEL_H__

#include <al/sig.h>

#include <QLabel>

class QWheelEvent;
class QMouseEvent;

namespace MusEGui {

//---------------------------------------------------------
//   SigLabel
//    show/edit time signature
//---------------------------------------------------------

class SigLabel : public QLabel {
      Q_OBJECT
      virtual void mousePressEvent(QMouseEvent*);
      virtual void wheelEvent(QWheelEvent*);
      void incValue(bool zaehler, bool inc, int&, int&);

   protected:
      int z, n;
      
   signals:
      void valueChanged(const AL::TimeSignature&);

   public slots:
      virtual void setValue(int, int);
      virtual void setValue(const AL::TimeSignature& sig) { setValue(sig.z, sig.n); }

   public:
      SigLabel(int z, int n, QWidget*);
      SigLabel(const AL::TimeSignature&, QWidget*);
      void value(int& a, int& b) const { a = z; b = n; }
      AL::TimeSignature value() const { return AL::TimeSignature(z, n); }
      void setFrame(bool);
      };

} // namespace MusEGui

#endif

