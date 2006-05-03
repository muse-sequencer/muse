//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: editevent.h,v 1.16 2005/11/04 12:03:47 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __EDIT_EVENT_H__
#define __EDIT_EVENT_H__

#include "ui_editnotedialog.h"
// #include "widgets/editsysexdialogbase.h"
#include "event.h"

namespace Awl {
      class PosEdit;
      class PitchLabel;
      };
using Awl::PosEdit;
using Awl::PitchLabel;

// class IntLabel;
class MidiPart;
class MidiInstrument;

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

// class EditNoteDialog : public EditNoteDialogBase {
class EditNoteDialog : public QDialog {
      Q_OBJECT

   public:
      EditNoteDialog(int tick, const Event&, QWidget* parent=0);
      static Event getEvent(int tick, const Event&,
         QWidget* parent = 0);
      virtual Event event();
      };

//---------------------------------------------------------
//   EditSysExDialog
//---------------------------------------------------------

// class EditSysexDialog : public EditSysexDialogBase {
class EditSysexDialog : public QDialog {
      Q_OBJECT

      unsigned char* sysex;
      MidiInstrument* mi;

      int len;

   protected:
      QGridLayout* layout;

   private slots:
      virtual void accept();
      void addPressed();
      void listSelectionChanged();

   public:
      EditSysexDialog(int tick, MidiInstrument*, const Event&, QWidget* parent=0);
      ~EditSysexDialog();
      static Event getEvent(int tick, MidiInstrument*, const Event&, QWidget* parent = 0);
      virtual Event event();
      };

//---------------------------------------------------------
//   EditMetaDialog
//---------------------------------------------------------

class EditMetaDialog : public EditEventDialog {
      Q_OBJECT

      unsigned char* meta;
      int len;
      PosEdit* pos;
      QTextEdit* edit;
//      IntLabel* il2;
      QRadioButton* hexButton;

   protected:
      QGridLayout* layout;

   private slots:
      virtual void accept();
      void toggled(bool);

   public:
      EditMetaDialog(int tick, const Event&, QWidget* parent=0);
      ~EditMetaDialog();
      static Event getEvent(int tick, const Event&, QWidget* parent = 0);
      virtual Event event();
      };

//---------------------------------------------------------
//   EditCAfterDialog
//---------------------------------------------------------

class EditCAfterDialog : public EditEventDialog {
      Q_OBJECT

      PosEdit* pos;
//      IntLabel* il2;

   protected:
      QGridLayout* layout;

   public:
      EditCAfterDialog(int tick, const Event&, QWidget* parent=0);
      static Event getEvent(int tick, const Event&, QWidget* parent = 0);
      virtual Event event();
      };

//---------------------------------------------------------
//   EditPAfterDialog
//---------------------------------------------------------

class EditPAfterDialog : public EditEventDialog {
      Q_OBJECT

      PosEdit* pos;
      PitchLabel* pl;
//      IntLabel* il2;

   protected:
      QGridLayout* layout;

   public:
      EditPAfterDialog(int tick, const Event&, QWidget* parent=0);
      static Event getEvent(int tick, const Event&, QWidget* parent = 0);
      virtual Event event();
      };

#endif

