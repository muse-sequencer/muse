//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: markerview.h,v 1.16 2006/02/06 17:57:21 wschweer Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MARKERVIEW_H__
#define __MARKERVIEW_H__

#include "cobject.h"

namespace Awl {
      class PosEdit;
      };

namespace AL {
      class Pos;
      class Xml;
      class Marker;
      };
using AL::Pos;
using AL::Xml;


//---------------------------------------------------------
//   MarkerItem
//---------------------------------------------------------

class MarkerItem : public QTreeWidgetItem {
      AL::Marker* _marker;

   public:
      MarkerItem(QTreeWidget* parent, AL::Marker* m);
      AL::Marker* marker() const { return _marker; }
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
      Q_OBJECT

      QTreeWidget* table;
      QLineEdit* editName;
      Awl::PosEdit* editSMPTE;
      Awl::PosEdit* editTick;
      QToolButton* lock;

      void closeEvent(QCloseEvent* e) {
            emit closed();
            QMainWindow::closeEvent(e);
            }

   signals:
      void closed();

   private slots:
      void addMarker();
      void addMarker(const AL::Pos&);
      void removeMarker(const AL::Pos&);
      void deleteMarker();
      void markerSelectionChanged(QTreeWidgetItem*);
      void nameChanged(const QString&);
      void tickChanged(const Pos&);
      void lockChanged(bool);
      void markerChanged(int);
      void clicked(QTreeWidgetItem*);
      void updateList();

   public:
      MarkerView();
      virtual ~MarkerView();
      };

#endif

