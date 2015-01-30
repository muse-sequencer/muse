//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: editevent.h,v 1.6.2.1 2008/05/21 00:28:53 terminator356 Exp $
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

#ifndef __EDIT_EVENT_H__
#define __EDIT_EVENT_H__

#include "ui_editnotedialogbase.h"
#include "ui_editsysexdialogbase.h"
#include "ui_editctrlbase.h"

class QDialog;
class QLabel;
class QGridLayout;
class QTextEdit;
class QRadioButton;
class QListWidgetItem;
class QMenu;

namespace Awl {
      class PosEdit;
      };

namespace MusECore {
class Event;
class MidiPart;
class MidiInstrument;
}

namespace MusEGui {

class IntLabel;
class PitchEdit;

//---------------------------------------------------------
//   EditEventDialog
//---------------------------------------------------------

class EditEventDialog : public QDialog {
      Q_OBJECT

   protected:
      QGridLayout* layout1;

   public:
      EditEventDialog(QWidget* parent=0);
      virtual MusECore::Event getEvent() = 0;
      };

//---------------------------------------------------------
//   EditNoteDialog
//---------------------------------------------------------

class EditNoteDialog : public QDialog, public Ui::EditNoteDialogBase {
      Q_OBJECT

   public:
      EditNoteDialog(int tick, const MusECore::Event&,
         QWidget* parent=0);
      static MusECore::Event getEvent(int tick, const MusECore::Event&,
         QWidget* parent = 0);
      virtual MusECore::Event getEvent();
      };

//---------------------------------------------------------
//   EditSysExDialog
//---------------------------------------------------------

class EditSysexDialog : public QDialog, public Ui::EditSysexDialogBase {
      Q_OBJECT

      MusECore::MidiInstrument* _instr;
      unsigned char* sysex;
      int len;

   protected:
      QGridLayout* layout;

   private slots:
      virtual void accept();
      virtual void editChanged();
      virtual void selectSysex();

   public:
      EditSysexDialog(int tick, const MusECore::Event&,
         QWidget* parent=0, MusECore::MidiInstrument* instr = 0);
      ~EditSysexDialog();
      static MusECore::Event getEvent(int tick, const MusECore::Event&,
         QWidget* parent = 0, MusECore::MidiInstrument* instr = 0);
      virtual MusECore::Event getEvent();
      };

//---------------------------------------------------------
//   EditCtrlDialog
//---------------------------------------------------------

class EditCtrlDialog : public QDialog, public Ui::EditCtrlBase  {
      Q_OBJECT

      const MusECore::MidiPart* part;
      void updatePatch(int val);

   private slots:
      void ctrlListClicked(QListWidgetItem*);
      void newController();
      void programChanged();
      void instrPopup();

   protected:
      QGridLayout* layout;

   public:
      EditCtrlDialog(int tick, const MusECore::Event&,
         const MusECore::MidiPart*, QWidget* parent=0);
      static MusECore::Event getEvent(int tick, const MusECore::Event&, const MusECore::MidiPart*,
         QWidget* parent = 0);
      virtual MusECore::Event getEvent();
      };

//---------------------------------------------------------
//   EditMetaDialog
//---------------------------------------------------------

class EditMetaDialog : public EditEventDialog {
      Q_OBJECT

      unsigned char* meta;
      int len;
      Awl::PosEdit* epos;
      QTextEdit* edit;
      MusEGui::IntLabel* il2;
      QRadioButton* hexButton;
      QLabel* typeLabel;

   protected:
      QGridLayout* layout;

   private slots:
      virtual void accept();
      void toggled(bool);
      void typeChanged(int);

   public:
      EditMetaDialog(int tick, const MusECore::Event&,
         QWidget* parent=0);
      ~EditMetaDialog();
      static MusECore::Event getEvent(int tick, const MusECore::Event&,
         QWidget* parent = 0);
      virtual MusECore::Event getEvent();
      };

} // namespace MusEGui

#endif

