//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: mixdowndialog.cpp,v 1.1.1.1 2003/10/27 18:55:02 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#include "mixdowndialog.h"
#include "globals.h"
#include <qtoolbutton.h>
#include <qpushbutton.h>
#include <q3filedialog.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include "wave.h"

//---------------------------------------------------------
//   sndFileOpen
//    sf - old soundfile, used to preset file parameters
//---------------------------------------------------------

SndFile* getSndFile(const SndFile* sf, QWidget* parent, const char* name)
      {
      MixdownFileDialog* dialog = new MixdownFileDialog(sf, parent,
         name, true);
      dialog->exec();
      SndFile* sndFile = dialog->sndFile();
      delete dialog;
      return sndFile;
      }

//---------------------------------------------------------
//   MixdownFileDialog
//---------------------------------------------------------

MixdownFileDialog::MixdownFileDialog(const SndFile* _sf,
   QWidget* parent, const char* name, bool /*modal*/, Qt::WFlags fl)
   : MixdownFileDialogBase(parent, name, true, fl)
      {
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
            comboChannel->setCurrentItem(channels);
            comboFormat->setCurrentItem(format);
            }
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void MixdownFileDialog::accept()
      {
      QString oldpath;
      unsigned channel = comboChannel->currentItem();
      unsigned format  = comboFormat->currentItem();
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
      sf->setFormat(format, channel, sampleRate);
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
      QString path = Q3FileDialog::getSaveFileName(
         oldpath, tr("Wave Files (*.wav);;All Files (*)"), this, "MixdownFileDialog");
      if (!path.isEmpty())
            editPath->setText(path);
      }

