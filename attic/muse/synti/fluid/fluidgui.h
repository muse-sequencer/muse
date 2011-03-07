//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: fluidgui.h,v 1.2 2004/02/12 17:32:29 wschweer Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __GUI_H__
#define __GUI_H

#include "fluidguibase.h"
#include "libsynti/gui.h"

//---------------------------------------------------------
//   FLUIDGui
//---------------------------------------------------------

class FLUIDGui : public FLUIDGuiBase, public MessGui {

      Q_OBJECT

   private slots:
      void soundFontFileDialog();
      void loadFont();

   public:
      FLUIDGui();
      };

#endif
