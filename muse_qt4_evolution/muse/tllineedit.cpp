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

#include "tllineedit.h"

//---------------------------------------------------------
//   TLLineEdit
//---------------------------------------------------------

TLLineEdit::TLLineEdit(const QString& contents, QWidget* parent)
   : QLineEdit(contents, parent)
      {
      setReadOnly(true);
      setFrame(false);
      setAlignment(Qt::AlignLeft);
      setCursorPosition(0);
      connect(this, SIGNAL(editingFinished()), SLOT(contentHasChanged()));
      }

//---------------------------------------------------------
//   contentHasChanged
//---------------------------------------------------------

void TLLineEdit::contentHasChanged()
      {
      setReadOnly(true);
      setFrame(false);
      if (isModified())
            emit contentChanged(text());
      setModified(false);
      }

//---------------------------------------------------------
//   mouseDoubleClickEvent
//---------------------------------------------------------

void TLLineEdit::mouseDoubleClickEvent(QMouseEvent*)
      {
      setReadOnly(false);
      setFocus();
      setFrame(true);
      }

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void TLLineEdit::mousePressEvent(QMouseEvent* ev)
      {
      QLineEdit::mousePressEvent(ev);
      emit mousePress();
      }

