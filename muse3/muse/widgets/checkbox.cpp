//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: checkbox.cpp,v 1.2.2.2 2006/10/29 07:54:52 terminator356 Exp $
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

#include "checkbox.h"

#include <QMouseEvent>

namespace MusEGui {

//---------------------------------------------------------
//   CheckBox
//---------------------------------------------------------

CheckBox::CheckBox(QWidget* parent, int i, const char* name)
   : QCheckBox(parent)
      {
      setObjectName(name);
      _id = i;
      connect(this, SIGNAL(toggled(bool)), SLOT(hasToggled(bool)));
      }

void CheckBox::hasToggled(bool val)
      {
      emit toggleChanged(val, _id);
      }

//------------------------------------------------------------
//  mousePressEvent
//------------------------------------------------------------

void CheckBox::mousePressEvent(QMouseEvent *e)
{
  if(e->button() == Qt::RightButton)
    emit checkboxRightClicked(e->globalPos(), _id);
  else
  {
    if(isChecked())
      setChecked(false);
    else
      setChecked(true);
    emit checkboxPressed(_id);
  }
}

//------------------------------------------------------------
//  mouseReleaseEvent
//------------------------------------------------------------

void CheckBox::mouseReleaseEvent(QMouseEvent *e)
{
  if(e->button() == Qt::RightButton)
    return;
    
  emit checkboxReleased(_id);
}

} // namespace MusEGui
