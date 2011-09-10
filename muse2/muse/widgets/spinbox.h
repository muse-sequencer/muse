//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: spinbox.h,v 1.1.2.2 2009/02/02 21:38:01 terminator356 Exp $
//    (C) Copyright 2001 Werner Schweer (ws@seh.de)
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

// SpinBox:
// Click up/down, or mousewheel, or hit enter with un-modified text (which means enter TWICE for modified text), 
// and the control will give up focus, thereby allowing you to use global shortcut keys afterwards. 
// Up/down keys still keep the focus.
#ifndef __SPINBOX_H__
#define __SPINBOX_H__

#include <QSpinBox>
#include <QEvent>

namespace MusEWidget { 

//---------------------------------------------------------
//   SpinBox
//---------------------------------------------------------

class SpinBox : public QSpinBox {
      Q_OBJECT

      bool _clearFocus; 

   protected:
      bool eventFilter(QObject* obj, QEvent* ev);
      
   public slots:
      virtual void stepUp();
      virtual void stepDown();
   
   signals:
      void doubleClicked();

   public:
      SpinBox(QWidget* parent=0);
      SpinBox(int minValue, int maxValue, int step = 1, QWidget* parent=0);
};

} // namespace MusEWidget

#endif

