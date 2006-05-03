//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

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

