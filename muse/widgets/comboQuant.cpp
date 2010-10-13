//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: comboQuant.cpp,v 1.1.1.1 2003/10/27 18:54:52 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#include <q3listbox.h>
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

ComboQuant::ComboQuant(QWidget* parent, const char* name)
   : QComboBox(parent, name)
      {
      Q3ListBox* qlist = new Q3ListBox(this);
      qlist->setMinimumWidth(95);
      //setListBox(qlist); ddskrjo
      qlist->setColumnMode(3);
      for (int i = 0; i < 24; i++)
            qlist->insertItem(tr(quantStrings[i]), i);
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
                  setCurrentItem(i);
                  return;
                  }
            }
      }

