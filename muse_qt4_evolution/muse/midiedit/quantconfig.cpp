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

