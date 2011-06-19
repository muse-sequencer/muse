//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: fluidgui.h,v 1.2 2004/02/12 17:32:29 wschweer Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __GUI_H__
#define __GUI_H

#include "ui_fluidguibase.h"
#include "libsynti/gui.h"

#define FS_SEND_SOUNDFONT_NAME 1

//#define FS_DEBUG  

class QDialog;

//---------------------------------------------------------
//   FLUIDGui
//---------------------------------------------------------

class FLUIDGui : public QDialog, public Ui::FLUIDGuiBase, public MessGui {

      Q_OBJECT

   private slots:
      void readMessage(int);
      void soundFontFileDialog();
      void loadFont();

   public:
      FLUIDGui();
      virtual void processEvent(const MidiPlayEvent&);
      };

#endif
