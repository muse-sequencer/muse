//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: fluidgui.cpp,v 1.6.2.1 2009/08/12 20:47:01 spamatica Exp $
//
//    This is a simple GUI implemented with QT for
//    fluid software synthesizer.
//
//  (C) Copyright 2001-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <unistd.h>
#include <stdlib.h>

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

#include "fluidgui.h"
#include "muse/midi.h"
#include "muse/icons.h"

//---------------------------------------------------------
//   FLUIDGui
//---------------------------------------------------------

FLUIDGui::FLUIDGui()
   : QDialog(0, Qt::Window), MessGui()
      {
      setupUi(this);
      fdialogButton->setIcon(QIcon(*openIcon));
      connect(fdialogButton, SIGNAL(clicked()), SLOT(soundFontFileDialog()));
      connect(loadButton, SIGNAL(clicked()), SLOT(loadFont()));

      // work around for probable QT/WM interaction bug.
      // for certain window managers, e.g xfce, this window is
      // is displayed although not specifically set to show();
      // bug: 2811156  	 Softsynth GUI unclosable with XFCE4 (and a few others)
      show();
      hide();
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
      const char* path = pathEntry->text().toLatin1();
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
      QString s = QFileDialog::getOpenFileName(QString::null, QString("*.[Ss][Ff]2"), this);
      if (!s.isEmpty()) {
            pathEntry->setText(s);
            }
      }

