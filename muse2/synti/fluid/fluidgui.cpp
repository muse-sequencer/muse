//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: fluidgui.cpp,v 1.6.2.1 2009/08/12 20:47:01 spamatica Exp $
//
//    This is a simple GUI implemented with QT for
//    fluid software synthesizer.
//
//  (C) Copyright 2001-2004 Werner Schweer (ws@seh.de)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#include <unistd.h>
#include <stdlib.h>

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSocketNotifier>

#include "common_defs.h"
#include "fluidgui.h"
#include "muse/midi.h"
#include "muse/mpevent.h"
#include "muse/icons.h"

//---------------------------------------------------------
//   FLUIDGui
//---------------------------------------------------------

FLUIDGui::FLUIDGui()
   : QDialog(0, Qt::Window), MessGui()
      {
      setupUi(this);
      //Connect socketnotifier to fifo
      QSocketNotifier* s = new QSocketNotifier(readFd, QSocketNotifier::Read);
      connect(s, SIGNAL(activated(int)), SLOT(readMessage(int)));
      
      fdialogButton->setIcon(QIcon(*MusEGui::openIcon));
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
      QByteArray ba = pathEntry->text().toLatin1();
      const char* path = ba.constData();
      int len     = ba.length() + 1 + 3;
      unsigned char buffer[len];
      int k       = 0;
      buffer[k++] = MUSE_SYNTH_SYSEX_MFG_ID;
      buffer[k++] = FLUID_UNIQUE_ID;       // fluid
      buffer[k++] = SF_REPLACE;       // load sound font
      strcpy((char*)(&buffer[k]), path);
      sendSysex(buffer, len);
      }

//---------------------------------------------------------
//   soundFontFileDialog
//---------------------------------------------------------

void FLUIDGui::soundFontFileDialog()
      {
      //QString s = QFileDialog::getOpenFileName(this, QString(), QString(), QString("Soundfonts (*.[Ss][Ff]2);;All files (*)"));
      QString s = QFileDialog::getOpenFileName(this, QString(), QString(), QString("Soundfonts (*.sf2);;All files (*)"));
      if (!s.isEmpty()) {
            pathEntry->setText(s);
            }
      }

void FLUIDGui::processEvent(const MusECore::MidiPlayEvent& ev) 
{
      // p4.0.27 
      if (ev.type() == MusECore::ME_SYSEX)   {
            const unsigned char* data = ev.data();
            switch (*data) {
                  case FS_SEND_SOUNDFONT_NAME: {
                        //const char* filename = data+1;
                        //pathEntry->setText(QString(filename));
                        pathEntry->setText((const char*)(data+1));
                        break;
                        }
                  default:
                          #ifdef FS_DEBUG
                          printf("FLUIDGui::processEvent() : Unknown Sysex received: %d\n", ev.type());
                          #endif
                        break;
            }
      }
      else
      {
          #ifdef FS_DEBUG
          printf("FLUIDGui::processEvent - unknown event of type %dreceived from synth.\n", ev.type());
          #endif
      }    
}

//---------------------------------------------------------
//   readMessage
//---------------------------------------------------------

void FLUIDGui::readMessage(int)
      {
      MessGui::readMessage();   // p4.0.27
      }


