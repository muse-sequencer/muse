//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: quantconfig.cpp,v 1.2 2004/04/24 14:58:52 wschweer Exp $
//
//  (C) Copyright 1999/2003 Werner Schweer (ws@seh.de)
//=========================================================

#include <qspinbox.h>
#include <QLayout>
#include <qlabel.h>
#include <qradiobutton.h>
#include <q3groupbox.h>
#include <q3whatsthis.h>

#include "quantconfig.h"
//Added by qt3to4:
#include <QVBoxLayout>

const char* wtStrengthTxt = QT_TR_NOOP("sets amount of quantization:\n"
                            "0   - no quantization\n"
                            "100 - full quantization");
const char* wtQLimitTxt = QT_TR_NOOP("don't quantize notes above this tick limit");
const char* wtQLenTxt   = QT_TR_NOOP("quantize also note len as default");

//---------------------------------------------------------
//   QuantConfig
//---------------------------------------------------------

QuantConfig::QuantConfig(int s, int l, bool lenFlag)
   : QDialog()
      {
      setCaption(tr("MusE: Config Quantize"));
      QVBoxLayout* layout = new QVBoxLayout(this);
      Q3GroupBox* gb = new Q3GroupBox(2, Qt::Horizontal, tr("Config Quantize"), this);
      layout->addWidget(gb);

      QLabel* l1 = new QLabel(tr("Strength"), gb);
      QSpinBox* sb1 = new QSpinBox(0, 100, 1, gb);
      sb1->setSuffix(QString("%"));
      sb1->setValue(s);
      QLabel* l2 = new QLabel(tr("Don´t Quantize"), gb);
      QSpinBox* sb2 = new QSpinBox(0, 500, 1, gb);
      sb2->setValue(l);
      QLabel* l3 = new QLabel(tr("Quant Len"), gb);
      QRadioButton* but = new QRadioButton(gb);
      but->setChecked(lenFlag);
      connect(sb1, SIGNAL(valueChanged(int)), SIGNAL(setQuantStrength(int)));
      connect(sb2, SIGNAL(valueChanged(int)), SIGNAL(setQuantLimit(int)));
      connect(but, SIGNAL(toggled(bool)), SIGNAL(setQuantLen(bool)));

      Q3WhatsThis::add(l1,  tr(wtStrengthTxt));
      Q3WhatsThis::add(sb1, tr(wtStrengthTxt));
      Q3WhatsThis::add(l2,  tr(wtQLimitTxt));
      Q3WhatsThis::add(sb2, tr(wtQLimitTxt));
      Q3WhatsThis::add(l3,  tr(wtQLenTxt));
      Q3WhatsThis::add(but, tr(wtQLenTxt));
      }

