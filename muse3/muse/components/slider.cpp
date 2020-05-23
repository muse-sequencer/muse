//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./muse/widgets/slider.cpp $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//  (C) Copyright 2011 Orcan Ogetbil (ogetbilo at sf.net)
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
#include "mmath.h"

#include <QPainter>
#include <QResizeEvent>

#include "utils.h"
#include "slider.h"

// For debugging output: Uncomment the fprintf section.
#define DEBUG_SLIDER(dev, format, args...) // fprintf(dev, format, ##args);


namespace MusEGui {

//-------------------------------------------------------------
//  Slider - The Slider Widget
//
//  Slider is a slider widget which operates on an interval
//  of type double. Slider supports different layouts as
//  well as a scale.
//------------------------------------------------------------

//------------------------------------------------------------
//.F  Slider::Slider
//
//    Constructor
//
//.u  Syntax:
//.f  Slider::Slider(QWidget *parent, const char *name, Orientation orient = Horizontal, ScalePos scalePos = None, int bgStyle = BgTrough)
//
//.u  Parameters
//.p
//  QWidget *parent --  parent widget
//  const char *name -- The Widget's name. Default = 0.
//  Orientation Orient -- Orientation of the slider. Can be Slider::Horizontal
//        or Slider::Vertical.
//                    Defaults to Horizontal.
//  ScalePos scalePos --  Position of the scale.  Can be Slider::None,
//        Slider::Left, Slider::Right, Slider::Top,
//        or Slider::Bottom. Defaults to Slider::None.  !!! CURRENTLY only Slider::None supported - oget 20110913
//  QColor fillcolor -- the color used to fill in the full side
//        of the Slider. If fillColor is invalid (default) it will draw with the palette highlight colour.
//------------------------------------------------------------

Slider::Slider(QWidget *parent, const char *name,
               Qt::Orientation orient,
               ScalePos scalePos, 
               int grooveWidth, 
               QColor fillColor, 
               ScaleDraw::TextHighlightMode textHighlightMode,
               QColor handleColor)
      : SliderBase(parent,name), d_scalePos(scalePos), d_grooveWidth(grooveWidth),
        d_fillColor(fillColor), d_handleColor(handleColor)
      {
      setPagingButtons(Qt::RightButton);
      
      d_thumbLength = 16;
      d_thumbHalf = 8;
      d_thumbWidth = 16;
      d_fillThumb = true;
      d_fillEmptySide = true;

      d_radius = 4;
      d_radiusHandle = 2;
      d_useGradient = true;

      d_scaleDist   = 4;
      d_scaleStep   = 0.0;
      d_xMargin     = 0;
      d_yMargin     = 0;
      d_mMargin    = 1;

      horizontal_hint = 40;
      vertical_hint = 40;
      
      d_sliderRect.setRect(0, 0, 8, 8);
      setOrientation(orient);
      d_scale.setTextHighlightMode(textHighlightMode);
      }

//------------------------------------------------------------
//.F  Slider::setSizeHint
//------------------------------------------------------------

void Slider::setSizeHint(uint w, uint h)
      {
      horizontal_hint = w;
      vertical_hint = h;
      }

//------------------------------------------------------------
//.F  Slider::~Slider
//    Destructor
//.u  Syntax
//.f  Slider::~Slider()
//------------------------------------------------------------

Slider::~Slider()
      {
      }

//----------------------------------------------------
//
//.F  Slider::setThumbLength
//
//    Set the slider's thumb length
//
//.u  Syntax
//  void Slider::setThumbLength(int l)
//
//.u  Parameters
//.p  int l   --    new length
//
//-----------------------------------------------------
void Slider::setThumbLength(int l)
{
//     d_thumbLength = MusECore::qwtMax(l,8);
    d_thumbLength = l;
    d_thumbHalf = d_thumbLength / 2;
    resize(size());
}

//------------------------------------------------------------
//
//.F  Slider::setThumbWidth
//  Change the width of the thumb
//
//.u  Syntax
//.p  void Slider::setThumbWidth(int w)
//
//.u  Parameters
//.p  int w -- new width
//
//------------------------------------------------------------
void Slider::setThumbWidth(int w)
{
    d_thumbWidth = MusECore::qwtMax(w,4);
    resize(size());
}


//------------------------------------------------------------
//.-  
//.F  Slider::scaleChange
//  Notify changed scale
//
//.u  Syntax
//.f  void Slider::scaleChange()
//
//.u  Description
//  Called by QwtScaledWidget
//
//------------------------------------------------------------
void Slider::scaleChange()
{
    if (!hasUserScale())
       d_scale.setScale(minValue(), maxValue(), d_maxMajor, d_maxMinor);
    update();
}


//------------------------------------------------------------
//.-
//.F  Slider::fontChange
//  Notify change in font
//  
//.u  Syntax
//.f   Slider::fontChange(const QFont &oldFont)
//
//------------------------------------------------------------
void Slider::fontChange(const QFont & /*oldFont*/)
{
//     repaint();
    update();
}

void Slider::drawThumb(QPainter *p, const QRect &r)
{
  p->setRenderHint(QPainter::Antialiasing);

  QColor thumb_edge;
  QColor thumb_center;
  const QPalette& pal = palette();
  if (d_handleColor.isValid()) {
      thumb_edge = d_handleColor;
      thumb_center = d_handleColor.lighter();
  } else {
      thumb_edge = pal.dark().color();
      thumb_center = pal.mid().color();
  }
  QLinearGradient thumbGrad;
  thumbGrad.setColorAt(0, thumb_edge);
  thumbGrad.setColorAt(0.5, thumb_center);
  thumbGrad.setColorAt(1, thumb_edge);
  
  const double rpos = (value(ConvertNone) - minValue(ConvertNone)) / (maxValue(ConvertNone) - minValue(ConvertNone));

  if(d_orient == Qt::Horizontal)
  {
    int crh, thh;
//     if(d_scalePos == InsideHorizontal)
//     {
//       crh = height() - r.y() - d_yMargin - 2 * d_mMargin;
//       thh = height() - r.y() - d_yMargin - d_mMargin;
//     }
//     else
    {
      crh = r.height() - 2 * d_mMargin;
      thh = r.height();
    }
    
    const QRect cr(r.x(),
                   r.y() + d_mMargin,
                   r.width(),
                   //r.height() - 2*d_mMargin);
                   crh);
    
    const int dist1 = int(double(cr.width() - d_thumbLength) * rpos);
    const int ipos =  cr.x() + dist1;
    markerPos = ipos + d_thumbHalf;
    
    //
    //  Draw thumb
    //
        
    QPainterPath thumb_rect = MusECore::roundedPath(ipos, r.y(), 
                                          //d_thumbLength, r.height(), 
                                          d_thumbLength, thh, 
                                          d_radiusHandle, d_radiusHandle,
                                          (MusECore::Corner) (MusECore::UpperLeft | MusECore::UpperRight | MusECore::LowerLeft | MusECore::LowerRight) );

//     thumbGrad.setStart(QPointF(0, cr.y()));
//     thumbGrad.setFinalStop(QPointF(0, cr.y() + cr.height()));
    thumbGrad.setStart(QPointF(ipos, 0));
    thumbGrad.setFinalStop(QPointF(ipos + d_thumbLength, 0));
        
    if(d_fillThumb)
      p->fillPath(thumb_rect, QBrush(thumbGrad));
    else
    {
      p->setPen(pal.shadow().color());
      p->drawPath(thumb_rect);
    }
        
    // center line
    p->fillRect(ipos + d_thumbHalf, cr.y(), 1, cr.height(), pal.dark().color());
  }
  else
  {
    int crw, thw;
//     if(d_scalePos == InsideVertical)
//     {
//       crw = width() - r.x() - d_xMargin - 2 * d_mMargin;
//       thw = width() - r.x() - d_xMargin - d_mMargin;
//     }
//     else
    {
      crw = r.width() - 2 * d_mMargin;
      thw = r.width();
    }
    
    const QRect cr(r.x() + d_mMargin,
                   r.y(),
                   //r.width() - 2*d_mMargin,
                   crw,
                   r.height());
    
    const int dist1 = int(double(cr.height() - d_thumbLength) * (1.0 - rpos));
    const int ipos = cr.y() + dist1;
    markerPos = ipos + d_thumbHalf;
    
    //
    //  Draw thumb
    //
        
    QPainterPath thumb_rect = MusECore::roundedPath(r.x(), ipos, 
                                          //r.width(), d_thumbLength,
                                          thw, d_thumbLength,
                                          d_radiusHandle, d_radiusHandle,
                                          (MusECore::Corner) (MusECore::UpperLeft | MusECore::UpperRight | MusECore::LowerLeft | MusECore::LowerRight) );
        
//     thumbGrad.setStart(QPointF(cr.x(), 0));
//     thumbGrad.setFinalStop(QPointF(cr.x() + cr.width(), 0));
    thumbGrad.setStart(QPointF(0, ipos));
    thumbGrad.setFinalStop(QPointF(0, ipos + d_thumbLength));
        
    if(d_fillThumb)
      p->fillPath(thumb_rect, QBrush(thumbGrad));
    else
    {
      p->setPen(pal.shadow().color());
      p->drawPath(thumb_rect);
    }
        
    // center line
    p->fillRect(cr.x(), ipos + d_thumbHalf, cr.width(), 1, pal.dark().color());
  }
  
}

//------------------------------------------------------------
//    drawSlider
//     Draw the slider into the specified rectangle.  
//------------------------------------------------------------

void Slider::drawSlider(QPainter *p, const QRect &r)
{
    p->setRenderHint(QPainter::Antialiasing);

    const QPalette& pal = palette();

    // for the full side
    const double rpos = (value(ConvertNone)  - minValue(ConvertNone)) / (maxValue(ConvertNone) - minValue(ConvertNone));
    
    QColor f_mask_min(d_fillColor.isValid() ? d_fillColor : pal.highlight().color());
    QColor f_mask_max(f_mask_min);
    if (d_useGradient) {
        f_mask_min.setAlpha(40);
        //f_mask_max.setAlpha(200);
        f_mask_max.setAlpha(255);
    }
    QLinearGradient f_mask;
	   
    if (d_orient == Qt::Horizontal)
        {

        const QRect cr(r.x(),
                       r.y() + r.height() / 2 - d_grooveWidth / 2,
                       r.width(),
                       d_grooveWidth);


        //
        // Draw background
        //
        
        const int dist1 = int(double(cr.width() - (d_fillThumb ? d_thumbLength : d_thumbHalf)) * rpos);
        const int ipos =  cr.x() + dist1;
        markerPos = ipos + d_thumbHalf;

        //
        // Draw groove empty right side
        // 
	   
        if(d_fillEmptySide)
        {
          QPainterPath e_rect = MusECore::roundedPath(ipos + (d_fillThumb ? d_thumbLength : d_thumbHalf), cr.y(), 
                                            cr.width() - (d_fillThumb ? d_thumbLength : d_thumbHalf) - dist1, cr.height(), 
                                            d_radius, d_radius, (MusECore::Corner) (MusECore::UpperRight | MusECore::LowerRight) );
    
          p->fillPath(e_rect, f_mask_min);
        }
   
   
        //
        // Draw groove full left side
        //
           
        f_mask.setColorAt(0, f_mask_min);
        f_mask.setColorAt(1, f_mask_max);
        f_mask.setStart(QPointF(cr.x(), cr.y()));
        f_mask.setFinalStop(QPointF(cr.x() + ipos + (d_fillThumb ? 0 : d_thumbHalf), cr.y()));
          
        QPainterPath f_rect = MusECore::roundedPath(cr.x(), cr.y(), 
                                          ipos + (d_fillThumb ? 0 : d_thumbHalf), cr.height(),
                                          d_radius, d_radius,
                                          (MusECore::Corner) (MusECore::LowerLeft | MusECore::UpperLeft) );

        p->fillPath(f_rect, QBrush(f_mask));
        }
    else // (d_orient == Qt::Vertical)
        {
        const QRect cr(r.x() + r.width() / 2 - d_grooveWidth / 2,
                       r.y(),
                       d_grooveWidth,
                       r.height());

        //
        // Draw background
        //
        
        const int dist1 = int(double(cr.height() - (d_fillThumb ? d_thumbLength : d_thumbHalf)) * (1.0 - rpos));
        const int ipos = cr.y() + dist1;
        markerPos = ipos + d_thumbHalf;

        //
        // Draw groove empty upper filling
        // 

        if(d_fillEmptySide)
        {
          QPainterPath e_rect = MusECore::roundedPath(cr.x(), cr.y(), 
                                            cr.width(), ipos + (d_fillThumb ? 0 : d_thumbHalf),
                                            d_radius, d_radius,
                                            (MusECore::Corner) (MusECore::UpperLeft | MusECore::UpperRight) );
              
          p->fillPath(e_rect, QBrush(f_mask_min));
        }            
            
        //
        // Draw groove lower filling mask
        //

        f_mask.setColorAt(0, f_mask_max);
        f_mask.setColorAt(1, f_mask_min);
        f_mask.setStart(QPointF(cr.x(), markerPos));
        f_mask.setFinalStop(QPointF(cr.x(), cr.y() + cr.height()));
            
        QPainterPath f_rect = MusECore::roundedPath(cr.x(), ipos + (d_fillThumb ? d_thumbLength : d_thumbHalf), 
                                          cr.width(), cr.height() - (d_fillThumb ? d_thumbLength : d_thumbHalf) - dist1,
                                          d_radius, d_radius, (MusECore::Corner) (MusECore::LowerLeft | MusECore::LowerRight) );
	    
        p->fillPath(f_rect, QBrush(f_mask));
        }

}

//------------------------------------------------------------
//.-
//.F  Slider::getValue
//  Determine the value corresponding to a specified
//  mouse location.
//
//.u  Syntax
//.f     double Slider::getValue(const QPoint &p)
//
//.u  Parameters
//.p  const QPoint &p --
//
//.u  Description
//  Called by SliderBase
//------------------------------------------------------------
double Slider::getValue( const QPoint &p)
{
  double rv;
  const QRect r = d_sliderRect;
  const double val = value(ConvertNone);

  if(borderlessMouse() && d_scrollMode != ScrDirect)
  {
    DEBUG_SLIDER(stderr, "Slider::getValue value:%.20f p x:%d y:%d step:%.20f x change:%.20f\n", 
                         val, p.x(), p.y(), step(), p.x() * step());
    if(d_orient == Qt::Horizontal)
      return val + p.x() * step();
    else
      return val - p.y() * step();
  }
  
  const double min = minValue(ConvertNone);
  const double max = maxValue(ConvertNone);
  const double drange = max - min;
  
  if(d_orient == Qt::Horizontal)
  {
    if(r.width() <= d_thumbLength)
      rv = 0.5 * (min + max);
    else
    {
      const double dpos = double(p.x() - r.x() - d_thumbHalf);
      const double dwidth = double(r.width() - d_thumbLength);
      rv  =  min + rint(drange * dpos / dwidth / step()) * step();
    }
  }
  else
  {
    if(r.height() <= d_thumbLength)
      rv = 0.5 * (min + max);
    else
    {
      const double dpos = double(p.y() - r.y() - d_thumbHalf);
      double dheight = double(r.height() - d_thumbLength);
      rv =  min + rint(drange * (1.0 - dpos / dheight) / step()) * step();
    }
  }
  return(rv);
}

//------------------------------------------------------------
//
//.F  Slider::moveValue
//  Determine the value corresponding to a specified mouse movement.
//
//.u  Syntax
//.f  void Slider::moveValue(const QPoint &deltaP, bool fineMode)
//
//.u  Parameters
//.p  const QPoint &deltaP -- Change in position
//.p  bool fineMode -- Fine mode if true, coarse mode if false.
//
//.u  Description
//    Called by SliderBase
//    Coarse mode (the normal mode) maps pixels to values depending on range and width,
//     such that the slider follows the mouse cursor. Fine mode maps one step() value per pixel.
//------------------------------------------------------------
double Slider::moveValue(const QPoint &deltaP, bool fineMode)
{
  double rv;
  const QRect r = d_sliderRect;

  const double val = value(ConvertNone);

  if((fineMode || borderlessMouse()) && d_scrollMode != ScrDirect)
  {
    DEBUG_SLIDER(stderr, "Slider::moveValue value:%.20f p x:%d y:%d step:%.20f x change:%.20f\n", 
                         val, deltaP.x(), deltaP.y(), step(), deltaP.x() * step());
    
    double newval;
    if(d_orient == Qt::Horizontal)
      newval = val + deltaP.x() * step();
    else
      newval = val - deltaP.y() * step();
    d_valAccum = newval; // Reset.
    return newval;
  }
  
  const double min = minValue(ConvertNone);
  const double max = maxValue(ConvertNone);
  const double drange = max - min;

  if(d_orient == Qt::Horizontal)
  {
    if(r.width() <= d_thumbLength)
      rv = 0.5 * (min + max);
    else
    {
      const double dpos = double(deltaP.x());
      const double dwidth = double(r.width() - d_thumbLength);
      const double dval_diff = (drange * dpos) / dwidth;
      d_valAccum += dval_diff;
      rv = rint(d_valAccum / step()) * step();
      
      DEBUG_SLIDER(stderr, "Slider::moveValue Horizontal value:%.20f p dx:%d dy:%d drange:%.20f step:%.20f dval_diff:%.20f d_valAccum:%.20f rv:%.20f\n", 
                       val, deltaP.x(), deltaP.y(), drange, step(), dval_diff, d_valAccum, rv);
    }
  }
  else
  {
    if(r.height() <= d_thumbLength)
      rv = 0.5 * (min + max);
    else
    {
      const double dpos = double(-deltaP.y());
      const double dheight = double(r.height() - d_thumbLength);
      const double dval_diff = (drange * dpos) / dheight;
      d_valAccum += dval_diff;
      rv = rint(d_valAccum / step()) * step();
      
      DEBUG_SLIDER(stderr, "Slider::moveValue Vertical value:%.20f p dx:%d dy:%d drange:%.20f step:%.20f dval_diff:%.20f d_valAccum:%.20f rv:%.20f\n", 
                       val, deltaP.x(), deltaP.y(), drange, step(), dval_diff, d_valAccum, rv);
    }
  }
  return(rv);
}

//------------------------------------------------------------
//.-
//.F  Slider::getScrollMode
//  Determine scrolling mode and direction
//
//.u  Syntax
//.f   void Slider::getScrollMode( const QPoint &p, int &scrollMode, int &direction )
//
//.u  Parameters
//.p  const QPoint &p -- point
//
//.u  Description
//  Called by SliderBase
//
//------------------------------------------------------------
void Slider::getScrollMode( QPoint &p, const Qt::MouseButton &button, const Qt::KeyboardModifiers& modifiers, int &scrollMode, int &direction )
{
  // If modifier or button is held, jump directly to the position at first.
  // After handling it, the caller can change to SrcMouse scroll mode.
  if(modifiers & Qt::ControlModifier || button == Qt::MidButton)
  {
    scrollMode = ScrDirect;
    direction = 0;
    return;
  }
  
  if(borderlessMouse())
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
    if(cursorHoming() && button == Qt::LeftButton)
    {
      if(d_sliderRect.contains(p))
      {
        DEBUG_SLIDER(stderr, "Slider::getScrollMode cursor homing + left button: ScrMouse\n");
        scrollMode = ScrMouse;
        direction = 0;

        int mp = 0;
        QRect cr;
        QPoint cp;
        int ipos,dist1;
        double rpos;

        cr = d_sliderRect;
  
        rpos = (value(ConvertNone)  - minValue(ConvertNone)) / (maxValue(ConvertNone) - minValue(ConvertNone));
  
        if(d_orient == Qt::Horizontal)
        {
          dist1 = int(double(cr.width() - d_thumbLength) * rpos);
          ipos =  cr.x() + dist1;
          mp = ipos + d_thumbHalf;
        
          p.setX(mp);
          cp = mapToGlobal( QPoint(mp, p.y()) );
        }  
        else
        {
          dist1 = int(double(cr.height() - d_thumbLength) * (1.0 - rpos));
          ipos = cr.y() + dist1;
          mp = ipos + d_thumbHalf;
          p.setY(mp);
          cp = mapToGlobal( QPoint(p.x(), mp) );
        }  
        cursor().setPos(cp.x(), cp.y());
        return;
      }
    }
    else
    {
      int currentPos;
      if(d_orient == Qt::Horizontal)
       currentPos = p.x();
      else
       currentPos = p.y();
      
      if(d_sliderRect.contains(p))
      {
        if((currentPos > markerPos - d_thumbHalf)  
            && (currentPos < markerPos + d_thumbHalf))
        {
          DEBUG_SLIDER(stderr, "Slider::getScrollMode ScrMouse\n");
          scrollMode = ScrMouse;
          direction = 0;
          return;
        }
        else if(pagingButtons().testFlag(button))
        {
          DEBUG_SLIDER(stderr, "Slider::getScrollMode ScrPage\n");
          scrollMode = ScrPage;
          if (((currentPos > markerPos) && (d_orient == Qt::Horizontal))
              || ((currentPos <= markerPos) && (d_orient != Qt::Horizontal)))
            direction = 1;
          else
            direction = -1;
          return;
        }
      }
    }
  }
  
