//======================================================================
//  MusE
//  Linux Music Editor
//    $Id: knob.cpp,v 1.3.2.3 2009/03/09 02:05:18 terminator356 Exp $
//
//  Adapted from Qwt Lib:
//  Copyright (C) 1997  Josef Wilgen
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Orcan Ogetbil (ogetbilo at sf.net) completely redesigned.
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
#include "knob.h"
#include <cmath>
#include "mmath.h"

#include <QPainter>
#include <QPalette>
#include <QPaintEvent>
#include <QResizeEvent>

namespace MusEGui {

//---------------------------------------------------------
//  The QwtKnob widget imitates look and behaviour of a volume knob on a radio.
//  It contains
//  a scale around the knob which is set up automatically or can
//  be configured manually (see @^QwtScaleIf@).
//  Automatic scrolling is enabled when the user presses a mouse
//  button on the scale. For a description of signals, slots and other
//  members, see QwtSliderBase@.
//---------------------------------------------------------


//---------------------------------------------------------
//   Knob
//---------------------------------------------------------

Knob::Knob(QWidget* parent, const char* name)
   : SliderBase(parent, name)
      {
      hasScale = false;

      d_borderWidth   = 4;
      d_shineWidth    = 3;
      d_totalAngle    = 270.0;
      d_scaleDist     = 1;
      d_symbol        = Line;
      d_maxScaleTicks = 11;
      d_knobWidth     = 30;
      _faceColSel     = FALSE; 
      d_faceColor     = palette().color(QPalette::Window);
      d_rimColor      = palette().mid().color();
      d_shinyColor    = palette().mid().color();
      d_curFaceColor  = d_faceColor;
      d_altFaceColor  = d_faceColor;
      d_markerColor   = palette().dark().color().darker(125);
      d_dotWidth      = 8;

      l_slope = 0;
      l_const = 100;

      setMinimumSize(30,30);
      setUpdateTime(50);
      }

//------------------------------------------------------------
//  QwtKnob::setTotalAngle
//  Set the total angle by which the knob can be turned
//
//  Syntax
//  void QwtKnob::setTotalAngle(double angle)
//
//  Parameters
//  double angle  --  angle in degrees.
//
//  Description
//  The default angle is 270 degrees. It is possible to specify
//  an angle of more than 360 degrees so that the knob can be
//  turned several times around its axis.
//------------------------------------------------------------

void Knob::setTotalAngle (double angle)
      {
      if (angle < 10.0)
            d_totalAngle = 10.0;
      else
            d_totalAngle = angle;
      d_scale.setAngleRange( -0.5 * d_totalAngle, 0.5 * d_totalAngle);
      }

//------------------------------------------------------------
// Knob::setRange
// Set the range and step size of the knob
//
// Sets the paramaters that define the shininess of the ring
// surrounding the knob and then proceeds by passing the
// parameters to the parent class' setRange() function.
//------------------------------------------------------------

void Knob::setRange(double vmin, double vmax, double vstep, int pagesize)
      {
      //if(vmin == d_minValue && vmax == d_maxValue && vstep == d_step && pageSize == d_pageSize)    // p4.0.45
      //  return;
      
      // divide by zero protection. probably too cautious
      if (! (vmin == vmax || qMax(-vmin, vmax) == 0))
            {
            if (vmin * vmax < 0)
                  l_slope = 80.0 / qMax(-vmin, vmax);
            else
                  {
                  l_slope = 80.0 / (vmax - vmin);
                  l_const = 100 - l_slope * vmin;
                  }
            }
      SliderBase::setRange(vmin, vmax, vstep, pagesize);
      }

//------------------------------------------------------------
//   QwtKnob::drawKnob
//    const QRect &r --   borders of the knob
//------------------------------------------------------------

void Knob::drawKnob(QPainter* p, const QRect& r)
      {
      const QPalette& pal = palette();
      
      QRect aRect;
      aRect.setRect(r.x() + d_borderWidth,
            r.y() + d_borderWidth,
            r.width()  - 2*d_borderWidth,
            r.height() - 2*d_borderWidth);
      
      int width = r.width();
      int height = r.height();
      int size = qMin(width, height);

      p->setRenderHint(QPainter::Antialiasing, true);

      //
      // draw the rim
      //
   
      QLinearGradient linearg(QPoint(r.x(),r.y()), QPoint(size, size));
      linearg.setColorAt(1 - M_PI_4, d_faceColor.lighter(125));
      linearg.setColorAt(M_PI_4, d_faceColor.darker(175));
      p->setBrush(linearg);
      p->setPen(Qt::NoPen);
      p->drawEllipse(r.x(),r.y(),size,size);


      //
      // draw shiny surrounding
      //
    
      QPen pn;
      pn.setCapStyle(Qt::FlatCap);

      pn.setColor(d_shinyColor.lighter(l_const + abs(value() * l_slope)));
      pn.setWidth(d_shineWidth * 2);
      p->setPen(pn);
      p->drawArc(aRect, 0, 360 * 16);
    
      //
      // draw button face
      //
    
      QRadialGradient gradient(size/2, size/2, size-d_borderWidth, size/2-d_borderWidth, size/2-d_borderWidth);
      gradient.setColorAt(0, d_curFaceColor.lighter(150));
      gradient.setColorAt(1, d_curFaceColor.darker(150));
      p->setBrush(gradient);
      p->setPen(Qt::NoPen);
      p->drawEllipse(aRect);
    
      //
      // draw marker
      //
      //drawMarker(p, d_angle, isEnabled() ? d_markerColor : Qt::gray);
      drawMarker(p, d_angle, pal.currentColorGroup() == QPalette::Disabled ? 
                              pal.color(QPalette::Disabled, QPalette::WindowText) : d_markerColor);
      }

//------------------------------------------------------------
//.F  QwtSliderBase::valueChange
//  Notify change of value
//
//.u  Parameters
//  double x  --    new value
//
//.u  Description
//  Sets the slider's value to the nearest multiple
//          of the step size.
//------------------------------------------------------------

void Knob::valueChange()
      {
      recalcAngle();
      d_newVal++;
      repaint(kRect);
      SliderBase::valueChange();
      }

//------------------------------------------------------------
//.F  QwtKnob::getValue
//  Determine the value corresponding to a specified position
//
//.u  Parameters:
//  const QPoint &p -- point
//
//.u  Description:
//  Called by QwtSliderBase
//------------------------------------------------------------

double Knob::getValue(const QPoint &p)
      {
      double newValue;
      double oneTurn;
      double eqValue;
      double arc;

    const QRect& r = rect();

    double dx = double((r.x() + r.width() / 2) - p.x() );
    double dy = double((r.y() + r.height() / 2) - p.y() );

    arc = atan2(-dx,dy) * 180.0 / M_PI;

    newValue =  0.5 * (minValue() + maxValue())
       + (arc + d_nTurns * 360.0) * (maxValue() - minValue())
    / d_totalAngle;

    oneTurn = fabs(maxValue() - minValue()) * 360.0 / d_totalAngle;
    eqValue = value() + d_mouseOffset;

    if (fabs(newValue - eqValue) > 0.5 * oneTurn)
    {
  if (newValue < eqValue)
     newValue += oneTurn;
  else
     newValue -= oneTurn;
    }

    return newValue;  

}



//------------------------------------------------------------
//.-
//.F  QwtKnob::setScrollMode
//  Determine the scrolling mode and direction
//  corresponding to a specified position
//
//.u  Parameters
//  const QPoint &p -- point in question
//
//.u  Description
//  Called by QwtSliderBase
//------------------------------------------------------------
void Knob::getScrollMode( QPoint &p, const Qt::MouseButton &/*button*/, int &scrollMode, int &direction)// prevent compiler warning : unsused parameter 
{
    int dx, dy, r;
    double arc;

    /*Qt::ButtonState but= button ;*/ // prevent compiler warning : unsused variable
    r = kRect.width() / 2;

    dx = kRect.x() + r - p.x();
    dy = kRect.y() + r - p.y();

    if ( (dx * dx) + (dy * dy) <= (r * r)) // point is inside the knob
    {
  scrollMode = ScrMouse;
  direction = 0;
    }
    else                // point lies outside
    {
  scrollMode = ScrTimer;
  arc = atan2(double(-dx),double(dy)) * 180.0 / M_PI;
  if ( arc < d_angle)
     direction = -1;
  else if (arc > d_angle)
     direction = 1;
  else
     direction = 0;
    }
    return;
}



//------------------------------------------------------------
//.F  QwtKnob::rangeChange
//  Notify a change of the range
//
//.u  Description
//  Called by QwtSliderBase
//------------------------------------------------------------

void Knob::rangeChange()
{
    if (!hasUserScale())
    {
  d_scale.setScale(minValue(), maxValue(),
       d_maxMajor, d_maxMinor);
    }
    recalcAngle();
    resize(size());
    repaint();
}

void Knob::mousePressEvent(QMouseEvent *e)
{
  if (e->button() == Qt::MidButton || e->modifiers() & Qt::ControlModifier) {
    int xpos = e->x() - width() /2;
    double v = float(e->y()) / height() * 1.2;

    double halfRange = (maxValue() - minValue())/2;
    double midValue = minValue() + halfRange;
    // apply to range
    if (xpos < 0) { // left values
     v = -v;
    }
    setValue(v * halfRange + midValue);
    SliderBase::valueChange();
    emit sliderMoved(value(),id()); // sliderMoved is used by auxChanged
    
    // fake a left-click to make the knob still "stick" to
    // the mouse.
    QMouseEvent temp(e->type(), e->pos(), Qt::LeftButton, e->buttons(), e->modifiers());
    SliderBase::mousePressEvent(&temp);
    return;
  }
  SliderBase::mousePressEvent(e);
}

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void Knob::resizeEvent(QResizeEvent* ev)
      {
      SliderBase::resizeEvent(ev);
      int width, width_2;

      const QRect& r = rect();

// printf("resize %d %d %d\n", r.height(), r.width(), d_knobWidth);

//      width = MusECore::qwtMin(MusECore::qwtMin(r.height(), r.width()), d_knobWidth);
      width = MusECore::qwtMin(r.height(), r.width());
      width_2 = width / 2;

      int x = r.x() + r.width()  / 2 - width_2;
      int y = r.y() + r.height() / 2 - width_2;

      kRect.setRect(x, y, width, width);

      x = kRect.x() - d_scaleDist;
      y = kRect.y() - d_scaleDist;
      int w = width + 2 * d_scaleDist;

      d_scale.setGeometry(x, y, w, ScaleDraw::Round);
      }

//------------------------------------------------------------
//    paintEvent
//------------------------------------------------------------

void Knob::paintEvent(QPaintEvent*)
      {
/*      QPainter p(this);
      const QRect &r = e->rect();

      if ((r == kRect) && d_newVal ) {        // event from valueChange()
        if (d_newVal > 1)               // lost paintEvents()?
          drawKnob(&p, kRect);
            else {
                  drawMarker(&p, d_oldAngle, d_curFaceColor);
                  drawMarker(&p, d_angle, d_markerColor);
                  }
            }
      else {
            p.eraseRect(rect());
            if (hasScale)
                  d_scale.draw(&p);
            drawKnob(&p, kRect);
            }
      d_newVal = 0;
*/
      
      QPainter p(this);
      p.setRenderHint(QPainter::Antialiasing, true);
      if(hasScale)
        d_scale.draw(&p);
      drawKnob(&p, kRect);
      //drawMarker(&p, d_oldAngle, d_curFaceColor);
      //drawMarker(&p, d_angle, d_markerColor);
 
      d_newVal = 0;
      }

//------------------------------------------------------------
//.-
//.F  QwtKnob::drawMarker
//  Draw the marker at the knob's front
//
//.u  Parameters
//.p  QPainter *p --  painter
//  double arc  --  angle of the marker
//  const QColor &c  -- marker color
//
//.u  Syntax
//        void QwtKnob::drawMarker(QPainter *p)
//
//------------------------------------------------------------
void Knob::drawMarker(QPainter *p, double arc, const QColor &c)
{

    QPen pn;
    int radius;
    double rb,re;
    double rarc;

    rarc = arc * M_PI / 180.0;
    double ca = cos(rarc);
    double sa = - sin(rarc);
        
    radius = kRect.width() / 2 - d_borderWidth + d_shineWidth;
    if (radius < 3) radius = 3; 
    int ym = kRect.y() + radius + d_borderWidth - d_shineWidth;
    int xm = kRect.x() + radius + d_borderWidth - d_shineWidth;

    switch (d_symbol)
    {
    case Dot:
  
  p->setBrush(c);
  p->setPen(Qt::NoPen);
  rb = double(MusECore::qwtMax(radius - 4 - d_dotWidth / 2, 0));
  p->drawEllipse(xm - int(rint(sa * rb)) - d_dotWidth / 2,
           ym - int(rint(ca * rb)) - d_dotWidth / 2,
           d_dotWidth, d_dotWidth);
  
  break;
  
    case Line:
  
  pn.setColor(c);
  pn.setWidth(2);
  p->setPen(pn);
  
  rb = MusECore::qwtMax(double((radius - 1) / 3.0), 0.0);
  re = MusECore::qwtMax(double(radius - 1), 0.0);

  p->setRenderHint(QPainter::Antialiasing, true);
  p->drawLine( xm,
	       ym,
	       xm - int(rint(sa * re)),
	       ym - int(rint(ca * re)));

  break;
    }


}

//------------------------------------------------------------
//
//.F  QwtKnob::setKnobWidth
//    Change the knob's width.
//
//.u  Syntax
//.f  void QwtKnob::setKnobWidth(int w)
//
//.u  Parameters
//.p  int w     --  new width
//
//.u  Description
//    The specified width must be >= 5, or it will be clipped.
//
//------------------------------------------------------------
void Knob::setKnobWidth(int w)
{
    d_knobWidth = MusECore::qwtMax(w,5);
    resize(size());
    repaint();
}

//------------------------------------------------------------
//
//.F  QwtKnob::setBorderWidth
//    Set the knob's border width
//
//.u  Syntax
//.f  void QwtKnob::setBorderWidth(int bw)
//
//.u  Parameters
//.p  int bw -- new border width
//
//------------------------------------------------------------
void Knob::setBorderWidth(int bw)
{
    d_borderWidth = MusECore::qwtMax(bw, 0);
    resize(size());
    repaint();
}

//------------------------------------------------------------
//.-
//.F  QwtKnob::recalcAngle
//    Recalculate the marker angle corresponding to the
//    current value
//
//.u  Syntax
//.f  void QwtKnob::recalcAngle()
//
//------------------------------------------------------------
void Knob::recalcAngle()
{
    d_oldAngle = d_angle;

    //
    // calculate the angle corresponding to the value
    //
    if (maxValue() == minValue())
    {
  d_angle = 0;
  d_nTurns = 0;
    }
    else
    {
  d_angle = (value() - 0.5 * (minValue() + maxValue()))
     / (maxValue() - minValue()) * d_totalAngle;
  d_nTurns = floor((d_angle + 180.0) / 360.0);
  d_angle = d_angle - d_nTurns * 360.0;
  
    }

}

//------------------------------------------------------------
//  setFaceColor
//------------------------------------------------------------
void Knob::setFaceColor(const QColor c) 
{ 
  d_faceColor = c; 
  if(!_faceColSel)
    //update(FALSE);
    repaint();
}

//------------------------------------------------------------
//  setAltFaceColor
//------------------------------------------------------------
void Knob::setAltFaceColor(const QColor c) 
{ 
  d_altFaceColor = c; 
  if(_faceColSel)
    //update(FALSE);
    repaint();
}

//------------------------------------------------------------
//  selectFaceColor
//------------------------------------------------------------
void Knob::selectFaceColor(bool alt) 
{ 
  _faceColSel = alt;
  if(alt)
    d_curFaceColor = d_altFaceColor; 
  else
    d_curFaceColor = d_faceColor; 
  //update(FALSE);
  repaint();
}

//------------------------------------------------------------
//  setMarkerColor
//------------------------------------------------------------
void Knob::setMarkerColor(const QColor c) 
{ 
  d_markerColor = c; 
  //update(FALSE);
  repaint();
}

} // namespace MusEGui
