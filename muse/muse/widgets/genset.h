//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: genset.h,v 1.3 2004/01/25 09:55:17 wschweer Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __GENSET_H__
#define __GENSET_H__

#include "gensetbase.h"

//---------------------------------------------------------
//   GlobalSettingsConfig
//---------------------------------------------------------

class GlobalSettingsConfig : public GlobalSettingsDialogBase {
      Q_OBJECT

   private slots:
      void apply();
      void ok();
      void cancel();
      void mixerCurrent();
      void bigtimeCurrent();
      void arrangerCurrent();
      void transportCurrent();

   public:
      GlobalSettingsConfig(QWidget* parent=0, const char* name=0);
      };

#endif
