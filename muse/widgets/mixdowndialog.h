//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: mixdowndialog.h,v 1.1.1.1 2003/10/27 18:54:28 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MIXDOWNDIALOG_H__
#define __MIXDOWNDIALOG_H__

#include "mixdowndialogbase.h"

class SndFile;

extern SndFile* getSndFile(const SndFile* sf, QWidget* parent, 
   const char* name);

//---------------------------------------------------------
//   MixdownFileDialog
//---------------------------------------------------------

class MixdownFileDialog : public MixdownFileDialogBase {
    Q_OBJECT
      SndFile* sf;

   private slots:
      void fdialog();
      virtual void accept();

   public:
      MixdownFileDialog(const SndFile* f, QWidget* parent = 0,
         const char* name = 0, bool modal = false, Qt::WFlags fl = 0);
      SndFile* sndFile() { return sf; }
      };

#endif

