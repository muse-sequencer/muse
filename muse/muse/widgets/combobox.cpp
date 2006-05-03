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

#include "combobox.h"

//---------------------------------------------------------
//   ComboBox
//---------------------------------------------------------

ComboBox::ComboBox(QWidget* parent, const char* name)
   : QLabel(parent, name)
      {
      _currentItem = 0;
      _id = -1;
      list = new Q3PopupMenu(0, "comboPopup");
      connect(list, SIGNAL(activated(int)), SLOT(activatedIntern(int)));
      setFrameStyle(Q3Frame::Panel | Q3Frame::Raised);
      setLineWidth(2);
      }

ComboBox::~ComboBox()
      {
      delete list;
      }

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void ComboBox::mousePressEvent(QMouseEvent*)
      {
      list->exec(QCursor::pos());
      }

//---------------------------------------------------------
//   activated
//---------------------------------------------------------

void ComboBox::activatedIntern(int n)
      {
      _currentItem = n;
      emit activated(n, _id);
      setText(list->text(_currentItem));
      }

//---------------------------------------------------------
//   setCurrentItem
//---------------------------------------------------------

void ComboBox::setCurrentItem(int i)
      {
      _currentItem = i;
      setText(list->text(list->idAt(_currentItem)));
      }

//---------------------------------------------------------
//   insertItem
//---------------------------------------------------------

void ComboBox::insertItem(const QString& s, int id, int idx)
      {
      list->insertItem(s, id, idx);
      }

