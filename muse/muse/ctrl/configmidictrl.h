//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: configmidictrl.h,v 1.1 2005/10/24 20:38:56 wschweer Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __CONFIGMIDICTRL_H__
#define __CONFIGMIDICTRL_H__

#include "ui_configmidictrl.h"

class MidiTrack;

//---------------------------------------------------------
//   ConfigMidiCtrl
//---------------------------------------------------------

class ConfigMidiCtrl : public QDialog, public Ui::ConfigMidiCtrlBase {
      Q_OBJECT

      MidiTrack* track;

   private slots:
      void addClicked();
      void removeClicked();
      void availableSelected(QListWidgetItem*);
      void managedSelected(QListWidgetItem*);
      virtual void done(int);

   public:
      ConfigMidiCtrl(MidiTrack*);
      };

#endif


