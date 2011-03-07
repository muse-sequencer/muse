//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: menutitleitem.h,v 1.1.2.1 2009/06/10 00:34:59 terminator356 Exp $
//  (C) Copyright 1999-2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MENU_TITLE_ITEM_H__
#define __MENU_TITLE_ITEM_H__

#include <qmenudata.h>

//---------------------------------------------------------
//   MenuTitleItem
//---------------------------------------------------------

class MenuTitleItem : public QCustomMenuItem {
      QString s;
      virtual bool fullSpan() const    { return true; }
      virtual bool isSeparator() const { return true; }
      virtual void paint(QPainter* p, const QColorGroup& cg, bool act,
         bool, int, int, int, int);
      virtual QSize sizeHint();

   public:
      MenuTitleItem(QString s);
      };

#endif
