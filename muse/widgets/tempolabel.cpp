//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tempolabel.cpp,v 1.1.1.1 2003/10/27 18:54:29 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include <qapplication.h>
#include <qstyle.h>
#include <qvalidator.h>
//Added by qt3to4:
#include <QLabel>
#include "tempolabel.h"

//---------------------------------------------------------
//   TempoLabel
//---------------------------------------------------------

TempoLabel::TempoLabel(QWidget* parent, const char* name)
   : QLabel(parent, name)
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

//---------------------------------------------------------
//   TempoSpinBox
//---------------------------------------------------------

TempoEdit::TempoEdit(QWidget* parent, const char* name)
   : QSpinBox(parent, name)
      {
      setLineStep(100);
      setMaxValue(60000);
      setMinValue(3000);
      //setValidator(new QDoubleValidator(this)); ddskrjo
      connect(this, SIGNAL(valueChanged(int)), SLOT(tempoChanged(int)));
      }

//---------------------------------------------------------
//   sizeHint
//---------------------------------------------------------

QSize TempoEdit::sizeHint() const
      {
      QFontMetrics fm(font());
      int fw = style()->pixelMetric(QStyle::PM_DefaultFrameWidth,0, this); // ddskrjo
      int h  = fm.height() + fw * 2;
      int w  = 2 + fm.width(QString("000.00")) +  fw * 4 + 30;
      return QSize(w, h).expandedTo(QApplication::globalStrut());
      }

//---------------------------------------------------------
//   mapValueToText
//---------------------------------------------------------

QString TempoEdit::mapValueToText(int val)
      {
      double v = val / 100.0;
      return QString("%1").arg(v, 3, 'f', 2);
      }

//---------------------------------------------------------
//   mapTextToValue
//---------------------------------------------------------

int TempoEdit::mapTextToValue(bool* ok)
      {
      double v = text().toDouble(ok);
      return int(v * 100);
      }

//---------------------------------------------------------
//   tempoChanged
//---------------------------------------------------------

void TempoEdit::tempoChanged(int val)
      {
      emit valueChanged(double(val)/100.0);
      }

//---------------------------------------------------------
//   setValue
//---------------------------------------------------------

void TempoEdit::setValue(double val)
      {
      QSpinBox::setValue(int(val*100));
      }

