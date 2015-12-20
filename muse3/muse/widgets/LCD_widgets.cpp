//=========================================================
//  MusE
//  Linux Music Editor
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  LCD_widgets.cpp
//  (C) Copyright 2015 Tim E. Real (terminator356 on sourceforge)
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


// #include <stdio.h>  // REMOVE Tim. For messages.

#include <QPainter>
#include <QRect>
#include <QString>

#include "LCD_widgets.h"

namespace MusEGui {

// Segment numbers:
//        5
//      *****
//      *   *
//    4 *   * 0
//      * 6 *
//      *****
//      *   *
//    3 *   * 1
//      *   *
//      *****
//        2
//

static const bool _LCD_DigitSegments[10][7] = { 
  { true, true, true, true, true, true, false},      // 0
  { true, true, false, false, false, false, false},  // 1
  { true, false, true, true, false, true, true},     // 2
  { true, true, true, false, false, true, true},     // 3
  { true, true, false, false, true, false, true},    // 4
  { false, true, true, false, true, true, true},     // 5
  { false, true, true, true, true, true, true},      // 6
  { true, true, false, false, false, true, false},   // 7
  { true, true, true, true, true, true, true},       // 8
  { true, true, true, false, true, true, true}       // 9
};  
  
LCDPainter::LCDPainter()
{
  
}

// Static.  
void LCDPainter::drawSegment(QPainter* painter, const QRect& characterRect, int segment)
{
  const int left   = characterRect.x();
  const int right  = characterRect.x() + characterRect.width() - 1;
  const int top    = characterRect.y();
  const int bottom = characterRect.y() + characterRect.height() - 1;
  const int half   = characterRect.y() + characterRect.height() / 2 - 1;
  int x1, x2, y1, y2;
  switch(segment)
  {
    case 0:
      x1 = right;
      x2 = right;
      y1 = top;
      y2 = half;
    break;

    case 1:
      x1 = right;
      x2 = right;
      y1 = half;
      y2 = bottom;
    break;

    case 2:
      x1 = left;
      x2 = right;
      y1 = bottom;
      y2 = bottom;
    break;

    case 3:
      x1 = left;
      x2 = left;
      y1 = half;
      y2 = bottom;
    break;

    case 4:
      x1 = left;
      x2 = left;
      y1 = top;
      y2 = half;
    break;

    case 5:
      x1 = left;
      x2 = right;
      y1 = top;
      y2 = top;
    break;

    case 6:
      x1 = left;
      x2 = right;
      y1 = half;
      y2 = half;
    break;
    
    default:
      return;
  }
  painter->drawLine(x1, y1, x2, y2);
}
  
// Static.  
void LCDPainter::drawDigit(QPainter* painter, const QRect& rect, char ascii_char)
{
  if(ascii_char < 0x30 || ascii_char >= 39)
    return;
  const int idx = ascii_char - 0x30;
  const bool* segs = _LCD_DigitSegments[idx];
  for(int i = 0; i < 7; ++i)
  {
    if(segs[i])
      drawSegment(painter, rect, i);
  }
}
  
// Static.  
void LCDPainter::drawText(QPainter* /*painter*/, const QRect& /*rect*/, const QString& text)
{
  if(text.isEmpty())
    return;
//   const int sz = text.size();
//   
//   int x = rect.x();
//   
//   for(int idx = 0; idx < sz; ++idx)
//   {
//     
//   }
}
  
} // namespace MusEGui
