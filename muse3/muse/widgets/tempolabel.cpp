//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tempolabel.cpp,v 1.1.1.1 2003/10/27 18:54:29 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
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
//
//=========================================================

#include <QApplication>
#include <QStyle>
#include <QStyleOption>

#include "tempolabel.h"
#include "globaldefs.h"

namespace MusEGui {

//---------------------------------------------------------
//   TempoLabel
//---------------------------------------------------------

TempoLabel::TempoLabel(QWidget* parent, const char* name)
   : QLabel(parent)
      {
      setObjectName(name);
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

//---------------------------------------------------------
//   TempoSpinBox
//---------------------------------------------------------

TempoEdit::TempoEdit(QWidget* parent)
   : DoubleSpinBox(parent)
      {
      setSingleStep(1.0);
      _extern = false;
      // Be consistent with other tempo boxes such as the one in transport.
      setDecimals(2);
      setRange(MUSE_MIN_TEMPO_VAL, MUSE_MAX_TEMPO_VAL);
      curVal = -1.0;
      
      connect(this, SIGNAL(valueChanged(double)), SLOT(newValue(double)));
      }

//---------------------------------------------------------
//   sizeHint
//---------------------------------------------------------

QSize TempoEdit::sizeHint() const
      {
      if(const QStyle* st = style())
      {
        st = st->proxy();
        
        QStyleOptionSpinBox option;
        option.initFrom(this);
        option.rect = rect();
        option.state = QStyle::State_Active | QStyle::State_Enabled;
        const QRect b_rect = st->subControlRect(QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxUp);
        
        QFontMetrics fm(font());
        const int fw = st->pixelMetric(QStyle::PM_SpinBoxFrameWidth);
        int h  = fm.height() + fw * 2;
        int w  = fw * 2 + b_rect.width() + fm.width(QString("000.00"));
        return QSize(w, h).expandedTo(QApplication::globalStrut());
      }
      return QSize(20, 20).expandedTo(QApplication::globalStrut());
      }
      
//---------------------------------------------------------
//   tempoChanged
//---------------------------------------------------------

void TempoEdit::newValue(double val)
      {
      if(_extern)
        return;
      if (val != curVal) {
      curVal = val;
          emit tempoChanged(curVal);
          }
      }

//---------------------------------------------------------
//   setValue
//---------------------------------------------------------

void TempoEdit::setValue(double val)
      {
      if (val != curVal) {
        curVal = val;
        if(!_extern) {
          blockSignals(true);
          QDoubleSpinBox::setValue(val);
          blockSignals(false);
                     }
                }
      }

//---------------------------------------------------------
//   setExternalMode
//---------------------------------------------------------

void TempoEdit::setExternalMode(bool on)
{
  if(_extern == on)
    return;
  
  _extern = on;
  if(_extern)
  {
    setEnabled(false);
    
    // Set the special text.
    setSpecialValueText(QString("extern"));
    
    // Force to minimum.
    blockSignals(true);
    QDoubleSpinBox::setValue(minimum());
    blockSignals(false);
  }
  else
  {
    // Reset the special text.
    setSpecialValueText(QString());
    
    // Restore.
    blockSignals(true);
    QDoubleSpinBox::setValue(curVal);
    blockSignals(false);
    
    setEnabled(true);
  }
}


} // namespace MusEGui
