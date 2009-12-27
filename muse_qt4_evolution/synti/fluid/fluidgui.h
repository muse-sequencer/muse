//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: fluidgui.h,v 1.3 2005/10/04 21:37:44 lunar_shuttle Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __GUI_H__
#define __GUI_H

#include "ui_fluidgui.h"
#include "libsynti/gui.h"

//---------------------------------------------------------
//   FLUIDGui
//---------------------------------------------------------

class FLUIDGui : public QDialog, public Ui::FLUIDGuiBase, public MessGui {

      Q_OBJECT

   private slots:
      void soundFontFileDialog();
      void loadFont();

   public:
      FLUIDGui();
      };

#endif
