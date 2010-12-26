//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: alayout.h,v 1.3.2.1 2008/01/19 13:33:46 wschweer Exp $
//  (C) Copyright 2002 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __ALAYOUT_H__
#define __ALAYOUT_H__

#include <QLayout>
#include <QList>

class QLayoutItem;
class QScrollBar;

class WidgetStack;

//---------------------------------------------------------
//   TLLayout
//    arranger trackList layout manager
//---------------------------------------------------------

class TLLayout : public QLayout
      {
      Q_OBJECT

      QList<QLayoutItem*> ilist;
      QLayoutItem* li[6];
      QScrollBar* sb;
      WidgetStack* stack;

    public:
      //TLLayout(QWidget *parent) : QLayout(parent, 0, -1) {}
      TLLayout(QWidget *parent) : QLayout(parent) { setContentsMargins(0, 0, 0, 0); setSpacing(-1); }
      ~TLLayout() { clear(); }

      void addItem(QLayoutItem *item) { ilist.append(item); }
      virtual Qt::Orientations expandingDirections() const { return 0; }
      virtual bool hasHeightForWidth() const { return false; }
      virtual int count() const { return ilist.size(); }
      void clear();

      void wadd(int idx, QWidget* w);
      virtual QSize sizeHint() const;
      virtual QSize minimumSize() const;
      virtual QSize maximumSize() const;
      //QSize sizeHint() const;
      //QSize minimumSize() const;
      //QSize maximumSize() const;
      ///QLayoutIterator iterator();
      virtual void setGeometry(const QRect &rect);

      //virtual QLayoutItem* itemAt(int) const { return 0;} // ddskrjo, is pure virtual, overridden
      virtual QLayoutItem* itemAt(int i) const { return ilist.value(i);} 
      virtual QLayoutItem* takeAt(int); // { return 0;} // ddskrjo, is pure virtual, overridden
      ///virtual int count() const { return ilist.count(); } // ddskrjo, is pure virtual, overridden
      };
#endif
