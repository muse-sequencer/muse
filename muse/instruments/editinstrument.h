//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: editinstrument.h,v 1.1.1.1.2.4 2009/05/31 05:12:12 terminator356 Exp $
//
//  (C) Copyright 2003 Werner Schweer (ws@seh.de)
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

#ifndef __EDITINSTRUMENT_H__
#define __EDITINSTRUMENT_H__

#include "ui_editinstrumentbase.h"
#include "minstrument.h"
#include "midictrl.h"

class QDialog;
class QMenu;
class QCloseEvent;

namespace MusEGui {

//---------------------------------------------------------
//   EditInstrument
//---------------------------------------------------------

class EditInstrument : public QMainWindow, public Ui::EditInstrumentBase {
    Q_OBJECT

      MusECore::MidiInstrument workingInstrument;
      QListWidgetItem*  oldMidiInstrument;
      QTreeWidgetItem* oldPatchItem;
      void closeEvent(QCloseEvent*);
      int checkDirty(MusECore::MidiInstrument*, bool isClose = false);
      bool fileSave(MusECore::MidiInstrument*, const QString&);
      void saveAs();
      void updateInstrument(MusECore::MidiInstrument*);
      void updatePatch(MusECore::MidiInstrument*, MusECore::Patch*);
      void updatePatchGroup(MusECore::MidiInstrument*, MusECore::PatchGroup*);
      void changeInstrument();
      QTreeWidgetItem* addControllerToView(MusECore::MidiController* mctrl);
      QString getPatchItemText(int);
      void enableDefaultControls(bool, bool);
      void setDefaultPatchName(int);
      int getDefaultPatchNumber();
      void setDefaultPatchNumbers(int);
      void setDefaultPatchControls(int);
      QString getPatchName(int);
      void deleteInstrument(QListWidgetItem*);
      
   private slots:
      virtual void fileNew();
      virtual void fileOpen();
      virtual void fileSave();
      virtual void fileSaveAs();
      virtual void fileExit();
      virtual void helpWhatsThis();
      void instrumentChanged();
      void tabChanged(QWidget*);
      void patchChanged();
      void controllerChanged();
      void instrumentNameReturn();
      void patchNameReturn();
      void deletePatchClicked();
      void newPatchClicked();
      void newGroupClicked();
      void patchButtonClicked();
      void defPatchChanged(int);
      void deleteControllerClicked();
      void newControllerClicked();
      void addControllerClicked();
      void ctrlTypeChanged(int);
      void ctrlNameReturn();
      void ctrlHNumChanged(int);
      void ctrlLNumChanged(int);
      void ctrlMinChanged(int);
      void ctrlMaxChanged(int);
      void ctrlDefaultChanged(int);
      //void sysexChanged(); DELETETHIS?
      //void deleteSysexClicked();
      //void newSysexClicked();
      void ctrlNullParamHChanged(int);
      void ctrlNullParamLChanged(int);

   public:
      EditInstrument(QWidget* parent = 0, Qt::WFlags fl = Qt::Window);
      };

} // namespace MusEGui

#endif

