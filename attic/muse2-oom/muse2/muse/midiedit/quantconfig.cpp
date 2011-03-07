//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: quantconfig.cpp,v 1.2 2004/04/24 14:58:52 wschweer Exp $
//
//  (C) Copyright 1999/2003 Werner Schweer (ws@seh.de)
//=========================================================

#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

#include "quantconfig.h"

const char* wtStrengthTxt = QT_TRANSLATE_NOOP("@default", "sets amount of quantization:\n"
                            "0   - no quantization\n"
                            "100 - full quantization");
const char* wtQLimitTxt = QT_TRANSLATE_NOOP("@default", "don't quantize notes above this tick limit");
const char* wtQLenTxt   = QT_TRANSLATE_NOOP("@default", "quantize also note len as default");

//---------------------------------------------------------
//   QuantConfig
//---------------------------------------------------------

QuantConfig::QuantConfig(int s, int l, bool lenFlag)
   : QDialog()
      {
      setWindowTitle(tr("MusE: Config Quantize"));
      QVBoxLayout *mainlayout = new QVBoxLayout;

      QGridLayout* layout = new QGridLayout;
      QGroupBox* gb = new QGroupBox(tr("Config Quantize"));

      QLabel* l1 = new QLabel(tr("Strength"));
      layout->addWidget(l1, 0, 0);
      QSpinBox* sb1 = new QSpinBox;
      sb1->setMinimum(0);
      sb1->setMaximum(100);
      sb1->setSingleStep(1);
      sb1->setSuffix(QString("%"));
      sb1->setValue(s);
      layout->addWidget(sb1, 0, 1);

      QLabel* l2 = new QLabel(tr("Don´t Quantize"));
      layout->addWidget(l2, 1, 0);
      QSpinBox* sb2 = new QSpinBox;
      sb2->setMinimum(0);
      sb2->setMaximum(500);
      sb2->setSingleStep(1);
      sb2->setValue(l);
      layout->addWidget(sb2, 1, 1);

      QLabel* l3 = new QLabel(tr("Quant Len"));
      layout->addWidget(l3, 2, 0);
      QCheckBox* but = new QCheckBox;
      but->setChecked(lenFlag);
      layout->addWidget(but, 2, 1);
      
      connect(sb1, SIGNAL(valueChanged(int)), SIGNAL(setQuantStrength(int)));
      connect(sb2, SIGNAL(valueChanged(int)), SIGNAL(setQuantLimit(int)));
      connect(but, SIGNAL(toggled(bool)), SIGNAL(setQuantLen(bool)));

      gb->setLayout(layout);
      mainlayout->addWidget(gb);
      setLayout(mainlayout);

      l1->setWhatsThis(tr(wtStrengthTxt));
      l1->setToolTip(tr(wtStrengthTxt));
      sb1->setWhatsThis(tr(wtStrengthTxt));
      l2->setWhatsThis(tr(wtQLimitTxt));
      l2->setToolTip(tr(wtQLimitTxt));
      sb2->setWhatsThis(tr(wtQLimitTxt));
      l3->setWhatsThis(tr(wtQLenTxt));
      l3->setToolTip(tr(wtQLenTxt));
      but->setWhatsThis(tr(wtQLenTxt));
      }

