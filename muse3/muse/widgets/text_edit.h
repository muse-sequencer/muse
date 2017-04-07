//=========================================================
//  MusE
//  Linux Music Editor
//
//  text_edit.h
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

#ifndef __TEXT_EDIT_H__
#define __TEXT_EDIT_H__

#include <QPlainTextEdit>

class QMouseEvent;

namespace MusEGui {

//---------------------------------------------------------
//   TextEdit
//---------------------------------------------------------

class TextEdit : public QPlainTextEdit
{
  Q_OBJECT

  private:
    bool _doubleClickFocus;

  protected:

    virtual void mouseDoubleClickEvent(QMouseEvent*);

  public:
    TextEdit(QWidget* parent = 0, const char* name = 0);
    TextEdit(const QString& txt, QWidget* parent = 0, const char* name = 0);

    bool doubleClickFocus() const { return _doubleClickFocus; }
    void setDoubleClickFocus(bool v) { _doubleClickFocus = v; }

    QString text() const { return toPlainText(); }
    virtual void setText(const QString& s) { setPlainText(s); }

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const { return sizeHint(); }
};


} // namespace MusEGui

#endif
