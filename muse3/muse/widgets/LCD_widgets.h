//=========================================================
//  MusE
//  Linux Music Editor
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  LCD_widgets.h
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

#ifndef __LCD_WIDGETS_H__
#define __LCD_WIDGETS_H__

//#include <QDoubleSpinBox>

// #include "sclif.h"
// #include "sliderbase.h"
// #include "scldraw.h"

class QPainter;
class QRect;
class QString;

namespace MusEGui {

// //---------------------------------------------------------
// //   PopupDoubleSpinBox
// //---------------------------------------------------------
// 
// class PopupDoubleSpinBox : public QDoubleSpinBox {
//   Q_OBJECT
// 
//   private:
//     bool _closePending;
//     
//   protected:
// //       virtual void keyPressEvent(QKeyEvent*);
// //       virtual void wheelEvent(QWheelEvent*);
// //       virtual void focusOutEvent(QFocusEvent*);
//     //virtual void paintEvent(QPaintEvent*);
//     virtual bool event(QEvent*);
//     
//   signals:
// //       void doubleClicked();
// //       void ctrlDoubleClicked();
// //       //void ctrlClicked();
//     void returnPressed();
//     void escapePressed();
// 
//   public:
//     PopupDoubleSpinBox(QWidget* parent=0);
// };
  
//---------------------------------------------------------
//   LCDPainter
//---------------------------------------------------------

class LCDPainter
{
  public:
      enum ScalePos { None, Left, Right, Top, Bottom, Embedded };
      enum TextHighlightMode { TextHighlightNone, TextHighlightAlways, TextHighlightSplit, TextHighlightShadow };

  private:
    bool _digitSegments[10][7];
    
  public:
    LCDPainter();
    
    static void drawSegment(QPainter* painter, const QRect& characterRect, int segment);
    static void drawDigit(QPainter* painter, const QRect& rect, char asciiChar);
    static void drawText(QPainter* painter, const QRect& rect, const QString& text);
};

} // namespace MusEGui

#endif