  scrollMode = ScrNone;
  direction = 0;
}

//------------------------------------------------------------
//.F  Slider::paintEvent
//  Qt paint event
//
//.u  Syntax
//.f  void Slider::paintEvent(QPaintEvent *e)
//------------------------------------------------------------

void Slider::paintEvent(QPaintEvent* /*ev*/)
{
  QPainter p(this);
  if(d_grooveWidth != 0)
    drawSlider(&p, d_sliderRect);

  if(d_thumbLength != 0)
    drawThumb(&p, d_sliderRect);
  if(d_scalePos != None) 
  {
//     p.fillRect(rect(), palette().window());
    p.setRenderHint(QPainter::Antialiasing, false);
    d_scale.draw(&p, palette(), value());
  }
}

void Slider::adjustSize(const QSize& s)
{
    const QFontMetrics fm = fontMetrics();
    const int sliderWidth = d_thumbWidth;
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

        case InsideHorizontal:
            d_sliderRect.setRect(this->rect().x() + d_xMargin,
              this->rect().y() + s.height() - 1
              - d_yMargin - sliderWidth,
              s.width() - 2 * d_xMargin,
              sliderWidth);
            d_scale.setGeometry(d_sliderRect.x() + d_thumbHalf,
              //d_sliderRect.y() - d_scaleDist,
              this->rect().y() + d_yMargin + d_scale.maxHeight(fm) + d_scaleDist,
              d_sliderRect.width() - d_thumbLength,
              ScaleDraw::InsideHorizontal);
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

        case InsideVertical:
        {
//             d_sliderRect.setRect(this->rect().x() + s.width()
//               - sliderWidth - 1 - d_xMargin,
//             d_sliderRect.setRect(this->rect().x() + d_xMargin,
//             d_sliderRect.setRect(this->rect().x() + d_xMargin + d_scale.maxLabelWidth(fm, false) - sliderWidth,
            const int mxlw = d_scale.maxLabelWidth(fm, false);
            const int sclw = d_scale.scaleWidth();
            const int sldw = mxlw > sliderWidth ? sliderWidth : mxlw;
            const int sldoffs = mxlw > sliderWidth ? ((mxlw - sldw) / 2) : 0;
            const int fh = fm.ascent() + 2;
            const int fh2 = fh / 2;
            const int margin = d_thumbLength > fh ? d_thumbLength : fh;
            const int margin2 = d_thumbHalf > fh2 ? d_thumbHalf : fh2;
            const int sldymargin = fh > d_thumbLength ? fh - d_thumbLength : 0;
            const int sldymargin2 = fh2 > d_thumbHalf ? fh2 - d_thumbHalf : 0;

//             d_sliderRect.setRect(this->rect().x() + (s.width() - 1) - sliderWidth - sclw + sldoffs, // - d_xMargin,
            d_sliderRect.setRect(this->rect().x() + s.width() - sliderWidth - sclw + sldoffs, // - d_xMargin,
//               this->rect().y() + d_yMargin,
              this->rect().y() + d_yMargin + sldymargin2,
              sliderWidth,
//               s.height() - 2 * d_yMargin);
//               s.height() - margin - 2 * d_yMargin);
              s.height() - sldymargin - 2 * d_yMargin);

            //d_scale.setGeometry(d_sliderRect.x() - d_scaleDist,
//             d_scale.setGeometry(this->rect().x() + d_xMargin + d_scale.maxWidth(fm, false) + d_scaleDist,
            d_scale.setGeometry(this->rect().x() + d_xMargin + mxlw + sclw + d_scaleDist,
//               d_sliderRect.y() + d_thumbHalf,
//               d_sliderRect.y(),
              this->rect().y() + d_yMargin + margin2,
//               s.height() - d_thumbLength,
//               s.height() - margin,
//               d_sliderRect.height(),
              s.height() - margin - 2 * d_yMargin,
              ScaleDraw::InsideVertical);
        }
        break;

        default:
            d_sliderRect.setRect(this->rect().x(), this->rect().x(),
              s.width(), s.height());
            break;
      }
    }

  adjustScale();
}

