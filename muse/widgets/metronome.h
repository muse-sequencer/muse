//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: metronome.h,v 1.1.1.1.2.1 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __METRONOME_H__
#define __METRONOME_H__

#include "ui_metronomebase.h"

class QDialog;

//---------------------------------------------------------
//   MetronomeConfig
//---------------------------------------------------------

class MetronomeConfig : public QDialog, public Ui::MetronomeConfigBase {
      Q_OBJECT

   private slots:
      virtual void accept();
      void apply();
      virtual void reject();
      virtual void audioBeepRoutesClicked();
      void midiClickChanged(bool);
      void precountEnableChanged(bool);
      void precountFromMastertrackChanged(bool);
      void beepVolumeChanged(int);

   public:
      MetronomeConfig(QDialog* parent=0);
      };
#endif
