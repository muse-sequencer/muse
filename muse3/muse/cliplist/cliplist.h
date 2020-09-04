//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: cliplist.h,v 1.3.2.1 2005/12/11 21:29:23 spamatica Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#ifndef __CLIPLIST_H__
#define __CLIPLIST_H__

#include "type_defs.h"
#include "../cobject.h"
#include "event.h"

#include "ui_cliplisteditorbase.h"

class QCloseEvent;
class QDialog;
class QWidget;
class QTreeWidgetItem;

namespace MusECore {
class Xml;
class Pos;
}

namespace MusEGui {

//---------------------------------------------------------
//   ClipListEditorBaseWidget
//   Wrapper around Ui::ClipListEditorBase
//---------------------------------------------------------

class ClipListEditorBaseWidget : public QWidget, public Ui::ClipListEditorBase
{
      Q_OBJECT

   public:
      ClipListEditorBaseWidget(QWidget *parent = 0) : QWidget(parent) { setupUi(this); }
};

//---------------------------------------------------------
//   ClipListEdit
//---------------------------------------------------------

class ClipListEdit : public QWidget {
      Q_OBJECT
      ClipListEditorBaseWidget* editor;

      void updateList();

   private slots:
      void songChanged(MusECore::SongChangedStruct_t);
      void startChanged(const MusECore::Pos&);
      void lenChanged(const MusECore::Pos&);
      void clipSelectionChanged();
      void clicked(QTreeWidgetItem*, int);

   public:
      ClipListEdit(QWidget* parent);
      ~ClipListEdit();
      };

} // namespace MusEGui

#endif

