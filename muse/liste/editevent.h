//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: editevent.h,v 1.6.2.1 2008/05/21 00:28:53 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __EDIT_EVENT_H__
#define __EDIT_EVENT_H__

#include <qdialog.h>
//Added by qt3to4:
#include <Q3PopupMenu>
#include <QGridLayout>
#include <QLabel>

#include "editnotedialogbase.h"
#include "editsysexdialogbase.h"
#include "editctrlbase.h"
#include "event.h"

class PosEdit;
class IntLabel;
class PitchEdit;
class QGridLayout;
class Q3MultiLineEdit;
class QRadioButton;
class PosEdit;
class MidiPart;
class Q3ListBoxItem;
class Q3PopupMenu;
//---------------------------------------------------------
//   EditEventDialog
//---------------------------------------------------------

class EditEventDialog : public QDialog {
      Q_OBJECT

   protected:
      QGridLayout* layout1;

   public:
      EditEventDialog(QWidget* parent=0, const char* name=0);
      virtual Event event() = 0;
      };

//---------------------------------------------------------
//   EditNoteDialog
//---------------------------------------------------------

class EditNoteDialog : public EditNoteDialogBase {
      Q_OBJECT

   public:
      EditNoteDialog(int tick, const Event&,
         QWidget* parent=0, const char* name=0);
      static Event getEvent(int tick, const Event&,
         QWidget* parent = 0);
      virtual Event event();
      };

//---------------------------------------------------------
//   EditSysExDialog
//---------------------------------------------------------

class EditSysexDialog : public EditSysexDialogBase {
      Q_OBJECT

      unsigned char* sysex;
      int len;

   protected:
      QGridLayout* layout;

   private slots:
      virtual void accept();

   public:
      EditSysexDialog(int tick, const Event&,
         QWidget* parent=0, const char* name=0);
      ~EditSysexDialog();
      static Event getEvent(int tick, const Event&,
         QWidget* parent = 0);
      virtual Event event();
      };

//---------------------------------------------------------
//   EditCtrlDialog
//---------------------------------------------------------

class EditCtrlDialog : public EditCtrlBase  {
      Q_OBJECT

      int num;          // controller number
      int val;          // controller value (for prog. changes)

      const MidiPart* part;
      Q3PopupMenu* pop;

      void updatePatch();

   private slots:
      void ctrlListClicked(Q3ListBoxItem*);
      void newController();
      void programChanged();
      void instrPopup();

   protected:
      QGridLayout* layout;


   public:
      EditCtrlDialog(int tick, const Event&,
         const MidiPart*, QWidget* parent=0, const char* name=0);
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
      PosEdit* epos;
      Q3MultiLineEdit* edit;
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
         QWidget* parent=0, const char* name=0);
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

      PosEdit* epos;
      IntLabel* il2;

   protected:
      QGridLayout* layout;

   public:
      EditCAfterDialog(int tick, const Event&,
         QWidget* parent=0, const char* name=0);
      static Event getEvent(int tick, const Event&,
         QWidget* parent = 0);
      virtual Event event();
      };

//---------------------------------------------------------
//   EditPAfterDialog
//---------------------------------------------------------

class EditPAfterDialog : public EditEventDialog {
      Q_OBJECT

      PosEdit* epos;
      PitchEdit* pl;
      IntLabel* il2;

   protected:
      QGridLayout* layout;

   public:
      EditPAfterDialog(int tick, const Event&,
         QWidget* parent=0, const char* name=0);
      static Event getEvent(int tick, const Event&,
         QWidget* parent = 0);
      virtual Event event();
      };

#endif

