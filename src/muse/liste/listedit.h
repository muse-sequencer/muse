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

#include <QUuid>
#ifdef MOVE_LISTEDIT_FROM_DOCK_TO_WINDOW_PULL1099
class QMenu;
#endif
#include <QMetaObject>


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

#ifdef MOVE_LISTEDIT_FROM_DOCK_TO_WINDOW_PULL1099
class ListEdit : public MidiEditor {
#else
class ListEdit : public QWidget {
#endif
      Q_OBJECT
    
      QTreeWidget* liste;
      MusECore::MidiTrack* curTrack;
      MusECore::MidiPart* curPart;
      int selectedTick;
      QUuid curPartId;
      bool _isDeleting;
      std::set<QUuid> _pidSet;
      MusECore::PartList* _pl;

      QMetaObject::Connection _configChangedConnection;

      enum { CMD_DELETE, CMD_INC, CMD_DEC };

      
      void closeEvent(QCloseEvent*) override;
      QSize minimumSizeHint() const override;
      bool eventFilter(QObject *, QEvent *event) override;

      void initShortcuts();
      void genPartlist();

#ifdef MOVE_LISTEDIT_FROM_DOCK_TO_WINDOW_PULL1099
      QMenu *menuConfig;
#endif

      QAction *noteAction, *sysexAction, *ctrlAction, *metaAction;
      QAction *incAction, *decAction, *deleteAction;

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
#ifdef MOVE_LISTEDIT_FROM_DOCK_TO_WINDOW_PULL1099
      void focusCanvas() override;
#else
      void focusCanvas();
#endif

   public:
#ifdef MOVE_LISTEDIT_FROM_DOCK_TO_WINDOW_PULL1099
      ListEdit(MusECore::PartList*, QWidget* parent = 0, const char* name = 0);
#else
      ListEdit(MusECore::PartList*, QWidget* parent = 0);
#endif
      virtual ~ListEdit();
      MusECore::PartList* parts()            { return _pl;  }
      const MusECore::PartList* parts() const { return _pl;  }
      };

} // namespace MusEGui

#endif

