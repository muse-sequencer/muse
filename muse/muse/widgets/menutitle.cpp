//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: menutitle.cpp,v 1.1 2005/05/06 15:02:11 wschweer Exp $
//
//  (C) Copyright 2005 Werner Schweer (ws@seh.de)
//=========================================================

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
