//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: tlwlayout.h,v 1.4 2005/11/01 10:00:29 wschweer Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __TLWLAYOUT_H__
#define __TLWLAYOUT_H__

//---------------------------------------------------------
//   TLWidgetLayout
//---------------------------------------------------------

class TLWidgetLayout : public QLayout {
      QList<QLayoutItem*> itemList;

      int doLayout(const QRect& rect, bool testOnly) const;

   public:
      TLWidgetLayout(QWidget* parent);
      TLWidgetLayout();
      ~TLWidgetLayout() { clear(); }

      void addItem(QLayoutItem* item) { itemList.append(item); }
      Qt::Orientations expandingDirections() const { return 0; }
      bool hasHeightForWidth() const { return false; }
      int count() const { return itemList.size(); }
      QSize minimumSize() const;
      void setGeometry(const QRect &rect);
      QSize sizeHint() const;
      QLayoutItem *itemAt(int index) const { return itemList.value(index); }
      QLayoutItem *takeAt(int index);
      void clear();
      };

#endif

