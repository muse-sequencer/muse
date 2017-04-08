//=========================================================
//  MusE
//  Linux Music Editor
//
//  text_edit.cpp
//  (C) Copyright 2017 Tim E. Real (terminator356 on sourceforge)
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

#include <QString>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QResizeEvent>

#include "text_edit.h"

// For debugging output: Uncomment the fprintf section.
#define DEBUG_TEXT_EDIT(dev, format, args...) // fprintf(dev, format, ##args);

namespace MusEGui {

TextEdit::TextEdit(QWidget* parent, const char* name)
 : QPlainTextEdit(parent)
{
  setObjectName(name);
}

TextEdit::TextEdit(const QString& txt, QWidget* parent, const char* name)
 : QPlainTextEdit(txt, parent)
{
  setObjectName(name);
}

void TextEdit::mouseDoubleClickEvent(QMouseEvent* ev)
{
  if(!hasFocus())
  {
//     ev->accept();
    setFocus();
//     return;
  }
  ev->ignore();
  QPlainTextEdit::mouseDoubleClickEvent(ev);
}

QSize TextEdit::sizeHint() const
{
  const int w = QPlainTextEdit::sizeHint().width();
  const int lines = document()->lineCount();
  const int fh = fontMetrics().lineSpacing();
  const int marg = contentsMargins().bottom() + contentsMargins().top() +
                   //viewportMargins().bottom() + viewportMargins().top() +  // Only in Qt 5.5
                   viewport()->contentsMargins().bottom() + viewport()->contentsMargins().top();
  const int h = fh * lines + marg + 6; // Extra for some kind of voodoo added deep down, can't find it.
  return QSize(w, h);
}

} // namespace MusEGui
