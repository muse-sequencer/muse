//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: listedit.h,v 1.3.2.3 2006/09/19 22:03:33 spamatica Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
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

#ifndef __LIST_EDIT_H__
#define __LIST_EDIT_H__

#include "type_defs.h"
#include "midieditor.h"
#include "noteinfo.h"
#include "cobject.h"

class QAction;
class QActionGroup;
class QCloseEvent;
class QKeyEvent;
class QTreeWidget;
class QTreeWidgetItem;


namespace MusECore {
class Event;
class MidiPart;
class MidiTrack;
class PartList;
class Xml;
}

namespace MusEGui {

//---------------------------------------------------------
//   ListEdit
//---------------------------------------------------------

class ListEdit : public QWidget {
      Q_OBJECT
    
      QTreeWidget* liste;
      MusECore::MidiTrack* curTrack;
      MusECore::MidiPart* curPart;
      int selectedTick;
      int curPartId;
      bool _isDeleting;
      std::set<int> _pidSet;
      MusECore::PartList* _pl;

      enum { CMD_DELETE, CMD_INC, CMD_DEC };

      
      virtual void closeEvent(QCloseEvent*) override;
//      virtual void keyPressEvent(QKeyEvent*);
//      virtual QSize sizeHint() const override;
      virtual QSize minimumSizeHint() const override;

      void initShortcuts();
      void genPartlist();

      QAction *noteAction, *sysexAction, *ctrlAction, *metaAction;

   private slots:
      void editInsertNote();
      void editInsertSysEx();
      void editInsertCtrl();
      void editInsertMeta();
      void editEvent(MusECore::Event&, MusECore::MidiPart*);
      void selectionChanged();
      void doubleClicked(QTreeWidgetItem*);
      void cmd(int cmd);
      void configChanged();

   public slots:
      void songChanged(MusECore::SongChangedStruct_t);
      void focusCanvas();

   public:
      ListEdit(MusECore::PartList*, QWidget* parent = 0);
      ~ListEdit();
      };

} // namespace MusEGui

#endif

