//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: pitchedit.cpp,v 1.2 2004/01/09 17:12:54 wschweer Exp $
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

//#include <stdio.h>
#include "pitchedit.h"
#include "helper.h"
#include "note_names.h"
#include "gconfig.h"
#include <QStyle>
#include <QStyleOptionSpinBox>
#include <QApplication>

namespace MusEGui {

//---------------------------------------------------------
//   PitchEdit
//---------------------------------------------------------

PitchEdit::PitchEdit(QWidget* parent)
  : SpinBox(parent)
      {
      setMinimum(0);
      setMaximum(127);
      setSingleStep(1);
      deltaMode = false;
      // Save the original size policy
      originalPolicy = sizePolicy();
      connect(lineEdit(), &QLineEdit::textChanged, this, &PitchEdit::updateWidth);
      updateWidth();
      }

// FIXME: Neither the sizeHint nor the updateWidth method are being honoured.
//        The sizeHint is being called and returning the correct desired (dynamic) size.
//        But for some reason the spinbox is completely ignoring our size hint.
//        I tried changing size policy etc. No luck.
//        I added minimumSizeHint and pointed it to sizeHint but that did not help.
//        The minimumSizeHint was called and the value was used, but apparently only once, not dynamically.
//        It was working! The spin box was dynamically resizing OK. Now it's not.
//        The only thing I can think of is a recent system update. Nothing else changed. Feb 17 2026 Tim.

// QSize PitchEdit::sizeHint() const
//       {
//       const QString t = lineEdit()->text();
//       const QFontMetrics fm(font());
//
//       // Enough space for a lower limit to fit for example "C#-2" plus extra space for good luck.
//       // Must display 14Bit controller values.
// // Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
// #if QT_VERSION >= 0x050b00
//       int minTextWidth = fm.horizontalAdvance("W#-2");
//       // Enough space for an upper limit of say 7 'W' characters.
//       int maxTextWidth = fm.horizontalAdvance("WWWWWWW");
//       int textWidth = fm.horizontalAdvance(t);
// #else
//       int minTextWidth = fm.width("W#-2");
//       // Enough space for an upper limit of say 7 'W' characters.
//       int maxTextWidth = fm.width("WWWWWWW");
//       int textWidth = fm.width(t);
// #endif
//       if(textWidth < minTextWidth)
//         textWidth = minTextWidth;
//       if(textWidth > maxTextWidth)
//         textWidth = maxTextWidth;
//
//       QStyle *s = style();
//       QStyleOptionSpinBox opt;
//       initStyleOption(&opt);
//
//       // Get the up/down button sizes
//       const QRect upRect   = s->subControlRect(QStyle::CC_SpinBox, &opt,
//                                          QStyle::SC_SpinBoxUp, this);
//       const QRect downRect = s->subControlRect(QStyle::CC_SpinBox, &opt,
//                                          QStyle::SC_SpinBoxDown, this);
//
//       int buttonWidth = qMax(upRect.width(), downRect.width());
//
//       // Frame width on each side
//       int frameWidth = s->pixelMetric(QStyle::PM_DefaultFrameWidth, &opt, this);
//
//       int extraPadding = 6;     // breathing room for cursor
//
//       int w = textWidth +
//           buttonWidth +
//           frameWidth * 2 +
//           extraPadding;
//       int h  = fm.height() + frameWidth * 2;
//
//       return QSize(w, h).expandedTo(QApplication::globalStrut());
//       }

//---------------------------------------------------------
//   updateWidth
//---------------------------------------------------------

void PitchEdit::updateWidth()
{
    if(deltaMode)
      return;

    const QString t = lineEdit()->text();
    const QFontMetrics fm(font());
// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
    // Enough space for a lower limit to fit for example "C#-2" plus extra space for good luck.
    int minTextWidth = fm.horizontalAdvance("W#-2");
    // Enough space for an upper limit of say 7 'W' characters.
    int maxTextWidth = fm.horizontalAdvance("WWWWWWW");
    int textWidth = fm.horizontalAdvance(t);
#else
    // Enough space for a lower limit to fit for example "C#-2" plus extra space for good luck.
    int minTextWidth = fm.width("W#-2");
    // Enough space for an upper limit of say 7 'W' characters.
    int maxTextWidth = fm.width("WWWWWWW");
    int textWidth = fm.width(t);
#endif
    if(textWidth < minTextWidth)
      textWidth = minTextWidth;
    if(textWidth > maxTextWidth)
      textWidth = maxTextWidth;

    QStyle *s = style();
    QStyleOptionSpinBox opt;
    initStyleOption(&opt);

    // Get the up/down button sizes
    const QRect upRect   = s->subControlRect(QStyle::CC_SpinBox, &opt,
                                       QStyle::SC_SpinBoxUp, this);
    const QRect downRect = s->subControlRect(QStyle::CC_SpinBox, &opt,
                                       QStyle::SC_SpinBoxDown, this);

    int buttonWidth = qMax(upRect.width(), downRect.width());

    // Frame width on each side
    int frameWidth = s->pixelMetric(QStyle::PM_DefaultFrameWidth, &opt, this);

    int extraPadding = 6;     // breathing room for cursor

    int total =
        textWidth +
        buttonWidth +
        frameWidth * 2 +
        extraPadding;

    // Enforce dynamic width
    setFixedWidth(total);
    // setMinimumWidth(total);
}

//---------------------------------------------------------
//   mapValueToText
//---------------------------------------------------------

QString PitchEdit::textFromValue(int v) const
      {
      if (deltaMode) {
            QString s;
            s.setNum(v);
            return s;
            }
      else
            return MusECore::pitch2string(v);
            //// Support right-to-left text.
            //return MusECore::pitch2string(v, MusEGlobal::config.noteNameList.isRTLNoteList());
      }

QValidator::State PitchEdit::validate(QString &input, int &pos) const
{
    if (deltaMode)
      return SpinBox::validate(input, pos);

    if (input.isEmpty())
    {
        //fprintf(stderr, "PitchEdit::validate():input%s pos:%d\n", input.toLocal8Bit().constData(), pos);
        return QValidator::Intermediate;
    }
    return MusECore::validatePitch(input);
}

//---------------------------------------------------------
//   mapTextToValue
//---------------------------------------------------------

int PitchEdit::valueFromText(const QString &s) const
      {
      if (deltaMode)
            return s.toInt();
      else
            return MusECore::string2pitch(s);
}

//---------------------------------------------------------
//   setDeltaMode
//---------------------------------------------------------

void PitchEdit::setDeltaMode(bool val)
      {
      if(deltaMode == val)
        return;
      
      deltaMode = val;
      if (deltaMode)
            setRange(-127, 127);
      else
            setRange(0, 127);

      if(val)
      {
        // Remove enforced width constraints
        setMinimumWidth(0);
        setMaximumWidth(QWIDGETSIZE_MAX);

        // Restore the original size policy
        setSizePolicy(originalPolicy);

        updateGeometry();  // Tell the layout to recompute
        // adjustSize();   // Optional; usually not needed
      }
      else
      {
        updateWidth(); // Enforce initial width
      }
      }

void PitchEdit::midiNote(int pitch, int velo)
{
  // Ignore invalid pitches such as rest notes.
  if(pitch < 0)
    return;

  if (hasFocus() && velo)
    setValue(pitch);
}

} // namespace MusEGui
