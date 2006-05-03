//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: quantconfig.cpp,v 1.7 2006/01/25 16:24:33 wschweer Exp $
//
//  (C) Copyright 2006 Werner Schweer (ws@seh.de)
//=========================================================

#include "quantconfig.h"

const char* wtStrengthTxt = QT_TR_NOOP("sets amount of quantization:\n"
                            "0   - no quantization\n"
                            "100 - full quantization");
const char* wtQLimitTxt = QT_TR_NOOP("don't quantize notes above this tick limit");
const char* wtQLenTxt   = QT_TR_NOOP("quantize also note len as default");

//---------------------------------------------------------
//   QuantConfig
//---------------------------------------------------------

QuantConfig::QuantConfig(int s, int l, bool lenFlag, QWidget* parent)
   : QDialog(parent)
      {
      setupUi(this);
      strength->setValue(s);
      dontQuantize->setValue(l);
      quantLen->setChecked(lenFlag);
      }

//---------------------------------------------------------
//   quantStrength
//---------------------------------------------------------

int QuantConfig::quantStrength() const
	{
      return strength->value();
      }

//---------------------------------------------------------
//   quantLimit
//---------------------------------------------------------

int QuantConfig::quantLimit() const
	{
      return dontQuantize->value();
      }

//---------------------------------------------------------
//   doQuantLen
//---------------------------------------------------------

bool QuantConfig::doQuantLen() const
	{
      return quantLen->isChecked();
      }

