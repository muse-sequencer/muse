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

#ifndef __LISTEDIT_H__
#define __LISTEDIT_H__

#include "cobject.h"

namespace AL {
      class Pos;
      };

class Track;
class Part;
class Ctrl;
class CtrlListEditor;

//---------------------------------------------------------
//   ListType
//---------------------------------------------------------

enum { LIST_TRACK, LIST_PART, LIST_CTRL };

struct ListType {
      int id;
      Track* track;
      Part* part;
      Ctrl* ctrl;
      
      bool operator==(const ListType& t) const;
      };

Q_DECLARE_METATYPE(struct ListType);

//---------------------------------------------------------
//   ListWidget
//    interface class
//---------------------------------------------------------

class ListWidget : public QWidget {
      Q_OBJECT;

   public:
      ListWidget(QWidget* = 0) {}
      virtual void setup(const ListType&) = 0;
      };

//---------------------------------------------------------
//   ListEdit
//---------------------------------------------------------

class ListEdit : public TopWin {
      Q_OBJECT;

      ListType lt;

      QStackedWidget* stack;
      QTreeWidget* list;
      CtrlListEditor* ctrlPanel;

      void buildList();
      QTreeWidgetItem* findItem(const ListType& lt, QTreeWidgetItem* item);
      void selectItem(const ListType& lt);

   private slots:
      void itemChanged(QTreeWidgetItem*, QTreeWidgetItem*);
      void itemExpanded(QTreeWidgetItem*);

   public:
      ListEdit(QWidget* parent = 0);
      void selectItem(const AL::Pos&, Track*, Part*, Ctrl*);
      virtual void read(QDomNode);
	virtual void write(Xml& xml) const;
      };

#endif