void Slider::adjustScale()
{
  const double range = maxValue() - minValue();
  if(range == 0.0)
    return;

  int maxMaj = 5;
  int maxMin = 3;
  double mstep = scaleStep();

  QFontMetrics fm = fontMetrics();
  if(d_orient == Qt::Horizontal)
  {
// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
    int unit_w = fm.horizontalAdvance("888.8888");
#else
    int unit_w = fm.width("888.8888");
#endif
    if(unit_w == 0)
      unit_w = 20;

    if(hasUserScale())
    {
      if(d_sliderRect.width() != 0)
      {
        const int fact = (int)(3.0 * range / (double)(d_sliderRect.width())) + 1;
        mstep *= fact;
      }
    }
    else
    {
      maxMaj = (int)((double)(d_sliderRect.width()) / (1.5 * ((double)unit_w)));
      if(maxMaj < 1)
        maxMaj = 1;
      if(maxMaj > 5)
        maxMaj = 5;
    }
    maxMin = (int)((double)(d_sliderRect.width()) / (1.5 * ((double)unit_w)));
    if(maxMin < 1)
      maxMin = 1;
    if(maxMin > 5)
      maxMin = 5;
  }
  else
  {
    int unit_h = fm.height();
    if(unit_h == 0)
      unit_h = 20;

    if(hasUserScale())
    {
      if(d_sliderRect.height() != 0)
      {
        const int fact = (int)(3.0 * range / (double)(d_sliderRect.height())) + 1;
        mstep *= fact;
      }
    }
    else
    {
      maxMaj = (int)((double)(d_sliderRect.height()) / (1.5 * ((double)unit_h)));
      if(maxMaj < 1)
        maxMaj = 1;
      if(maxMaj > 5)
        maxMaj = 5;
    }
    maxMin = (int)((double)(d_sliderRect.height()) / (1.5 * ((double)unit_h)));
    if(maxMin < 1)
      maxMin = 1;
    if(maxMin > 5)
      maxMin = 5;
  }

  DEBUG_SLIDER(stderr, "Slider::adjustScale: maxMaj:%d maxMin:%d scaleStep:%f\n", maxMaj, maxMin, mstep);
  d_maxMajor = maxMaj;
  d_maxMinor = maxMin;
  if(hasUserScale())
    d_scale.setScale(minValue(), maxValue(), d_maxMajor, d_maxMinor, mstep, log());
  else
    d_scale.setScale(minValue(), maxValue(), d_maxMajor, d_maxMinor, log());
//   update();
  updateGeometry();
}

