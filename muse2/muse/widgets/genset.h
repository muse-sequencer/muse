//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: genset.h,v 1.3 2004/01/25 09:55:17 wschweer Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __GENSET_H__
#define __GENSET_H__

#include "ui_gensetbase.h"
#include "cobject.h"
#include "mdisettings.h"

#include <QShowEvent>
#include <list>

//---------------------------------------------------------
//   GlobalSettingsConfig
//---------------------------------------------------------

class GlobalSettingsConfig : public QDialog, public Ui::GlobalSettingsDialogBase {
      Q_OBJECT

   private slots:
      void updateSettings();
      void updateMdiSettings();
      void addMdiSettings(TopWin::ToplevelType t);
      void applyMdiSettings();
      void apply();
      void ok();
      void cancel();
      void mixerCurrent();
      void mixer2Current();
      void bigtimeCurrent();
      void mainCurrent();
      void transportCurrent();
      void selectInstrumentsPath();
      void defaultInstrumentsPath();
      void traditionalPreset();
      void mdiPreset();
      void borlandPreset();

    protected:
      void showEvent(QShowEvent*);
      QButtonGroup *startSongGroup;
      std::list<MdiSettings*> mdisettings;
      
   public:
      GlobalSettingsConfig(QWidget* parent=0);
      };

#endif
