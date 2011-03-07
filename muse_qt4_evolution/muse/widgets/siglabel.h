//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __SIGLABEL_H__
#define __SIGLABEL_H__

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
      void valueChanged(int, int);

   public slots:
      virtual void setValue(int, int);

   public:
      SigLabel(QWidget* parent = 0);
      void value(int& a, int& b) const { a = z; b = n; }
      void setFrame(bool);
      };
#endif

