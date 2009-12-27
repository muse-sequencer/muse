//=============================================================================
//  Awl
//  Audio Widget Library
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

#include "tempoedit.h"

namespace Awl {

//---------------------------------------------------------
//   TempoEdit
//---------------------------------------------------------

TempoEdit::TempoEdit(QWidget* parent)
   : QDoubleSpinBox(parent)
      {
      curVal = -1.0;
      setSingleStep(1.0);
      setRange(30.0, 600.0);
      connect(this, SIGNAL(valueChanged(double)), SLOT(newValue(double)));
      }

//---------------------------------------------------------
//   sizeHint
//---------------------------------------------------------

QSize TempoEdit::sizeHint() const
      {
      QFontMetrics fm(font());
      int fw = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
      int h  = fm.height() + fw * 2;
      int w  = 2 + fm.width(QString("000.00")) +  fw * 4 + 30;
      return QSize(w, h).expandedTo(QApplication::globalStrut());
      }

//---------------------------------------------------------
//   newValue
//---------------------------------------------------------

void TempoEdit::newValue(double val)
      {
	if (val != curVal) {
      	curVal = val;
            emit tempoChanged(tempo());
            }
      }

//---------------------------------------------------------
//   setTempo
//---------------------------------------------------------

void TempoEdit::setTempo(int val)
      {
      double d = 60000000.0/double(val);
      if (d != curVal) {
      	curVal = d;
        	blockSignals(true);
      	setValue(d);
        	blockSignals(false);
         	}
      }

//---------------------------------------------------------
//   tempo
//---------------------------------------------------------

int TempoEdit::tempo() const
      {
	return lrint(60000000.0/value());
      }

}

