//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mrconfig.cpp,v 1.1.1.1 2003/10/27 18:52:43 wschweer Exp $
//
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

#include "pitchedit.h"
#include "mrconfig.h"
#include "globals.h"

#include <QCloseEvent>

namespace MusEGui {

//---------------------------------------------------------
//   MRConfig
//    Midi Remote Control Config
//---------------------------------------------------------

MRConfig::MRConfig(QWidget* parent, Qt::WFlags fl)
   : QWidget(parent, fl)
      {
      setupUi(this);
      b1->setChecked(MusEGlobal::rcEnable);
      sb1->setValue(MusEGlobal::rcStopNote);
      sb2->setValue(MusEGlobal::rcRecordNote);
      sb3->setValue(MusEGlobal::rcGotoLeftMarkNote);
      sb4->setValue(MusEGlobal::rcPlayNote);
      steprec_box->setValue(MusEGlobal::rcSteprecNote);

      connect(b1,  SIGNAL(toggled(bool)), SLOT(setRcEnable(bool)));
      connect(sb1, SIGNAL(valueChanged(int)), SLOT(setRcStopNote(int)));
      connect(sb2, SIGNAL(valueChanged(int)), SLOT(setRcRecordNote(int)));
      connect(sb3, SIGNAL(valueChanged(int)), SLOT(setRcGotoLeftMarkNote(int)));
      connect(sb4, SIGNAL(valueChanged(int)), SLOT(setRcPlayNote(int)));
      connect(steprec_box, SIGNAL(valueChanged(int)), SLOT(setRcSteprecNote(int)));
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
      MusEGlobal::rcEnable = f;
      }

void MRConfig::setRcStopNote(int val)
      {
      MusEGlobal::rcStopNote = val;
      }

void MRConfig::setRcRecordNote(int val)
      {
      MusEGlobal::rcRecordNote = val;
      }

void MRConfig::setRcGotoLeftMarkNote(int val)
      {
      MusEGlobal::rcGotoLeftMarkNote = val;
      }

void MRConfig::setRcPlayNote(int val)
      {
      MusEGlobal::rcPlayNote = val;
      }

void MRConfig::setRcSteprecNote(int val)
      {
      MusEGlobal::rcSteprecNote = val;
      }

} // namespace MusEGui
