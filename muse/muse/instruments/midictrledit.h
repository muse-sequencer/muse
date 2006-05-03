//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midictrledit.h,v 1.2 2005/09/22 20:12:59 wschweer Exp $
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