//------------------------------------------------------------
//.F  Slider::resizeEvent
//  Qt resize event
//
//.u  Parameters
//.p  QResizeEvent *e
//
//.u  Syntax
//.f  void Slider::resizeEvent(QResizeEvent *e)
//------------------------------------------------------------

void Slider::resizeEvent(QResizeEvent *e)
{
    SliderBase::resizeEvent(e);
    adjustSize(e->size());
}

void Slider::setScale(double vmin, double vmax, int logarithmic)
{
  ScaleIf::setScale(vmin, vmax, logarithmic);
  // Must adjust the scale.
   adjustScale();
}

void Slider::setScale(double vmin, double vmax, double step, int logarithmic)
{
  ScaleIf::setScale(vmin, vmax, step, logarithmic);
  // Must adjust the scale.
  adjustScale();
}

void Slider::setScale(const ScaleDiv &s)
{
  ScaleIf::setScale(s);
  // Must adjust the scale.
   adjustScale();
}

void Slider::setScaleMaxMajor(int ticks)
{
  ScaleIf::setScaleMaxMajor(ticks);
  // Must adjust the scale.
   adjustScale();
}

void Slider::setScaleMaxMinor(int ticks)
{
  ScaleIf::setScaleMaxMinor(ticks);
  // Must adjust the scale.
   adjustScale();
}

