//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: markerview.h,v 1.4.2.3 2008/08/18 00:15:25 terminator356 Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MARKERVIEW_H__
#define __MARKERVIEW_H__

#include "cobject.h"
#include <qmainwindow.h>
#include <qlistview.h>

class QLineEdit;
class PosEdit;
class QToolButton;
class Marker;
class QToolBar;
class Pos;

//---------------------------------------------------------
//   MarkerItem
//---------------------------------------------------------

class MarkerItem : public QListViewItem {
      Marker* _marker;

   public:
      MarkerItem(QListView* parent, Marker* m);
      Marker* marker() const { return _marker; }
      unsigned tick() const;
      const QString name() const;
      bool lock() const;
      void setName(const QString& s);
      void setTick(unsigned t);
      void setLock(bool lck);
      };

//---------------------------------------------------------
//   MarkerView
//---------------------------------------------------------

class MarkerView : public TopWin {
      QListView* table;
      QLineEdit* editName;
      PosEdit* editSMPTE;
      PosEdit* editTick;
      QToolButton* lock;
      QToolBar* tools;

      Q_OBJECT
      virtual void closeEvent(QCloseEvent*);

   private slots:
      void addMarker();
      void addMarker(int);
      void deleteMarker();
      void markerSelectionChanged();
      void nameChanged(const QString&);
      void tickChanged(const Pos&);
      void lockChanged(bool);
      void markerChanged(int);
      void clicked(QListViewItem*);
      void updateList();
      void songChanged(int);

   signals:
      void deleted(unsigned long);
      void closed();

   public:
      MarkerView(QWidget* parent);
      ~MarkerView();
      virtual void readStatus(Xml&);
      virtual void writeStatus(int, Xml&) const;
      void nextMarker();
      void prevMarker();
      };

#endif

