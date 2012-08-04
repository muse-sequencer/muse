//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: lmaster.h,v 1.1.1.1.2.5 2005/12/11 21:29:23 spamatica Exp $
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

#ifndef __LMASTER_EDIT_H__
#define __LMASTER_EDIT_H__

#include "midieditor.h"
#include "noteinfo.h"
#include "cobject.h"
#include "tempo.h"
#include "keyevent.h"
#include "al/sig.h"

#include <QTreeWidgetItem>

namespace Awl {
      class PosEdit;
      class SigEdit;
      };
using Awl::PosEdit;
using Awl::SigEdit;

class QLineEdit;
class QComboBox;

namespace MusEGui {

enum LMASTER_LVTYPE
   {
      LMASTER_TEMPO = 0,
      LMASTER_SIGEVENT,
      LMASTER_KEYEVENT
   };


//---------------------------------------------------------
//   LMasterLViewItem
//!  QListViewItem base class for LMasterTempoItem and LMasterSigEventItem
//---------------------------------------------------------
class LMasterLViewItem : public QTreeWidgetItem {
   protected:
      QString c1, c2, c3, c4;

   public:
      LMasterLViewItem(QTreeWidget* parent)
           : QTreeWidgetItem(QTreeWidgetItem::UserType) {parent->insertTopLevelItem(0, this);}
      virtual QString text(int column) const;
      virtual LMASTER_LVTYPE getType() = 0;
      virtual unsigned tick() = 0;
      };

//---------------------------------------------------------
//   LMasterTempoItem
//!  QListViewItem which holds data for a TEvent
//---------------------------------------------------------
class LMasterTempoItem : public LMasterLViewItem {

   private:
      const MusECore::TEvent* tempoEvent;

   public:
      LMasterTempoItem(QTreeWidget* parent, const MusECore::TEvent* t);
      virtual LMASTER_LVTYPE getType() { return LMASTER_TEMPO; }
      const MusECore::TEvent* getEvent() { return tempoEvent; }
      virtual unsigned tick() { return tempoEvent->tick; }
      int tempo() { return tempoEvent->tempo; }
      };

//---------------------------------------------------------
//   LMasterKeyItem
//!  QListViewItem which holds data for a KetEvent
//---------------------------------------------------------
class LMasterKeyEventItem : public LMasterLViewItem {

   private:
      MusECore::KeyEvent keyEvent;

   public:
      LMasterKeyEventItem(QTreeWidget* parent, const MusECore::KeyEvent& t);
      virtual LMASTER_LVTYPE getType() { return LMASTER_KEYEVENT; }
      const MusECore::KeyEvent& getEvent() { return keyEvent; }
      virtual unsigned tick() { return keyEvent.tick; }
      MusECore::key_enum key() { return keyEvent.key; }
      };
//---------------------------------------------------------
//   LMasterTempoItem
//!  QListViewItem which holds data for a SigEvent
//---------------------------------------------------------
class LMasterSigEventItem : public LMasterLViewItem {

   private:
      const AL::SigEvent* sigEvent;

   public:
      LMasterSigEventItem(QTreeWidget* parent, const AL::SigEvent* s);
      virtual LMASTER_LVTYPE getType() { return LMASTER_SIGEVENT; }
      const AL::SigEvent* getEvent() { return sigEvent; }
      virtual unsigned tick() { return sigEvent->tick; }
      int z() { return sigEvent->sig.z; }
      int n() { return sigEvent->sig.n; }
      };


//---------------------------------------------------------
//   LMaster
//---------------------------------------------------------

class LMaster : public MidiEditor {
      Q_OBJECT
    
      QTreeWidget* view;
      QToolBar* tools;
      QMenu* menuEdit;
      QTimer* comboboxTimer;

      enum { CMD_DELETE, CMD_INSERT_SIG, CMD_INSERT_TEMPO, CMD_EDIT_BEAT, CMD_EDIT_VALUE, CMD_INSERT_KEY };

      
      virtual void closeEvent(QCloseEvent*);
      void updateList();
      void insertTempo(const MusECore::TEvent*);
      void insertSig(const AL::SigEvent*);
      void insertKey(const MusECore::KeyEvent&);
      LMasterLViewItem* getItemAtPos(unsigned tick, LMASTER_LVTYPE t);
      void initShortcuts();
      QLineEdit* tempo_editor;
      PosEdit*   pos_editor;
      QComboBox*  key_editor;
      // State-like members:
      LMasterLViewItem* editedItem;
      SigEdit* sig_editor;
      int editorColumn;
      bool editingNewItem;

      QAction *tempoAction, *signAction, *posAction, *valAction, *delAction, *keyAction;

   private slots:
      void select(QTreeWidgetItem*, QTreeWidgetItem*);
      void itemDoubleClicked(QTreeWidgetItem* item);
      void returnPressed();
      void itemPressed(QTreeWidgetItem* i, int column);
      void tempoButtonClicked();
      void timeSigButtonClicked();
      void insertKey();
      void cmd(int cmd);
      void comboboxTimerSlot();

   public slots:
      void songChanged(int);
      void configChanged();
      void focusCanvas();

   signals:
      void isDeleting(MusEGui::TopWin*);
      void seekTo(int tick);

   public:
      LMaster();
      ~LMaster();
      virtual void readStatus(MusECore::Xml&);
      virtual void writeStatus(int, MusECore::Xml&) const;
      static void readConfiguration(MusECore::Xml&);
      static void writeConfiguration(int, MusECore::Xml&);
      LMasterLViewItem* getLastOfType(LMASTER_LVTYPE t);
      };

} // namespace MusEGui

#endif

