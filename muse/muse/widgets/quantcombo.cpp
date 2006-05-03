//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: quantcombo.cpp,v 1.2 2006/01/25 16:24:33 wschweer Exp $
//  (C) Copyright 2006 Werner Schweer (ws@seh.de)
//=========================================================

#include "quantcombo.h"

static int quantTable[] = {
      1, 24, 48,  96, 192, 384,  768, 1536,
      16, 32,  64, 128, 256,  512, 1024,
      36, 72, 144, 288, 576, 1152, 2304
      };

static const char* quantStrings[] = {
      QT_TR_NOOP("Off"), "64",  "32",  "16",  "8",  "4",  "2",  "1",
      "64T", "32T", "16T", "8T", "4T", "2T", "1T",
      "64.", "32.", "16.", "8.", "4.", "2.", "1."
      };

//---------------------------------------------------------
//   QuantCombo
//---------------------------------------------------------

QuantCombo::QuantCombo(QWidget* parent)
   : QComboBox(parent)
      {
      for (unsigned i = 0; i < sizeof(quantStrings)/sizeof(*quantStrings); i++)
            addItem(tr(quantStrings[i]), i);
      connect(this,  SIGNAL(activated(int)), SLOT(_quantChanged(int)));
      }

//---------------------------------------------------------
//   _quantChanged
//---------------------------------------------------------

void QuantCombo::_quantChanged(int idx)
      {
      emit quantChanged(quantTable[idx]);
      }

//---------------------------------------------------------
//   quant
//---------------------------------------------------------

int QuantCombo::quant() const
      {
      return quantTable[currentIndex()];
      }

//---------------------------------------------------------
//   setQuant
//---------------------------------------------------------

void QuantCombo::setQuant(int val)
      {
      for (unsigned i = 0; i < sizeof(quantTable)/sizeof(*quantTable); i++) {
            if (val == quantTable[i]) {
                  setCurrentIndex(i);
                  return;
                  }
            }
      printf("setQuant(%d) not defined\n", val);
abort();
      setCurrentIndex(0);
      }
