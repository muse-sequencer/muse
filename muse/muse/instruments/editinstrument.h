//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: editinstrument.h,v 1.5 2005/12/12 22:09:09 wschweer Exp $
//
//  (C) Copyright 2003 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __EDITINSTRUMENT_H__
#define __EDITINSTRUMENT_H__

#include "ui_editinstrument.h"

//---------------------------------------------------------
//   EditInstrument
//---------------------------------------------------------

class EditInstrument : public QMainWindow, public Ui::EditInstrumentBase {
    Q_OBJECT

   private slots:
      virtual void fileNew();
      virtual void fileOpen();
      virtual void fileSave();
      virtual void fileSaveAs();
      virtual void fileExit();
      void instrumentChanged(QListWidgetItem*);
      void patchChanged(QTreeWidgetItem*);

   public:
      EditInstrument(QWidget* parent = 0);
      };

#endif

