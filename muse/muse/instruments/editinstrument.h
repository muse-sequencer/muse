//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __EDITINSTRUMENT_H__
#define __EDITINSTRUMENT_H__

#include "ui_editinstrument.h"

class MidiInstrument;

//---------------------------------------------------------
//   EditInstrument
//---------------------------------------------------------

class EditInstrument : public QMainWindow, public Ui::EditInstrumentBase {
    Q_OBJECT

      bool fileSave(MidiInstrument*, const QString& name);
      void closeEvent(QCloseEvent*);
      bool checkDirty(MidiInstrument*);

   private slots:
      virtual void fileNew();
      virtual void fileSave();
      virtual void fileSaveAs();
      void instrumentChanged(QListWidgetItem*, QListWidgetItem*);
      void patchChanged(QTreeWidgetItem*, QTreeWidgetItem*);
      void controllerChanged(QListWidgetItem* sel);
      void sysexChanged(QListWidgetItem* sel);
      void instrumentNameChanged(const QString&);
      void deletePatchClicked();
      void newPatchClicked();
      void newGroupClicked();
      void newCategoryClicked();
      void deleteControllerClicked();
      void newControllerClicked();
      void deleteSysexClicked();
      void newSysexClicked();

   public:
      EditInstrument(QWidget* parent = 0);
      };

#endif

