//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./muse/widgets/slider.cpp $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//  (C) Copyright 2011 Orcan Ogetbil (ogetbilo at sf.net)
//  (C) Copyright 2015-2023 Tim E. Real (terminator356 on sourceforge)
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
#include "mmath.h"

#include <QPainterPath>
#include <QMouseEvent>

#include "utils.h"
#include "slider.h"

// For debugging output: Uncomment the fprintf section.
//#include <stdio.h>
#define DEBUG_SLIDER(dev, format, args...)        // fprintf(dev, format, ##args);
#define DEBUG_SLIDER_PAINT(dev, format, args...)  // fprintf(dev, format, ##args);


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
      d_frame = false;

      d_radius = 4;
      d_radiusHandle = 2;
      d_useGradient = true;

      d_scaleDist   = 2;
      d_scaleStep   = 0.0;
      d_xMargin     = 0;
      d_yMargin     = 0;
      d_mMargin    = 1;

      horizontal_hint = 40;
      vertical_hint = 40;

      // set to sane values to avoid erratic size hint
      //  calculation -> drawing problems (kybos)
      if (orient == Qt::Vertical)
      {
          d_sliderRect.setRect(0, 0, 20, 100);
      }
      else
      {
          d_sliderRect.setRect(0, 0, 100, 20);
      }

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
  adjustScale();
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
    update();
}

void Slider::drawThumb(QPainter *p, QPaintEvent *e)
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

  // Special for log scale: It wants unconverted log values.
  const DoubleRange::ConversionMode cm = d_scale.logarithmic() ? ConvertNone : ConvertDefault;
  // Use the limiting transform in case the value is out of bounds.
  markerPos = d_scale.limTransform(internalValue(cm));
  const int ipos = markerPos - d_thumbHalf;

  QRect mkr;
  QPainterPath tpp;
  if(d_orient == Qt::Horizontal)
  {
    // Thumb painter path.
    tpp = MusECore::roundedPath(
      ipos, d_sliderRect.y(),
      d_thumbLength, d_sliderRect.height(),
      d_radiusHandle, d_radiusHandle,
      (MusECore::Corner) (MusECore::CornerUpperLeft | MusECore::CornerUpperRight | MusECore::CornerLowerLeft | MusECore::CornerLowerRight));
    thumbGrad.setStart(QPointF(ipos, 0));
    thumbGrad.setFinalStop(QPointF(ipos + d_thumbLength, 0));
    mkr = QRect(markerPos, d_sliderRect.y(), 1, d_sliderRect.height());
  }
  else
  {
    // Thumb painter path.
    tpp = MusECore::roundedPath(
      d_sliderRect.x(), ipos,
      d_sliderRect.width(), d_thumbLength,
      d_radiusHandle, d_radiusHandle,
      (MusECore::Corner) (MusECore::CornerUpperLeft | MusECore::CornerUpperRight | MusECore::CornerLowerLeft | MusECore::CornerLowerRight));
    thumbGrad.setStart(QPointF(0, ipos));
    thumbGrad.setFinalStop(QPointF(0, ipos + d_thumbLength));
    mkr = QRect(d_sliderRect.x(), markerPos, d_sliderRect.width(), 1);
  }

  const QBrush thumbbr(thumbGrad);

  QRegion::const_iterator ireg_end = e->region().cend();
  for(QRegion::const_iterator ireg = e->region().cbegin(); ireg != ireg_end; ++ireg)
  {
    const QRect& rect = *ireg;

    // Draw thumb.
    QPainterPath tp;
    tp.addRect(rect);
    tp = (tp & tpp).simplified();
    if(!tp.isEmpty())
    {
      DEBUG_SLIDER_PAINT(stderr, "Slider::drawThumb: thumb\n");
      //printQPainterPath(tp);

      if(d_fillThumb)
        p->fillPath(tp, thumbbr);
      else
      {
        p->setPen(pal.shadow().color());
        p->drawPath(tp);
      }
    }

    // Draw thumb center line.
    const QRect mpp = mkr & rect;
    if(!mpp.isEmpty())
    {
      DEBUG_SLIDER_PAINT(stderr, "Slider::drawThumb: centre line\n");
      //printQPainterPath(mpp);

      p->fillRect(mpp, pal.dark().color());
    }
  }
}

