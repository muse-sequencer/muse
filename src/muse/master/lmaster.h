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

#include "type_defs.h"
#include "keyevent.h"

#include <QWidget>
#include <QTreeWidgetItem>


// Forward declarations:
class QTreeWidget;
class QLineEdit;
class QComboBox;
class QToolBar;
class QMenu;
class QTimer;
class QKeyEvent;
class QCloseEvent;
class QAction;

namespace MusECore {
//class TopWin;
struct SigEvent;
struct TEvent;
class Xml;
};

namespace MusEGui {

class PosEdit;
class SigEdit;

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
      LMasterLViewItem(QTreeWidget* parent);
      virtual QString text(int column) const;
      virtual LMASTER_LVTYPE getType() const = 0;
      virtual unsigned tick() const = 0;
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
      virtual LMASTER_LVTYPE getType() const;
      const MusECore::TEvent* getEvent() const;
      virtual unsigned tick() const;
      int tempo() const;
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
      virtual LMASTER_LVTYPE getType() const;
      const MusECore::KeyEvent& getEvent() const;
      virtual unsigned tick() const;
      MusECore::key_enum key() const;
      bool isMinor() const;
     };
//---------------------------------------------------------
//   LMasterTempoItem
//!  QListViewItem which holds data for a SigEvent
//---------------------------------------------------------
class LMasterSigEventItem : public LMasterLViewItem {

   private:
      const MusECore::SigEvent* sigEvent;

   public:
      LMasterSigEventItem(QTreeWidget* parent, const MusECore::SigEvent* s);
      virtual LMASTER_LVTYPE getType() const;
      const MusECore::SigEvent* getEvent() const;
      virtual unsigned tick() const;
      int z() const;
      int n() const;
      };


//---------------------------------------------------------
//   LMaster
//---------------------------------------------------------

class LMaster : public QWidget {
      Q_OBJECT
    
      QTreeWidget* view;
      QToolBar* tools;
      QMenu* menuEdit;
      QTimer* comboboxTimer;
      bool _isDeleting;

      enum { CMD_DELETE, CMD_INSERT_SIG, CMD_INSERT_TEMPO, CMD_EDIT_BEAT, CMD_EDIT_VALUE, CMD_INSERT_KEY };

      
      void updateList();
      void insertTempo(const MusECore::TEvent*);
      void insertSig(const MusECore::SigEvent*);
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

      virtual void closeEvent(QCloseEvent*);
      bool eventFilter(QObject *, QEvent *event) override;
      virtual QSize sizeHint() const;

      QAction *tempoAction, *signAction, *posAction, *valAction, *delAction, *keyAction;

   private slots:
      void itemDoubleClicked(QTreeWidgetItem* item);
      void editingFinished();
      void itemPressed(QTreeWidgetItem* i, int column);
      void tempoButtonClicked();
      void timeSigButtonClicked();
      void insertKey();
      void cmd(int cmd);
      void comboboxTimerSlot();

   public slots:
      void songChanged(MusECore::SongChangedStruct_t);
      void configChanged();
      void focusCanvas();

   signals:
//      void isDeleting(MusEGui::TopWin*);
      void seekTo(int tick);

   public:
      LMaster(QWidget* parent = 0);
      ~LMaster();

      LMasterLViewItem* getLastOfType(LMASTER_LVTYPE t);
      };

} // namespace MusEGui

#endif
