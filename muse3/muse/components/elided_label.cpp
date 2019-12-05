//=========================================================
//  MusE
//  Linux Music Editor
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  elided_label.cpp
//  (C) Copyright 2015-2016 Tim E. Real (terminator356 on sourceforge)
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

#include "muse_math.h"

#include <QPainter>
#include <QPaintEvent>
#include <QFontMetrics>
#include <QSize>

#include "background_painter.h"
#include "elided_label.h"

namespace MusEGui {

ElidedLabel::ElidedLabel(QWidget* parent, 
                         Qt::TextElideMode elideMode,
                         Qt::Alignment alignment,
                         //int maxFontPoint, 
                         int minFontPoint,
                         bool ignoreHeight, bool ignoreWidth,
                         const QString& text,
                         const char* name,
                         Qt::WindowFlags flags)
    : QFrame(parent, flags), 
    _elideMode(elideMode),
    //_fontPointMax(maxFontPoint),
    _fontPointMin(minFontPoint), 
    _fontIgnoreHeight(ignoreHeight),
    _fontIgnoreWidth(ignoreWidth),
    _text(text) 
{
  setObjectName(name);
  setMouseTracking(true);
  setEnabled(true);
  setFocusPolicy(Qt::StrongFocus);

  //setAutoFillBackground(false);
  //setAttribute(Qt::WA_NoSystemBackground);
  //setAttribute(Qt::WA_StaticContents);
  // This is absolutely required for speed! Otherwise painfully slow because of full background
  //  filling, even when requesting small udpdates! Background is drawn by us.
  //setAttribute(Qt::WA_OpaquePaintEvent);

  _id = -1;
  _hasOffMode = false;
  _off = false;
  _hovered = false;

  _alignment = alignment;

  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
//   updateSizeHint();
  _curFont = font();
  autoAdjustFontSize();
}

void ElidedLabel::keyPressEvent(QKeyEvent* e)
{
  switch (e->key())
  {
    case Qt::Key_Escape:
      // Don't let ancestor grab it, let it pass up the chain.
      e->ignore();
      return;
    break;

    case Qt::Key_Enter:
    case Qt::Key_Return:
      e->accept();
      emit returnPressed(pos(), _id, e->modifiers());
      return;
    break;

    default:
    break;
  }

  e->ignore();
  return QFrame::keyPressEvent(e);
}

void ElidedLabel::setOff(bool v)
{
  if(v && !_hasOffMode)
    _hasOffMode = true;
  if(_off == v)
    return;
  _off = v;
  update();
  //emit valueStateChanged(value(), isOff(), id(), d_scrollMode);
}

void ElidedLabel::setHasOffMode(bool v)
{
  _hasOffMode = v;
  setOff(false);
}

void ElidedLabel::paintEvent(QPaintEvent* e)
{
  QFrame::paintEvent(e);
  if(rect().width() <= 0 || rect().height() <= 0)
    return;
  QPainter painter(this);

  const QRect r = rect();
  const QRect ar = r.adjusted(1, 1, -1, -1);

  ItemBackgroundPainter ibp;
  ibp.drawBackground(&painter, r, palette(), 1, 1, !hasOffMode() || !isOff() ? r : QRect());

  if (hasFocus())
        {
        if (_hovered)
              painter.setPen(QPen(QColor(239,239,239)));
        else
              painter.setPen(QPen(Qt::white));
        }
  else if (_hovered)
        painter.setPen(QPen(QColor(48,48,48)));
  else
        painter.setPen(QPen(Qt::black));

  painter.setRenderHint(QPainter::Antialiasing);
//   painter.setFont(_curFont);
//   QFontMetrics fm = painter.fontMetrics();
  //painter.setFont(font());
  QFontMetrics fm = fontMetrics();
  QString elidedText = fm.elidedText(_text, _elideMode, r.width());
//   painter.drawText(QPoint(0, fm.ascent()), elidedText);

  painter.drawText(ar, _alignment, elidedText);
}
  
//---------------------------------------------------------
//   autoAdjustFontSize
//   w: Widget to auto adjust font size
//   s: String to fit
//   ignoreWidth: Set if dealing with a vertically constrained widget - one which is free to resize horizontally.
//   ignoreHeight: Set if dealing with a horizontally constrained widget - one which is free to resize vertically. 
//---------------------------------------------------------

bool ElidedLabel::autoAdjustFontSize()
{
// FIXME: Disabled for now, the font modulates back and forth, not very good ATM.
//        May have to revert to the font-checking iteration loop scheme.
  
//   QFont fnt = font(); // This is the maximum font.
//   int max = fnt.pointSize();
//   int min = _fontPointMin;
//   
//   // In case the max or min was obtained from QFont::pointSize() which returns -1 
//   //  if the font is a pixel font, or if min is greater than max...
//   // Limit the minimum and maximum sizes to something at least readable.
//   if(max < 4)
//     max = 4;
//   if(min < 4)
//     min = 4;
//   if(max < min)
//     max = min;
//     
//   //qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());
//   //QRectF r = boundingRect();
//   QRectF r = rect();
//   //QFont f = painter->font();
//   
//   
//   //if(ignoreWidth || req_w == 0) // Also avoid divide by zero below.
//   if(_fontIgnoreWidth || _text.isEmpty()) // Also avoid divide by zero below.
//   {
//     if(fnt.pointSize() != max)
//     {
//       fnt.setPointSize(max);
// //       setFont(fnt);
//       _curFont = fnt;
//       update();
//     }
//   }
//   else
//   {
//     //qreal aspectRatio = painter->fontMetrics().lineSpacing() / painter->fontMetrics().averageCharWidth();
//     qreal aspectRatio = fontMetrics().lineSpacing() / fontMetrics().averageCharWidth();
// //     int pixelsize = sqrt(r.width() * r.height() / aspectRatio / (_text.length() * 3)) * aspectRatio;
//     int pixelsize = sqrt(r.width() * r.height() / aspectRatio / _text.length()) * aspectRatio;
//     fnt.setPixelSize(pixelsize);
//     //int flags = Qt::AlignCenter|Qt::TextDontClip|Qt::TextWordWrap;
//     int flags = Qt::AlignCenter;
//     //if ((pixelsize * lod) < 13)
//     //    flags |= Qt::TextWrapAnywhere;
//     QFontMetricsF fmf(fnt);
//     QRectF tbr = fmf.boundingRect(r, flags, _text);
//     pixelsize = fnt.pixelSize() * qMin(r.width() * 0.95 / tbr.width(), r.height() * 0.95 / tbr.height());
// //     if(pixelsize < min)
// //       pixelsize = min;
// //     else if(pixelsize > max)
// //       pixelsize = max;
//     fnt.setPixelSize(pixelsize);
//     const QFontInfo fi(fnt);
//     const int pointsize = fi.pointSize();
//     if(pointsize <= min)
//       fnt.setPointSize(min);
//     else if(pointsize >= max)
//       fnt.setPointSize(max);
// //     setFont(fnt);
//     _curFont = fnt;
//     //painter->drawText(r,flags,stitle);
    update();
//   }
  
  // Force minimum height. Use the expected height for the highest given point size.
  // This way the mixer strips aren't all different label heights, but can be larger if necessary.
  // Only if ignoreHeight is set (therefore the height is adjustable).
  if(_fontIgnoreHeight)
  {
// FIXME Disabled for now, as per above.
//     fnt.setPointSize(max);
//     const QFontMetrics fm(fnt);
    const QFontMetrics fm(font());
    
    // Set the label's minimum height equal to the height of the font.
    setMinimumHeight(fm.height() + 2 * frameWidth());
  }
  
  return true;  
}

void ElidedLabel::setText(const QString& txt) 
{ 
  if(_text == txt)
    return;
  _text = txt; 
  autoAdjustFontSize();
}

void ElidedLabel::resizeEvent(QResizeEvent* e)
{
  e->ignore();
  QFrame::resizeEvent(e);
  autoAdjustFontSize();
}

void ElidedLabel::mousePressEvent(QMouseEvent* e)
{
  e->accept();
  emit pressed(e->pos(), _id, e->buttons(), e->modifiers());
}

void ElidedLabel::mouseReleaseEvent(QMouseEvent* e)
{
  e->accept();
  emit released(e->pos(), _id, e->buttons(), e->modifiers());
}

void ElidedLabel::leaveEvent(QEvent *e)
{
  if(_hovered)
  {
    _hovered = false;
    update();
  }
  e->ignore();
  QFrame::leaveEvent(e);
}

void ElidedLabel::mouseMoveEvent(QMouseEvent *e)
{
  //e->ignore();
  //QFrame::mouseMoveEvent(e);
  e->accept();
  if(!_hovered)
  {
    _hovered = true;
    update();
  }
}

void ElidedLabel::mouseDoubleClickEvent(QMouseEvent* ev)
{
  ev->accept();
  emit doubleClicked();
}

void ElidedLabel::setFontIgnoreDimensions(bool ignoreHeight, bool ignoreWidth)
{
  _fontIgnoreWidth = ignoreWidth;
  _fontIgnoreHeight = ignoreHeight;
  autoAdjustFontSize();
}

void ElidedLabel::setFontPointMin(int point)
{
  _fontPointMin = point;
  autoAdjustFontSize();
}

QSize ElidedLabel::sizeHint() const
{
// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
  QSize sz(fontMetrics().horizontalAdvance(_text) + 8, fontMetrics().height() + 4);
#else
  QSize sz(fontMetrics().width(_text) + 8, fontMetrics().height() + 4);
#endif
  return sz;
}



// ==============================================================================



ElidedTextLabel::ElidedTextLabel(QWidget* parent,
                         const char* name,
                         Qt::WindowFlags flags
                         )
    : QFrame(parent, flags)
{
  setObjectName(name);
  setMouseTracking(true);
  setEnabled(true);
  setFocusPolicy(Qt::StrongFocus);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

  //setAutoFillBackground(false);
  //setAttribute(Qt::WA_NoSystemBackground);
  //setAttribute(Qt::WA_StaticContents);
  // This is absolutely required for speed! Otherwise painfully slow because of full background
  //  filling, even when requesting small udpdates! Background is drawn by us.
  //setAttribute(Qt::WA_OpaquePaintEvent);

  _elideMode = Qt::ElideNone;
  _alignment = Qt::AlignLeft | Qt::AlignVCenter;
  _id = -1;
  _hasOffMode = false;
  _off = false;
  _hovered = false;
}

ElidedTextLabel::ElidedTextLabel(const QString& text,
                         QWidget* parent, 
                         const char* name,
                         Qt::WindowFlags flags)
    : QFrame(parent, flags), 
    _text(text) 
{
  setObjectName(name);
  setMouseTracking(true);
  setEnabled(true);
  setFocusPolicy(Qt::StrongFocus);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

  //setAutoFillBackground(false);
  //setAttribute(Qt::WA_NoSystemBackground);
  //setAttribute(Qt::WA_StaticContents);
  // This is absolutely required for speed! Otherwise painfully slow because of full background
  //  filling, even when requesting small udpdates! Background is drawn by us.
  //setAttribute(Qt::WA_OpaquePaintEvent);

  _elideMode = Qt::ElideNone;
  _alignment = Qt::AlignLeft | Qt::AlignVCenter;
  _id = -1;
  _hasOffMode = false;
  _off = false;
  _hovered = false;
  
  setToolTip(_text);
}

void ElidedTextLabel::keyPressEvent(QKeyEvent* e)
{
  switch (e->key())
  {
    case Qt::Key_Escape:
      // Don't let ancestor grab it, let it pass up the chain.
      e->ignore();
      return;
    break;

    case Qt::Key_Enter:
    case Qt::Key_Return:
      e->accept();
      emit returnPressed(pos(), _id, e->modifiers());
      return;
    break;

    default:
    break;
  }

  e->ignore();
  return QFrame::keyPressEvent(e);
}

void ElidedTextLabel::setOff(bool v)
{
  if(v && !_hasOffMode)
    _hasOffMode = true;
  if(_off == v)
    return;
  _off = v;
  update();
  //emit valueStateChanged(value(), isOff(), id(), d_scrollMode);
}

void ElidedTextLabel::setHasOffMode(bool v)
{
  _hasOffMode = v;
  setOff(false);
}

void ElidedTextLabel::paintEvent(QPaintEvent* ev)
{
  QFrame::paintEvent(ev);
  ev->accept();

  if(rect().width() <= 0 || rect().height() <= 0)
    return;

  QPainter p(this);

  p.save();
  const QRect r = rect();
  const QRect ar = r.adjusted(1, 1, -1, -1);

  if (hasFocus())
        {
        if (_hovered)
              p.setPen(QPen(QColor(239,239,239)));
        else
              p.setPen(QPen(Qt::white));
        }
  else if (_hovered)
        p.setPen(QPen(QColor(48,48,48)));
  else
        p.setPen(QPen(Qt::black));

  p.setRenderHint(QPainter::Antialiasing);
  QFontMetrics fm = p.fontMetrics();
  QString elidedText = fm.elidedText(_text, _elideMode, r.width());
//   painter.drawText(QPoint(0, fm.ascent()), elidedText);

  p.drawText(ar, _alignment, elidedText);
  p.restore();
}
  
void ElidedTextLabel::setText(const QString& txt) 
{ 
  if(_text == txt)
    return;
  _text = txt; 
  setToolTip(_text);
}

void ElidedTextLabel::leaveEvent(QEvent *e)
{
  if(_hovered)
  {
    _hovered = false;
    update();
  }
  e->ignore();
  QFrame::leaveEvent(e);
}

void ElidedTextLabel::mouseMoveEvent(QMouseEvent *e)
{
  e->ignore();
  QFrame::mouseMoveEvent(e);
//   e->accept();
  if(!_hovered)
  {
    _hovered = true;
    update();
  }
}

QSize ElidedTextLabel::sizeHint() const
{
// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
  QSize sz(fontMetrics().horizontalAdvance(_text) + 8, fontMetrics().height() + 4);
#else
  QSize sz(fontMetrics().width(_text) + 8, fontMetrics().height() + 4);
#endif
  return sz;
}

} // namespace MusEGui
