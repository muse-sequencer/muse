//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: menutitle.h,v 1.1 2005/05/06 15:02:11 wschweer Exp $
//
//  (C) Copyright 2005 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MENUTITLE_H__
#define __MENUTITLE_H__

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

