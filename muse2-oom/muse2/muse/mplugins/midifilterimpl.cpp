//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midifilterimpl.cpp,v 1.1.1.1 2003/10/27 18:52:49 wschweer Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//=========================================================

#include "midifilterimpl.h"
#include "ctrlcombo.h"

#include <QDialog>
#include <QCloseEvent>

//---------------------------------------------------------
//   setCtrl
//---------------------------------------------------------

void MidiFilterConfig::setCtrl1(int n)
      {
      midiFilterCtrl1 = n-1;
      }
void MidiFilterConfig::setCtrl2(int n)
      {
      midiFilterCtrl2 = n-1;
      }
void MidiFilterConfig::setCtrl3(int n)
      {
      midiFilterCtrl3 = n-1;
      }
void MidiFilterConfig::setCtrl4(int n)
      {
      midiFilterCtrl4 = n-1;
      }


//---------------------------------------------------------
//   MidiFilterConfig
//---------------------------------------------------------

MidiFilterConfig::MidiFilterConfig(QDialog* parent)
  : QDialog(parent)
      {
      setupUi(this);
      cb1->setCurrentIndex(midiFilterCtrl1);
      cb2->setCurrentIndex(midiFilterCtrl2);
      cb3->setCurrentIndex(midiFilterCtrl3);
      cb4->setCurrentIndex(midiFilterCtrl4);

      rf1->setChecked(midiRecordType & 1);
      rf2->setChecked(midiRecordType & 2);
      rf3->setChecked(midiRecordType & 4);
      rf4->setChecked(midiRecordType & 8);
      rf5->setChecked(midiRecordType & 16);
      rf6->setChecked(midiRecordType & 32);
      rf7->setChecked(midiRecordType & 64);
      connect(rf1, SIGNAL(toggled(bool)), SLOT(recordChanged1(bool)));
      connect(rf2, SIGNAL(toggled(bool)), SLOT(recordChanged2(bool)));
      connect(rf3, SIGNAL(toggled(bool)), SLOT(recordChanged3(bool)));
      connect(rf4, SIGNAL(toggled(bool)), SLOT(recordChanged4(bool)));
      connect(rf5, SIGNAL(toggled(bool)), SLOT(recordChanged5(bool)));
      connect(rf6, SIGNAL(toggled(bool)), SLOT(recordChanged6(bool)));
      connect(rf7, SIGNAL(toggled(bool)), SLOT(recordChanged7(bool)));

      tf1->setChecked(midiThruType & 1);
      tf2->setChecked(midiThruType & 2);
      tf3->setChecked(midiThruType & 4);
      tf4->setChecked(midiThruType & 8);
      tf5->setChecked(midiThruType & 16);
      tf6->setChecked(midiThruType & 32);
      tf7->setChecked(midiThruType & 64);
      connect(tf1, SIGNAL(toggled(bool)), SLOT(thruChanged1(bool)));
      connect(tf2, SIGNAL(toggled(bool)), SLOT(thruChanged2(bool)));
      connect(tf3, SIGNAL(toggled(bool)), SLOT(thruChanged3(bool)));
      connect(tf4, SIGNAL(toggled(bool)), SLOT(thruChanged4(bool)));
      connect(tf5, SIGNAL(toggled(bool)), SLOT(thruChanged5(bool)));
      connect(tf6, SIGNAL(toggled(bool)), SLOT(thruChanged6(bool)));
      connect(tf7, SIGNAL(toggled(bool)), SLOT(thruChanged7(bool)));

      cf1->setChecked(midiInputChannel  & 1);
      cf2->setChecked(midiInputChannel  & 2);
      cf3->setChecked(midiInputChannel  & 4);
      cf4->setChecked(midiInputChannel  & 8);
      cf5->setChecked(midiInputChannel  & 0x10);
      cf6->setChecked(midiInputChannel  & 0x20);
      cf7->setChecked(midiInputChannel  & 0x40);
      cf8->setChecked(midiInputChannel  & 0x80);
      cf9->setChecked(midiInputChannel  & 0x100);
      cf10->setChecked(midiInputChannel & 0x200);
      cf11->setChecked(midiInputChannel & 0x400);
      cf12->setChecked(midiInputChannel & 0x800);
      cf13->setChecked(midiInputChannel & 0x1000);
      cf14->setChecked(midiInputChannel & 0x2000);
      cf15->setChecked(midiInputChannel & 0x4000);
      cf16->setChecked(midiInputChannel & 0x8000);

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


