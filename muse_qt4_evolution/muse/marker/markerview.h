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

      bool lockChange;

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
      void deleteMarker();
      void currentChanged(QTreeWidgetItem*);
      void selectionChanged();
      void nameChanged(const QString&);
      void tickChanged(const Pos&);
      void lockChanged(bool);
      void markerChanged(int);
      void clicked(QTreeWidgetItem*);
      void updateList();

   public:
      MarkerView();
      };

#endif

