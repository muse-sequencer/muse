//=========================================================
//  MusE
//  Linux Music Editor
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  compact_slider.cpp
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

#include <stdio.h>  // REMOVE Tim. For messages.

#include <cmath>
#include "mmath.h"

// #include <QApplication>
// #include <QDesktopWidget>
#include <QPainter>
#include <QPainterPath>
// #include <QStyle>
// #include <QStyleOptionFrame>
#include <QResizeEvent>
#include <QLocale>
#include <QEvent>
// #include <QCursor>

#include "utils.h"
#include "compact_slider.h"
#include "slider.h"
#include "icons.h"

namespace MusEGui {

//---------------------------------------------------------
//   PopupDoubleSpinBox
//---------------------------------------------------------

PopupDoubleSpinBox::PopupDoubleSpinBox(QWidget* parent)
   : QDoubleSpinBox(parent)
{
  // Reset these since our parent will typically turn them on for speed.
  setAutoFillBackground(true);
  setAttribute(Qt::WA_NoSystemBackground, false);
  setAttribute(Qt::WA_StaticContents, false);
  setAttribute(Qt::WA_OpaquePaintEvent, false);    

  _closePending = false;
  
//   DoubleSpinBoxLineEdit* le = new DoubleSpinBoxLineEdit(this);
//   setLineEdit(le);
//   setKeyboardTracking(false);
//   
//   connect(le, SIGNAL(doubleClicked()),     this, SIGNAL(doubleClicked()));
//   connect(le, SIGNAL(ctrlDoubleClicked()), this, SIGNAL(ctrlDoubleClicked()));
//   //connect(le, SIGNAL(ctrlClicked()), this, SIGNAL(ctrlClicked()));
}

bool PopupDoubleSpinBox::event(QEvent* e)
{
  switch(e->type())
  {
    case QEvent::KeyPress:
    {
      QKeyEvent* ke = static_cast<QKeyEvent*>(e);
      switch(ke->key()) 
      {
        // For return, we want to close the editor but don't want the
        //  parent to receive the event which will just open the box again.
        case Qt::Key_Return:
        case Qt::Key_Enter:
          //e->ignore();
//           QDoubleSpinBox::event(e);
          e->accept();
          //emit editingFinished();
          if(!_closePending)
          {
           _closePending = true;
            emit returnPressed();
          }
//           // Will emit editingFinished.
//           deleteLater();
          return true;
        break;
        
        case Qt::Key_Escape:
        {
          e->accept();
          //emit editingFinished();
          if(!_closePending)
          {
           _closePending = true;
            emit escapePressed();
          }
//           // Will emit editingFinished.
//           deleteLater();
          return true;
        }
        break;
        
        default:
        break;
      }
    }
    break;
    
    case QEvent::NonClientAreaMouseButtonPress:
      // FIXME: Doesn't work.
      fprintf(stderr, "PopupDoubleSpinBox::event NonClientAreaMouseButtonPress\n"); // REMOVE Tim. Trackinfo.
    case QEvent::FocusOut:
      e->accept();
      if(!_closePending)
      {
        _closePending = true;
        emit escapePressed();
      }
      return true;
    break;
    
    default:
    break;
  }
  
  // Do not pass ANY events on to the parent.
  QDoubleSpinBox::event(e);
  e->accept();
  return true;
}

// void PopupDoubleSpinBox::keyPressEvent(QKeyEvent* e)
// {
// //     switch (e->key()) {
// //       // For return, we want to close the editor but don't want the
// //       //  parent to receive the event which will just open the box again.
// //       case Qt::Key_Return:
// //       case Qt::Key_Escape:
// //         e->accept();
// //         //emit editingFinished(); // Already emitted
// //         return;
// //       break;
// //       default:
// //       break;
// //     }
// //     e->ignore();
//     
//     // Do not pass ANY events on to the parent.
//     e->accept();
//     QDoubleSpinBox::keyPressEvent(e);
// }

// void PopupDoubleSpinBox::wheelEvent(QWheelEvent* e)
// {
//   QDoubleSpinBox::wheelEvent(e);
//   // Need this because Qt doesn't deselect the text if not focused.
//   if(!hasFocus() && lineEdit())
//     lineEdit()->deselect();
// }

// void PopupDoubleSpinBox::focusOutEvent(QFocusEvent*)
// {
//   emit editingFinished();
// }
  
  
  
//-------------------------------------------------------------
//  Slider - The Slider Widget
//
//  Slider is a slider widget which operates on an interval
//  of type double. Slider supports different layouts as
//  well as a scale.
//------------------------------------------------------------

//------------------------------------------------------------
//.F  CompactSlider::Slider
//
//    Constructor
//
//.u  Syntax:
//.f  CompactSlider::Slider(QWidget *parent, const char *name, Orientation orient = Horizontal, ScalePos scalePos = None, int bgStyle = BgTrough)
//
//.u  Parameters
//.p
//  QWidget *parent --  parent widget
//  const char *name -- The Widget's name. Default = 0.
//  Orientation Orient -- Orientation of the slider. Can be CompactSlider::Horizontal
//        or CompactSlider::Vertical.
//                    Defaults to Horizontal.
//  ScalePos scalePos --  Position of the scale.  Can be CompactSlider::None,
//        CompactSlider::Left, CompactSlider::Right, CompactSlider::Top,
//        or CompactSlider::Bottom. Defaults to CompactSlider::None.  !!! CURRENTLY only CompactSlider::None supported - oget 20110913
//  QColor fillcolor -- the color used to fill in the full side
//        of the Slider
//------------------------------------------------------------

CompactSlider::CompactSlider(QWidget *parent, const char *name,
               Qt::Orientation orient, ScalePos scalePos, 
               const QString& labelText, 
               const QString& valPrefix, 
               const QString& valSuffix,
               const QString& specialValueText, 
               QColor thumbColor)
               : SliderBase(parent,name)
      {
      if(objectName().isEmpty())
        setObjectName(QStringLiteral("CompactSlider"));

      setMouseTracking(true);
      setEnabled(true);
      setFocusPolicy(Qt::WheelFocus);
      
      setAutoFillBackground(false);
      setAttribute(Qt::WA_NoSystemBackground);
      //setAttribute(Qt::WA_StaticContents);
      // This is absolutely required for speed! Otherwise painfully slow because of full background 
      //  filling, even when requesting small udpdates! Background is drawn by us.
      setAttribute(Qt::WA_OpaquePaintEvent);    

      setBorderlessMouse(true);
      
      _onPath = 0;
      _offPath = 0;
      _editor = 0;
      _editMode = false;
      
//       const QPalette& pal = palette();
      //d_fillColor = fillColor;
//       d_fillColor = pal.window().color();
//       d_marginColor = pal.mid().color();
//       d_textColor= pal.text().color();
//       d_barColor= pal.highlight().color();
//       d_thumbColor= pal.midlight().color();
//       d_thumbColor= QColor(255, 255, 0);
      d_thumbColor= thumbColor;
        
      d_labelText = labelText;
      d_valPrefix = valPrefix;
      d_valSuffix = valSuffix;
      d_specialValueText = specialValueText;
      _textHighlightMode = TextHighlightSplit;
      _valueDecimals = 2;
      _off = false;
      d_offText = tr("off");

      _detectThumb = false;
      _autoHideThumb = true;
      _hasOffMode = false;
//       d_thumbLength = 7;
      d_thumbLength = 0;
//       d_thumbHitLength = d_thumbLength;
//       d_thumbHitLength = 7;
      d_thumbHitLength = 0;
      d_thumbHalf = d_thumbLength / 2;
      d_thumbWidth = 16;
      d_thumbWidthMargin = 0;
      _mouseOverThumb = false;
      _hovered = false;

      d_scaleDist   = 4;
      d_scaleStep   = 0.0;
      d_scalePos    = scalePos;
      d_xMargin     = 1;
      d_yMargin     = 1;
      d_mMargin    = 1;

      _entered = false;
//       bPressed      = false;
      
//       d_sliderRect.setRect(0, 0, 8, 8);

      setOrientation(orient);
      getActiveArea();
      d_valuePixel = 0;
      d_valuePixelWidth = 0;
      getPixelValues();
      updatePainterPaths();

      connect(this, SIGNAL(sliderPressed(int)), this, SLOT(processSliderPressed(int)));
      connect(this, SIGNAL(sliderReleased(int)), this, SLOT(processSliderReleased(int)));
      }

// Static.      
QSize CompactSlider::getMinimumSizeHint(const QFontMetrics& fm, 
                                        Qt::Orientation orient, 
                                        ScalePos /*scalePos*/, 
                                        int /*xMargin*/, 
                                        int yMargin)
{
  const int font_height = fm.height();
  switch(orient) {
        case Qt::Vertical:
              return QSize(16, font_height + 2 * yMargin);
              break;
        case Qt::Horizontal:
              return QSize(16, font_height + 2 * yMargin);
              break;
        }
  return QSize(10, 10);
}
      
void CompactSlider::processSliderPressed(int)
{
   //sliderCurrent = &sliderPngPressed;
//    bPressed = true;
   update();
}

void CompactSlider::processSliderReleased(int)
{
  //sliderCurrent = &sliderPng;
//   bPressed = false;
//    if(!_entered)
//      _mouseOverThumb = false;
  QPoint p = mapFromGlobal(QCursor::pos());
  getMouseOverThumb(p);
  
  update();
  // SliderBase::buttonReleased emits valueChanged. Emit our own combined signal.
//   if((!tracking()) || (value() != prevValue()))
  if((!tracking()) || valHasChanged())
    emit valueStateChanged(value(), isOff(), id());
}

//------------------------------------------------------------
//.F  CompactSlider::~Slider
//    Destructor
//.u  Syntax
//.f  CompactSlider::~Slider()
//------------------------------------------------------------

CompactSlider::~CompactSlider()
      {
        if(_onPath)
          delete _onPath;
        if(_offPath)
          delete _offPath;
      }

//----------------------------------------------------
//
//.F  CompactSlider::setThumbLength
//
//    Set the slider's thumb length
//
//.u  Syntax
//  void CompactSlider::setThumbLength(int l)
//
//.u  Parameters
//.p  int l   --    new length
//
//-----------------------------------------------------
void CompactSlider::setThumbLength(int l)
{
    d_thumbLength = MusECore::qwtMax(l,8);
    d_thumbHalf = d_thumbLength / 2;
    resize(this->size());
}

//------------------------------------------------------------
//
//.F  CompactSlider::setThumbWidth
//  Change the width of the thumb
//
//.u  Syntax
//.p  void CompactSlider::setThumbWidth(int w)
//
//.u  Parameters
//.p  int w -- new width
//
//------------------------------------------------------------
void CompactSlider::setThumbWidth(int w)
{
    d_thumbWidth = MusECore::qwtMax(w,4);
    resize(this->size());
}


//------------------------------------------------------------
//.-  
//.F  CompactSlider::scaleChange
//  Notify changed scale
//
//.u  Syntax
//.f  void CompactSlider::scaleChange()
//
//.u  Description
//  Called by QwtScaledWidget
//
//------------------------------------------------------------
void CompactSlider::scaleChange()
{
    if (!hasUserScale())
       d_scale.setScale(minValue(), maxValue(), d_maxMajor, d_maxMinor);
    update();
}


//------------------------------------------------------------
//.-
//.F  CompactSlider::fontChange
//  Notify change in font
//  
//.u  Syntax
//.f   CompactSlider::fontChange(const QFont &oldFont)
//
//------------------------------------------------------------
void CompactSlider::fontChange(const QFont & /*oldFont*/)
{
    repaint();
}

//------------------------------------------------------------
//    drawSlider
//     Draw the slider into the specified rectangle.  
//------------------------------------------------------------
/* replaced by graphical draw of png slider image in paintEvent()
void CompactSlider::drawSlider(QPainter *p, const QRect &r)
{
    p->setRenderHint(QPainter::Antialiasing);

    const QPalette& pal = palette();
    QBrush brBack(pal.window());
    QBrush brMid(pal.mid());
    QBrush brDark(pal.dark());

    QRect cr;
    
    int ipos,dist1;
    double rpos;

    int xrad = 4;
    int yrad = 4;

    // for the empty side
    QColor e_mask_edge = pal.mid().color();
    QColor e_mask_center = pal.midlight().color();
    int e_alpha = 215;
    e_mask_edge.setAlpha(e_alpha);
    e_mask_center.setAlpha(e_alpha);
    
    QLinearGradient e_mask;
    e_mask.setColorAt(0, e_mask_edge);
    e_mask.setColorAt(0.5, e_mask_center);
    e_mask.setColorAt(1, e_mask_edge);
    
    // for the full side
    rpos = (value()  - minValue()) / (maxValue() - minValue());
    
    int f_brightness = 155 * rpos + 100;
    int f_alpha;
    int f_edge;
    if (pal.currentColorGroup() == QPalette::Disabled)
        {
        f_alpha = 185;
        f_edge = 100;
        }
    else
        {
        f_alpha = 127;
        f_edge = 0;
        }
           
    QColor f_mask_center = QColor(f_brightness, f_brightness, f_brightness, f_alpha);
    QColor f_mask_edge = QColor(f_edge, f_edge, f_edge, f_alpha);
    QLinearGradient f_mask;
           
    f_mask.setColorAt(0, f_mask_edge);
    f_mask.setColorAt(0.5, f_mask_center);
    f_mask.setColorAt(1, f_mask_edge);
    
    // for the thumb
    QLinearGradient thumbGrad;
    QColor thumb_edge = pal.dark().color();
    QColor thumb_center = pal.midlight().color();
    
    thumbGrad.setColorAt(0, thumb_edge);
    thumbGrad.setColorAt(0.5, thumb_center);
    thumbGrad.setColorAt(1, thumb_edge);
    
    
    if (d_orient == Qt::Horizontal)
        {

        cr.setRect(r.x(),
                   r.y() + d_mMargin,
                   r.width(),
                   r.height() - 2*d_mMargin);


        //
        // Draw background
        //
        QPainterPath bg_rect = MusECore::roundedPath(cr, 
                                           xrad, yrad, 
                                           (MusECore::Corner) (MusECore::UpperLeft | MusECore::UpperRight | MusECore::LowerLeft | MusECore::LowerRight) );
           
        p->fillPath(bg_rect, d_fillColor);
           
        dist1 = int(double(cr.width() - d_thumbLength) * rpos);
        ipos =  cr.x() + dist1;
        markerPos = ipos + d_thumbHalf;
           

        //
        // Draw empty right side
        // 
           
        e_mask.setStart(QPointF(0, cr.y()));
        e_mask.setFinalStop(QPointF(0, cr.y() + cr.height()));

        QPainterPath e_rect = MusECore::roundedPath(ipos + d_thumbLength, cr.y(), 
                                          cr.width() - d_thumbLength - dist1, cr.height(), 
                                          xrad, yrad, (MusECore::Corner) (MusECore::UpperRight | MusECore::LowerRight) );
   
        p->fillPath(e_rect, QBrush(e_mask));
   
   
        //
        // Draw full left side
        //
           
        f_mask.setStart(QPointF(0, cr.y()));
        f_mask.setFinalStop(QPointF(0, cr.y() + cr.height()));
          
        QPainterPath f_rect = MusECore::roundedPath(cr.x(), cr.y(), 
                                          ipos + 1, cr.height(),
                                          xrad, yrad, 
                                          (MusECore::Corner) (MusECore::LowerLeft | MusECore::UpperLeft) );

        p->fillPath(f_rect, QBrush(f_mask));
          
           
        //
        //  Draw thumb
        //
           
        QPainterPath thumb_rect = MusECore::roundedPath(ipos, r.y(), 
                                              d_thumbLength, r.height(), 
                                              2, 2, 
                                              (MusECore::Corner) (MusECore::UpperLeft | MusECore::UpperRight | MusECore::LowerLeft | MusECore::LowerRight) );
   
        thumbGrad.setStart(QPointF(0, cr.y()));
        thumbGrad.setFinalStop(QPointF(0, cr.y() + cr.height()));
           
           
        p->fillPath(thumb_rect, QBrush(thumbGrad));
           
        // center line
        p->fillRect(ipos + d_thumbHalf, cr.y(), 1, cr.height(), pal.dark().color());
           

        }
    else // (d_orient == Qt::Vertical)
        {
              
        cr.setRect(r.x() + d_mMargin,
                   r.y(),
                   r.width() - 2*d_mMargin,
                   r.height());
            

        //
        // Draw background
        //
        QPainterPath bg_rect = MusECore::roundedPath(cr,
                                           xrad, yrad, 
                                           (MusECore::Corner) (MusECore::UpperLeft | MusECore::UpperRight | MusECore::LowerLeft | MusECore::LowerRight) );
            
        p->fillPath(bg_rect, d_fillColor);

        dist1 = int(double(cr.height() - d_thumbLength) * (1.0 - rpos));
        ipos = cr.y() + dist1;
        markerPos = ipos + d_thumbHalf;

  
        //
        // Draw empty upper filling
        // 

        e_mask.setStart(QPointF(cr.x(), 0));
        e_mask.setFinalStop(QPointF(cr.x() + cr.width(), 0));
            
        QPainterPath e_rect = MusECore::roundedPath(cr.x(), cr.y(), 
                                          cr.width(), ipos + 1,
                                          xrad, yrad, 
                                          (MusECore::Corner) (MusECore::UpperLeft | MusECore::UpperRight) );
            
        p->fillPath(e_rect, QBrush(e_mask));
            
            
        //
        // Draw lower filling mask
        //

        f_mask.setStart(QPointF(cr.x(), 0));
        f_mask.setFinalStop(QPointF(cr.x() + cr.width(), 0));
            
        QPainterPath f_rect = MusECore::roundedPath(cr.x(), ipos + d_thumbLength, 
                                          cr.width(), cr.height() - d_thumbLength - dist1,
                                          xrad, yrad, (MusECore::Corner) (MusECore::LowerLeft | MusECore::LowerRight) );
            
        p->fillPath(f_rect, QBrush(f_mask));
            
            
        //
        //  Draw thumb
        //
            
        QPainterPath thumb_rect = MusECore::roundedPath(r.x(), ipos, 
                                              r.width(), d_thumbLength,
                                              2, 2, 
                                              (MusECore::Corner) (MusECore::UpperLeft | MusECore::UpperRight | MusECore::LowerLeft | MusECore::LowerRight) );
            
        thumbGrad.setStart(QPointF(cr.x(), 0));
        thumbGrad.setFinalStop(QPointF(cr.x() + cr.width(), 0));
            
            
        p->fillPath(thumb_rect, QBrush(thumbGrad));
            
        // center line
        p->fillRect(cr.x(), ipos + d_thumbHalf, cr.width(), 1, pal.dark().color());
            
        }

}
*/
//------------------------------------------------------------
//.-
//.F  CompactSlider::getValue
//  Determine the value corresponding to a specified
//  mouse location.
//
//.u  Syntax
//.f     double CompactSlider::getValue(const QPoint &p)
//
//.u  Parameters
//.p  const QPoint &p --
//
//.u  Description
//  Called by SliderBase
//  If borderless mouse is enabled p is a delta value not absolute, so can be negative.
//------------------------------------------------------------
double CompactSlider::getValue( const QPoint &p)
{
  double rv;
  int pos;
  const QRect r = d_sliderRect;

  if(borderlessMouse())
  {
    if(d_orient == Qt::Horizontal)
//       return value() + p.x() * step();
      return value(ConvertNone) + p.x() * step();
    else
//       return value() - p.y() * step();
      return value(ConvertNone) - p.y() * step();
  }
  
  if(d_orient == Qt::Horizontal)
  {
    if(r.width() <= d_thumbLength)
//       rv = 0.5 * (minValue() + maxValue());
      rv = 0.5 * (minValue(ConvertNone) + maxValue(ConvertNone));
    else
    {
      pos = p.x() - r.x() - d_thumbHalf;
//       rv  =  minValue() +
//         rint( (maxValue() - minValue()) * double(pos)
//         / double(r.width() - d_thumbLength)
//         / step() ) * step();
        
//       rv = minValue() + double(pos) * step();
      rv = minValue(ConvertNone) + double(pos) * step();
    }
  }
  else
  {
    if(r.height() <= d_thumbLength)
//       rv = 0.5 * (minValue() + maxValue());
      rv = 0.5 * (minValue(ConvertNone) + maxValue(ConvertNone));
    else
    {
      pos = p.y() - r.y() - d_thumbHalf;
//       rv =  minValue() +
//         rint( (maxValue() - minValue()) *
//         (1.0 - double(pos)
//         / double(r.height() - d_thumbLength))
//         / step() ) * step();
        
//       rv = minValue() + (maxValue() - double(pos) * step());
      rv = minValue(ConvertNone) + (maxValue(ConvertNone) - double(pos) * step());
    }
  }
  return(rv);
}


//------------------------------------------------------------
//.-
//.F  CompactSlider::getScrollMode
//  Determine scrolling mode and direction
//
//.u  Syntax
//.f   void CompactSlider::getScrollMode( const QPoint &p, int &scrollMode, int &direction )
//
//.u  Parameters
//.p  const QPoint &p -- point
//
//.u  Description
//  Called by SliderBase
//
//------------------------------------------------------------
void CompactSlider::getScrollMode( QPoint &p, const Qt::MouseButton &button, int &scrollMode, int &direction )
{
  if((!detectThumb() || borderlessMouse()))
  {
    if(button != Qt::NoButton && d_sliderRect.contains(p))
    {
      scrollMode = ScrMouse;
      direction = 0;
      return;
    }
  }
  else
  {
    int currentPos;
    if (d_orient == Qt::Horizontal)
      currentPos = p.x() - d_sliderRect.x();
    else
      currentPos = p.y() - d_sliderRect.y();
    
    if (d_sliderRect.contains(p))
    {
//         if ((currentPos > d_valuePixel - d_thumbHalf)  
//             && (currentPos < d_valuePixel + d_thumbHalf))
      if ((currentPos >= d_valuePixel - d_thumbHitLength / 2)  
          && (currentPos <= d_valuePixel + d_thumbHitLength / 2))
      {
        scrollMode = ScrMouse;
        direction = 0;
        return;
      }
      else if(pagingButtons().testFlag(button))
      {
        scrollMode = ScrPage;
        if (((currentPos > d_valuePixel) && (d_orient == Qt::Horizontal))
            || ((currentPos <= d_valuePixel) && (d_orient != Qt::Horizontal)))
          direction = 1;
        else
          direction = -1;
        return;
      }
    }
  }
    
  scrollMode = ScrNone;
  direction = 0;
    
}

// //------------------------------------------------------------
// //.F  CompactSlider::paintEvent
// //  Qt paint event
// //
// //.u  Syntax
// //.f  void CompactSlider::paintEvent(QPaintEvent *e)
// //------------------------------------------------------------
// 
// void CompactSlider::paintEvent(QPaintEvent* /*ev*/)
// {
//    QPainter p(this);
// 
//    /* Scale is not supported
//       if (p.begin(this)) {
//             if (d_scalePos != None) {
//                   p.fillRect(rect(), palette().window());
//                   d_scale.draw(&p);
//                   }
//             drawSlider(&p, d_sliderRect);
//             }
//       p.end();
//       */
//    //drawSlider(&p, d_sliderRect);
//    double __minV = minValue();
//    double __maxV = maxValue();
//    //int __w = width();
//    double __sHeight = MusEGui::sliderPngImage->height();
//    double __middleLine = __sHeight / 2;
//    double __h = (double)height() - __sHeight;
//    double __scaleStep = (__h / (maxValue() - minValue())) / (double)pageSize();
//    double __maxH = __h - __middleLine;
//    QColor __bkColor = p.background().color();
//    QColor __bkInvert(255 - __bkColor.red(), 255 - __bkColor.green(), 255 - __bkColor.green());
// 
//    double ypos = __h - __h * (value()  - __minV) / (__maxV - __minV);
// 
// 
// 
//    //draw current value with gradoent rectangle
//    p.setPen(__bkColor);
//    QLinearGradient __scaleGradient(0, __h, 0, 0);
//    __scaleGradient.setColorAt(0.0, __bkColor);//QColor(63, 169, 255));
//    __scaleGradient.setColorAt(1.0, QColor(62, 37, 255));
//    QBrush __gradientBrush(__scaleGradient);
//    p.setBrush(__gradientBrush);
//    long __ly = roundl(ypos + __middleLine);
//    p.drawRect(15 + 7, __ly, 11, __h - __ly);
// 
//    //draw ruler scale
//    p.setPen(__bkInvert);
//    int __k = 0;
//    int __c = 0;
//    int __v = minValue();
//    QFont __numberFont;
//    __numberFont.setFamily("Sans");
//    __numberFont.setPixelSize(8);
// 
//    QFont __numberFontBold;
//    __numberFontBold.setFamily("Sans");
//    __numberFontBold.setPixelSize(8);
//    __numberFontBold.setBold(true);
//    if((int)__scaleStep == 0)
//    {
//       __scaleStep = 1;
//    }
//    for(double i = -__middleLine - __scaleStep; (int)i > -__sHeight; i -= __scaleStep, __c = ((__c + 1) % 2))
//    {
//       //fprintf(stderr, "__middleLine=%f, __scaleStep=%f, i=%f\n", __middleLine, __scaleStep, i);      
//       if(__c== 0)
//       {
//          long __ly = roundl(__h - i);
//          p.drawLine(23, __ly, 23 - 1, __ly);
//       }
//    }
//    __c = 0;
//    for(double i = -__middleLine; i < __maxH; i += __scaleStep, ++__k, ++__v, __c = ((__c + 1) % 2))
//    {
//       bool __b10 = (__k % 10) ? false : true;
//       if(!__b10 && __c != 0)
//       {
//          continue;
//       }
// 
//       long __ly = roundl(__h - i);
// 
//       p.drawLine(23, __ly, 23 - (!__b10 ? 2 : 5), __ly);
//       if(__b10)
//       {
//          if(__v <= roundl(value()))
//          {
//             p.setFont(__numberFontBold);
//          }
//          else
//          {
//             p.setFont(__numberFont);
//          }
//          QString __n = QString::number(__v);
//          QRectF __bRect = p.boundingRect(QRectF(0, 0, 8, 8), __n);
//          p.drawText(23 - __bRect.width() - 6, __ly + 4, __n);
//       }
//    }
// 
//    int __offs = bPressed ? 1 : 0;
// 
//    //fprintf(stderr, "value() = %f, __minv=%f, __maxV=%f, __h=%f, ypos=%f, __offs=%d\n", value(), __minV, __maxV, __h, ypos, __offs);
// 
//    //draw slider button
//    p.drawPixmap(15 + __offs, roundl(ypos + __offs), *sliderPngImage);
// 
// 
// }

void CompactSlider::getActiveArea()
{
  const QRect& geo = rect();
//   const int req_thumb_margin = (d_thumbHalf - d_xMargin) > 1 ? (d_thumbHalf - d_xMargin) : 1;
  const int req_thumb_margin = d_thumbLength == 0 ? 0 : ((d_thumbHalf - d_xMargin) > 1 ? (d_thumbHalf - d_xMargin) : 1);
  
//   QStyleOptionFrame option;
//   option.initFrom(this);
/*  
  // FIXME: These aren't working for PE_Frame. We just get a 1px-wide square dark grey frame.
  //        For others like PE_FrameLineEdit which looks OK, these have no effect and items 
  //         such as rounded frame are determined by the style.
  option.lineWidth = d_xMargin;
  option.features = QStyleOptionFrame::Rounded;
  option.frameShape = QFrame::StyledPanel;
  option.midLineWidth = 0;
  option.state = QStyle::State_Sunken;
  option.state |= QStyle::State_Active;
  if(isEnabled())
    option.state |= QStyle::State_Enabled;
  if(hasFocus()) 
  {
    option.state |= QStyle::State_HasFocus;
//     option.state |= QStyle::State_Selected;
  }
  if(underMouse())
  {
    option.state |= QStyle::State_MouseOver;  // FIXME: Not working.
  }
//   if(hasEditFocus())
//     option.state |= (QStyle::State_HasFocus | QStyle::State_HasEditFocus | QStyle::State_Selected);
  
  const QStyle* st = style();
  if(st)
    st = st->proxy();
//   if(st)
  if(0)
  {

// FIXME: Can't seem to get true inside area. All these methods return 1px frame width but the visible control has 2px width.
//        Maybe ask for QStyle::PM_DefaultFrameWidth.
//     return st->subElementRect(QStyle::SE_FrameContents, &option);
//     return contentsRect();
    d_sliderRect = st->subElementRect(QStyle::SE_ShapedFrameContents, &option).adjusted(1, 1, -1, -1);
  }
  else*/
  {
    d_sliderRect = QRect(
//                   geo.x() + d_xMargin + active_thumb_margin, 
                  geo.x() + d_xMargin + req_thumb_margin, 
                  geo.y() + d_yMargin, 
//                   geo.width() - 2 * (d_xMargin + active_thumb_margin), 
                  geo.width() - 2 * (d_xMargin + req_thumb_margin), 
                  geo.height() - 2 * d_yMargin); 
  }
}

void CompactSlider::getPixelValues()
{
  const int val_width_range = ((d_orient == Qt::Horizontal) ? d_sliderRect.width(): d_sliderRect.height());
  const int val_pix_range = val_width_range - 1;
//   const double minV = minValue();
//   const double maxV = maxValue();
  const double minV = minValue(ConvertNone);
  const double maxV = maxValue(ConvertNone);
  const double range = maxV - minV;
//   const double val = value();
  const double val = value(ConvertNone);

  if(range == 0.0)
  {
    d_valuePixel = 0;
    d_valuePixelWidth = 0;
    return;
  }
  const double val_fact = (val - minV) / range;
  
  d_valuePixel = (val_fact * (double)val_pix_range);
  d_valuePixelWidth = (val_fact * (double)val_width_range);
}

//------------------------------------------------------------
//.F  CompactSlider::paintEvent
//  Qt paint event
//
//.u  Syntax
//.f  void CompactSlider::paintEvent(QPaintEvent *e)
//------------------------------------------------------------

void CompactSlider::paintEvent(QPaintEvent* /*ev*/)
{
  const QRect& geo = rect();
  if(geo.width() <= 0 || geo.height() <= 0)
    return;
  
  QPainter p(this);
  
  const QPalette& pal = palette();

//   const int req_thumb_length = d_thumbLength > d_thumbHitLength ? d_thumbLength : d_thumbHitLength;
//   const int active_thumb_margin = d_thumbLength / 2;
//   const int active_thumb_margin = d_thumbLength / 2;
//   const int active_thumb_margin = d_thumbHalf;
  
//   const int req_thumb_margin = req_thumb_length / 2;
  const int req_thumb_margin = d_thumbLength == 0 ? 0 : ((d_thumbHalf - d_xMargin) > 1 ? (d_thumbHalf - d_xMargin) : 1);

  const QPen orig_pen = p.pen();
  
//   QStyleOptionFrame option;
//   option.initFrom(this);
//   
//   // FIXME: These aren't working for PE_Frame. We just get a 1px-wide square dark grey frame.
//   //        For others like PE_FrameLineEdit which looks OK, these have no effect and items 
//   //         such as rounded frame are determined by the style.
//   option.lineWidth = d_xMargin;
//   option.features = QStyleOptionFrame::Rounded;
//   option.frameShape = QFrame::StyledPanel;
//   option.midLineWidth = 0;
//   option.state = QStyle::State_Sunken;
//   option.state |= QStyle::State_Active;
//   if(isEnabled())
//     option.state |= QStyle::State_Enabled;
//   if(hasFocus()) 
//   {
//     option.state |= QStyle::State_HasFocus;
// //     option.state |= QStyle::State_Selected;
//   }
//   if(underMouse())
//   {
//     option.state |= QStyle::State_MouseOver;  // FIXME: Not working.
//   }
//   if(hasEditFocus())
//     option.state |= (QStyle::State_HasFocus | QStyle::State_HasEditFocus | QStyle::State_Selected);
  
// //   const QStyle* st = style();
// //   if(st)
// //     st = st->proxy();
// // //   if(st)
// //   if(0)
// //   {
// // 
// // // FIXME: Can't seem to get true inside area. All these methods return 1px frame width but the visible control has 2px width.
// // //        Maybe ask for QStyle::PM_DefaultFrameWidth.
// // //     active_area = st->subElementRect(QStyle::SE_FrameContents, &option);
// // //     active_area = contentsRect();
// //     
// // //     active_area = st->subElementRect(QStyle::SE_ShapedFrameContents, &option).adjusted(1, 1, -1, -1);
// //     
// // //     st->drawPrimitive(QStyle::PE_FrameLineEdit, &option, &p);
// //     st->drawPrimitive(QStyle::PE_Frame, &option, &p);
// //   }
// //   else
  {
//     QPen pen;
//     pen.setCosmetic(true);
//     pen.setColor(d_marginColor);
//     pen.setWidth(d_yMargin);
//     p.setPen(pen);
//     p.drawLine(0, 0, geo.width() - 1, 0);
//     p.drawLine(0, geo.height() - 1, geo.width() - 1, geo.height() - 1);
// 
//     pen.setWidth(d_xMargin + active_thumb_margin);
//     p.setPen(pen);
//     p.drawLine(0, 0, 0, geo.height() - 1);
//     p.drawLine(geo.width() - 1 - d_xMargin - active_thumb_margin, 
//                0, 
//                geo.width() - 1 - d_xMargin - active_thumb_margin, 
//                geo.height() - 1);

//     const QColor& margin_color = option.palette.mid().color();
//     const QColor border_color = hasFocus() ? option.palette.highlight().color().lighter() : margin_color;
//     const QColor& margin_color = pal.mid().color();
//     const QColor& margin_color = pal.window().color().lighter(125);
    const QColor& margin_color = pal.button().color();
    //const QColor border_color = hasFocus() ? pal.highlight().color().lighter() : margin_color;
    QColor border_color;
    
    switch(_textHighlightMode)
    {
      case TextHighlightFocus:
      case TextHighlightHoverOrFocus:
        border_color = _hovered ? pal.highlight().color().lighter() : margin_color;
      break;
      
      case TextHighlightAlways:
      case TextHighlightNone:
      case TextHighlightShadow:
      case TextHighlightSplit:
      case TextHighlightHover:
      case TextHighlightSplitAndShadow:
        border_color = hasFocus() ? pal.highlight().color().lighter() : margin_color;
      break;
    }
      
    // Draw margins:
    if(d_yMargin)
    {
      // Top
      p.fillRect(geo.x(), 
                 geo.y(), 
                 geo.width(), 
                 d_yMargin, 
                 border_color);
    
      // Bottom
      p.fillRect(geo.x(), 
                 geo.height() - d_yMargin, 
                 geo.width(), 
                 d_yMargin, 
                 border_color);
    }
    
    if(d_xMargin)
    {
      // Left
      p.fillRect(geo.x(), 
                 geo.y(), 
                 d_xMargin, 
                 geo.height(), 
                 border_color);
      
      // Right
      p.fillRect(geo.width() - d_xMargin, 
                 geo.y(), 
                 d_xMargin, 
                 geo.height(), 
                 border_color);
    }
    
    // Extra left margin
    p.fillRect(d_xMargin, 
               d_yMargin, 
  //              d_xMargin + active_thumb_margin, 
               req_thumb_margin, 
               geo.height() - 2 * d_yMargin, 
               margin_color);
    
    // Extra right margin
    p.fillRect(
  //              geo.width() - d_xMargin - active_thumb_margin, 
               geo.width() - d_xMargin - req_thumb_margin, 
               d_yMargin, 
  //              d_xMargin + active_thumb_margin, 
               req_thumb_margin, 
               geo.height() - 2 * d_yMargin, 
               margin_color);
  }
  
//   const QRect active_area(
// //                           geo.x() + d_xMargin + active_thumb_margin, 
//                           geo.x() + d_xMargin + req_thumb_margin, 
//                           geo.y() + d_yMargin, 
// //                           geo.width() - 2 * (d_xMargin + active_thumb_margin), 
//                           geo.width() - 2 * (d_xMargin + req_thumb_margin), 
//                           geo.height() - 2 * d_yMargin); 
  
//   const QRect val_area(geo.x() + 1 + d_xMargin, 
//                        geo.y() + 1 + d_yMargin, 
//                        geo.width() - 2 - 2 * d_xMargin, 
//                        geo.height() - 2 - 2 * d_yMargin); 


  const QPainterPath& onPath = *_onPath;
//   const QPainterPath& offPath = *_offPath;
  
  // Draw corners as normal background colour. Path subtraction.
  QPainterPath cornerPath;
  cornerPath.addRect(d_sliderRect);
  cornerPath -= onPath;
//   cornerPath -= offPath;
  if(!cornerPath.isEmpty())
//     p.fillPath(cornerPath, palette().dark());
    p.fillPath(cornerPath, palette().window().color().darker(125));

// //   const int val_pix_range = ((d_orient == Qt::Horizontal) ? active_area.width(): active_area.height()) - 2 * active_thumb_margin;
//   const int val_width_range = ((d_orient == Qt::Horizontal) ? d_sliderRect.width(): d_sliderRect.height());
//   const int val_pix_range = val_width_range - 1;
//   const double minV = minValue();
  const double minV = minValue(ConvertNone);
//   const double maxV = maxValue();
//   const double range = maxV - minV;
//   const double val = value();
  const double val = value(ConvertNone);
// 
//   if(range == 0.0)
//     return;
//   const double val_fact = (val - minV) / range;
//   
//   //const int val_pix = (val_fact * (double)val_pix_range) + active_thumb_margin;
//   const int val_pix = (val_fact * (double)val_pix_range);
//   const int val_width = (val_fact * (double)val_width_range);
//   
  
  const QRect bar_area((d_orient == Qt::Horizontal) ? 
                      QRect(d_sliderRect.x(), 
                            d_sliderRect.y(), 
                            d_valuePixelWidth, 
                            d_sliderRect.height()) :
                      QRect(d_sliderRect.x(), 
                            d_sliderRect.y() + d_sliderRect.height() - d_valuePixelWidth, 
                            d_sliderRect.width(), 
                            d_sliderRect.height() - d_valuePixelWidth)); // FIXME
  const QRect bkg_area((d_orient == Qt::Horizontal) ? 
                      QRect(d_sliderRect.x() + d_valuePixelWidth, 
                            d_sliderRect.y(), 
                            //active_area.width() - (active_area.x() + val_pix), 
                            d_sliderRect.width() - d_valuePixelWidth, 
                            d_sliderRect.height()) : 
                      QRect(d_sliderRect.x(), 
                            d_sliderRect.y(), 
                            d_sliderRect.width(), 
                            d_sliderRect.height() - d_valuePixelWidth)); // FIXME

// //   p.fillRect(bar_area, option.palette.highlight().color());
// //   p.fillRect(bkg_area, option.palette.window().color());
//   p.fillRect(bar_area, pal.highlight().color());
//   p.fillRect(bkg_area, pal.window().color());

  p.setClipPath(onPath);
  p.fillPath(onPath, _onPixmap);
  
//   p.setClipPath(offPath);
//   p.fillPath(offPath, _offPixmap);
  
  // Restore the clipper to full size.
  p.setClipRect(geo);
  
  if((!_autoHideThumb || _mouseOverThumb) && d_thumbLength > 0)
  {
    //fprintf(stderr, "CompactSlider::PaintEvent _mouseOverThumb:%d\n", _mouseOverThumb); // REMOVE Tim. Trackinfo.
    const QRect thumb_rect((d_orient == Qt::Horizontal) ?
                          QRect(d_sliderRect.x() + d_valuePixel - d_thumbHalf, 
                                d_sliderRect.y() + d_thumbWidthMargin,
                                d_thumbLength,
                                d_sliderRect.height() - 2 * d_thumbWidthMargin) : 
                          QRect(d_sliderRect.x() + d_thumbWidthMargin,
                                d_sliderRect.y() + d_valuePixel - d_thumbHalf,
                                d_sliderRect.width() - 2 * d_thumbWidthMargin,
                                d_thumbLength));
    p.fillRect(thumb_rect, d_thumbColor);
  }


  const QRect text_area(d_sliderRect.adjusted(1, 1, -1, -1));
  const QString elided_label_text = fontMetrics().elidedText(d_labelText, Qt::ElideMiddle, text_area.width());
  //const QRect elided_label_text_rect = fontMetrics().boundingRect(elided_label_text);
  //fprintf(stderr, "CompactSlider::PaintEvent locale:%s\n", locale().name().toLatin1().constData()); // REMOVE Tim. Trackinfo.
  const QString comp_val_text = isOff() ? d_offText :
                                ((val <= minV && !d_specialValueText.isEmpty()) ? 
                                d_specialValueText : (d_valPrefix + locale().toString(val, 'f', _valueDecimals) + d_valSuffix));
  //const QRect comp_val_rect = fontMetrics().boundingRect(active_area, Qt::AlignRight | Qt::AlignVCenter, comp_val_text);

  
  switch(_textHighlightMode)
  {
    case TextHighlightShadow:
    {
      const QRect text_area_text = text_area.adjusted(0, 0, -1, -1);
      const QRect text_area_shadow = text_area_text.adjusted(1, 1, 1, 1);
      
      // Text shadow:
      //p.setPen(pal.shadow().color());
      p.setPen(Qt::black);
      p.drawText(text_area_shadow, Qt::AlignRight | Qt::AlignVCenter, comp_val_text);
      p.drawText(text_area_shadow, Qt::AlignLeft | Qt::AlignVCenter, elided_label_text);
      
      // Text:
      //p.setPen(pal.brightText().color());
      p.setPen(Qt::white);
      p.drawText(text_area_text, Qt::AlignRight | Qt::AlignVCenter, comp_val_text);
      p.drawText(text_area_text, Qt::AlignLeft | Qt::AlignVCenter, elided_label_text);
    } 
    break;

    case TextHighlightNone:
      p.setPen(pal.text().color());
      p.drawText(text_area, Qt::AlignRight | Qt::AlignVCenter, comp_val_text);
      p.drawText(text_area, Qt::AlignLeft | Qt::AlignVCenter, elided_label_text);
    break;

    case TextHighlightAlways:
      p.setPen(pal.brightText().color());
      p.drawText(text_area, Qt::AlignRight | Qt::AlignVCenter, comp_val_text);
      p.drawText(text_area, Qt::AlignLeft | Qt::AlignVCenter, elided_label_text);
    break;

    case TextHighlightSplit:
      // Highlighted section:
      p.setClipRect(bar_area);
      p.setPen(pal.brightText().color());
      p.drawText(text_area, Qt::AlignRight | Qt::AlignVCenter, comp_val_text);
      p.drawText(text_area, Qt::AlignLeft | Qt::AlignVCenter, elided_label_text);
      
      // Normal non-highlighted section:
      p.setClipRect(bkg_area);
      p.setPen(pal.text().color());
      p.drawText(text_area, Qt::AlignRight | Qt::AlignVCenter, comp_val_text);
      p.drawText(text_area, Qt::AlignLeft | Qt::AlignVCenter, elided_label_text);
      
      // Restore the clipper to full size.
      //p.setClipRect(geo);
    break;
    
    case TextHighlightHover:
      p.setPen(_hovered ? pal.brightText().color() : pal.text().color());
      p.drawText(text_area, Qt::AlignRight | Qt::AlignVCenter, comp_val_text);
      p.drawText(text_area, Qt::AlignLeft | Qt::AlignVCenter, elided_label_text);
    break;
    
    case TextHighlightFocus:
      p.setPen(hasFocus() ? pal.brightText().color() : pal.text().color());
      p.drawText(text_area, Qt::AlignRight | Qt::AlignVCenter, comp_val_text);
      p.drawText(text_area, Qt::AlignLeft | Qt::AlignVCenter, elided_label_text);
    break;
    
    case TextHighlightHoverOrFocus:
      p.setPen((_hovered || hasFocus()) ? pal.brightText().color() : pal.text().color());
      p.drawText(text_area, Qt::AlignRight | Qt::AlignVCenter, comp_val_text);
      p.drawText(text_area, Qt::AlignLeft | Qt::AlignVCenter, elided_label_text);
    break;
    
    case TextHighlightSplitAndShadow:
    {
      const QRect text_area_text = text_area.adjusted(0, 0, -1, -1);
      const QRect text_area_shadow = text_area_text.adjusted(1, 1, 1, 1);

      // Highlighted section:
      p.setClipRect(bar_area);
      // Text shadow:
      //p.setPen(pal.shadow().color());
      p.setPen(Qt::black);
      p.drawText(text_area_shadow, Qt::AlignRight | Qt::AlignVCenter, comp_val_text);
      p.drawText(text_area_shadow, Qt::AlignLeft | Qt::AlignVCenter, elided_label_text);
      // Text:
      //p.setPen(pal.brightText().color());
      p.setPen(Qt::white);
      p.drawText(text_area_text, Qt::AlignRight | Qt::AlignVCenter, comp_val_text);
      p.drawText(text_area_text, Qt::AlignLeft | Qt::AlignVCenter, elided_label_text);
      
      // Normal non-highlighted section:
      p.setClipRect(bkg_area);
      p.setPen(pal.text().color());
      p.drawText(text_area, Qt::AlignRight | Qt::AlignVCenter, comp_val_text);
      p.drawText(text_area, Qt::AlignLeft | Qt::AlignVCenter, elided_label_text);
      //p.drawText(text_area_shadow, Qt::AlignRight | Qt::AlignVCenter, comp_val_text);
      //p.drawText(text_area_shadow, Qt::AlignLeft | Qt::AlignVCenter, elided_label_text);
    }
    break;
  }
  
//   p.setPen(orig_pen);
//   if(st)
//   {
//     // Draw the focus.
//     //if(hasEditFocus() || option.state & QStyle::State_HasFocus || option.state & QStyle::State_HasFocus || option.state & QStyle::State_MouseOver || option.state & QStyle::State_Selected)
//     if(option.state & QStyle::State_HasFocus || option.state & QStyle::State_MouseOver || option.state & QStyle::State_Selected)
//     {
//       QStyleOptionFocusRect o;
//       o.QStyleOption::operator=(option);
// //       o.rect = active_area;
//       QPalette::ColorGroup cg = (option.state & QStyle::State_Enabled) ? QPalette::Normal : QPalette::Disabled;
//       o.backgroundColor = option.palette.color(cg, (option.state & QStyle::State_Selected) ? QPalette::Highlight : QPalette::Window);
// //       o.backgroundColor = option.palette.color(cg, QPalette::Window);
// //       o.backgroundColor = option.palette.highlight().color();
//       st->drawPrimitive(QStyle::PE_FrameFocusRect, &o, &p);
//     }
//   }
}

void CompactSlider::mouseMoveEvent(QMouseEvent *e)
{
//   const Qt::MouseButtons btns = e->buttons();
//   const bool left_btn = btns & Qt::LeftButton;
  e->ignore();
  SliderBase::mouseMoveEvent(e);
  
  QPoint p = e->pos();
//   int currentPos;
//   if (d_orient == Qt::Horizontal)
//     currentPos = p.x() - d_sliderRect.x();
//   else
//     currentPos = p.y() - d_sliderRect.y();
  
//   bool v = _mouseOverThumb;
//   if (d_sliderRect.contains(p))
//   {
//     //if ((currentPos > d_valuePixel - d_thumbHalf)  
//     //    && (currentPos < d_valuePixel + d_thumbHalf))
//     v = (currentPos >= d_valuePixel - d_thumbHitLength / 2)  
//         && (currentPos <= d_valuePixel + d_thumbHitLength / 2);
//   }
  
//   int scrollMode;
//   int direction;
//   //getScrollMode(p, left_btn ? Qt::LeftButton : 0, scrollMode, direction);
//   // Force left button in case getScrollMode depends on it.
//   getScrollMode(p, Qt::LeftButton, scrollMode, direction);
//   const bool v = scrollMode == ScrMouse;
//   
//   fprintf(stderr, "CompactSlider::mouseMoveEvent _mouseOverThumb:%d v:%d\n", _mouseOverThumb, v); // REMOVE Tim. Trackinfo.
// //   if(_mouseOverThumb != v && !(left_btn && !v))
//   if(_mouseOverThumb != v && !(bPressed && !v))
//   {
//     fprintf(stderr, "   setting _mouseOverThumb to v:%d\n", v); // REMOVE Tim. Trackinfo.
//     _mouseOverThumb = v;
//     update();
//   }
  
  const bool oldv = _mouseOverThumb;
  getMouseOverThumb(p);
  if(_autoHideThumb &&_mouseOverThumb != oldv)
    update();
}

void CompactSlider::mouseDoubleClickEvent(QMouseEvent* e)
{
  //fprintf(stderr, "CompactSlider::mouseDoubleClickEvent\n"); // REMOVE Tim. Trackinfo.
  const Qt::MouseButtons buttons = e->buttons();
  const Qt::KeyboardModifiers keys = e->modifiers();
  //const bool left_btn = buttons & Qt::LeftButton;
  //const bool ctrl_key = keys & Qt::ControlModifier;
  //if(left_btn && _mouseOverThumb && !_editMode)
  
  if(buttons == Qt::LeftButton && _mouseOverThumb && !_editMode)
  {  
    //fprintf(stderr, "   left button\n"); // REMOVE Tim. Trackinfo.
    //if(_mouseOverThumb && !_editMode)
    //{
      //if(ctrl_key)
      if(keys == Qt::ControlModifier)
      {
        if(_hasOffMode)
        {
          setOff(!isOff()); // Just toggle the off state.
          emit valueChanged(value(), id()); 
          e->accept();    
          return;
        }
      }
      // A disabled spinbox up or down button will pass the event to the parent! Causes pseudo 'wrapping'. Eat it up.
      else if(keys == Qt::NoModifier && (!_editor || !_editor->hasFocus()))
      {
        showEditor();
        e->accept();    
        return;
      }
    //}
    //e->accept();    
    //return;
  }  

  e->ignore();
  SliderBase::mouseDoubleClickEvent(e);
}

// void CompactSlider::editorReturnPressed()
// {
//   fprintf(stderr, "CompactSlider::editingFinished\n"); // REMOVE Tim. Trackinfo.
//   // We may get editingFinished a couple of times after editing: Once when enter or esc pressed, 
//   //  and once more (possibly when it loses focus from being deleted - but tests showed not so).
//   if(_editMode && _editor)
//   {
//     if(value() != _editor->value())
//       setValue(_editor->value());
//     
//     ////_editJustFinished=true;
//   //   if(_editor->isVisible())
//   //   {  
//   //     _editor->blockSignals(true);  
//   //     _editor->hide();
//   //     _editor->blockSignals(false);
//   //   }  
//     //delete _editor;
//     _editor->deleteLater();
//     _editor = 0;
//     _editMode = false;
//     setFocus();
//   }
// }

void CompactSlider::editorReturnPressed()
{
  //fprintf(stderr, "CompactSlider::editorReturnPressed\n"); // REMOVE Tim. Trackinfo.
  _editMode = false;
  if(_editor)
  {
    if(value() != _editor->value())
      setValue(_editor->value());
    _editor->deleteLater();
    _editor = 0;
    setFocus();
  }
}

void CompactSlider::editorEscapePressed()
{
  //fprintf(stderr, "CompactSlider::editorEscapePressed\n"); // REMOVE Tim. Trackinfo.
  _editMode = false;
  if(_editor)
  {
    _editor->deleteLater();
    _editor = 0;
    setFocus();
  }
}

void CompactSlider::keyPressEvent(QKeyEvent* e)
{
  if(e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter)
  {
    // A disabled spinbox up or down button will pass the event to the parent! Causes pseudo 'wrapping'. Eat it up.
    if(!_editor || !_editor->hasFocus())
      showEditor();
    e->accept();
    return;
  }

//   if(_editMode)
//   {
//     if(e->key() == Qt::Key_Escape)
//     {
// //       if(_editor && _editor->isVisible())
// //       {
// //         _editor->blockSignals(true);
// //         _editor->hide();
// //         _editor->blockSignals(false);
// //       }
//       
//       if(_editor)
//       {
//         fprintf(stderr, "CompactSlider::keyPressEvent deleting editor\n"); // REMOVE Tim. Trackinfo.
//         //delete _editor;
//         _editor->deleteLater();
//         fprintf(stderr, "   deleted\n"); // REMOVE Tim. Trackinfo.
//       }
//       _editor = 0;
//       _editMode = false;
//       
//       setFocus();
//     }
//     e->accept();
//     return;
//   }


//   else if (!editJustFinished)
//   {
//     emit keyPressExt(e); //redirect keypress events to main app. don't call this when confirming an editor
//   }
//   else
//     editJustFinished=false;

//   emit keyPressExt(e); //redirect keypress events to main app

  e->ignore();
  SliderBase::keyPressEvent(e);
}

void CompactSlider::enterEvent(QEvent *e)
{
  _entered = true;
  if(!_hovered)
  {
    _hovered = true;
    switch(_textHighlightMode)
    {
      case TextHighlightHover:
      case TextHighlightHoverOrFocus:
        update();
      break;
      case TextHighlightAlways:
      case TextHighlightNone:
      case TextHighlightShadow:
      case TextHighlightSplitAndShadow:
      case TextHighlightSplit:
      case TextHighlightFocus:
      break;
    }
  }

  e->ignore();
  SliderBase::enterEvent(e);
}

void CompactSlider::leaveEvent(QEvent *e)
{
  _entered = false;
//   if(!bPressed)
  if(!_pressed)
  {
    if(_hovered)
    {
      _hovered = false;
      switch(_textHighlightMode)
      {
        case TextHighlightHover:
        case TextHighlightHoverOrFocus:
          update();
        break;
        case TextHighlightAlways:
        case TextHighlightNone:
        case TextHighlightShadow:
        case TextHighlightSplitAndShadow:
        case TextHighlightSplit:
        case TextHighlightFocus:
        break;
      }
    }
    _mouseOverThumb = false;
    if(_autoHideThumb)
      update();
  }
  e->ignore();
  SliderBase::leaveEvent(e);
}

bool CompactSlider::event(QEvent* e)
{
  switch(e->type())
  {
    // FIXME: Doesn't work.
    case QEvent::NonClientAreaMouseButtonPress:
      fprintf(stderr, "CompactSlider::event NonClientAreaMouseButtonPress\n"); // REMOVE Tim. Trackinfo.
      e->accept();
      _editMode = false;
      if(_editor)
      {
        _editor->deleteLater();
        _editor = 0;
      }
      return true;
    break;
    
    default:
    break;
  }
  
  e->ignore();
  return SliderBase::event(e);
}


//------------------------------------------------------------
//.F  CompactSlider::resizeEvent
//  Qt resize event
//
//.u  Parameters
//.p  QResizeEvent *e
//
//.u  Syntax
//.f  void CompactSlider::resizeEvent(QResizeEvent *e)
//------------------------------------------------------------

void CompactSlider::resizeEvent(QResizeEvent *e)
{
    SliderBase::resizeEvent(e);
    d_resized = true;
//     QSize s = e->size();
    
    /* Scale is not supported
    int sliderWidth = d_thumbWidth;

    // reposition slider
    if(d_orient == Qt::Horizontal)
    {
  switch(d_scalePos)
  {
  case Top:
  
      d_sliderRect.setRect(this->rect().x() + d_xMargin,
         this->rect().y() + s.height() - 1
         - d_yMargin - sliderWidth,
         s.width() - 2 * d_xMargin,
         sliderWidth);
      d_scale.setGeometry(d_sliderRect.x() + d_thumbHalf,
        d_sliderRect.y() - d_scaleDist,
        d_sliderRect.width() - d_thumbLength,
        ScaleDraw::Top);
  
      break;
  
  case Bottom:
  
      d_sliderRect.setRect(this->rect().x() + d_xMargin,
         this->rect().y() + d_yMargin,
         s.width() - 2*d_xMargin,
         sliderWidth);
      d_scale.setGeometry(d_sliderRect.x() + d_thumbHalf,
        d_sliderRect.y() + d_sliderRect.height() +  d_scaleDist,
        d_sliderRect.width() - d_thumbLength,
        ScaleDraw::Bottom);
  
      break;
  
  default:
      d_sliderRect.setRect(this->rect().x(), this->rect().x(),
         s.width(), s.height());
      break;
  }
    }
    else // d_orient == Qt::Vertical
    {
  switch(d_scalePos)
  {
  case Left:
      d_sliderRect.setRect(this->rect().x() + s.width()
         - sliderWidth - 1 - d_xMargin,
         this->rect().y() + d_yMargin,
         sliderWidth,
         s.height() - 2 * d_yMargin);
      d_scale.setGeometry(d_sliderRect.x() - d_scaleDist,
        d_sliderRect.y() + d_thumbHalf,
        s.height() - d_thumbLength,
        ScaleDraw::Left);
  
      break;
  case Right:
      d_sliderRect.setRect(this->rect().x() + d_xMargin,
         this->rect().y() + d_yMargin,
         sliderWidth,
         s.height() - 2* d_yMargin);
      d_scale.setGeometry(this->rect().x() + d_sliderRect.width()
        + d_scaleDist,
        d_sliderRect.y() + d_thumbHalf,
        s.height() - d_thumbLength,
        ScaleDraw::Right);
      break;
  default:
      d_sliderRect.setRect(this->rect().x(), this->rect().x(),
         s.width(), s.height());
      break;
  }
    }
    */
    
    
//   d_sliderRect.setRect(this->rect().x(), this->rect().y(),
//                        s.width(), s.height());
  getActiveArea(); 
  getPixelValues();
  
  
//   int h  = height();
//   int kh = sliderSize().height();
//   int mh  = h - kh;
//   int mw = _meterWidth / _channel;

  int x = d_sliderRect.x();
  int y = d_sliderRect.y();
  int w = d_sliderRect.width();
  int h = d_sliderRect.height();

// For an indented on area:  
//   const int margin = d_thumbHalf / 4;
//   const int margin = 0;
//   switch(d_orient)
//   {
//     case Qt::Vertical:
//       x += margin;
//       w -= 2 * margin;
//     break;
//     
//     case Qt::Horizontal:
//       y += margin;
//       h -= 2 * margin;
//     break;
//   }
  
  _onPixmap  = QPixmap(w, h);
  _offPixmap = QPixmap(w, h);

//   const int x1_a = x;
// //   const int w1 = d_valuePixelWidth;
//   const int x1_b = x1_a + d_valuePixelWidth - 1;
// 
//   const int x2_a = x + d_valuePixelWidth;
// //   const int w2 = w - d_valuePixelWidth;
//   const int x2_b = x2_a + w - d_valuePixelWidth - 1;

  const int y1 = y;
  const int y2 = y1 + h - 1;


  const QPalette& pal = palette();
  
//   QColor yellowRed;
//   yellowRed.setHsv(QColor(Qt::yellow).hue()-8,
//                 QColor(Qt::yellow).saturation(),
//                 QColor(Qt::yellow).value());
//   QColor yellRedRed;
//   yellRedRed.setHsv(QColor(Qt::yellow).hue()-16,
//                 QColor(Qt::yellow).saturation(),
//                 QColor(Qt::yellow).value());

//     QLinearGradient linearGrad(QPointF(0, 0), QPointF(0, mh));
//   QLinearGradient linearGrad_a(x1_a, y1, x1_b, y2);
//   QLinearGradient linearGrad_b(x2_a, y1, x2_b, y2);
//   QLinearGradient linearGrad_a(x1_a, y1, x1_a, y2);
//   QLinearGradient linearGrad_b(x2_a, y1, x2_a, y2);
  QLinearGradient linearGrad_a(x, y1, x, y2);
  
  const QColor c1 = pal.highlight().color().darker(125);
//   const QColor c2 = pal.highlight().color().lighter();
  const QColor c2 = pal.highlight().color();
  
  linearGrad_a.setColorAt(0, c1);
  linearGrad_a.setColorAt(0.5, c2);
  linearGrad_a.setColorAt(1, c1);
  
//   linearGrad_b.setColorAt(0, pal.window().color());
//   linearGrad_b.setColorAt(0, pal.highlight().color().darker());

//   QColor darkYellowRed;
//     darkYellowRed.setHsv(QColor(Qt::darkYellow).hue()-8,
//                       QColor(Qt::darkYellow).saturation(),
//                       QColor(Qt::darkYellow).value());
//     QColor darkYellRedRed;
//     darkYellRedRed.setHsv(QColor(Qt::darkYellow).hue()-16,
//                       QColor(Qt::darkYellow).saturation(),
//                       QColor(Qt::darkYellow).value());
//     QLinearGradient linearDarkGrad(QPointF(0, 0), QPointF(0, mh));
//     linearDarkGrad.setColorAt(0, Qt::darkRed);
//     linearDarkGrad.setColorAt(1-(double)(h1-5)/(double)mh, darkYellRedRed);
//     linearDarkGrad.setColorAt(1-(double)(h1-6)/(double)mh, darkYellowRed);
//     linearDarkGrad.setColorAt(1-(double)h2/(double)mh, Qt::darkYellow);
//     linearDarkGrad.setColorAt(1, Qt::darkGreen);

  if(w > 0 && h > 0)
  {
    QPainter p;
    p.begin(&_onPixmap);
    p.fillRect(0, 0, w, h, linearGrad_a);
    p.end();
    p.begin(&_offPixmap);
      //p.fillRect(0, 0, w, h, linearGrad_b);
      p.fillRect(0, 0, w, h, pal.window());
    p.end();
  }

  updatePainterPaths();
}

void CompactSlider::showEditor()
{
  if(_editMode)
    return;
  
  if(!_editor)
  {
    //fprintf(stderr, "   creating editor\n"); // REMOVE Tim. Trackinfo.
    _editor = new PopupDoubleSpinBox(this);
    _editor->setFrame(false);
    _editor->setFocusPolicy(Qt::WheelFocus);
    _editor->setDecimals(_valueDecimals);
    _editor->setSingleStep(step());
    _editor->setPrefix(valPrefix());
    _editor->setSuffix(valSuffix());
    _editor->setMinimum(minValue());
    _editor->setMaximum(maxValue());
    _editor->setValue(value());
    //connect(_editor, SIGNAL(editingFinished()), SLOT(editingFinished()));
    connect(_editor, SIGNAL(returnPressed()), SLOT(editorReturnPressed()));
    connect(_editor, SIGNAL(escapePressed()), SLOT(editorEscapePressed()));
  }
  int w = width();
  if (w < _editor->sizeHint().width()) 
    w = _editor->sizeHint().width();
  _editor->setGeometry(0, 0, w, height());
  //fprintf(stderr, "   x:%d y:%d w:%d h:%d\n", _editor->x(), _editor->y(), w, _editor->height()); // REMOVE Tim. Trackinfo.
  _editor->selectAll();
  _editMode = true;     
  _editor->show();
  _editor->setFocus();
}

void CompactSlider::getMouseOverThumb(QPoint &p)
{
  int scrollMode;
  int direction;
  //getScrollMode(p, left_btn ? Qt::LeftButton : 0, scrollMode, direction);
  // Force left button in case getScrollMode depends on it.
  getScrollMode(p, Qt::LeftButton, scrollMode, direction);
  const bool v = scrollMode == ScrMouse;
  //if(_mouseOverThumb != v && !(left_btn && !v))
//   if(_mouseOverThumb != v && !(bPressed && !v))
  if(_mouseOverThumb != v && !(_pressed && !v))
  {
    //fprintf(stderr, "CompactSlider::getMouseOverThumb setting _mouseOverThumb:%d to v:%d\n", _mouseOverThumb, v); // REMOVE Tim. Trackinfo.
    _mouseOverThumb = v;
  }
  const bool hv = rect().contains(p);
//   if(_hovered != hv && !bPressed)
  if(_hovered != hv && !_pressed)
    _hovered = hv;
}      

void CompactSlider::setOff(bool v) 
{ 
  if(v && !_hasOffMode)
    _hasOffMode = true;
  if(_off == v)
    return;
  _off = v; 
  update(); 
  emit valueStateChanged(value(), isOff(), id()); 
}

void CompactSlider::setHasOffMode(bool v)
{
  _hasOffMode = v;
  setOff(false);
}

void CompactSlider::setValueState(double v, bool off, ConversionMode mode)
{
  // Do not allow setting value from the external while mouse is pressed.
  if(_pressed)
    return;
  
  bool do_off_upd = false;
  bool do_val_upd = false;
  // Both setOff and setValue emit valueStateChanged and setValue emits valueChanged.
  // We will block them and emit our own here. Respect the current block state.
  const bool blocked = signalsBlocked();
  if(!blocked)
    blockSignals(true);
  if(isOff() != off)
  {
    do_off_upd = true;
    setOff(off); 
  }
//   if(value() != v)
  if(value(mode) != v)
  {
    do_val_upd = true;
//     setValue(v);
    setValue(v, mode);
  }
  if(!blocked)
    blockSignals(false);
  
  if(do_off_upd || do_val_upd)
    update(); 
  if(do_val_upd)
    emit valueChanged(value(), id());
  if(do_off_upd || do_val_upd)
    emit valueStateChanged(value(), isOff(), id()); 
}


//------------------------------------------------------------
//.-
//.F  CompactSlider::valueChange
//  Notify change of value
//
//.u  Syntax
//.f  void CompactSlider::valueChange()
//
//------------------------------------------------------------

void CompactSlider::valueChange()
      {
      // Turn the control back on with any value set.
      // Wanted to make this flaggable, but actually we 
      //  have to in order to see any value changing,
      if(isOff())
        setOff(false);
      
      getPixelValues();

      updatePainterPaths();

      QPoint p = mapFromGlobal(QCursor::pos());
//       int scrollMode;
//       int direction;
//       //getScrollMode(p, left_btn ? Qt::LeftButton : 0, scrollMode, direction);
//       // Force left button in case getScrollMode depends on it.
//       getScrollMode(p, Qt::LeftButton, scrollMode, direction);
//       const bool v = scrollMode == ScrMouse;
//       //if(_mouseOverThumb != v && !(left_btn && !v))
//       if(_mouseOverThumb != v && !(bPressed && !v))
//       {
//         fprintf(stderr, "CompactSlider::valueChange setting _mouseOverThumb:%d to v:%d\n", _mouseOverThumb, v); // REMOVE Tim. Trackinfo.
//         _mouseOverThumb = v;
//       }

      getMouseOverThumb(p);
      update();
      // Emits valueChanged if tracking enabled.
      SliderBase::valueChange();
      // Emit our own combined signal.
      if(tracking())
        emit valueStateChanged(value(), isOff(), id());
      }

//------------------------------------------------------------
//.-  
//.F  CompactSlider::rangeChange
//  Notify change of range
//
//.u  Description
//
//.u  Syntax
//.f  void CompactSlider::rangeChange()
//
//------------------------------------------------------------
void CompactSlider::rangeChange()
{
    if (!hasUserScale())
       d_scale.setScale(minValue(), maxValue(), d_maxMajor, d_maxMinor);
    getPixelValues();
    SliderBase::rangeChange();
    repaint();
}

//------------------------------------------------------------
//
//.F  CompactSlider::setMargins
//  Set distances between the widget's border and
//  internals.
//
//.u  Syntax
//.f  void CompactSlider::setMargins(int hor, int vert)
//
//.u  Parameters
//.p  int hor, int vert -- Margins
//
//------------------------------------------------------------
void CompactSlider::setMargins(int hor, int vert)
{
    d_xMargin = MusECore::qwtMax(0, hor);
//     d_yMargin = MusECore::qwtMin(0, vert); // REMOVE Tim. Trackinfo. BUG Controls were not being given total space. FIXED! Surely this was wrong!
    d_yMargin = MusECore::qwtMax(0, vert);
    resize(this->size());
}

//------------------------------------------------------------
//
//.F  CompactSlider::sizeHint
//  Return a recommended size
//
//.u  Syntax
//.f  QSize CompactSlider::sizeHint() const
//
//.u  Note
//  The return value of sizeHint() depends on the font and the
//  scale.
//------------------------------------------------------------

QSize CompactSlider::sizeHint() const
      {
      /* Scale is not supported
      int w = 40;
      int h = 40;
      QPainter p;
      int msWidth = 0, msHeight = 0;

      if (d_scalePos != None) {
            if (p.begin(this)) {
                  msWidth = d_scale.maxWidth(&p, FALSE);
                  msHeight = d_scale.maxHeight(&p);
                  }
            p.end();

            switch(d_orient) {
                  case Qt::Vertical:
                        w = 2*d_xMargin + d_thumbWidth + msWidth + d_scaleDist + 2;
                        break;
                  case Qt::Horizontal:
                        h = 2*d_yMargin + d_thumbWidth + msHeight + d_scaleDist;
                        break;
                  }
            }
      else {      // no scale
            switch(d_orient) {
                  case Qt::Vertical:
                        w = 16;
                        break;
                  case Qt::Horizontal:
                        h = 16;
                        break;
                  }
            }
      */
   
//       const int font_height = fontMetrics().height();
//       switch(d_orient) {
//             case Qt::Vertical:
//                   return QSize(16, font_height + 2 * d_yMargin);
//                   break;
//             case Qt::Horizontal:
//                   return QSize(16, font_height + 2 * d_yMargin);
//                   break;
//             }
//       return QSize(16, 16);
//       
      
      return getMinimumSizeHint(fontMetrics(), d_orient, d_scalePos, d_xMargin, d_yMargin);
      }

// QSize CompactSlider::minimumSizeHint() const
// {
// //   return sizeHint();
//   return QSize(0, 0);
// }
      
//---------------------------------------------------------
//   setOrientation
//---------------------------------------------------------

void CompactSlider::setOrientation(Qt::Orientation o)
      {
      d_orient = o;
      /* Scale is not supported
      ScaleDraw::OrientationX so = ScaleDraw::Bottom;
      switch(d_orient) {
            case Qt::Vertical:
                  if (d_scalePos == Right)
                        so = ScaleDraw::Right;
                  else
                        so = ScaleDraw::Left;
                  break;
            case Qt::Horizontal:
                  if (d_scalePos == Bottom)
                        so = ScaleDraw::Bottom;
                  else
                        so = ScaleDraw::Top;
                  break;
            }

      d_scale.setGeometry(0, 0, 40, so);
      if (d_orient == Qt::Vertical)
            setMinimumSize(10,20);
      else
            setMinimumSize(20,10);
      QRect r = geometry();
      setGeometry(r.x(), r.y(), r.height(), r.width());
      update();
      */

      }

Qt::Orientation CompactSlider::orientation() const
      {
      return d_orient;
      }

double CompactSlider::lineStep() const
      {
      return 1.0;
      }

double CompactSlider::pageStep() const
      {
      return 1.0;
      }

void CompactSlider::setLineStep(double)
      {
      }

void CompactSlider::setPageStep(double)
      {
      }

void CompactSlider::updatePainterPaths()
{
  if(_offPath)
    delete _offPath;
  if(_onPath)
    delete _onPath;
  
  _offPath = new QPainterPath;
  _onPath = new QPainterPath;

  int x = d_sliderRect.x();
  int y = d_sliderRect.y();
  int w = d_sliderRect.width();
  int h = d_sliderRect.height();

// For an indented on area:  
//   const int xmargin = w / 6;
//   const int ymargin = h / 6;
//   switch(d_orient)
//   {
//     case Qt::Vertical:
//       x += xmargin;
//       w -= 2 * xmargin;
//     break;
//     
//     case Qt::Horizontal:
//       y += ymargin;
//       h -= 2 * ymargin;
//     break;
//   }

  const int x1 = x;
//   const int x1_a = x;
  const int w1 = d_valuePixelWidth;
//   const int x1_b = x1_a + d_valuePixelWidth - 1;

  const int x2 = x + d_valuePixelWidth;
//   const int x2_a = x + d_valuePixelWidth;
  const int w2 = w - d_valuePixelWidth;
//   const int x2_b = x2_a + w - d_valuePixelWidth - 1;

//   const int y1 = y;
//   const int y2 = y1 + h - 1;

  
  
  MusECore::addRoundedPath(_onPath, QRect (x1, y, w1, h), 4, 4, 
    //(MusECore::Corner) (MusECore::UpperLeft | MusECore::UpperRight | MusECore::LowerLeft | MusECore::LowerRight) );
    //(MusECore::Corner) (MusECore::UpperLeft | MusECore::LowerLeft) );
    (MusECore::Corner) (MusECore::UpperRight | MusECore::LowerRight) );
  
  MusECore::addRoundedPath(_offPath, QRect (x2, y, w2, h), 4, 4, 
    //(MusECore::Corner) (MusECore::UpperLeft | MusECore::UpperRight | MusECore::LowerLeft | MusECore::LowerRight) );
    (MusECore::Corner) (MusECore::UpperRight | MusECore::LowerRight) );
}
      
} // namespace MusEGui
