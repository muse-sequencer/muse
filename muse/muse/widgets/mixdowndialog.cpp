//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: mixdowndialog.cpp,v 1.8 2006/01/21 13:42:05 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#include "al/al.h"
#include "mixdowndialog.h"
#include "globals.h"
#include "wave.h"

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
      QString oldpath;
      if (sf)
            oldpath = sf->finfo()->filePath();
      QString path = QFileDialog::getSaveFileName(
         this,
         tr("MusE: set mixdown file name"),
         oldpath,
         tr("Wave Files (*.wav);;All Files (*)")
         );
      if (!path.isEmpty())
            editPath->setText(path);
      }

