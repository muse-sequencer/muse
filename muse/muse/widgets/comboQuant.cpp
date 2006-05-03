//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: comboQuant.cpp,v 1.5 2005/11/04 12:03:48 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#include "comboQuant.h"

static int quantTable[] = {
      1, 16, 32,  64, 128, 256,  512, 1024,
      1, 24, 48,  96, 192, 384,  768, 1536,
      1, 36, 72, 144, 288, 576, 1152, 2304
      };

static const char* quantStrings[] = {
      QT_TR_NOOP("Off"), "64T", "32T", "16T", "8T", "4T", "2T", "1T",
      QT_TR_NOOP("Off"), "64",  "32",  "16",  "8",  "4",  "2",  "1",
      QT_TR_NOOP("Off"), "64.", "32.", "16.", "8.", "4.", "2.", "1."
      };

//---------------------------------------------------------
//   ComboQuant
//---------------------------------------------------------

ComboQuant::ComboQuant(QWidget* parent)
   : QComboBox(parent)
      {
      for (int i = 0; i < 24; i++)
            addItem(tr(quantStrings[i]), i);
      connect(this, SIGNAL(activated(int)), SLOT(activated(int)));
      }

//---------------------------------------------------------
//   activated
//---------------------------------------------------------

void ComboQuant::activated(int index)
      {
      emit valueChanged(quantTable[index]);
      }

//---------------------------------------------------------
//   setQuant
//---------------------------------------------------------

void ComboQuant::setValue(int val)
      {
      for (int i = 0; i < 24; i++) {
            if (val == quantTable[i]) {
                  setCurrentIndex(i);
                  return;
                  }
            }
      }

