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

#include "tempolabel.h"

#include <QApplication>

namespace Awl {

//---------------------------------------------------------
//   TempoLabel
//---------------------------------------------------------

TempoLabel::TempoLabel(QWidget* parent)
   : QLabel(parent)
      {
      setFrameStyle(WinPanel | Sunken);
      setLineWidth(2);
      setMidLineWidth(3);
      _value = 1.0;
      setValue(0.0);
      setIndent(3);
      setMinimumSize(sizeHint());
      }

//---------------------------------------------------------
//   setVal
//---------------------------------------------------------

void TempoLabel::setValue(int val)
      {
      setValue(double(val/1000.0));
      }

void TempoLabel::setValue(double val)
      {
      if (val == _value)
            return;
      _value = val;
      QString s = QString("%1").arg(val, 3, 'f', 2);
      setText(s);
      }

//---------------------------------------------------------
//   sizeHint
//---------------------------------------------------------

QSize TempoLabel::sizeHint() const
      {
      QFontMetrics fm(font());
      int fw = 4;
      int h  = fm.height() + fw * 2;
      int w  = 6 + fm.width(QString("000.00")) +  fw * 2;  // 6=indent
      return QSize(w, h).expandedTo(QApplication::globalStrut());
      }
}

