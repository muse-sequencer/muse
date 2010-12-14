//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: editevent.h,v 1.6.2.1 2008/05/21 00:28:53 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __EDIT_EVENT_H__
#define __EDIT_EVENT_H__

#include "ui_editnotedialogbase.h"
#include "ui_editsysexdialogbase.h"
#include "ui_editctrlbase.h"
#include "event.h"

namespace Awl {
      class PosEdit;
      };

///class PosEdit;
class IntLabel;
class PitchEdit;
class QDialog;
class QLabel;
class QGridLayout;
class QTextEdit;
class QRadioButton;
class MidiPart;
class QListWidgetItem;
class QMenu;
//---------------------------------------------------------
//   EditEventDialog
//---------------------------------------------------------

class EditEventDialog : public QDialog {
      Q_OBJECT

   protected:
      QGridLayout* layout1;

   public:
      EditEventDialog(QWidget* parent=0);
      virtual Event event() = 0;
      };

//---------------------------------------------------------
//   EditNoteDialog
//---------------------------------------------------------

class EditNoteDialog : public QDialog, public Ui::EditNoteDialogBase {
      Q_OBJECT

   public:
      EditNoteDialog(int tick, const Event&,
         QWidget* parent=0);
      static Event getEvent(int tick, const Event&,
         QWidget* parent = 0);
      virtual Event event();
      };

//---------------------------------------------------------
//   EditSysExDialog
//---------------------------------------------------------

class EditSysexDialog : public QDialog, public Ui::EditSysexDialogBase {
      Q_OBJECT

      unsigned char* sysex;
      int len;

   protected:
      QGridLayout* layout;

   private slots:
      virtual void accept();

   public:
      EditSysexDialog(int tick, const Event&,
         QWidget* parent=0);
      ~EditSysexDialog();
      static Event getEvent(int tick, const Event&,
         QWidget* parent = 0);
      virtual Event event();
      };

//---------------------------------------------------------
//   EditCtrlDialog
//---------------------------------------------------------

class EditCtrlDialog : public QDialog, public Ui::EditCtrlBase  {
      Q_OBJECT

      int num;          // controller number
      int val;          // controller value (for prog. changes)

      const MidiPart* part;
      ///QMenu* pop;

      void updatePatch();

   private slots:
      void ctrlListClicked(QListWidgetItem*);
      void newController();
      void programChanged();
      void instrPopup();

   protected:
      QGridLayout* layout;


   public:
      EditCtrlDialog(int tick, const Event&,
         const MidiPart*, QWidget* parent=0);
      static Event getEvent(int tick, const Event&, const MidiPart*,
         QWidget* parent = 0);
      virtual Event event();
      };

//---------------------------------------------------------
//   EditMetaDialog
//---------------------------------------------------------

class EditMetaDialog : public EditEventDialog {
      Q_OBJECT

      unsigned char* meta;
      int len;
      ///PosEdit* epos;
      Awl::PosEdit* epos;
      QTextEdit* edit;
      IntLabel* il2;
      QRadioButton* hexButton;
      QLabel* typeLabel;

   protected:
      QGridLayout* layout;

   private slots:
      virtual void accept();
      void toggled(bool);
      void typeChanged(int);

   public:
      EditMetaDialog(int tick, const Event&,
         QWidget* parent=0);
      ~EditMetaDialog();
      static Event getEvent(int tick, const Event&,
         QWidget* parent = 0);
      virtual Event event();
      };

//---------------------------------------------------------
//   EditCAfterDialog
//---------------------------------------------------------

class EditCAfterDialog : public EditEventDialog {
      Q_OBJECT

      ///PosEdit* epos;
      Awl::PosEdit* epos;
      IntLabel* il2;

   protected:
      QGridLayout* layout;

   public:
      EditCAfterDialog(int tick, const Event&,
         QWidget* parent=0);
      static Event getEvent(int tick, const Event&,
         QWidget* parent = 0);
      virtual Event event();
      };

//---------------------------------------------------------
//   EditPAfterDialog
//---------------------------------------------------------

class EditPAfterDialog : public EditEventDialog {
      Q_OBJECT

      ///PosEdit* epos;
      Awl::PosEdit* epos;
      PitchEdit* pl;
      IntLabel* il2;

   protected:
      QGridLayout* layout;

   public:
      EditPAfterDialog(int tick, const Event&,
         QWidget* parent=0);
      static Event getEvent(int tick, const Event&,
         QWidget* parent = 0);
      virtual Event event();
      };

#endif

