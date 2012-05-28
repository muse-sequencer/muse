//=========================================================
//  MusE
//  Linux Music Editor
//    doublespinbox.h (C) Copyright 2012 Tim E. Real (terminator356 at users dot sourceforge dot net)  
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

#ifndef __DOUBLESPINBOX_H__
#define __DOUBLESPINBOX_H__

#include <QDoubleSpinBox>
#include <QLineEdit>

namespace MusEGui { 

class DoubleSpinBoxLineEdit : public QLineEdit
{
  Q_OBJECT
  
  protected:
    virtual void mouseDoubleClickEvent(QMouseEvent* e);
    //virtual void mousePressEvent(QMouseEvent* e);

  signals:
    void doubleClicked();
    void ctrlDoubleClicked();
    //void ctrlClicked();

  public:
    DoubleSpinBoxLineEdit(QWidget* parent = 0) : QLineEdit(parent) {};
};

//---------------------------------------------------------
//   DoubleSpinBox
//---------------------------------------------------------

class DoubleSpinBox : public QDoubleSpinBox {
   Q_OBJECT

   protected:
      virtual void keyPressEvent(QKeyEvent*);
      virtual void wheelEvent(QWheelEvent*);

   signals:
      void doubleClicked();
      void ctrlDoubleClicked();
      //void ctrlClicked();
      void returnPressed();
      void escapePressed();

   public:
      DoubleSpinBox(QWidget* parent=0);
      DoubleSpinBox(double minValue, double maxValue, double step = 1.0, QWidget* parent=0);
};

} // namespace MusEGui

#endif

