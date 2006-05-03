//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midisyncimpl.h,v 1.3 2005/12/05 20:40:39 wschweer Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MIDISYNC_H__
#define __MIDISYNC_H__

#include "ui_midisync.h"

//---------------------------------------------------------
//   MidiSyncConfig
//---------------------------------------------------------

class MidiSyncConfig : public QDialog, private Ui::MidiSyncConfigBase {
      Q_OBJECT

   private slots:
      void syncMasterChanged(bool);
      void syncSlaveChanged(bool);
      void ok();
      void cancel();
      void apply();

   signals:
      void syncChanged();

   public:
      MidiSyncConfig(QWidget* parent=0);
      MidiSyncConfig();
      };

#endif

