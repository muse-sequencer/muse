//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: alayout.h,v 1.3.2.1 2008/01/19 13:33:46 wschweer Exp $
//  (C) Copyright 2002 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __ALAYOUT_H__
#define __ALAYOUT_H__

#include <qlayout.h>
#include <qptrlist.h>

class QScrollBar;
class WidgetStack;

//---------------------------------------------------------
//   TLLayout
//    arranger trackList layout manager
//---------------------------------------------------------

class TLLayout : public QLayout
      {
      Q_OBJECT

      QPtrList<QLayoutItem> ilist;
      QLayoutItem* li[6];
      QScrollBar* sb;
      WidgetStack* stack;

    public:
      TLLayout(QWidget *parent) : QLayout(parent, 0, -1) {}
      ~TLLayout();

      void addItem(QLayoutItem *item);

      void wadd(int idx, QWidget* w);
      virtual QSize sizeHint() const;
      virtual QSize minimumSize() const;
      virtual QSize maximumSize() const;
      QLayoutIterator iterator();
      void setGeometry(const QRect &rect);
      };
#endif
