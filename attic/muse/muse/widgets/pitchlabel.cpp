//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: pitchlabel.cpp,v 1.2 2004/05/16 16:55:01 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#include <qapplication.h>
#include <qstyle.h>
#include "pitchedit.h"
#include "pitchlabel.h"
#include "helper.h"

//---------------------------------------------------------
//   PitchLabel
//---------------------------------------------------------

PitchLabel::PitchLabel(QWidget* parent, const char* name)
  : QLabel(parent, name)
      {
      _pitchMode = true;
      _value = -1;
      setFrameStyle(WinPanel | Sunken);
      setLineWidth(2);
      setMidLineWidth(3);
      setValue(0);
      int fw = style().pixelMetric(QStyle::PM_DefaultFrameWidth, this);
      setIndent(fw);
      }

//---------------------------------------------------------
//   setPitchMode
//---------------------------------------------------------

void PitchLabel::setPitchMode(bool val)
      {
      _pitchMode = val;
      }

//---------------------------------------------------------
//   sizeHint
//---------------------------------------------------------

QSize PitchLabel::sizeHint() const
      {
      QFontMetrics fm(font());
      int fw = style().pixelMetric(QStyle::PM_DefaultFrameWidth, this);
      int h  = fm.height() + fw * 2;
//      int w = 2 + fm.width(QString("A#8")) +  fw * 4;
      int w = 2 + fm.width(QString("-9999")) + fw * 4;     // must display 14Bit controller values
      return QSize(w, h).expandedTo(QApplication::globalStrut());
      }

//---------------------------------------------------------
//   setValue
//---------------------------------------------------------

void PitchLabel::setValue(int val)
      {
      if (val == _value)
            return;
      _value = val;
      QString s;
      if (_pitchMode)
            s = pitch2string(_value);
      else
            s.sprintf("%d", _value);
      setText(s);
      }

//---------------------------------------------------------
//   setInt
//---------------------------------------------------------

void PitchLabel::setInt(int val)
      {
      if (_pitchMode)
            setPitchMode(false);
      setValue(val);
      }

//---------------------------------------------------------
//   setPitch
//---------------------------------------------------------

void PitchLabel::setPitch(int val)
      {
      if (!_pitchMode) {
            setPitchMode(true);
            }
      setValue(val);
      }

