//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: pitchlabel.cpp,v 1.2 2004/05/16 16:55:01 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
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

#include "pitchedit.h"
#include "pitchlabel.h"
#include "helper.h"

namespace MusEGui {

//---------------------------------------------------------
//   PitchLabel
//---------------------------------------------------------

PitchLabel::PitchLabel(QWidget* parent, const char* name)
  : QLabel(parent)
      {
      setObjectName(name);
      _pitchMode = true;
      _value = -1;
      setFrameStyle(WinPanel | Sunken);
      setLineWidth(2);
      setMidLineWidth(3);
      setValue(0);
      //int fw = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, 0, this); // ddskrjo 0
      int fw = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
      setIndent(fw);
      //setContentsMargins(0,0,0,0);  
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
      const QString t = text();
      const QFontMetrics fm(font());

      // Enough space for a lower limit to fit for example "C#-2" plus extra space for good luck.
      // Must display 14Bit controller values.
// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
      int minTextWidth = fm.horizontalAdvance("W#-2");
      // Enough space for an upper limit of say 7 'W' characters.
      int maxTextWidth = fm.horizontalAdvance("WWWWWWW");
      int textWidth = fm.horizontalAdvance(t);
#else
      int minTextWidth = fm.width("W#-2");
      // Enough space for an upper limit of say 7 'W' characters.
      int maxTextWidth = fm.width("WWWWWWW");
      int textWidth = fm.width(t);
#endif
      if(textWidth < minTextWidth)
        textWidth = minTextWidth;
      if(textWidth > maxTextWidth)
        textWidth = maxTextWidth;

      //int fw = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, 0, this); // ddskrjo 0
      int fw = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);

      int w = textWidth + fw * 2;
      int h  = fm.height() + fw * 2;
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
            s = MusECore::pitch2string(_value);
      else
            s = QString::number(_value);
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

} // namespace MusEGui
