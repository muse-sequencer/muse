//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midifilterimpl.cpp,v 1.1.1.1 2003/10/27 18:52:49 wschweer Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
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

#include "midifilterimpl.h"
#include "ctrlcombo.h"

#include <QDialog>
#include <QCloseEvent>

namespace MusEGui {

//---------------------------------------------------------
//   setCtrl
//---------------------------------------------------------

void MidiFilterConfig::setCtrl1(int n)
      {
      MusEGlobal::midiFilterCtrl1 = n-1;
      }
void MidiFilterConfig::setCtrl2(int n)
      {
      MusEGlobal::midiFilterCtrl2 = n-1;
      }
void MidiFilterConfig::setCtrl3(int n)
      {
      MusEGlobal::midiFilterCtrl3 = n-1;
      }
void MidiFilterConfig::setCtrl4(int n)
      {
      MusEGlobal::midiFilterCtrl4 = n-1;
      }


//---------------------------------------------------------
//   MidiFilterConfig
//---------------------------------------------------------

MidiFilterConfig::MidiFilterConfig(QDialog* parent)
  : QDialog(parent)
      {
      setupUi(this);
      cb1->setCurrentIndex(MusEGlobal::midiFilterCtrl1);
      cb2->setCurrentIndex(MusEGlobal::midiFilterCtrl2);
      cb3->setCurrentIndex(MusEGlobal::midiFilterCtrl3);
      cb4->setCurrentIndex(MusEGlobal::midiFilterCtrl4);

      rf1->setChecked(MusEGlobal::midiRecordType & 1);
      rf2->setChecked(MusEGlobal::midiRecordType & 2);
      rf3->setChecked(MusEGlobal::midiRecordType & 4);
      rf4->setChecked(MusEGlobal::midiRecordType & 8);
      rf5->setChecked(MusEGlobal::midiRecordType & 16);
      rf6->setChecked(MusEGlobal::midiRecordType & 32);
      rf7->setChecked(MusEGlobal::midiRecordType & 64);
      connect(rf1, SIGNAL(toggled(bool)), SLOT(recordChanged1(bool)));
      connect(rf2, SIGNAL(toggled(bool)), SLOT(recordChanged2(bool)));
      connect(rf3, SIGNAL(toggled(bool)), SLOT(recordChanged3(bool)));
      connect(rf4, SIGNAL(toggled(bool)), SLOT(recordChanged4(bool)));
      connect(rf5, SIGNAL(toggled(bool)), SLOT(recordChanged5(bool)));
      connect(rf6, SIGNAL(toggled(bool)), SLOT(recordChanged6(bool)));
      connect(rf7, SIGNAL(toggled(bool)), SLOT(recordChanged7(bool)));

      tf1->setChecked(MusEGlobal::midiThruType & 1);
      tf2->setChecked(MusEGlobal::midiThruType & 2);
      tf3->setChecked(MusEGlobal::midiThruType & 4);
      tf4->setChecked(MusEGlobal::midiThruType & 8);
      tf5->setChecked(MusEGlobal::midiThruType & 16);
      tf6->setChecked(MusEGlobal::midiThruType & 32);
      tf7->setChecked(MusEGlobal::midiThruType & 64);
      connect(tf1, SIGNAL(toggled(bool)), SLOT(thruChanged1(bool)));
      connect(tf2, SIGNAL(toggled(bool)), SLOT(thruChanged2(bool)));
      connect(tf3, SIGNAL(toggled(bool)), SLOT(thruChanged3(bool)));
      connect(tf4, SIGNAL(toggled(bool)), SLOT(thruChanged4(bool)));
      connect(tf5, SIGNAL(toggled(bool)), SLOT(thruChanged5(bool)));
      connect(tf6, SIGNAL(toggled(bool)), SLOT(thruChanged6(bool)));
      connect(tf7, SIGNAL(toggled(bool)), SLOT(thruChanged7(bool)));

      cf1->setChecked(MusEGlobal::midiInputChannel  & 1);
      cf2->setChecked(MusEGlobal::midiInputChannel  & 2);
      cf3->setChecked(MusEGlobal::midiInputChannel  & 4);
      cf4->setChecked(MusEGlobal::midiInputChannel  & 8);
      cf5->setChecked(MusEGlobal::midiInputChannel  & 0x10);
      cf6->setChecked(MusEGlobal::midiInputChannel  & 0x20);
      cf7->setChecked(MusEGlobal::midiInputChannel  & 0x40);
      cf8->setChecked(MusEGlobal::midiInputChannel  & 0x80);
      cf9->setChecked(MusEGlobal::midiInputChannel  & 0x100);
      cf10->setChecked(MusEGlobal::midiInputChannel & 0x200);
      cf11->setChecked(MusEGlobal::midiInputChannel & 0x400);
      cf12->setChecked(MusEGlobal::midiInputChannel & 0x800);
      cf13->setChecked(MusEGlobal::midiInputChannel & 0x1000);
      cf14->setChecked(MusEGlobal::midiInputChannel & 0x2000);
      cf15->setChecked(MusEGlobal::midiInputChannel & 0x4000);
      cf16->setChecked(MusEGlobal::midiInputChannel & 0x8000);

      connect(cb1, SIGNAL(activated(int)), SLOT(setCtrl1(int)));
      connect(cb2, SIGNAL(activated(int)), SLOT(setCtrl2(int)));
      connect(cb3, SIGNAL(activated(int)), SLOT(setCtrl3(int)));
      connect(cb4, SIGNAL(activated(int)), SLOT(setCtrl4(int)));

      connect(cf1,  SIGNAL(toggled(bool)), SLOT(channelChanged1(bool)));
      connect(cf2,  SIGNAL(toggled(bool)), SLOT(channelChanged2(bool)));
      connect(cf3,  SIGNAL(toggled(bool)), SLOT(channelChanged3(bool)));
      connect(cf4,  SIGNAL(toggled(bool)), SLOT(channelChanged4(bool)));
      connect(cf5,  SIGNAL(toggled(bool)), SLOT(channelChanged5(bool)));
      connect(cf6,  SIGNAL(toggled(bool)), SLOT(channelChanged6(bool)));
      connect(cf7,  SIGNAL(toggled(bool)), SLOT(channelChanged7(bool)));
      connect(cf8,  SIGNAL(toggled(bool)), SLOT(channelChanged8(bool)));
      connect(cf9,  SIGNAL(toggled(bool)), SLOT(channelChanged9(bool)));
      connect(cf10, SIGNAL(toggled(bool)), SLOT(channelChanged10(bool)));
      connect(cf11, SIGNAL(toggled(bool)), SLOT(channelChanged11(bool)));
      connect(cf12, SIGNAL(toggled(bool)), SLOT(channelChanged12(bool)));
      connect(cf13, SIGNAL(toggled(bool)), SLOT(channelChanged13(bool)));
      connect(cf14, SIGNAL(toggled(bool)), SLOT(channelChanged14(bool)));
      connect(cf15, SIGNAL(toggled(bool)), SLOT(channelChanged15(bool)));
      connect(cf16, SIGNAL(toggled(bool)), SLOT(channelChanged16(bool)));
      }

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void MidiFilterConfig::closeEvent(QCloseEvent* ev)
      {
      emit hideWindow();
      QWidget::closeEvent(ev);
      }

} // namespace MusEGui