void Slider::setScaleBackBone(bool v)
{
  ScaleIf::setScaleBackBone(v);
  // Must adjust the scale.
  adjustScale();
}

//------------------------------------------------------------
//.-
//.F  Slider::valueChange
//  Notify change of value
//
//.u  Syntax
//.f  void Slider::valueChange()
//
//------------------------------------------------------------

void Slider::valueChange()
      {
      update();
      
      // HACK
      // In direct mode let the inherited classes (this) call these in their valueChange() methods, 
      //  so that they may be called BEFORE valueChanged signal is emitted by the setPosition() call above.
      // ScrDirect mode only happens once upon press with a modifier. After that, another mode is set.
      // Hack: Since valueChange() is NOT called if nothing changed, in that case these are called for us by the SliderBase.
      if(d_scrollMode == ScrDirect)
      {
        processSliderPressed(id());
        emit sliderPressed(value(), id());
      }
      
      // Emits valueChanged if tracking enabled.
      SliderBase::valueChange();
      }

//------------------------------------------------------------
//.-  
//.F  Slider::rangeChange
//  Notify change of range
//
//.u  Description
//
//.u  Syntax
//.f  void Slider::rangeChange()
//
//------------------------------------------------------------
void Slider::rangeChange()
{
    if (!hasUserScale())
       d_scale.setScale(minValue(), maxValue(), d_maxMajor, d_maxMinor);
    SliderBase::rangeChange();
//     repaint();
    update();
}

