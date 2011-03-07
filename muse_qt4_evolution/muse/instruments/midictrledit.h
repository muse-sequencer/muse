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

#ifndef __MIDICTRLEDIT_H__
#define __MIDICTRLEDIT_H__

#include "ccontrolbase.h"
#include "midictrl.h"

//---------------------------------------------------------
//   MidiControllerEditDialog
//---------------------------------------------------------

class MidiControllerEditDialog : public MidiControllerEditDialogBase {
      Q_OBJECT

      void addControllerToView(MidiController* mctrl);
      void mergeReplace(bool replace);

   private slots:
      void ctrlAdd();
      void ctrlDelete();
      virtual void accept();
      virtual void reject();
      void nameChanged(const QString&);
      void typeChanged(const QString&);
      void valueHChanged(int);
      void valueLChanged(int);
      void controllerChanged(Q3ListViewItem*);
      void controllerChanged();
      void minChanged(int);
      void maxChanged(int);

   public:
      MidiControllerEditDialog(QWidget* parent = 0, const char* name = 0, bool modal = false, Qt::WFlags fl = 0);
      };

extern MidiControllerEditDialog* midiControllerEditDialog;
extern void configMidiController();
#endif

