//=========================================================
//  MusE
//  Linux Music Editor
//
//  line_edit.cpp
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

#include <stdio.h>

#include <QWidget>
#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>
#include <QStyle>
#include <QFont>
#include <QStyleOptionFrame>
#include <QRect>
#include <QString>
//#include <QStyleOption>
//#include <QTextLayout>

#include "line_edit.h"

namespace MusEGui {

//---------------------------------------------------------
//   LineEdit
//---------------------------------------------------------

LineEdit::LineEdit(QWidget* parent, const char* name) : QLineEdit(parent)
{
  setObjectName(name);
  //setAutoFillBackground(true);
  _enableStyleHack = true;
}

LineEdit::LineEdit(const QString& contents, QWidget* parent, const char* name)
 : QLineEdit(contents, parent)
{
  setObjectName(name);
  //setAutoFillBackground(true);
  _enableStyleHack = true;
}

void LineEdit::setEnableStyleHack(bool v)
{
  _enableStyleHack = v;
  update();
}

void LineEdit::paintEvent(QPaintEvent* ev)
{
  QLineEdit::paintEvent(ev);

  ev->accept();

  if(!_enableStyleHack)
    return;

  // --------------------------------------------
  // Special hack to force the frame to be drawn:
  // --------------------------------------------

  if(const QStyle* st = style())
  {
    st = st->proxy();

    QPainter p(this);

    QStyleOptionFrame o;
    initStyleOption(&o);

    // KDE BUG: Breeze and Oxygen IGNORE the drawFrame setting if the text font is
    //           high enough such that it thinks the text is too big.
    //          It draws nothing but a flat, square panel.
    //          We end up with NO mouse over or focus rectangles at all !
    // Force a small font size to fool the line edit panel into drawing a frame.
    QFont fnt(font());
    fnt.setPointSize(1); // Zero not allowed.
    o.fontMetrics = QFontMetrics(fnt);

    QRect s_r(rect());
    s_r.adjust(4, 4, -4, -4);

    QPainterPath inner_rect_path;
    inner_rect_path.addRect(s_r);
    QPainterPath fin_rect_path;
    fin_rect_path.addRect(rect());
    fin_rect_path -= inner_rect_path;

    p.setClipPath(fin_rect_path);

    p.fillRect(rect(), palette().window());
    st->drawPrimitive(QStyle::PE_PanelLineEdit, &o, &p);
  }
}


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

LineEdit::LineEdit(QWidget* parent, const char* name) : QWidget(parent)
{
  setObjectName(name);
  setAutoFillBackground(true);

  _drawFrame = true;
  _readOnly = false;

  _textLayout = new QTextLayout();
}

LineEdit::~LineEdit()
{
  delete _textLayout;
}

void LineEdit::initStyleOption(QStyleOptionFrame *option) const
{
  if(!option)
    return;
  option->initFrom(this);
  option->rect = contentsRect();
  option->lineWidth = _drawFrame ?
    style()->pixelMetric(QStyle::PM_DefaultFrameWidth, option, this) : 0;
  option->midLineWidth = 0;
  option->state |= QStyle::State_Sunken;
  if(isReadOnly())
    option->state |= QStyle::State_ReadOnly;
#ifdef QT_KEYPAD_NAVIGATION
  if(hasEditFocus())
    option->state |= QStyle::State_HasEditFocus;
#endif
  option->features = QStyleOptionFrame::None;
}

void LineEdit::setFrame(bool v)
{
  if(v == _drawFrame)
    return;
  _drawFrame = v;
  update();
}

void LineEdit::setReadOnly(bool v)
{
  if(v == _readOnly)
    return;
  _readOnly = v;
  update();
}

void LineEdit::setText(const QString& s)
{
  if(s == _text)
    return;
  _text = s;
  _textLayout->setText(_text);
  redoTextLayout();
  update();
}

int LineEdit::redoTextLayout() const
{
  _textLayout->clearLayout();
  _textLayout->beginLayout();
  QTextLine l = _textLayout->createLine();
  _textLayout->endLayout();
  return qRound(l.ascent());
}

void LineEdit::draw(QPainter *painter, const QPoint &offset, const QRect &clip, int flags)
{
    QVector<QTextLayout::FormatRange> selections;
    if (flags & DrawSelections) {
        QTextLayout::FormatRange o;
        if (m_selstart < m_selend) {
            o.start = m_selstart;
            o.length = m_selend - m_selstart;
            o.format.setBackground(m_palette.brush(QPalette::Highlight));
            o.format.setForeground(m_palette.brush(QPalette::HighlightedText));
        } else {
            // mask selection
            if (m_blinkStatus){
                o.start = m_cursor;
                o.length = 1;
                o.format.setBackground(m_palette.brush(QPalette::Text));
                o.format.setForeground(m_palette.brush(QPalette::Window));
            }
        }
        selections.append(o);
    }

    if (flags & DrawText)
        textLayout()->draw(painter, offset, selections, clip);

    if (flags & DrawCursor){
        int cursor = m_cursor;
        if (m_preeditCursor != -1)
            cursor += m_preeditCursor;
        if (!m_hideCursor && m_blinkStatus)
            textLayout()->drawCursor(painter, offset, cursor, m_cursorWidth);
    }
}

void LineEdit::paintEvent(QPaintEvent* ev)
{
  ev->accept();

  //fprintf(stderr, "LineEdit::paintEvent x:%d y:%d w:%d h:%d \n", ev->rect().x(), ev->rect().y(), ev->rect().width(), ev->rect().height());

  QPainter p(this);

  if(const QStyle* st = style())
  {
    st = st->proxy();

    QStyleOptionFrame o;
    initStyleOption(&o);
    // Qt BUG: Breeze and Oxygen IGNORE the drawFrame setting if the text font is
    //          high enough such that it thinks the text is too big.
    //         It draws nothing but a flat, square panel.
    //         We end up with NO mouse over or focus rectangles at all !
    // Force a small font size to fool the line edit panel into drawing a frame.
    QFont fnt(font());
    fnt.setPointSize(1); // Zero not allowed.
    o.fontMetrics = QFontMetrics(fnt);
    st->drawPrimitive(QStyle::PE_PanelLineEdit, &o, &p);

    QRect s_r = st->subElementRect(QStyle::SE_LineEditContents, &o, this);
    //int ml, mt, mr, mb;
    //getTextMargins(&ml, &mt, &mr, &mb);
    //s_r.adjust(ml, mt, -mr, -mb);

    o.fontMetrics = fontMetrics();
    // Force no frame.
    o.features = QStyleOptionFrame::None;
    o.frameShape = QFrame::NoFrame;
    o.lineWidth = 0;
    o.midLineWidth = 0;

    QRect b_r = st->subElementRect(QStyle::SE_LineEditContents, &o, this);
    //b_r.adjust(ml, mt, -mr, -mb);

    int x_offset = s_r.x() - b_r.x();
    int y_offset = s_r.y() - b_r.y();

    QRect clip_r(b_r);
    clip_r.adjust(-x_offset, -x_offset, -y_offset, -y_offset);

    QRect paint_r(o.rect);
    //paint_r.adjust(-x_offset, -x_offset, -y_offset, -y_offset);
    //paint_r.adjust(x_offset, -x_offset, y_offset, -y_offset);
    paint_r.adjust(6, -6, 6, -6);
    //QRect paint_r(s_r);

    //p.setClipRect(clip_r);

    // Done with this painter.
    //~p();

    QPaintEvent paint_ev(paint_r);

    QLineEdit::paintEvent(&paint_ev);

  }
}
*/

} // namespace MusEGui
