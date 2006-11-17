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

#ifndef __CTRLLISTEDIT_H__
#define __CTRLLISTEDIT_H__

#include "al/pos.h"
#include "listedit.h"
#include "ui_ctrllistedit.h"

//---------------------------------------------------------
//   MidiTimeDelegate
//---------------------------------------------------------

class MidiTimeDelegate : public QItemDelegate {
      Q_OBJECT

      virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&,
         const QModelIndex& index) const;
      virtual void setEditorData(QWidget* editor, const QModelIndex&) const;
      virtual void setModelData(QWidget* editor, QAbstractItemModel*,
         const QModelIndex&) const;
      void paint(QPainter*, const QStyleOptionViewItem&, 
         const QModelIndex&) const;

   public:
      MidiTimeDelegate(QObject* parent = 0);      
      };

//---------------------------------------------------------
//   EscapeFilter
//---------------------------------------------------------

class EscapeFilter : public QObject {
      Q_OBJECT
   protected:
      bool eventFilter(QObject*, QEvent*);
   public:
      EscapeFilter(QObject* parent = 0): QObject(parent) {}
      };

//---------------------------------------------------------
//   CtrlListEditor
//---------------------------------------------------------

class CtrlListEditor : public ListWidget {
      Q_OBJECT

      Ui::CtrlListEdit le;
      Track* track;
      Ctrl* c;
      bool updateListDisabled;
      ListEdit* listEdit;
      MidiTimeDelegate* midiTimeDelegate;

      void updateList();

   private slots:
      void controllerChanged(int id);
      void itemActivated(QTreeWidgetItem*,int);
      void itemChanged(QTreeWidgetItem*,int);
      void currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*);
      void insertClicked();
      void deleteClicked();
      void nameEdited(const QString&);
      void minValChanged(double);
      void maxValChanged(double);
      void defaultValChanged(double);

   public:
      CtrlListEditor(ListEdit*, QWidget* parent = 0);
      virtual void setup(const ListType&);
      void sendEscape();
      Ctrl* ctrl() const { return c; }
      Track* getTrack() const { return track; }
      enum { TICK_COL, TIME_COL, VAL_COL };
      };
      
#endif

