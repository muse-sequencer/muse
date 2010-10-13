//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mrconfig.cpp,v 1.1.1.1 2003/10/27 18:52:43 wschweer Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#include "pitchedit.h"
#include "mrconfig.h"
#include "globals.h"
#include <qcheckbox.h>
//Added by qt3to4:
#include <QCloseEvent>

//---------------------------------------------------------
//   MRConfig
//    Midi Remote Control Config
//---------------------------------------------------------

MRConfig::MRConfig(QWidget* parent, const char* name, Qt::WFlags fl)
   : MRConfigBase(parent, name, fl)
      {
      b1->setChecked(rcEnable);
      sb1->setValue(rcStopNote);
      sb2->setValue(rcRecordNote);
      sb3->setValue(rcGotoLeftMarkNote);
      sb4->setValue(rcPlayNote);

      connect(b1,  SIGNAL(toggled(bool)), SLOT(setRcEnable(bool)));
      connect(sb1, SIGNAL(valueChanged(int)), SLOT(setRcStopNote(int)));
      connect(sb2, SIGNAL(valueChanged(int)), SLOT(setRcRecordNote(int)));
      connect(sb3, SIGNAL(valueChanged(int)), SLOT(setRcGotoLeftMarkNote(int)));
      connect(sb4, SIGNAL(valueChanged(int)), SLOT(setRcPlayNote(int)));
      }

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void MRConfig::closeEvent(QCloseEvent* ev)
      {
      emit hideWindow();
      QWidget::closeEvent(ev);
      }

void MRConfig::setRcEnable(bool f)
      {
      rcEnable = f;
      }

void MRConfig::setRcStopNote(int val)
      {
      rcStopNote = val;
      }

void MRConfig::setRcRecordNote(int val)
      {
      rcRecordNote = val;
      }

void MRConfig::setRcGotoLeftMarkNote(int val)
      {
      rcGotoLeftMarkNote = val;
      }

void MRConfig::setRcPlayNote(int val)
      {
      rcPlayNote = val;
      }

