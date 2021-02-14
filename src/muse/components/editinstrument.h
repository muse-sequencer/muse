//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: editinstrument.h,v 1.1.1.1.2.4 2009/05/31 05:12:12 terminator356 Exp $
//
//  (C) Copyright 2003 Werner Schweer (ws@seh.de)
//  (C) Copyright 2012, 2016 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#include <QString>
#include <QMainWindow>

#include "globaldefs.h"


// Forward declarations:
class QMenu;
class QCloseEvent;
class QKeyEvent;
class QGridLayout;
class QStringListModel;
class QString;
class QAction;
class QListWidgetItem;
class QTreeWidgetItem;
class QMainWindow;
class QScrollBar;

namespace MusECore {
class MidiInstrument;
class MidiController;
struct Patch;
struct PatchGroup;
struct SysEx;
}

namespace MusEGui {

class Header;
class DList;

//---------------------------------------------------------
//   EditInstrument
//---------------------------------------------------------

class EditInstrument : public QMainWindow, public Ui::EditInstrumentBase {
    Q_OBJECT

      MusECore::MidiInstrument* workingInstrument;
      QListWidgetItem*  oldMidiInstrument;
      QTreeWidgetItem* oldPatchItem;

      Header* dlist_header;
      DList* dlist;
      QScrollBar* dlist_vscroll;
      QGridLayout* dlist_grid;
      QStringListModel* patch_coll_model;

      void setHeaderToolTips();
      void setHeaderWhatsThis();
      virtual void closeEvent(QCloseEvent*);
      virtual void keyPressEvent(QKeyEvent*);

      int checkDirty(MusECore::MidiInstrument*, bool isClose = false);
      bool fileSave(MusECore::MidiInstrument*, const QString&);
      void saveAs();
      void updateInstrument(MusECore::MidiInstrument*);
      void updatePatch(MusECore::MidiInstrument*, MusECore::Patch*);
      void updatePatchGroup(MusECore::MidiInstrument*, MusECore::PatchGroup*);
      void updateSysex(MusECore::MidiInstrument*, MusECore::SysEx*);
      void changeInstrument();
      void populateInitEventList();
      QTreeWidgetItem* addControllerToView(MusECore::MidiController* mctrl);
      QString getPatchItemText(int);
      void enableDefaultControls(bool, bool);
      void enableNonCtrlControls(bool);
      void setDefaultPatchName(int);
      void setDefaultDrumPatchName(int);
      int getDefaultPatchNumber();
      int getDefaultDrumPatchNumber();
      int getDrummapCollectionPatchNumber();
      void setDefaultPatchNumbers(int);
      void setDefaultDrumPatchNumbers(int);
      void setDefaultPatchControls(int);
      void setDefaultDrumPatchControls(int);
      QString getPatchName(int prog, bool drum = false, bool includeDefault = true);
      QMenu* createPopupPatchList(bool drum = false);
      void popupDrummapPatchList();
      void popupControllerDefaultPatchList(bool drum = false);
      void deleteInstrument(QListWidgetItem*);

   private slots:
      virtual void fileNew();
      virtual void fileOpen();
      virtual void fileSave();
      virtual void fileSaveAs();
      virtual void fileClose();
      virtual void helpWhatsThis();
      void instrumentChanged();
      void noteOffModeChanged(int);
      void tabChanged(int);
      void patchChanged();
      void controllerChanged();
      void instrumentNameReturn();
      void patchNameReturn();
      void deletePatchClicked();
      void newPatchClicked();
      void newGroupClicked();
      void patchButtonClicked();
      void drumPatchButtonClicked();
      void drummapCollectionPatchButtonClicked();
      void defPatchChanged(int);
      void defDrumPatchChanged(int);
      void deleteControllerClicked();
      void newControllerClicked();
      void addControllerClicked();
      void ctrlPopupTriggered(QAction*);
      void ctrlTypeChanged(int);
      void ctrlNameReturn();
      void ctrlNumChanged();
      void ctrlMinChanged(int);
      void ctrlMaxChanged(int);
      void ctrlDefaultChanged(int);
      void ctrlDrumDefaultChanged(int);
      void ctrlShowInMidiChanged(int);
      void ctrlShowInDrumChanged(int);
      void sysexChanged(QListWidgetItem*, QListWidgetItem*);
      void deleteSysexClicked();
      void newSysexClicked();
      void editInitListItem(QTreeWidgetItem* item);
      void initListDeleteClicked();
      void initListAddClicked();
      void initListChangeClicked();
      
      void patchCollectionSpinboxChanged(int);
      void patchActivated(const QModelIndex&);
      void addPatchCollection();
      void delPatchCollection();
      void copyPatchCollection();
      void patchCollectionUp();
      void patchCollectionDown();
      void repopulatePatchCollections();
      void storePatchCollection();
      void fetchPatchCollection();

      void on_actionInfo_triggered();

public:
      EditInstrument(QWidget* parent = 0, Qt::WindowFlags fl = Qt::Window);
      virtual ~EditInstrument();
      void findInstrument(const QString& find_instrument);
      void showTab(EditInstrumentTabType);
      };

} // namespace MusEGui

#endif

