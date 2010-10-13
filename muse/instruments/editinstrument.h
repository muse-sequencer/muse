//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: editinstrument.h,v 1.1.1.1.2.4 2009/05/31 05:12:12 terminator356 Exp $
//
//  (C) Copyright 2003 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __EDITINSTRUMENT_H__
#define __EDITINSTRUMENT_H__

#include "editinstrumentbase.h"
#include "minstrument.h"
#include "midictrl.h"
//Added by qt3to4:
#include <Q3PopupMenu>
#include <QCloseEvent>
class ListBoxData;
class ListViewData;

//---------------------------------------------------------
//   EditInstrument
//---------------------------------------------------------

class EditInstrument : public EditInstrumentBase {
    Q_OBJECT

      MidiInstrument workingInstrument;
      ListBoxData*  oldMidiInstrument;
      ListViewData* oldPatchItem;
      void closeEvent(QCloseEvent*);
      int checkDirty(MidiInstrument*, bool isClose = false);
      bool fileSave(MidiInstrument*, const QString&);
      void saveAs();
      void updateInstrument(MidiInstrument*);
      void updatePatch(MidiInstrument*, Patch*);
      void updatePatchGroup(MidiInstrument*, PatchGroup*);
      void changeInstrument();
      ListViewData* addControllerToView(MidiController* mctrl);
      QString getPatchItemText(int);
      void enableDefaultControls(bool, bool);
      void setDefaultPatchName(int);
      int getDefaultPatchNumber();
      void setDefaultPatchNumbers(int);
      void setDefaultPatchControls(int);
      const char* getPatchName(int);
      void deleteInstrument(ListBoxData*);
      Q3PopupMenu* patchpopup;
      
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
      //void instrumentNameChanged(const QString&);
      void instrumentNameReturn();
      void patchNameReturn();
      void deletePatchClicked();
      void newPatchClicked();
      void newGroupClicked();
      void patchButtonClicked();
      void defPatchChanged(int);
      //void newCategoryClicked();
      void deleteControllerClicked();
      void newControllerClicked();
      void addControllerClicked();
      void ctrlTypeChanged(int);
      //void ctrlNameChanged(const QString&);
      void ctrlNameReturn();
      void ctrlHNumChanged(int);
      void ctrlLNumChanged(int);
      void ctrlMinChanged(int);
      void ctrlMaxChanged(int);
      void ctrlDefaultChanged(int);
      //void sysexChanged();
      //void deleteSysexClicked();
      //void newSysexClicked();
      void ctrlNullParamHChanged(int);
      void ctrlNullParamLChanged(int);

   public:
      EditInstrument(QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = Qt::WType_TopLevel);
      };

#endif

