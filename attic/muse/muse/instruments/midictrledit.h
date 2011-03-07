//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midictrledit.h,v 1.1.1.1.2.1 2008/08/18 00:15:25 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
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
      void controllerChanged(QListViewItem*);
      void controllerChanged();
      void minChanged(int);
      void maxChanged(int);
      void portChanged(int);
      void songChanged(int);

   public:
      MidiControllerEditDialog(QWidget* parent = 0, const char* name = 0, bool modal = false, WFlags fl = 0);
      };

extern MidiControllerEditDialog* midiControllerEditDialog;
extern void configMidiController();
#endif

