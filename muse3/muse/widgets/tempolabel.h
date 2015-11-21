//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tempolabel.h,v 1.1.1.1 2003/10/27 18:55:05 wschweer Exp $
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

#ifndef __TEMPOLABEL_H__
#define __TEMPOLABEL_H__

#include <QLabel>
#include "doublespinbox.h"

namespace MusEGui {

//---------------------------------------------------------
//   TempoLabel
//---------------------------------------------------------

class TempoLabel : public QLabel {
      Q_OBJECT
    
      double _value;

      

   protected:
      QSize sizeHint() const;

   public slots:
      void setValue(int);
      void setValue(double);

   public:
      TempoLabel(QWidget*, const char* name = 0);
      };

//---------------------------------------------------------
//   TempoEdit
//---------------------------------------------------------

class TempoEdit : public DoubleSpinBox {
      Q_OBJECT

      double curVal;
      
   protected:
      QSize sizeHint() const;

   private slots:
      void newValue(double);

   public slots:
      void setValue(double);

   signals:
      void tempoChanged(double);

   public:
      TempoEdit(QWidget*);
      //int tempo() const;
      };

} // namespace MusEGui

#endif

