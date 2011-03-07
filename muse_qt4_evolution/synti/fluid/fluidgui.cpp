//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: fluidgui.cpp,v 1.10 2005/11/23 13:55:32 wschweer Exp $
//
//    This is a simple GUI implemented with QT for
//    fluid software synthesizer.
//
//  (C) Copyright 2001-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include "fluidgui.h"
#include "muse/midi.h"

//---------------------------------------------------------
//   FLUIDGui
//---------------------------------------------------------

FLUIDGui::FLUIDGui()
   : QDialog(0), MessGui()
      {
      setupUi(this);
      connect(fdialogButton, SIGNAL(clicked()), SLOT(soundFontFileDialog()));
      connect(loadButton, SIGNAL(clicked()), SLOT(loadFont()));
      }

//---------------------------------------------------------
//   loadFont
//    sysex f0 lenH lenM lenL 7c 00 01 name
//---------------------------------------------------------

void FLUIDGui::loadFont()
      {
      if (pathEntry->text().isEmpty())
            return;
      QFileInfo fi(pathEntry->text());
      if (!fi.exists()) {
            QString s = QString("SoundFont ") + pathEntry->text() + QString(" does not exists");
            QMessageBox::critical(this, tr("FLUID: open Soundfile"), s);
            return;
            }
      const char* path = pathEntry->text().toLatin1().data();
      int len     = strlen(path) + 1 + 3;
      unsigned char buffer[len];
      int k       = 0;
      buffer[k++] = 0x7c;
      buffer[k++] = 0x00;       // fluid
      buffer[k++] = 0x01;       // load sound font
      strcpy((char*)(&buffer[k]), path);
      sendSysex(buffer, len);
      }

//---------------------------------------------------------
//   soundFontFileDialog
//---------------------------------------------------------

void FLUIDGui::soundFontFileDialog()
      {
      QString s = QFileDialog::getOpenFileName(
         this,
         tr("Fluid: select Sound Font"),
         ".",
         QString("*.[Ss][Ff]2"));
      if (!s.isEmpty()) {
            pathEntry->setText(s);
            }
      }