//------------------------------------------------------------
//
//.F  Slider::setMargins
//  Set distances between the widget's border and
//  internals.
//
//.u  Syntax
//.f  void Slider::setMargins(int hor, int vert)
//
//.u  Parameters
//.p  int hor, int vert -- Margins
//
//------------------------------------------------------------
void Slider::setMargins(int hor, int vert)
{
    d_xMargin = MusECore::qwtMax(0, hor);
    d_yMargin = MusECore::qwtMax(0, vert);
    resize(this->size());
}

//------------------------------------------------------------
//
//.F  Slider::sizeHint
//  Return a recommended size
//
//.u  Syntax
//.f  QSize Slider::sizeHint() const
//
//.u  Note
//  The return value of sizeHint() depends on the font and the
//  scale.
//------------------------------------------------------------

QSize Slider::sizeHint() const
      {
      int w = 40;
      int h = 40;
      const QFontMetrics fm = fontMetrics();
      int msWidth = 0, msHeight = 0;

      if (d_scalePos != None) 
      {
        msWidth = d_scale.maxWidth(fm, false);
        msHeight = d_scale.maxHeight(fm);

        switch(d_orient) 
        {
          case Qt::Vertical:
          {
            h = vertical_hint;
            const int smw = msWidth + d_scaleDist;
            switch(d_scalePos)
            {
              case Left:
              case Right:
                w = 2*d_xMargin + d_thumbWidth + smw + 2;
              break;
              
              case InsideVertical:
              {
                const int aw = smw > d_thumbWidth ? smw : d_thumbWidth;
                w = 2*d_xMargin + aw + 2;
              }
              break;
              
              case Top:
              case Bottom:
              case InsideHorizontal:
              case None:
              break;
            }
          }
          break;
            
          case Qt::Horizontal:
          {
            w = horizontal_hint;
            const int smh = msHeight + d_scaleDist;
            switch(d_scalePos)
            {
              case Top:
              case Bottom:
                h = 2*d_yMargin + d_thumbWidth + smh;
              break;
              
              case InsideHorizontal:
              {
                const int ah = smh > d_thumbWidth ? smh : d_thumbWidth;
                h = 2*d_yMargin + ah;
              }
              break;
              
              case Left:
              case Right:
              case InsideVertical:
              case None:
              break;
            }
          }
          break;
        }
      }
      else
      {      // no scale
        switch(d_orient) 
        {
          case Qt::Vertical:
                w = 16;
                h = vertical_hint;
                break;
          case Qt::Horizontal:
                h = 16;
                w = horizontal_hint;
                break;
        }
      }
      return QSize(w, h);
      }

