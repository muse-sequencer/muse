//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: mixdowndialog.cpp,v 1.1.1.1 2003/10/27 18:55:02 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
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

#include <QFileDialog>

#include "globals.h"
#include "mixdowndialog.h"
#include "wave.h"

namespace MusECore {

//---------------------------------------------------------
//   sndFileOpen
//    sf - old soundfile, used to preset file parameters
//---------------------------------------------------------

SndFile* getSndFile(const SndFile* sf, QWidget* parent)
      {
      MusEGui::MixdownFileDialog* dialog = new MusEGui::MixdownFileDialog(sf, parent);
      dialog->exec();
      MusECore::SndFile* sndFile = dialog->sndFile();
      delete dialog;
      return sndFile;
      }

} // namespace MusECore

namespace MusEGui {

//---------------------------------------------------------
//   MixdownFileDialog
//---------------------------------------------------------

MixdownFileDialog::MixdownFileDialog(const MusECore::SndFile* _sf,
   QWidget* parent, Qt::WFlags fl)
   : QDialog(parent, fl)
      {
      setupUi(this);
      sf   = 0;
      connect(buttonPath, SIGNAL(clicked()), SLOT(fdialog()));
      if (_sf) {
            int channels = _sf->channels();
            int format   = _sf->format();
            switch(channels) {
                  case 1:  channels = 1; break;
                  case 2:  channels = 0; break;
                  case 6:  channels = 2; break;
                  }
            editPath->setText(_sf->path());
            comboChannel->setCurrentIndex(channels);
            comboFormat->setCurrentIndex(format);
            }
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void MixdownFileDialog::accept()
      {
      QString oldpath;
      unsigned channel = comboChannel->currentIndex();
      unsigned format  = comboFormat->currentIndex();
      switch (channel) {
            case 0: channel = 2; break;
            case 1: channel = 1; break;
            case 2: channel = 6; break;     // not implemented!
            }
      switch (format) {
            case 0:     // 16 bit wave
                  format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
                  break;
            case 1:     // 24 bit wave
                  format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;
                  break;
            case 2:     // 32 bit float wave
                  format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
                  break;
            }
      QString path = editPath->text();
      if (path.isEmpty()) {
            sf = 0;
            reject();
            return;
            }
      if (path.right(4) != ".wav")
            path += ".wav";
      sf = new MusECore::SndFile(path);
      sf->setFormat(format, channel, MusEGlobal::sampleRate);
      done(1);
      }

//---------------------------------------------------------
//   fdialog
//---------------------------------------------------------

void MixdownFileDialog::fdialog()
      {
      QString oldpath;
      if (sf)
            oldpath = sf->path();
      QString path = QFileDialog::getSaveFileName(
         this, 0, oldpath, tr("Wave Files (*.wav);;All Files (*)"));
      if (!path.isEmpty())
            editPath->setText(path);
      }

} // namespace MusEGui
