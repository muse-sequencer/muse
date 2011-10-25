//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: checkbox.h,v 1.2.2.2 2006/10/29 07:54:52 terminator356 Exp $
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
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

#ifndef __CHECKBOX_H__
#define __CHECKBOX_H__

#include <QCheckBox>

namespace MusEGui {

//---------------------------------------------------------
//   CheckBox
//---------------------------------------------------------

class CheckBox : public QCheckBox {
      Q_OBJECT
      Q_PROPERTY( int id READ id WRITE setId )

      int _id;

   protected:
      void mousePressEvent(QMouseEvent *e);
      void mouseReleaseEvent(QMouseEvent *e);
   
   private slots:
      void hasToggled(bool val);

   signals:
      void toggleChanged(bool, int);
      void checkboxPressed(int);
      void checkboxReleased(int);
      void checkboxRightClicked(const QPoint &, int);

   public:
      CheckBox(QWidget* parent, int i, const char* name = 0);
      int id() const       { return _id; }
      void setId(int i)    { _id = i; }
      };

} // namespace MusEGui

#endif

