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

#include <math.h>

#include <QPainter>
#include <QPaintEvent>
#include <QFontMetrics>

#include "elided_label.h"

namespace MusEGui {

ElidedLabel::ElidedLabel(QWidget* parent, 
                         Qt::TextElideMode elideMode, 
                         //int maxFontPoint, 
                         int minFontPoint,
                         bool ignoreHeight, bool ignoreWidth,
                         const QString& text, 
                         Qt::WindowFlags flags)
    : QFrame(parent, flags), 
    _elideMode(elideMode), 
    //_fontPointMax(maxFontPoint),
    _fontPointMin(minFontPoint), 
    _fontIgnoreHeight(ignoreHeight),
    _fontIgnoreWidth(ignoreWidth),
    _text(text) 
{
  _id = -1;
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
//   updateSizeHint();
  _curFont = font();
  autoAdjustFontSize();
}
  
void ElidedLabel::paintEvent(QPaintEvent* e)
{
  QFrame::paintEvent(e);
  if(rect().width() <= 0 || rect().height() <= 0)
    return;
  QPainter painter(this);
  painter.setFont(_curFont);
  QFontMetrics fm = painter.fontMetrics();
  QString elidedText = fm.elidedText(_text, _elideMode, width());
//   painter.drawText(QPoint(0, fm.ascent()), elidedText);
  painter.drawText(rect(), Qt::AlignLeft | Qt::AlignVCenter, elidedText);
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

} // namespace MusEGui
