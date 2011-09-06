//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midictrledit.h,v 1.1.1.1.2.1 2008/08/18 00:15:25 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
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

#ifndef __MIDICTRLEDIT_H__
#define __MIDICTRLEDIT_H__

#include "ccontrolbase.h"
#include "midictrl.h"

//---------------------------------------------------------
//   MidiControllerEditDialog
//---------------------------------------------------------

class MidiControllerEditDialog : public MidiControllerEditDialogBase {
      Q_OBJECT

      int _lastPort;
      bool _modified;
      
      void addControllerToView(MidiController* mctrl);
      void mergeReplace(bool replace);
      void updatePredefinedList();
      void updateMidiPortsList();
      void updateViewController();
      void setModified(bool);

   private slots:
      void ctrlAdd();
      void ctrlDelete();
      virtual void accept();
      virtual void reject();
      void apply();
      void nameChanged(const QString&);
      void typeChanged(const QString&);
      void valueHChanged(int);
      void valueLChanged(int);
      void controllerChanged(Q3ListViewItem*);
      void controllerChanged();
      void minChanged(int);
      void maxChanged(int);
      void portChanged(int);
      void songChanged(int);

   public:
      MidiControllerEditDialog(QWidget* parent = 0, const char* name = 0, bool modal = false, Qt::WFlags fl = 0);
      };

extern MidiControllerEditDialog* midiControllerEditDialog;
extern void configMidiController();
#endif

