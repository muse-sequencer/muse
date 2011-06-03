//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mrconfig.h,v 1.1.1.1 2003/10/27 18:52:43 wschweer Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MRCONFIG_H__
#define __MRCONFIG_H__

#include "ui_mrconfigbase.h"

class QCloseEvent;
class QWidget;

//---------------------------------------------------------
//   MRConfig
//---------------------------------------------------------

class MRConfig : public QWidget, public Ui::MRConfigBase {
      Q_OBJECT

      virtual void closeEvent(QCloseEvent*);

   signals:
      void hideWindow();

   private slots:
      void setRcEnable(bool);
      void setRcStopNote(int);
      void setRcRecordNote(int);
      void setRcGotoLeftMarkNote(int);
      void setRcPlayNote(int);
      void setRcSteprecNote(int);

   public:
      MRConfig(QWidget* parent=0, Qt::WFlags fl = 0);
      };

#endif

