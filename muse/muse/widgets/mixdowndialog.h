//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: mixdowndialog.h,v 1.4 2005/11/04 12:03:48 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MIXDOWNDIALOG_H__
#define __MIXDOWNDIALOG_H__

#include "ui_mixdowndialog.h"

class SndFile;

extern SndFile* getSndFile(SndFile* sf, QWidget* parent);

//---------------------------------------------------------
//   MixdownFileDialog
//---------------------------------------------------------

class MixdownFileDialog : public QDialog, private Ui::MixdownFileDialogBase {
    Q_OBJECT
      SndFile* sf;

   private slots:
      void fdialog();
      virtual void accept();

   public:
      MixdownFileDialog(SndFile* f, QWidget* parent = 0);
      SndFile* sndFile() { return sf; }
      };

#endif

