//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "al/al.h"
#include "mixdowndialog.h"
#include "globals.h"
#include "wave.h"
#include "song.h"
#include "gconfig.h"

//---------------------------------------------------------
//   sndFileOpen
//    sf - old soundfile, used to preset file parameters
//---------------------------------------------------------

SndFile* getSndFile(SndFile* sf, QWidget* parent)
      {
      MixdownFileDialog* dialog = new MixdownFileDialog(sf, parent);
      dialog->exec();
      SndFile* sndFile = dialog->sndFile();
      delete dialog;
      return sndFile;
      }

//---------------------------------------------------------
//   MixdownFileDialog
//---------------------------------------------------------

MixdownFileDialog::MixdownFileDialog(SndFile* _sf, QWidget* parent)
   : QDialog(parent)
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
            editPath->setText(_sf->finfo()->filePath());
            comboChannel->setCurrentIndex(channels);
            comboFormat->setCurrentIndex(format);
            }
      else {
            // create unique mixdown file path
            QString path = QDir::homePath() + "/" + config.projectPath + "/" + song->projectPath();

            QDir dir(path);
            for (int i = 1; i < 1000; ++i) {
                  QString fp = QString("md%2.wav").arg(i);
                  if (!dir.exists(fp)) {
                        path = dir.filePath(fp);
                        break;
                        }
                  }
            editPath->setText(path);
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
      sf = new SndFile(path);
      sf->setFormat(format, channel, AL::sampleRate);
      done(1);
      }

//---------------------------------------------------------
//   fdialog
//---------------------------------------------------------

void MixdownFileDialog::fdialog()
      {
      QString path = QFileDialog::getSaveFileName(
         this,
         tr("MusE: set mixdown file name"),
         editPath->text(),
         tr("Wave Files (*.wav);;All Files (*)")
         );
      if (!path.isEmpty())
            editPath->setText(path);
      }

