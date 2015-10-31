//=============================================================================
//  Awl
//  Audio Widget Library
//  $Id:$
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//=============================================================================

#include <cmath>

#include "tempoedit.h"

#include <QApplication>
#include <QStyle>

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

