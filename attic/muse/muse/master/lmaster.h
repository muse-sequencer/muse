//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: lmaster.h,v 1.1.1.1.2.5 2005/12/11 21:29:23 spamatica Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __LMASTER_EDIT_H__
#define __LMASTER_EDIT_H__

#include <qwidget.h>
#include <qlineedit.h>
#include "midieditor.h"
#include "noteinfo.h"
#include "cobject.h"
#include <qmainwindow.h>
#include <qlistview.h>
#include "tempo.h"
#include "sig.h"

class QToolButton;
class QListView;
class SigEvent;
class QLineEdit;
class QMouseEvent;
class PosEdit;
class SigEdit;

enum LMASTER_LVTYPE
   {
      LMASTER_TEMPO = 0,
      LMASTER_SIGEVENT
   };

//---------------------------------------------------------
//   LMasterLViewItem
//!  QListViewItem base class for LMasterTempoItem and LMasterSigEventItem
//---------------------------------------------------------
class LMasterLViewItem : public QListViewItem {
   protected:
      QString c1, c2, c3, c4;

   public:
      LMasterLViewItem(QListView* parent)
         : QListViewItem(parent) { }
      LMasterLViewItem(QListView* parent, QListViewItem* after)
         : QListViewItem(parent, after) { }
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
      const TEvent* tempoEvent;

   public:
      LMasterTempoItem(QListView* parent, const TEvent* t);
      virtual LMASTER_LVTYPE getType() { return LMASTER_TEMPO; }
      const TEvent* getEvent() { return tempoEvent; }
      virtual unsigned tick() { return tempoEvent->tick; }
      int tempo() { return tempoEvent->tempo; }
      };

//---------------------------------------------------------
//   LMasterTempoItem
//!  QListViewItem which holds data for a SigEvent
//---------------------------------------------------------
class LMasterSigEventItem : public LMasterLViewItem {

   private:
      const SigEvent* sigEvent;

   public:
      LMasterSigEventItem(QListView* parent, const SigEvent* s);
      virtual LMASTER_LVTYPE getType() { return LMASTER_SIGEVENT; }
      const SigEvent* getEvent() { return sigEvent; }
      virtual unsigned tick() { return sigEvent->tick; }
      int z() { return sigEvent->z; }
      int n() { return sigEvent->n; }
      };


//---------------------------------------------------------
//   LMaster
//---------------------------------------------------------

class LMaster : public MidiEditor {
      QListView* view;
      QToolBar* tools;
      QPopupMenu* menuEdit;

      enum { CMD_DELETE, CMD_INSERT_SIG, CMD_INSERT_TEMPO, CMD_EDIT_BEAT, CMD_EDIT_VALUE };

      Q_OBJECT
      virtual void closeEvent(QCloseEvent*);
      void updateList();
      void insertTempo(const TEvent*);
      void insertSig(const SigEvent*);
      LMasterLViewItem* getItemAtPos(unsigned tick, LMASTER_LVTYPE t);
      void initShortcuts();
      QLineEdit* editor;
      PosEdit*   pos_editor;
      // State-like members:
      LMasterLViewItem* editedItem;
      SigEdit* sig_editor;
      int editorColumn;
      bool editingNewItem;

   private slots:
      void select(QListViewItem*);
      void itemDoubleClicked(QListViewItem* item);
      void returnPressed();
      void itemPressed(QListViewItem* i, const QPoint& p, int column);
      void tempoButtonClicked();
      void timeSigButtonClicked();
      void cmd(int cmd);

   public slots:
      void songChanged(int);
      void configChanged();

   signals:
      void deleted(unsigned long);

   public:
      LMaster();
      ~LMaster();
      virtual void readStatus(Xml&);
      virtual void writeStatus(int, Xml&) const;
      LMasterLViewItem* getLastOfType(LMASTER_LVTYPE t);
      };


#endif

