//=========================================================
//  MusE
//  Linux Music Editor
//
//  line_edit.h
//    (C) Copyright 2017 Tim E. Real (terminator356 on sourceforge)
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

#ifndef __LINE_EDIT_H__
#define __LINE_EDIT_H__

//#include <QWidget>
#include <QLineEdit>

class QPaintEvent;
// class QStyleOptionFrame;
// class QString;
// class QTextLayout;

namespace MusEGui {

//---------------------------------------------------------
//   LineEdit
//---------------------------------------------------------

class LineEdit : public QLineEdit
{
  Q_OBJECT
  bool _enableStyleHack;

  protected:
    virtual void paintEvent(QPaintEvent*);
//     virtual void keyPressEvent(QKeyEvent*);

  public:
    LineEdit(QWidget* parent = 0, const char* name = 0);
    LineEdit(const QString& contents, QWidget* parent = 0, const char* name = 0);

    bool enableStyleHack() const { return _enableStyleHack; }
    void setEnableStyleHack(bool);
};


//======================================================
//    TODO: Work in progress...
//    Since KDE Breeze and Oxygen are unlikely to change
//     the ignore-frame-if-too-small behaviour, muse really
//     needs its own line edit widget once and for all.
//    But it's a very complex and difficult control to make.
//======================================================

/*
//---------------------------------------------------------
//   LineEdit
//---------------------------------------------------------

class LineEdit : public QWidget
{
      Q_OBJECT

  public:
      enum DrawFlags { DrawText = 0x01, DrawSelections = 0x02,
                       DrawCursor = 0x04, DrawAll = DrawText | DrawSelections | DrawCursor };

  private:
      bool _drawFrame;
      bool _readOnly;

      QString _text;
      QTextLayout* _textLayout;

      // Updates the internal text layout. Returns the ascent of the
      //  created QTextLine.
      int redoTextLayout() const;
      void draw(QPainter* painter, const QPoint& offset, const QRect &clip, int flags);

      virtual void paintEvent(QPaintEvent*);

   protected:
      void initStyleOption(QStyleOptionFrame*) const;

   public:
      LineEdit(QWidget*, const char* name=0);
      virtual ~LineEdit();

      bool hasFrame() const { return _drawFrame; }
      void setFrame(bool);
      bool isReadOnly() const { return _readOnly; }
      void setReadOnly(bool);
      QString text() const { return _text; }
      QString setText();
};
*/


} // namespace MusEGui

#endif