//------------------------------------------------------------
//    drawSlider
//     Draw the slider into the specified rectangle.
//------------------------------------------------------------

void Slider::drawSlider(QPainter *p, QPaintEvent *e)
{
    p->setRenderHint(QPainter::Antialiasing);

    const QPalette& pal = palette();

    QColor f_mask_min(d_fillColor.isValid() ? d_fillColor : pal.highlight().color());
    QColor f_mask_max(f_mask_min);
    if (d_useGradient) {
        f_mask_min.setAlpha(40);
        f_mask_max.setAlpha(255);
    }

    // Special for log scale: It wants unconverted log values.
    const DoubleRange::ConversionMode cm = d_scale.logarithmic() ? ConvertNone : ConvertDefault;
    // Use the limiting transform in case the value is out of bounds.
    markerPos = d_scale.limTransform(internalValue(cm));

    QPainterPath e_rect;
    QPainterPath f_rect;
    QLinearGradient f_mask;
    const QBrush frame_br(d_frameColor);

    if (d_orient == Qt::Horizontal)
    {
        const int rhw = d_grooveRect.x() + d_grooveRect.width() - markerPos - (d_fillThumb ? d_thumbHalf : 0);
        const int lhw = markerPos - (d_fillThumb ? d_thumbHalf : 0) - d_grooveRect.x();

        //
        // Draw groove empty right side
        //

        if(rhw > 0 && d_fillEmptySide)
        {
          e_rect = MusECore::roundedPath(
            markerPos + (d_fillThumb ? d_thumbHalf : 0),
            d_grooveRect.y(),
            rhw,
            d_grooveRect.height(),
            d_radius,
            d_radius,
            (MusECore::Corner) (MusECore::CornerUpperRight | MusECore::CornerLowerRight) );
        }

        //
        // Draw groove full left side
        //

        if(lhw > 0)
        {
          f_mask.setColorAt(0, f_mask_min);
          f_mask.setColorAt(1, f_mask_max);
          f_mask.setStart(QPointF(d_grooveRect.x(), d_grooveRect.y()));
          f_mask.setFinalStop(QPointF(markerPos - (d_fillThumb ? d_thumbHalf : 0), d_grooveRect.y()));

          f_rect = MusECore::roundedPath(
            d_grooveRect.x(),
            d_grooveRect.y(),
            lhw,
            d_grooveRect.height(),
            d_radius,
            d_radius,
            (MusECore::Corner) (MusECore::CornerLowerLeft | MusECore::CornerUpperLeft) );
        }
    }
    else // (d_orient == Qt::Vertical)
    {
        const int lh = d_grooveRect.y() + d_grooveRect.height() - markerPos - (d_fillThumb ? d_thumbHalf : 0);
        const int uh = markerPos - (d_fillThumb ? d_thumbHalf : 0) - d_grooveRect.y();

        //
        // Draw groove empty upper filling
        //

        if(uh > 0 && d_fillEmptySide)
        {
          e_rect = MusECore::roundedPath(
            d_grooveRect.x(),
            d_grooveRect.y(),
            d_grooveRect.width(),
            uh,
            d_radius,
            d_radius,
            (MusECore::Corner) (MusECore::CornerUpperLeft | MusECore::CornerUpperRight) );
        }

        //
        // Draw groove lower filling mask
        //

        if(lh > 0)
        {
          f_mask.setColorAt(0, f_mask_max);
          f_mask.setColorAt(1, f_mask_min);
          f_mask.setStart(QPointF(d_grooveRect.x(), markerPos + (d_fillThumb ? d_thumbHalf : 0)));
          f_mask.setFinalStop(QPointF(d_grooveRect.x(), d_grooveRect.y() + d_grooveRect.height()));

          f_rect = MusECore::roundedPath(
            d_grooveRect.x(),
            markerPos + (d_fillThumb ? d_thumbHalf : 0),
            d_grooveRect.width(),
            lh,
            d_radius,
            d_radius,
            (MusECore::Corner) (MusECore::CornerLowerLeft | MusECore::CornerLowerRight) );
        }
    }

    const QBrush fmask_min_brush(f_mask_min);
    const QBrush fmask_brush(f_mask);
    QPainterPath fin_path;

    QRegion::const_iterator ireg_end = e->region().cend();
    for(QRegion::const_iterator ireg = e->region().cbegin(); ireg != ireg_end; ++ireg)
    {
      const QRect& rect = *ireg;
      QPainterPath tp;
      tp.addRect(rect);

      DEBUG_SLIDER_PAINT(stderr, "slider::drawSlider: region: x:%d y:%d w:%d h:%d\n", rect.x(), rect.y(), rect.width(), rect.height());

      fin_path = (e_rect & tp).simplified();
      if(!fin_path.isEmpty())
      {
        DEBUG_SLIDER_PAINT(stderr, "slider::drawSlider:: e_rect\n");
        //printQPainterPath(fin_path);

        p->fillPath(e_rect, fmask_min_brush);
      }

      fin_path = (f_rect & tp).simplified();
      if(!fin_path.isEmpty())
      {
        DEBUG_SLIDER_PAINT(stderr, "slider::drawSlider:: f_rect\n");
        //printQPainterPath(fin_path);

        p->fillPath(f_rect, fmask_brush);
      }

      if (d_frame)
      {
        fin_path = (d_framePath & tp).simplified();
        if(!fin_path.isEmpty())
        {
          DEBUG_SLIDER_PAINT(stderr, "slider::drawSlider:: frame\n");
          //printQPainterPath(fin_path);

          p->fillPath(d_framePath, frame_br);
        }
      }
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
  const double val = internalValue(ConvertDefault);

  if(borderlessMouse() && d_scrollMode != ScrDirect)
  {
    DEBUG_SLIDER(stderr, "Slider::getValue value:%.20f p x:%d y:%d step:%.20f x change:%.20f\n",
                         val, p.x(), p.y(), step(), p.x() * step());
    if(d_orient == Qt::Horizontal)
      return convertTo(val + p.x() * step(), ConvertDefault);
    else
      return convertTo(val - p.y() * step(), ConvertDefault);
  }

  const double min = internalMinValue(ConvertDefault);
  const double max = internalMaxValue(ConvertDefault);
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
  return(convertTo(rv, ConvertDefault));
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
  double rv = d_valAccum;
  const QRect r = d_sliderRect;

  const double fine_factor = fineMode ? 0.2 : 1.0;
  const double step_factored = step() * fine_factor;
  const double val = internalValue(ConvertDefault);

  if(borderlessMouse() && d_scrollMode != ScrDirect)
  {
    DEBUG_SLIDER(stderr, "Slider::moveValue value:%.20f p x:%d y:%d step:%.20f x change:%.20f\n",
                         val, deltaP.x(), deltaP.y(), step(), deltaP.x() * step());

    double newval;
    if(d_orient == Qt::Horizontal)
      newval = val + deltaP.x() * step_factored;
    else
      newval = val - deltaP.y() * step_factored;
    d_valAccum = convertTo(newval, ConvertDefault); // Reset.
    return d_valAccum;
  }

  const double min = internalMinValue(ConvertDefault);
  const double max = internalMaxValue(ConvertDefault);
  const double drange = max - min;

  if(d_orient == Qt::Horizontal)
  {
    if(r.width() <= d_thumbLength)
      return convertTo(0.5 * (min + max), ConvertDefault);
    else
    {
        const double dpos = double(deltaP.x());
        const double dwidth = double(r.width() - d_thumbLength);
        const double dval_diff = (drange * dpos * fine_factor) / dwidth;

        const double valacc = convertFrom(d_valAccum, ConvertDefault) + dval_diff;
        d_valAccum = convertTo(valacc, ConvertDefault);

        DEBUG_SLIDER(stderr, "Slider::moveValue Horizontal value:%.20f p dx:%d dy:%d drange:%.20f"
          " step:%.20f dval_diff:%.20f d_valAccum:%.20f rv:%.20f\n",
          val, deltaP.x(), deltaP.y(), drange, step(), dval_diff, d_valAccum, rv);

      // If it's integer or log + integer.
      if(integer())
        return rint(d_valAccum / step()) * step();
      else
        return convertTo(rint(valacc / step_factored) * step_factored, ConvertDefault);
    }
  }
  else
  {
    if(r.height() <= d_thumbLength)
      return convertTo(0.5 * (min + max), ConvertDefault);
    else
    {
        const double dpos = double(-deltaP.y());
        const double dheight = double(r.height() - d_thumbLength);
        const double dval_diff = (drange * dpos * fine_factor) / dheight;

        const double valacc = convertFrom(d_valAccum, ConvertDefault) + dval_diff;
        d_valAccum = convertTo(valacc, ConvertDefault);

        DEBUG_SLIDER(stderr, "Slider::moveValue Vertical value:%.20f p dx:%d dy:%d drange:%.20f step:%.20f"
          " dval_diff:%.20f d_valAccum:%.20f rv:%.20f\n",
          val, deltaP.x(), deltaP.y(), drange, step(), dval_diff, d_valAccum, rv);

      // If it's integer or log + integer.
      if(integer())
        return rint(d_valAccum / step()) * step();
      else
        return convertTo(rint(valacc / step_factored) * step_factored, ConvertDefault);
    }
  }

  return rv;
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
  if(modifiers & Qt::ControlModifier || button == Qt::MiddleButton)
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
        QPoint cp;

        if(d_orient == Qt::Horizontal)
        {
          // Special for log scale: It wants unconverted log values.
          const DoubleRange::ConversionMode cm = d_scale.logarithmic() ? ConvertNone : ConvertDefault;
          // Use the limiting transform in case the value is out of bounds.
          mp = d_scale.limTransform(internalValue(cm));

          p.setX(mp);
          cp = mapToGlobal( QPoint(mp, p.y()) );
        }
        else
        {
          // Special for log scale: It wants unconverted log values.
          const DoubleRange::ConversionMode cm = d_scale.logarithmic() ? ConvertNone : ConvertDefault;
          // Use the limiting transform in case the value is out of bounds.
          mp = d_scale.limTransform(internalValue(cm));

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

void Slider::paintEvent(QPaintEvent* ev)
{
  QPainter p(this);
  if(d_grooveWidth != 0)
    drawSlider(&p, ev);

  if(d_thumbLength != 0)
    drawThumb(&p, ev);

  if(d_scalePos != ScaleNone)
  {
    // If any part of the total requested rectangle is part of the scale area,
    //  draw the whole scale. TODO Try to refine scale draw routine.
    if(!(ev->rect() & d_scaleRect).isEmpty())
    {
      DEBUG_SLIDER_PAINT(stderr, "Slider::paintEvent: Drawing scale\n");

      //p.fillRect(rect(), palette().window());
      p.setRenderHint(QPainter::Antialiasing, false);
      // Special for log scale: It wants unconverted log values.
      const DoubleRange::ConversionMode cm = d_scale.logarithmic() ? ConvertNone : ConvertDefault;
      d_scale.draw(&p, palette(), internalValue(cm));
    }
  }
}

void Slider::adjustSize(const QSize& /*s*/)
{
    const QFontMetrics fm = fontMetrics();

    const QRect cr = contentsRect();
    const int w  = cr.width() - 2 * d_xMargin;
    const int h  = cr.height() - 2 * d_yMargin;
    const int left = cr.x() + d_xMargin;
    const int rightEnd = left + w;
    //const int right = rightEnd - 1;
    const int top = cr.y() + d_yMargin;
    const int botEnd = top + h;
    //const int bot = botEnd - 1;

    // reposition slider
    if(d_orient == Qt::Horizontal)
    {
      const int xoff = qMax(d_thumbHalf, d_scale.originOffsetHint(fm, true).x());
      const int slxfin = left + xoff;
      const int slwfin = w - 2 * xoff;
      const int scxfin = slxfin + d_thumbHalf;
      const int scwfin = slwfin - d_thumbLength;

      switch(d_scalePos)
      {
        case ScaleLeftOrTop:
        {
            const int smy = d_scale.maxHeight(fm);
            d_scaleRect.setRect(slxfin, top, slwfin, smy);
            d_scaleGeom.setRect(scxfin, top, scwfin, smy);
            d_spacerRect.setRect(left, top + smy, w, d_scaleDist);
            d_sliderRect.setRect(slxfin, top + smy + d_scaleDist, slwfin, h - smy);
            d_scale.setGeometry(scxfin, top, scwfin);
            break;
        }

        case ScaleRightOrBottom:
        {
            const int smy = d_scale.maxHeight(fm);
            d_sliderRect.setRect(slxfin, top, slwfin, h - smy - d_scaleDist);
            d_spacerRect.setRect(left, h - smy - d_scaleDist, w, d_scaleDist);
            d_scaleRect.setRect(slxfin, botEnd - smy, slwfin, smy);
            d_scaleGeom.setRect(scxfin, botEnd - smy, scwfin, smy);
            d_scale.setGeometry(scxfin, botEnd - smy, scwfin);
            break;
        }

        case ScaleInside:
            d_scaleRect.setRect(slxfin, top, slwfin, h);
            d_scaleGeom.setRect(scxfin, top, scwfin, h);
            d_spacerRect.setRect(left, top, 0, 0);
            d_sliderRect.setRect(slxfin, top, slwfin, h);;
            d_scale.setGeometry(scxfin, top, scwfin);
            break;

        case ScaleNone:
            d_sliderRect.setRect(left, top, w, h);
            d_spacerRect.setRect(left, top, 0, 0);
            d_scaleRect.setRect(left, top, 0, 0);
            d_scaleGeom.setRect(left, top, 0, 0);
            // Give the translators something to work with.
            d_scale.setGeometry(left, top, w);
            break;
      }
      d_grooveRect.setRect(
        d_sliderRect.x() + d_thumbHalf,
        d_sliderRect.y() + d_sliderRect.height() / 2 - d_grooveWidth / 2,
        d_sliderRect.width() - d_thumbLength,
        d_grooveWidth
        );
    }
    else // d_orient == Qt::Vertical
    {
      const int yoff = qMax(d_scale.originOffsetHint(fm, true).y() - d_thumbHalf, 0);
      const int slyfin = top + yoff;
      const int slhfin = h - 2 * yoff;
      const int scyfin = slyfin + d_thumbHalf;
      const int schfin = slhfin - d_thumbLength;
      switch(d_scalePos)
      {
        case ScaleLeftOrTop:
        {
            const int smx = d_scale.maxWidth(fm, false);
            d_scaleRect.setRect(left, slyfin, smx, slhfin);
            d_scaleGeom.setRect(left, scyfin, smx, schfin);
            d_spacerRect.setRect(left + smx, top, d_scaleDist, h);
            d_sliderRect.setRect(left + smx + d_scaleDist, slyfin, w - smx, slhfin);
            d_scale.setGeometry(left, scyfin, schfin);
            break;
        }

        case ScaleRightOrBottom:
        {
            const int smx = d_scale.maxWidth(fm, false);
            d_sliderRect.setRect(left, slyfin, w - smx - d_scaleDist, slhfin);
            d_spacerRect.setRect(w - smx - d_scaleDist, top, d_scaleDist, h);
            d_scaleRect.setRect(rightEnd - smx, slyfin, smx, slhfin);
            d_scaleGeom.setRect(rightEnd - smx, scyfin, smx, schfin);
            d_scale.setGeometry(rightEnd - smx, scyfin, schfin);
            break;
        }

        case ScaleInside:
        {
            const int mxlw = d_scale.maxLabelWidth(fm, false);
            const int sclw = d_scale.scaleWidth();
            const int sldoffs = mxlw > d_thumbWidth ? ((mxlw - d_thumbWidth) / 2) : 0;
            const int grooveoffs = d_thumbWidth > d_grooveWidth ? (d_thumbWidth - d_grooveWidth) / 2 : 0;
            const int scloffs = qMax(d_grooveWidth + grooveoffs, mxlw);

            d_sliderRect.setRect(
              left + sldoffs,
              slyfin,
              d_thumbWidth,
              slhfin
            );

            // Not used.
            d_spacerRect.setRect(left, top, 0, 0);

            d_scaleRect.setRect(
              left + grooveoffs /*+ d_scaleDist*/,
              scyfin,
              mxlw + sclw,
              schfin
            );

            d_scale.setGeometry(
              left + scloffs /*+ mxlw*/ + sclw /*+ d_scaleDist*/ - 3,
              scyfin,
              schfin
            );

            break;
        }

        case ScaleNone:
            d_sliderRect.setRect(left, top, w, h);
            d_spacerRect.setRect(left, top, 0, 0);
            d_scaleRect.setRect(left, top, 0, 0);
            d_scaleGeom.setRect(left, top, 0, 0);
            // Give the translators something to work with.
            d_scale.setGeometry(left, top, h);
            break;
      }
      d_grooveRect.setRect(
        d_sliderRect.x() + d_sliderRect.width() / 2 - d_grooveWidth / 2,
        d_sliderRect.y() + d_thumbHalf,
        d_grooveWidth,
        d_sliderRect.height() - d_thumbLength
        );
    }

  QPainterPath fr_path;
  if (d_frame)
  {
    fr_path.addRoundedRect(d_grooveRect, d_radius, d_radius);
    QPainterPath fr_inside_path;
    fr_inside_path.addRoundedRect(d_grooveRect.adjusted(1, 1, -1, -1), d_radius, d_radius);
    fr_path = (fr_path - fr_inside_path).simplified();
  }
  d_framePath = fr_path;

  adjustScale();
}

void Slider::adjustScale()
{
  const double range = internalMaxValue(ConvertDefault) - internalMinValue(ConvertDefault);
  if(range == 0.0)
    return;

  int maxMaj = 5;
  int maxMin = 3;
  double mstep = scaleStep();

  QFontMetrics fm = fontMetrics();
  if(d_orient == Qt::Horizontal)
  {
    int unit_w = fm.horizontalAdvance("888.8888");
    if(unit_w == 0)
      unit_w = 20;

    if(hasUserScale())
    {
      // Don't mess with the step in scale log mode, it can end up way too high.
      // Typically we want the scale log step to remain at 1 decade.
      // FIXME: Find a way to scale it.
      if(!d_scale.scaleDiv().logScale() && d_scaleRect.width() != 0)
      {
        const int fact = (int)(3.0 * range / (double)(d_scaleRect.width())) + 1;
        mstep *= fact;
      }
    }
    else
    {
      maxMaj = (int)((double)(d_scaleRect.width()) / (1.5 * ((double)unit_w)));
      if(maxMaj < 1)
        maxMaj = 1;
      if(maxMaj > 5)
        maxMaj = 5;
    }
    maxMin = (int)((double)(d_scaleRect.width()) / (1.5 * ((double)unit_w)));
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
      // Don't mess with the step in scale log mode, it can end up way too high.
      // Typically we want the scale log step to remain at 1 decade.
      // FIXME: Find a way to scale it.
      if(!d_scale.scaleDiv().logScale() && d_scaleRect.height() != 0)
      {
        const int fact = (int)(3.0 * range / (double)(d_scaleRect.height())) + 1;
        mstep *= fact;
      }
    }
    else
    {
      maxMaj = (int)((double)(d_scaleRect.height()) / (1.5 * ((double)unit_h)));
      if(maxMaj < 1)
        maxMaj = 1;
      if(maxMaj > 5)
        maxMaj = 5;
    }
    maxMin = (int)((double)(d_scaleRect.height()) / (1.5 * ((double)unit_h)));
    if(maxMin < 1)
      maxMin = 1;
    if(maxMin > 5)
      maxMin = 5;
  }

  DEBUG_SLIDER(stderr, "Slider::adjustScale: maxMaj:%d maxMin:%d scaleStep:%f\n", maxMaj, maxMin, mstep);
  d_maxMajor = maxMaj;
  d_maxMinor = maxMin;
  if(hasUserScale())
  {
    const ScaleDiv& sd = d_scale.scaleDiv();
    // Keep the existing minimum and maximum, and log scale option.
    d_scale.setScale(sd.lBound(), sd.hBound(), d_maxMajor, d_maxMinor, mstep, sd.logScale());
  }
  else
    d_scale.setScale(internalMinValue(), internalMaxValue(), d_maxMajor, d_maxMinor, mstep, log());

  updateGeometry();
  update();
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

void Slider::setScale(double vmin, double vmax, ScaleIf::ScaleType scaleType, double dBFactor, double logFactor)
{
  ScaleIf::setScale(vmin, vmax, scaleType, dBFactor, logFactor);
}

void Slider::setScale(double vmin, double vmax, double step, ScaleIf::ScaleType scaleType, double dBFactor, double logFactor)
{
  ScaleIf::setScale(vmin, vmax, step, scaleType, dBFactor, logFactor);
}

void Slider::setScale(const ScaleDiv &s)
{
  ScaleIf::setScale(s);
}

void Slider::setScaleMaxMajor(int ticks)
{
  ScaleIf::setScaleMaxMajor(ticks);
}

void Slider::setScaleMaxMinor(int ticks)
{
  ScaleIf::setScaleMaxMinor(ticks);
}

void Slider::setScaleBackBone(bool v)
{
  ScaleIf::setScaleBackBone(v);
  // Must adjust the scale.
  adjustScale();
}

void Slider::partialUpdate()
{
  // Special for log scale: It wants unconverted log values.
  const DoubleRange::ConversionMode cm = d_scale.logarithmic() ? ConvertNone : ConvertDefault;
  const int prevPix = d_scale.limTransform(prevValue(cm));
  const int curPix = d_scale.limTransform(internalValue(cm));
  QRegion reg;

  // If the left or lower side is using a gradient, we must update that
  //  whole side since the gradient changes with value.
  if(d_useGradient)
  {
    switch(d_orient)
    {
      case Qt::Vertical:
      {
        const int y1 = qMin(prevPix, curPix) - d_thumbHalf;
        const int y2 = d_sliderRect.y() + d_sliderRect.height();
        reg += QRect(
          d_sliderRect.x(),
          y1,
          d_sliderRect.width(),
          y2 - y1);
      }
      break;

      case Qt::Horizontal:
      {
        const int x1 = d_sliderRect.x();
        const int x2 = qMax(prevPix, curPix) + d_thumbHalf;
        reg += QRect(
          x1,
          d_sliderRect.y(),
          x2 - d_sliderRect.x(),
          d_sliderRect.height());
      }
      break;
    }
  }
  else
  {
    const int p1 = qMin(prevPix, curPix) - d_thumbHalf;
    const int p2 = qMax(prevPix, curPix) + d_thumbHalf;
    switch(d_orient)
    {
      case Qt::Vertical:
      {
        reg += QRect(
          d_sliderRect.x(),
          p1,
          d_sliderRect.width(),
          p2 - p1);
      }
      break;

      case Qt::Horizontal:
      {
        reg += QRect(
          p1,
          d_sliderRect.y(),
          p2 - p1,
          d_sliderRect.height());
      }
      break;
    }
  }

  if(!reg.isEmpty())
    update(reg);
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
      // Region-based update more fine-grained than update().
      partialUpdate();

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
    adjustScale();
    SliderBase::rangeChange();
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

      if (d_scalePos != ScaleNone)
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
              case ScaleLeftOrTop:
              case ScaleRightOrBottom:
                w = contentsMargins().left() + contentsMargins().right() + 2*d_xMargin + d_thumbWidth + smw;
              break;

              case ScaleInside:
              {
                const int mxlw = d_scale.maxLabelWidth(fm, false);
                const int sclw = d_scale.scaleWidth();
                const int scloffs = d_thumbWidth > mxlw ? ((d_thumbWidth - mxlw) /*/ 2*/) : 0;
                const int aw = scloffs + mxlw + sclw + d_scaleDist;
                w = contentsMargins().left() + contentsMargins().right() + 2*d_xMargin + aw;
              }
              break;

              case ScaleNone:
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
              case ScaleLeftOrTop:
              case ScaleRightOrBottom:
                h = contentsMargins().top() + contentsMargins().bottom() + 2*d_yMargin + d_thumbWidth + smh;
              break;

              case ScaleInside:
              {
                const int ah = smh > d_thumbWidth ? smh : d_thumbWidth;
                h = contentsMargins().top() + contentsMargins().bottom() + 2*d_yMargin + ah;
              }
              break;

              case ScaleNone:
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
                w = d_thumbWidth + 2*d_xMargin;
                h = vertical_hint;
                break;
          case Qt::Horizontal:
                w = horizontal_hint;
                h = d_thumbWidth + 2*d_yMargin;
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
      setScalePos(d_scalePos);
      }

Qt::Orientation Slider::orientation() const
      {
      return d_orient;
      }

QMargins Slider::scaleEndpointsMargins() const
{
  const QPoint offs = d_scale.originOffsetHint(fontMetrics(), true);
  const int ioffs = orientation() == Qt::Vertical ? offs.y() : offs.x();
  const int mg = qMax(d_thumbHalf, ioffs);
  int l, r, t, b;
  if(orientation() == Qt::Vertical)
  {
    l = r = 0;
    t = mg + d_yMargin + contentsMargins().top();
    b = mg + d_yMargin + contentsMargins().bottom();
  }
  else
  {
    l = mg + d_xMargin + contentsMargins().left();
    r = mg + d_xMargin + contentsMargins().right();
    t = b = 0;
  }
  return QMargins(l, t, r, b);
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

int Slider::grooveWidth() const { return d_grooveWidth; }
void Slider::setGrooveWidth(int w) { d_grooveWidth = w; updateGeometry(); update(); }

//  QColor Slider::fillColor() const { return d_fillColor; }
void Slider::setFillColor(const QColor& color) { d_fillColor = color; update(); }
void Slider::setHandleColor(const QColor& color) { d_handleColor = color; update(); }

bool Slider::fillThumb() const { return d_fillThumb; }
void Slider::setFillThumb(bool v) { d_fillThumb = v; updateGeometry(); update(); }

bool Slider::fillEmptySide() const { return d_fillEmptySide; }
void Slider::setFillEmptySide(bool v) { d_fillEmptySide = v; updateGeometry(); update(); }

void Slider::setRadius(int r) { d_radius = r; updateGeometry(); update(); }
void Slider::setRadiusHandle(int r) { d_radiusHandle = r; updateGeometry(); update(); }
void Slider::setHandleHeight(int h) { d_thumbLength = h; updateGeometry(); update(); }
void Slider::setHandleWidth(int w) { d_thumbWidth = w; d_thumbHalf = d_thumbLength / 2; updateGeometry(); update(); }
void Slider::setUseGradient(bool b) { d_useGradient = b; updateGeometry(); update(); }
void Slider::setScalePos(const ScalePos& s)
{
  d_scalePos = s;
  switch(d_orient)
  {
    case Qt::Vertical:
      switch(d_scalePos)
      {
        case ScaleLeftOrTop:
          d_scale.setOrientation(ScaleDraw::Left);
        break;
        case ScaleRightOrBottom:
          d_scale.setOrientation(ScaleDraw::Right);
        break;
        case ScaleInside:
          d_scale.setOrientation(ScaleDraw::InsideVertical);
        break;
        case ScaleNone:
          // At least set a sensible direction so the translators work.
          d_scale.setOrientation(ScaleDraw::InsideVertical);
        break;
      }
    break;

    case Qt::Horizontal:
      switch(d_scalePos)
      {
        case ScaleLeftOrTop:
          d_scale.setOrientation(ScaleDraw::Top);
        break;
        case ScaleRightOrBottom:
          d_scale.setOrientation(ScaleDraw::Bottom);
        break;
        case ScaleInside:
          d_scale.setOrientation(ScaleDraw::InsideHorizontal);
        break;
        case ScaleNone:
          // At least set a sensible direction so the translators work.
          d_scale.setOrientation(ScaleDraw::InsideHorizontal);
        break;
      }
    break;
  }
  updateGeometry();
  update();
}
void Slider::setFrame(bool b) { d_frame = b; updateGeometry(); update(); }
void Slider::setFrameColor(QColor c) { d_frameColor = c; update(); }
int Slider::scaleDist() const { return d_scaleDist; }
void Slider::setScaleDist(int d) {d_scaleDist = d; updateGeometry(); update(); }


} // namespace MusEGui