//---------------------------------------------------------
//   setOrientation
//---------------------------------------------------------

void Slider::setOrientation(Qt::Orientation o)
      {
      d_orient = o;
      ScaleDraw::OrientationX so = ScaleDraw::Bottom;
      switch(d_orient) {
            case Qt::Vertical:
                  switch(d_scalePos)
                  {
                    case Right:
                      so = ScaleDraw::Right;
                    break;
                    case Left:
                      so = ScaleDraw::Left;
                    break;
                    case InsideVertical:
                      so = ScaleDraw::InsideVertical;
                    break;
                    case Bottom:
                    case Top:
                    case InsideHorizontal:
                    case None:
                    break;
                  }
            break;
            case Qt::Horizontal:
                  switch(d_scalePos)
                  {
                    case Bottom:
                      so = ScaleDraw::Bottom;
                    break;
                    case Top:
                      so = ScaleDraw::Top;
                    break;
                    case InsideHorizontal:
                      so = ScaleDraw::InsideHorizontal;
                    break;
                    case Right:
                    case Left:
                    case InsideVertical:
                    case None:
                    break;
                  }
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
      }

Qt::Orientation Slider::orientation() const
      {
      return d_orient;
      }

double Slider::lineStep() const
      {
      return 1.0;
      }

double Slider::pageStep() const
      {
      return 1.0;
      }

void Slider::setLineStep(double)
      {
      }

void Slider::setPageStep(double)
      {
      }

} // namespace MusEGui
