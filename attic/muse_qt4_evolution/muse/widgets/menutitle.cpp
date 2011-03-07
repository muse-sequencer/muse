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

#include "menutitle.h"

//---------------------------------------------------------
//   MenuTitleItem
//---------------------------------------------------------

MenuTitleItem::MenuTitleItem(QString ss)
  : s(ss)
      {
      }

//---------------------------------------------------------
//   sizeHint
//---------------------------------------------------------

QSize MenuTitleItem::sizeHint()
      {
      return QSize(60, 20);
      }

//---------------------------------------------------------
//   drawItem
//---------------------------------------------------------

void MenuTitleItem::paint(QPainter* p, const QColorGroup&, bool,
   bool, int x, int y, int w, int h)
      {
      p->fillRect(x, y, w, h, QBrush(lightGray));
      p->drawText(x, y, w, h, AlignCenter, s);
      }
