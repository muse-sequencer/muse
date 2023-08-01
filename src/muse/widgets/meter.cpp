//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: meter.cpp,v 1.4.2.2 2009/05/03 04:14:00 terminator356 Exp $
//  redesigned by oget on 2011/08/15
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Orcan Ogetbil (ogetbilo at sf.net)
//  (C) Copyright 2011-2023 Tim E. Real (terminator356 on users DOT sourceforge DOT net)
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

#include <QVector>
#include <QLocale>
#include <algorithm>

#include "meter.h"
#include "fastlog.h"

// For debugging output: Uncomment the fprintf section.
//#include <stdio.h>
#define DEBUG_METER(dev, format, args...)        // fprintf(dev, format, ##args);
#define DEBUG_METER_PAINT(dev, format, args...)  // fprintf(dev, format, ##args);

namespace MusEGui {

//---------------------------------------------------------
//   MeterLayout
//---------------------------------------------------------

MeterLayout::MeterLayout(QWidget* parent)
  : QVBoxLayout(parent)
{
  _hlayout = new QHBoxLayout();
  addLayout(_hlayout);
}

QHBoxLayout* MeterLayout::hlayout()
{ 
  return _hlayout;
}

//---------------------------------------------------------
//   Meter
//---------------------------------------------------------

Meter::Meter(QWidget* parent, 
             bool isInteger, bool isLog,
             Qt::Orientation orient, 
             double scaleMin, double scaleMax,
             ScalePos scalePos, 
             const QColor& primaryColor,
             ScaleDraw::TextHighlightMode textHighlightMode,
             int refreshRate)
   : QFrame(parent), 
     _primaryColor(primaryColor), 
     _scalePos(scalePos),
     _refreshRate(refreshRate) //Qt::WNoAutoErase
      {
      setBackgroundRole(QPalette::NoRole);
      setAttribute(Qt::WA_NoSystemBackground);
      setAttribute(Qt::WA_StaticContents);
      // This is absolutely required for speed! Otherwise painfully slow because of full background 
      //  filling, even when requesting small udpdates! Background is drawn by us. (Just small corners.)
      setAttribute(Qt::WA_OpaquePaintEvent);    
      //setFrameStyle(QFrame::Raised | QFrame::StyledPanel);

//       QFont fnt;
//       fnt.setFamily("Sans");
//       fnt.setPixelSize(9);
//       //fnt.setStyleStrategy(QFont::PreferBitmap);
//       fnt.setStyleStrategy(QFont::NoAntialias);
//       fnt.setHintingPreference(QFont::PreferVerticalHinting);
//       setFont(fnt);
// //       setStyleSheet("font: 9px \"Sans\"; ");
//       setStyleSheet(MusECore::font2StyleSheet(fnt));
      
      _isLog = isLog;
      _isInteger = isInteger;
      _dBFactor = 20.0;
      _dBFactorInv = 1.0/_dBFactor;
      _logFactor = 1.0;

      _VUSizeHint = QSize(10, 10);
      _reverseDirection = false;
      d_scale.setTextHighlightMode(textHighlightMode);
      _scaleDist    = 2;
      _showText     = false;
      overflow      = false;
      cur_pixv      = -1;     // Flag as -1 to initialize in paint.
      last_pixv     = 0;
      cur_pixmax    = 0;
      last_pixmax   = 0;
      val           = 0.0;
      targetVal     = 0.0;
      targetValStep = 0.0;
      maxVal        = 0.0;
      targetMaxVal  = 0.0;
      minScale = minScaleLog = scaleMin;
      maxScale = maxScaleLog = scaleMax;
      setOrientation(orient);

      // If it's integer or log integer.
      if(_isInteger)
      {
        scaleMin = rint(scaleMin);
        scaleMax = rint(scaleMax);
      }

      if(_isLog)
      {
        if(_isInteger)
        {
          // Force a hard lower limit of integer 1.
          if(scaleMin <= 0.0)
            scaleMin = 1.0;
          if(scaleMax <= 0.0)
            scaleMax = 1.0;
          scaleMin /= _logFactor;
          scaleMax /= _logFactor;
          minScaleLog = scaleMin;
          maxScaleLog = scaleMax;
          minScale = _dBFactor * MusECore::fast_log10(scaleMin);
          maxScale = _dBFactor * MusECore::fast_log10(scaleMax);
        }
        else
        {
          // Force a hard lower limit of -120 dB.
          if(scaleMin <= 0.0)
          {
            minScaleLog = 0.000001;
            minScale = -120;
          }
          else
            minScale = _dBFactor * MusECore::fast_log10(scaleMin);

          if(scaleMax <= 0.0)
          {
            maxScaleLog = 0.000001;
            maxScale = -120;
          }
          else
            maxScale = _dBFactor * MusECore::fast_log10(scaleMax);
        }
      }

      yellowScale = -10;
      redScale    = 0;
      setLineWidth(0);
      setMidLineWidth(0);

      _radius = 4;
      _vu3d = true;
      _frame = false;
      _frameColor = Qt::darkGray;

      ensurePolished();

      if (_vu3d) {
          dark_red_end = QColor(0x8e0000);
          dark_red_begin = QColor(0x8e3800);

          darkGradRed.setColorAt(1, dark_red_begin);
          darkGradRed.setColorAt(0, dark_red_end);

          dark_yellow_end = QColor(0x8e6800);
          dark_yellow_center = QColor(0x8e8e00);
          dark_yellow_begin = QColor(0x6a8400);

          darkGradYellow.setColorAt(1, dark_yellow_begin);
          darkGradYellow.setColorAt(0.5, dark_yellow_center);
          darkGradYellow.setColorAt(0, dark_yellow_end);

          mask_center = QColor(225, 225, 225, 64);
          mask_edge = QColor(30, 30, 30, 64);

          maskGrad.setColorAt(0, mask_edge);
          maskGrad.setColorAt(0.5, mask_center);
          maskGrad.setColorAt(1, mask_edge);
      }

      light_red_end = QColor(0xff0000);
      light_red_begin = QColor(0xdd8800);

      light_yellow_end = QColor(0xddcc00);
      light_yellow_center = QColor(0xffff00);
      light_yellow_begin = QColor(0xddff00);

      lightGradYellow.setColorAt(1, light_yellow_begin);
      lightGradYellow.setColorAt(0.5, light_yellow_center);
      lightGradYellow.setColorAt(0, light_yellow_end);

      lightGradRed.setColorAt(1, light_red_begin);
      lightGradRed.setColorAt(0, light_red_end);

      separator_color = QColor(0x666666);
      peak_color = QColor(0xeeeeee);

      connect(&fallingTimer, SIGNAL(timeout()), this, SLOT(updateTargetMeterValue()));

      setPrimaryColor(_primaryColor);
      
//       updateText(targetVal);
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
void Meter::scaleChange()
{
  adjustScale();
}

QSize Meter::sizeHint() const
{
  int w = 40;
  int h = 40;
  const QFontMetrics fm = fontMetrics();
  const QMargins mg = contentsMargins();
  const int fw = frameWidth();
  const int vufw = _frame ? 1 : 0;

  const int msWidth = d_scale.maxWidth(fm, false);
  const int msHeight = d_scale.maxHeight(fm);

  switch(_orient)
  {
    case Qt::Vertical:
    {
      w = _VUSizeHint.width();
      switch(_scalePos)
      {
        case ScaleLeftOrTop:
        case ScaleRightOrBottom:
          w += msWidth + _scaleDist;
        break;

        case ScaleInside:
          if(msWidth > w)
            w = msWidth;
        break;

        case ScaleNone:
        break;
      }
      w += mg.left() + mg.right() + 2 * fw + 2 * vufw;
    }
    break;

    case Qt::Horizontal:
    {
      h = _VUSizeHint.height();
      switch(_scalePos)
      {
        case ScaleLeftOrTop:
        case ScaleRightOrBottom:
          h += msHeight + _scaleDist;
        break;

        case ScaleInside:
          if(msHeight > h)
            h = msHeight;
        break;

        case ScaleNone:
        break;
      }
      h += mg.top() + mg.bottom() + 2 * fw + 2 * vufw;
    }
    break;
  }

  DEBUG_METER(stderr, "Meter::sizeHint w:%d, h:%d\n", w, h);
  return QSize(w, h);
}

//---------------------------------------------------------
//   updateText
//---------------------------------------------------------

void Meter::updateText(double val)
{
  if(val > minScaleLog)
    _text = locale().toString(val, 'f', 1);
  else
  {
    _text = QString("-");
    _text += QChar(0x221e); // The infinity character
  }
  
  const QFontMetrics fm = fontMetrics();
  //const QFontMetrics fm(_textFont);
  //// Rotate 90 deg.
  //_textSize = fm.boundingRect(txt).size().transposed();
  const QSize sz = fm.boundingRect(_text).size();
  const int txtw = sz.width();
  const int txth = sz.height();
//   if(_textPM.isNull() || _textPM.size().width() < w || _textPM.size().height() < h)
//     _textPM = QPixmap(w, h);
//   QPainter p;
//   p.begin(&_textPM);
// //   p.eraseRect(0, 0, w, h);
//   p.fillRect(0, 0, w, h, Qt::darkYellow);
//   p.rotate(90);
//   p.setPen(Qt::cyan);
//   // Rotate 90 deg.
//   //p.drawText(0, 0, h, w, Qt::AlignLeft | Qt::AlignTop, txt);
//   p.drawText(0, 0, txt);
//   p.end();
  
  // Set the text size, incrementally expanding. Ensure that paint will erase the last largest size.
  // Rotate 90 degrees.
  const int fw = frameWidth();
  const int w  = width() - 2*fw;
  const int txtYOff = fw + (w > txth ? (w - txth) / 2 : 0);
  
  _textRect.setX(fw);
  _textRect.setY(txtYOff);
  
  if(txtw > _textRect.width())
    _textRect.setWidth(txtw);
  if(txth > _textRect.height())
    _textRect.setHeight(txth);
  
  const QRect rr(_textRect.y(), _textRect.x(), _textRect.height(), _textRect.width()); // Rotate -90 degrees.
  update(rr);
  //QRect ur(_textRect.y(), _textRect.x(), _textRect.height(), _textRect.width());
  //update(ur);
}
      
//---------------------------------------------------------
//   setVal
//---------------------------------------------------------

void Meter::setVal(double v, double max, bool ovl)
{
  overflow = ovl;
  bool ud = false;

  const int wend  = _VURect.width();
  const int hend  = _VURect.height();
  const int left = _VURect.x();
  const int rightEnd = left + wend;
  const int right = rightEnd - 1;
  const int top = _VURect.y();
  const int botEnd = top + hend;
  const int bot = botEnd - 1;

  //------------------------------------------------------------------------
  //   Prevent very small, slow decaying values from hogging the meter time.
  //   Do not start the fall timer in such a case.
  //   With log we can simply test if the value is below the finite minimum.
  //   With non-log the task is more complex since the minimum can be zero.
  //------------------------------------------------------------------------
  if(_isLog)
  {
    if(_isInteger)
    {
      v /= _logFactor;
      max /= _logFactor;
    }

    if(v <= minScaleLog)
    {
      if(targetVal != minScaleLog)
      {
        targetVal = minScaleLog;
        ud = true;
      }
    }
    else if(targetVal != v)
    {
      targetVal = v;
      ud = true;
    }

    if(max <= minScaleLog)
    {
      if(maxVal != minScaleLog)
      {
        targetMaxVal = minScaleLog;
        ud = true;
      }
    }
    else if(maxVal != max)
    {
      targetMaxVal = max;
      ud = true;
    }
  }
  else
  {
    // Test if the value is enough to light up pixel '1' or higher.
    // Otherwise if it would only light up pixel '0' then don't bother
    //  starting the fall timer because we are already at the lowest point.
    int pixv, mpixv, cmpv;
    int *cmpvA, *cmpvB, *cmpmA, *cmpmB;
    if(_orient == Qt::Vertical)
    {
      // Reverse here means top to bottom.
      if(_reverseDirection)
      {
        cmpv = top;
        cmpvA = &cmpv;
        cmpvB = &pixv;
        cmpmA = &cmpv;
        cmpmB = &mpixv;
      }
      else
      {
        cmpv = bot;
        cmpvA = &pixv;
        cmpvB = &cmpv;
        cmpmA = &mpixv;
        cmpmB = &cmpv;
      }
      pixv  = d_scale.limTransform(v);
      mpixv = d_scale.limTransform(max);
    }
    else
    {
      // Reverse here means right to left.
      if(_reverseDirection)
      {
        cmpv = right;
        cmpvA = &pixv;
        cmpvB = &cmpv;
        cmpmA = &mpixv;
        cmpmB = &cmpv;
      }
      else
      {
        cmpv = left;
        cmpvA = &cmpv;
        cmpvB = &pixv;
        cmpmA = &cmpv;
        cmpmB = &mpixv;
      }
      pixv = d_scale.limTransform(v);
      mpixv = d_scale.limTransform(max);
    }

    if(*cmpvA >= *cmpvB)
    {
      if(targetVal != minScaleLog)
      {
        targetVal = minScaleLog;
        targetMaxVal = max;
        ud = true;
      }
    }
    else if(targetVal != v)
    {
      targetVal = v;
      targetMaxVal = max;
      ud = true;
    }

    if(*cmpmA >= *cmpmB)
    {
      if(maxVal != minScaleLog)
      {
        targetMaxVal = minScaleLog;
        ud = true;
      }
    }
    else if(maxVal != max)
    {
      targetMaxVal = max;
      ud = true;
    }
  }

  if(ud)
  {
      if(!fallingTimer.isActive())
      {
        fallingTimer.start(1000/std::max(30, _refreshRate));
      }
  }
}

void Meter::updateTargetMeterValue()
{
   const int wend  = _VURect.width();
   const int hend  = _VURect.height();
   const int left = _VURect.x();
   const int rightEnd = left + wend;
   const int right = rightEnd - 1;
   const int top = _VURect.y();
   const int botEnd = top + hend;
   const int bot = botEnd - 1;

   bool ud = false;

   QRegion udRegion;

   // Is the falling value already less than or equal to the target value?
   if(val <= targetVal)
   {
      // Stop the timer but allow one final update.
      if(fallingTimer.isActive())
        fallingTimer.stop();
      val = targetVal;
      targetValStep = 0;
      ud = true;
   }
   // If the falling value is higher than the target value, keep on fallin'...
   else if(val > targetVal)
   {
      // Test if the value has already reached the minimum.
      // The pixel section below can also do this (sooner than this can).
      if(val <= minScaleLog)
      {
        // Stop the timer but allow one final update.
        if(fallingTimer.isActive())
          fallingTimer.stop();
        // Force val to targetVal, because it may not have reached its target.
        val = targetVal;
        targetValStep = 0;
      }
      else
      {
        targetValStep = (val - targetVal) / ((double)(1000 / std::max(30, _refreshRate + 1)) / 7.0f);
        // A catch-all to ensure that targetValStep is never allowed to be zero,
        //  so that the value always (eventually) reaches its target.
        if(targetValStep <= 0.0000000001) // 10^-9 (1 billionth)
          targetValStep = 0.0000000001;

        val -= targetValStep;
        if(val < targetVal)
        {
          val = targetVal;
          // Stop the timer but allow one final update.
          if(fallingTimer.isActive())
            fallingTimer.stop();
          targetValStep = 0;
        }
      }
      ud = true;
   }

   // Update the previous (erase) and current (draw) maximum line areas.
   if(maxVal != targetMaxVal)
   {
     maxVal = targetMaxVal;
     const double v = (_isLog) ? (MusECore::fast_log10(maxVal) * _dBFactor) : maxVal;

     if(_orient == Qt::Vertical)
     { 
       // Reverse here means top to bottom.
       if(_reverseDirection)
         cur_pixmax = d_scale.limTransform(v);
       else
         cur_pixmax = d_scale.limTransform(v);
       if(cur_pixmax > bot)
         cur_pixmax = bot;
       else if(cur_pixmax < top)
         cur_pixmax = top;

       if(_showText)
         updateText(v);
       if(cur_pixmax != last_pixmax)
       {
         udRegion += QRect(left, last_pixmax, wend, 1);
         udRegion += QRect(left, cur_pixmax, wend, 1);
       }
     }
     else
     {
       // Reverse here means top to bottom.
       if(_reverseDirection)
         cur_pixmax = d_scale.limTransform(v);
       else
         cur_pixmax = d_scale.limTransform(v);
       if(cur_pixmax < left)
         cur_pixmax = left;
       else if(cur_pixmax > right)
         cur_pixmax = right;

       if(_showText)
         updateText(v);
       if(cur_pixmax != last_pixmax)
       {
         udRegion += QRect(last_pixmax, top, 1, hend);
         udRegion += QRect(cur_pixmax, top, 1, hend);
       }
     }
     
     last_pixmax = cur_pixmax;
     ud = true;
   }

   if(ud)
   {
     if(_orient == Qt::Vertical)
     {
       // Reverse here means top to bottom.
       const int cmpv = _reverseDirection ? top : bot;
       if(_isLog)
       {
         if(val <= minScaleLog)
         {
           cur_pixv = cmpv;
         }
         else
         {
           const double vdb = MusECore::fast_log10(val) * _dBFactor;
           cur_pixv = d_scale.limTransform(vdb);
         }
       }
       else
       {
         if(val <= minScale)
         {
           cur_pixv = cmpv;
         }
         else
         {
           cur_pixv = d_scale.limTransform(val);
         }
       }

      // If the pixel representation of val is at pixel '0', then the 'falling value'
      //  timer has finished its job - even if val has not quite reached its target value.
      if(cur_pixv == cmpv)
      {
        if(fallingTimer.isActive())
          fallingTimer.stop();
        val = targetVal;
        targetValStep = 0;
      }

      if(cur_pixv != last_pixv)
      {
        int y1, y2;
        if(last_pixv < cur_pixv) { y1 = last_pixv; y2 = cur_pixv; } else { y1 = cur_pixv; y2 = last_pixv; }
        udRegion += QRect(left, y1, wend, y2 - y1 + 1);
        last_pixv = cur_pixv;
      }
     }
     else
     {
       const int cmpv = _reverseDirection ? right : left;
       if(_isLog)
       {
         if(val <= minScaleLog)
         {
           cur_pixv = cmpv;
         }
         else
         {
           const double vdb = MusECore::fast_log10(val) * _dBFactor;
           cur_pixv = d_scale.limTransform(vdb);
         }
       }
       else
       {
         if(val <= minScale)
         {
           cur_pixv = cmpv;
         }
         else
         {
           cur_pixv = d_scale.limTransform(val);
         }
       }

      // If the pixel representation of val is at pixel '0', then the 'falling value'
      //  timer has finished its job - even if val has not quite reached its target value.
      if(cur_pixv == cmpv)
      {
        if(fallingTimer.isActive())
          fallingTimer.stop();
        val = targetVal;
        targetValStep = 0;
      }

      if(cur_pixv != last_pixv)
      {
        int x1, x2;
        if(last_pixv < cur_pixv) { x1 = last_pixv; x2 = cur_pixv; } else { x1 = cur_pixv; x2 = last_pixv; }
        udRegion += QRect(x1, top, x2 - x1 + 1, hend);
        last_pixv = cur_pixv;
      }
     }
   }
   if(!udRegion.isEmpty())
     update(udRegion);
}


//---------------------------------------------------------
//   resetPeaks
//    reset peak and overflow indicator
//---------------------------------------------------------

void Meter::resetPeaks()
      {
      maxVal   = val;
      overflow = val > 0.0;
      cur_pixv = -1;  // Force re-initialization.
      update();               
      }

//---------------------------------------------------------
//   setRange
//---------------------------------------------------------

void Meter::setRange(double min, double max, bool isInteger, bool isLog)
      {
      double min_log = min;
      double max_log = max;

      _isInteger = isInteger;
      _isLog = isLog;

      // If it's integer or log integer.
      if(_isInteger)
      {
        min = rint(min);
        max = rint(max);
      }

      if(_isLog)
      {
        if(_isInteger)
        {
          // Force a hard lower limit of integer 1.
          if(min <= 0.0)
            min = 1.0;
          if(max <= 0.0)
            max = 1.0;
          min /= _logFactor;
          max /= _logFactor;
          min_log = min;
          max_log = max;
          min = _dBFactor * MusECore::fast_log10(min);
          max = _dBFactor * MusECore::fast_log10(max);
        }
        else
        {
          // Force a hard lower limit of -120 dB.
          if(min <= 0.0)
          {
            min_log = 0.000001;
            min = -120;
          }
          else
            min = _dBFactor * MusECore::fast_log10(min);

          if(max <= 0.0)
          {
            max_log = 0.000001;
            max = -120;
          }
          else
            max = _dBFactor * MusECore::fast_log10(max);
        }
      }

      if(min == minScale && max == maxScale && min_log == minScaleLog && max_log == maxScaleLog)
        return;

      minScale = min;
      maxScale = max;
      minScaleLog = min_log;
      maxScaleLog = max_log;
      cur_pixv = -1;  // Force re-initialization.

      adjustScale();
      }

//---------------------------------------------------------
//   setRefreshRate
//---------------------------------------------------------

void Meter::setRefreshRate(int rate)
{
  _refreshRate = rate;
}

void Meter::setPrimaryColor(const QColor& color, const QColor &bgColor)
{
  _primaryColor = color; 
  int r = 0;

  if (_vu3d) {
      dark_green_begin = _primaryColor.darker(200);
      dark_green_end = dark_green_begin;
      r = dark_green_end.red() + 0x46;
      if(r > 255)
          r = 255;
      dark_green_end.setRed(r);

      darkGradGreen.setColorAt(1, dark_green_begin);
      darkGradGreen.setColorAt(0, dark_green_end);
  } else
      _bgColor = bgColor;

  light_green_begin = _primaryColor;
  light_green_end = light_green_begin;
  r = light_green_end.red() + 0x88;
  if(r > 255)
    r = 255;
  light_green_end.setRed(r);

  lightGradGreen.setColorAt(1, light_green_begin);
  lightGradGreen.setColorAt(0, light_green_end);
  
  update(); 
}

//---------------------------------------------------------
//   paintEvent
//---------------------------------------------------------

void Meter::paintEvent(QPaintEvent* ev)
{
  // For some reason upon resizing we get double calls here and in resizeEvent.

  QPainter p(this);

  const int wend  = _VURect.width();
  const int hend  = _VURect.height();
  const int w  = wend - 1;
  const int h  = hend - 1;
  const int left = _VURect.x();
  const int rightEnd = left + wend;
  const int right = rightEnd - 1;
  const int top = _VURect.y();
  const int botEnd = top + hend;
  const int bot = botEnd - 1;

  p.setRenderHint(QPainter::Antialiasing);

  //p.fillRect(0, 0, width(), height(), QColor(50, 50, 50));


  QBrush maskgrad;
  if (_vu3d)
  {
    maskGrad.setStart(QPointF(left, top));
    if(_orient == Qt::Vertical)
      maskGrad.setFinalStop(QPointF(w, top));
    else
      maskGrad.setFinalStop(QPointF(left, h));
    maskgrad = QBrush(maskGrad);
  }

  const QBrush frame_br(_frameColor);
  const QBrush peak_br(peak_color);
  const QBrush& bkr_br = palette().window();

  bool textDrawn = false;
  QRegion::const_iterator ireg_end = ev->region().cend();
  for(QRegion::const_iterator ireg = ev->region().cbegin(); ireg != ireg_end; ++ireg)
  {
    const QRect& rect = *ireg;
    QPainterPath rectpath;
    rectpath.addRect(rect);

    const QRect vur = rect & _VURect;

    DEBUG_METER_PAINT(stderr, "meter::paint: region: x:%d y:%d w:%d h:%d\n", rect.x(), rect.y(), rect.width(), rect.height());

    // Fill any background path.
    const QPainterPath bkpath = _bkgPath & rectpath;
    if(!bkpath.isEmpty())
    {
      DEBUG_METER_PAINT(stderr, "meter::paint: background:\n");
      //printQPainterPath(bkpath);

      p.fillPath(bkpath, bkr_br);
    }

    // If no part of the requested rectangle is part of the VU area, just continue.
    if(!vur.isEmpty())
    {

      // Tested OK! Small non-overlapping rectangles.
      //DEBUG_METER_PAINT(stderr, "Meter::paintEvent rcount:%d ridx:%d rx:%d ry:%d rw:%d rh:%d w:%d h:%d\n",
      //                rectCount, ri, rect.x(), rect.y(), rect.width(), rect.height(), w, h);

      QPainterPath updatePath, finalPath;

      // Initialize. Can't do in ctor, must be done after layouts have been done. Most reliable to do it here.
      if(cur_pixv == -1)
      {
        if(_orient == Qt::Vertical)
        {
          if(_isLog)
          {
            cur_pixv = d_scale.limTransform(MusECore::fast_log10(val) * _dBFactor);
            cur_pixmax = d_scale.limTransform(MusECore::fast_log10(maxVal) * _dBFactor);
          }
          else
          {
            cur_pixv = d_scale.limTransform(val);
            cur_pixmax = d_scale.limTransform(maxVal);
          }
          if(cur_pixv > bot) cur_pixv = bot;
          last_pixv = cur_pixv;
          if(cur_pixmax > bot) cur_pixmax = bot;
          last_pixmax = cur_pixmax;
          // Update the whole VU area.
          updatePath.addRect(_VURect);
        }
        else
        {
          if(_isLog)
          {
            cur_pixv = d_scale.limTransform(MusECore::fast_log10(val) * _dBFactor);
            cur_pixmax = d_scale.limTransform(MusECore::fast_log10(maxVal) * _dBFactor);
          }
          else
          {
            cur_pixv = d_scale.limTransform(val);
            cur_pixmax = d_scale.limTransform(maxVal);
          }
          if(cur_pixv > right) cur_pixv = right;
          last_pixv = cur_pixv;
          if(cur_pixmax > right) cur_pixmax = right;
          last_pixmax = cur_pixmax;
          // Update the whole VU area.
          updatePath.addRect(_VURect);
        }
      }
      else
        updatePath.addRect(vur);  // Update only the requested rectangle

      finalPath = (_VUPath & updatePath).simplified();

      // Draw the red, green, and yellow sections.
      drawVU(p, vur, finalPath, cur_pixv);

      // Draw the peak white line.
      p.setRenderHint(QPainter::Antialiasing, false);  // No antialiasing. Makes the line fuzzy, double height, or not visible at all.
      QPainterPath path;
      if(_orient == Qt::Vertical)
        path.addRect(left, cur_pixmax, wend, 1);
      else
        path.addRect(cur_pixmax, top, 1, hend);
      path = (path & finalPath).simplified();
      if(!path.isEmpty())
        p.fillPath(path, peak_br);

      if (_vu3d)
      {
          // Draw the transparent layer on top of everything to give a 3d look
          p.setRenderHint(QPainter::Antialiasing);
          p.fillPath(finalPath, maskgrad);
      }

      if(_showText)
      {
        const QRect rr(rect.y(), rect.x(), rect.height(), rect.width()); // Rotate 90 degrees.
        if(!textDrawn && rr.intersects(_textRect))
        {
          textDrawn = true;
          DEBUG_METER_PAINT(stderr, "   Drawing text:%s\n", _text.toLatin1().constData());
          //p.setFont(_textFont);
          p.setPen(Qt::white);

          if(_orient == Qt::Vertical)
          {
            p.rotate(90);
            p.translate(0, -frameGeometry().width());
          }

          //p.drawText(txtXOff, txtYOff, _textSize.width(), _textSize.height(), Qt::AlignLeft | Qt::AlignVCenter, _text);
          p.drawText(_textRect, Qt::AlignLeft | Qt::AlignVCenter, _text);
          //p.drawPixmap(fw, fw, _textPM);

          if(_orient == Qt::Vertical)
          {
            // Restore.
            p.translate(0, frameGeometry().width());
            p.rotate(-90);
          }
        }
      }
    }

    if (_frame)
    {
      const QPainterPath fp = (_VUFramePath & rectpath).simplified();
      if(!fp.isEmpty())
      {
        DEBUG_METER_PAINT(stderr, "meter::paint: frame\n");
        //printQPainterPath(fp);

        p.fillPath(fp, frame_br);
      }
    }

  }
  
  if(_scalePos != ScaleNone)
  {
    // If any part of the total requested rectangle is part of the scale area,
    //  draw the whole scale. TODO Try to refine scale draw routine.
    if(!(ev->rect() & _scaleRect).isEmpty())
    {
      DEBUG_METER_PAINT(stderr, "meter::paint: scale\n");

      p.setRenderHint(QPainter::Antialiasing, false);
      d_scale.draw(&p, palette());
    }
  }
}

//---------------------------------------------------------
//   drawVU
//---------------------------------------------------------

void Meter::drawVU(QPainter& p, const QRect& rect, const QPainterPath& drawPath, int pixv)
{
      const int wend  = _VURect.width();
      const int hend  = _VURect.height();
      //const int w  = wend - 1;
      //const int h  = hend - 1;
      const int left = _VURect.x();
      const int rightEnd = left + wend;
      //const int right = rightEnd - 1;
      const int top = _VURect.y();
      const int botEnd = top + hend;
      //const int bot = botEnd - 1;

      // Test OK. We are passed small rectangles on small value changes.
      //DEBUG_METER_PAINT(stderr, "Meter::drawVU rx:%d ry:%d rw:%d rh:%d w:%d h:%d\n", rect.x(), rect.y(), rect.width(), rect.height(), w, h);

      if(_orient == Qt::Vertical)
      {  
        if(_isLog)     // Meter type is dB...
        {
          int y1, y2;
          if(_reverseDirection)
          {
            y1 = d_scale.limTransform(yellowScale);
            y2 = d_scale.limTransform(redScale);
          }
          else
          {
            y1 = d_scale.limTransform(redScale);
            y2 = d_scale.limTransform(yellowScale);
          }

          if(_reverseDirection)
          {
            if(pixv < y1)
            {
              // Green section:
              {
                QPainterPath path; path.addRect(left, top, wend, pixv - top); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, QBrush(lightGradGreen));   // light green
              }
              {
                QPainterPath path; path.addRect(left, pixv, wend, y1 - pixv); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, _vu3d ? QBrush(darkGradGreen) : _bgColor);       // dark green
              }

              // Yellow section:
              {
                QPainterPath path; path.addRect(left, y1, wend, y2 - y1); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, _vu3d ? QBrush(darkGradYellow) : _bgColor);   // dark yellow
              }

              // Red section:
              {
                QPainterPath path; path.addRect(left, y2, wend, botEnd - y2); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, _vu3d ? QBrush(darkGradRed) : _bgColor);       // dark red
              }
            }
            else
            if(pixv < y2)
            {
              // Green section:
              {
                QPainterPath path; path.addRect(left, top, wend, y1 - top); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, QBrush(lightGradGreen));   // light green
              }

              // Yellow section:
              {
                QPainterPath path; path.addRect(left, y1, wend, pixv - y1); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, QBrush(lightGradYellow));   // light yellow
              }
              {
                QPainterPath path; path.addRect(left, pixv, wend, y2 - pixv); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, _vu3d ? QBrush(darkGradYellow) : _bgColor);   // dark yellow
              }

              // Red section:
              {
                QPainterPath path; path.addRect(left, y2, wend, botEnd - y2); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, _vu3d ? QBrush(darkGradRed) : _bgColor);       // dark red
              }
            }
            else
            //if(yv <= y3)
            {
              // Green section:
              {
                QPainterPath path; path.addRect(left, top, wend, y1 - top); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, QBrush(lightGradGreen));   // light green
              }

              // Yellow section:
              {
                QPainterPath path; path.addRect(left, y1, wend, y2 - y1); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, QBrush(lightGradYellow));   // light yellow
              }

              // Red section:
              {
                QPainterPath path; path.addRect(left, y2, wend, pixv - y2); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, QBrush(lightGradRed));   // light red
              }
              {
                QPainterPath path; path.addRect(left, pixv, wend, botEnd - pixv); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, _vu3d ? QBrush(darkGradRed) : _bgColor);   // dark red
              }
            }

            // Separators:
            {
              QRect r(left, y1, wend, 1); r &= rect;
              if(!r.isNull())
                p.fillRect(r, separator_color);
            }
            {
              QRect r(left, y2, wend, 1); r &= rect;
              if(!r.isNull())
                p.fillRect(r, separator_color);
            }
          }
          else //========== Not _reverseDirection ==========
          {
            if(pixv < y1)
            {
              // Red section:
              {
                QPainterPath path; path.addRect(left, top, wend, pixv); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, _vu3d ? QBrush(darkGradRed) : _bgColor);       // dark red
              }
              {
                QPainterPath path; path.addRect(left, pixv, wend, y1 - pixv); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, QBrush(lightGradRed));       // light red
              }

              // Yellow section:
              {
                QPainterPath path; path.addRect(left, y1, wend, y2 - y1); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, QBrush(lightGradYellow));   // light yellow
              }

              // Green section:
              {
                QPainterPath path; path.addRect(left, y2, wend, botEnd - y2); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, QBrush(lightGradGreen));   // light green
              }
            }
            else
            if(pixv < y2)
            {
              // Red section:
              {
                QPainterPath path; path.addRect(left, top, wend, y1); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, _vu3d ? QBrush(darkGradRed) : _bgColor);       // dark red
              }

              // Yellow section:
              {
                QPainterPath path; path.addRect(left, y1, wend, pixv - y1); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, _vu3d ? QBrush(darkGradYellow) : _bgColor);   // dark yellow
              }
              {
                QPainterPath path; path.addRect(left, pixv, wend, y2 - pixv); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, QBrush(lightGradYellow));   // light yellow
              }

              // Green section:
              {
                QPainterPath path; path.addRect(left, y2, wend, botEnd - y2); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, QBrush(lightGradGreen));   // light green
              }
            }
            else
            //if(yv <= y3)
            {
              // Red section:
              {
                QPainterPath path; path.addRect(left, top, wend, y1); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, _vu3d ? QBrush(darkGradRed) : _bgColor);       // dark red
              }

              // Yellow section:
              {
                QPainterPath path; path.addRect(left, y1, wend, y2 - y1); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, _vu3d ? QBrush(darkGradYellow) : _bgColor);   // dark yellow
              }

              // Green section:
              {
                QPainterPath path; path.addRect(left, y2, wend, pixv - y2); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, _vu3d ? QBrush(darkGradGreen) : _bgColor);   // dark green
              }
              {
                QPainterPath path; path.addRect(left, pixv, wend, botEnd - pixv); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, QBrush(lightGradGreen));   // light green
              }
            }

            // Separators:
            {
              QRect r(left, y1, wend, 1); r &= rect;
              if(!r.isNull())
                p.fillRect(r, separator_color);
            }
            {
              QRect r(left, y2, wend, 1); r &= rect;
              if(!r.isNull())
                p.fillRect(r, separator_color);
            }
          }
        }
        else      // Meter type is linear...
        {
          {
            QPainterPath path; path.addRect(left, top, wend, pixv); path &= drawPath;
            if(!path.isEmpty())
              p.fillPath(path, _vu3d ? QBrush(darkGradGreen) : _bgColor);   // dark green
          }
          {
            QPainterPath path; path.addRect(left, pixv, wend, botEnd - pixv); path &= drawPath;
            if(!path.isEmpty())
              p.fillPath(path, QBrush(lightGradGreen));   // light green
          }
        }


      }
      else    // Horizontal meter
      {
        if(_isLog)     // Meter type is dB...
        {
          int x1, x2;
          if(_reverseDirection)
          {
            x1 = d_scale.limTransform(redScale);
            x2 = d_scale.limTransform(yellowScale);
          }
          else
          {
            x1 = d_scale.limTransform(yellowScale);
            x2 = d_scale.limTransform(redScale);
          }

          if(_reverseDirection)
          {
            if(pixv < x1)
            {
              // Red section:
              {
                QPainterPath path; path.addRect(left, top, pixv - left, hend); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, _vu3d ? QBrush(darkGradRed) : _bgColor);       // dark red
              }
              {
                QPainterPath path; path.addRect(pixv, top, x1 - pixv, hend); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, QBrush(lightGradRed));       // light red
              }

              // Yellow section:
              {
                QPainterPath path; path.addRect(x1, top, x2 - x1, hend); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, QBrush(lightGradYellow));   // light yellow
              }

              // Green section:
              {
                QPainterPath path; path.addRect(x2, top, rightEnd - x2, hend); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, QBrush(lightGradGreen));   // light green
              }
            }
            else
            if(pixv < x2)
            {
              // Red section:
              {
                QPainterPath path; path.addRect(left, top, x1, hend); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, _vu3d ? QBrush(darkGradRed) : _bgColor);       // dark red
              }

              // Yellow section:
              {
                QPainterPath path; path.addRect(x1, top, pixv - x1, hend); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, _vu3d ? QBrush(darkGradYellow) : _bgColor);   // dark yellow
              }
              {
                QPainterPath path; path.addRect(pixv, top, x2 - pixv, hend); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, QBrush(lightGradYellow));   // light yellow
              }

              // Green section:
              {
                QPainterPath path; path.addRect(x2, top, rightEnd - x2, hend); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, QBrush(lightGradGreen));   // light green
              }
            }
            else
            //if(yv <= y3)
            {
              // Red section:
              {
                QPainterPath path; path.addRect(left, top, x1, hend); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, _vu3d ? QBrush(darkGradRed) : _bgColor);       // dark red
              }

              // Yellow section:
              {
                QPainterPath path; path.addRect(x1, top, x2 - x1, hend); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, _vu3d ? QBrush(darkGradYellow) : _bgColor);   // dark yellow
              }

              // Green section:
              {
                QPainterPath path; path.addRect(x2, top, pixv - x2, hend); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, _vu3d ? QBrush(darkGradGreen) : _bgColor);   // dark green
              }
              {
                QPainterPath path; path.addRect(pixv, top, rightEnd - pixv, hend); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, QBrush(lightGradGreen));   // light green
              }
            }

            // Separators:
            {
              QRect r(x1, top, 1, hend); r &= rect;
              if(!r.isNull())
                p.fillRect(r, separator_color);
            }
            {
              QRect r(x2, top, 1, hend); r &= rect;
              if(!r.isNull())
                p.fillRect(r, separator_color);
            }
          }
          else  //========== Not _reverseDirection ==========
          {
            if(pixv < x1)
            {
              // Green section:
              {
                QPainterPath path; path.addRect(left, top, pixv - left, hend); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, QBrush(lightGradGreen));   // light green
              }
              {
                QPainterPath path; path.addRect(pixv, top, x1 - pixv, hend); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, _vu3d ? QBrush(darkGradGreen) : _bgColor);   // dark green
              }

              // Yellow section:
              {
                QPainterPath path; path.addRect(x1, top, x2 - x1, hend); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, _vu3d ? QBrush(darkGradYellow) : _bgColor);   // dark yellow
              }

              // Red section:
              {
                QPainterPath path; path.addRect(x2, top, rightEnd - x2, hend); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, _vu3d ? QBrush(darkGradRed) : _bgColor);   // dark red
              }
            }
            else
            if(pixv < x2)
            {
              // Green section:
              {
                QPainterPath path; path.addRect(left, top, x1 - left, hend); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, QBrush(lightGradGreen));   // light green
              }

              // Yellow section:
              {
                QPainterPath path; path.addRect(x1, top, pixv - x1, hend); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, QBrush(lightGradYellow));   // light yellow
              }
              {
                QPainterPath path; path.addRect(pixv, top, x2 - pixv, hend); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, _vu3d ? QBrush(darkGradYellow) : _bgColor);   // dark yellow
              }

              // Red section:
              {
                QPainterPath path; path.addRect(x2, top, rightEnd - x2, hend); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, _vu3d ? QBrush(darkGradRed) : _bgColor);       // dark red
              }
            }
            else
            //if(yv <= y3)
            {
              // Green section:
              {
                QPainterPath path; path.addRect(left, top, x1 - left, hend); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, QBrush(lightGradGreen));   // light green
              }

              // Yellow section:
              {
                QPainterPath path; path.addRect(x1, top, x2 - x1, hend); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, QBrush(lightGradYellow));   // light yellow
              }

              // Red section:
              {
                QPainterPath path; path.addRect(x2, top, pixv - x2, hend); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, QBrush(lightGradRed));   // light red
              }
              {
                QPainterPath path; path.addRect(pixv, top, rightEnd - pixv, hend); path &= drawPath;
                if(!path.isEmpty())
                  p.fillPath(path, _vu3d ? QBrush(darkGradRed) : _bgColor);   // dark red
              }
            }

            // Separators:
            {
              QRect r(x1, top, 1, hend); r &= rect;
              if(!r.isNull())
                p.fillRect(r, separator_color);
            }
            {
              QRect r(x2, top, 1, hend); r &= rect;
              if(!r.isNull())
                p.fillRect(r, separator_color);
            }
          }
        }  
        else      // Meter type is linear...
        {
          {
            QPainterPath path; path.addRect(left, top, pixv - left, hend); path &= drawPath;
            if(!path.isEmpty())
              p.fillPath(path, QBrush(lightGradGreen));   // light green
          }
          {
            QPainterPath path; path.addRect(pixv, top, rightEnd - pixv, hend); path &= drawPath;
            if(!path.isEmpty())
              p.fillPath(path, _vu3d ? QBrush(darkGradGreen) : _bgColor);   // dark green
          }
        }
      }
}

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void Meter::resizeEvent(QResizeEvent* ev)
{
   QFrame::resizeEvent(ev);

   // Tested: Hm, we are given zero height at first, even in the event's size() member,
   //  and even after calling the base resizeEvent()!

   // For some reason upon resizing we get double calls here and in paintEvent.
   //DEBUG_METER("Meter::resizeEvent w:%d h:%d\n", ev->size().width(), ev->size().height());

   const int fw = frameWidth();
   const QRect cr = contentsRect();
   const int crw  = cr.width() - 2*fw;
   const int crh  = cr.height() - 2*fw;
   const int crleft = cr.x() + fw;
   const int crrightEnd = crleft + crw;
   //const int crright = crrightEnd - 1;
   const int crtop = cr.y() + fw;
   const int crbotEnd = crtop + crh;
   //const int crbot = crbotEnd - 1;
   const int vufw = _frame ? 1 : 0;

   cur_pixv = -1;  // Force re-initialization.

    const QFontMetrics fm = fontMetrics();

    // Clear might be a bit too new at 5.13
    //_bkgPath.clear();
    QPainterPath bkgpath;

    if(_orient == Qt::Horizontal)
    {
      const int xoff = d_scale.originOffsetHint(fm, true).x();
      const int xfin = crleft + xoff;
      const int wfin = crw - 2 * xoff;

      const int amleft = qMax(_alignmentMargins.left() - (xfin + vufw), 0);
      const int amright = qMax(_alignmentMargins.right() - (xoff + contentsMargins().right() + fw + vufw), 0);
      const int amtotal = amleft + amright;

      switch(_scalePos)
      {
        case ScaleLeftOrTop:
        {
            const int smy = d_scale.maxHeight(fm);
            _VUFrameRect.setRect(xfin + amleft, crtop + smy + _scaleDist, wfin - amtotal, crh - smy);
            _VURect = _VUFrameRect.adjusted(vufw, vufw, -vufw, -vufw);
            _scaleRect.setRect(crleft + amleft, crtop, crw - amtotal, smy);
            _scaleGeom.setRect(_VURect.x(), crtop, _VURect.width(), smy);
            _spacerRect.setRect(crleft, crtop + smy, crw, _scaleDist);
            d_scale.setGeometry(_VURect.x(), crtop, _VURect.width());
            break;
        }

        case ScaleRightOrBottom:
        {
            const int smy = d_scale.maxHeight(fm);
            _VUFrameRect.setRect(xfin + amleft, crtop, wfin - amtotal, crh - smy - _scaleDist);
            _VURect = _VUFrameRect.adjusted(vufw, vufw, -vufw, -vufw);
            _spacerRect.setRect(crleft, crh - smy - _scaleDist, crw, _scaleDist);
            _scaleRect.setRect(crleft + amleft, crbotEnd - smy, crw - amtotal, smy);
            _scaleGeom.setRect(_VURect.x(), crbotEnd - smy, _VURect.width(), smy);
            d_scale.setGeometry(_VURect.x(), crbotEnd - smy, _VURect.width());
            break;
        }

        case ScaleInside:
            _VUFrameRect.setRect(xfin + amleft, crtop, wfin - amtotal, crh);
            _VURect = _VUFrameRect.adjusted(vufw, vufw, -vufw, -vufw);
            _scaleRect.setRect(crleft + amleft, _VURect.y(), crw - amtotal, _VURect.height());
            _scaleGeom.setRect(_VURect.x(), _VURect.y(), _VURect.width(), _VURect.height());
            _spacerRect.setRect(crleft, crtop, 0, 0);
            d_scale.setGeometry(_VURect.x(), _VURect.y(), _VURect.width());
            break;

        case ScaleNone:
            _VUFrameRect.setRect(xfin + amleft, crtop, wfin - amtotal, crh);
            _VURect = _VUFrameRect.adjusted(vufw, vufw, -vufw, -vufw);
            _spacerRect.setRect(crleft, crtop, 0, 0);
            _scaleRect.setRect(crleft, crtop, 0, 0);
            _scaleGeom.setRect(crleft, crtop, 0, 0);
            // Give the translators something to work with.
            d_scale.setGeometry(_VURect.x(), _VURect.y(), _VURect.width());
            break;
      }
    }
    else // d_orient == Qt::Vertical
    {
      const int yoff = d_scale.originOffsetHint(fm, true).y();
      const int yfin = crtop + yoff;
      const int hfin = crh - 2 * yoff;
      const int smx = d_scale.maxWidth(fm, false);

      const int amtop = qMax(_alignmentMargins.top() - (yfin + vufw), 0);
      const int ambot = qMax(_alignmentMargins.bottom() - (yoff + contentsMargins().bottom() + fw + vufw), 0);
      const int amtotal = amtop + ambot;

      switch(_scalePos)
      {
        case ScaleLeftOrTop:
        {
            _VUFrameRect.setRect(crleft + smx + _scaleDist, yfin + amtop, crw - smx, hfin - amtotal);
            _VURect = _VUFrameRect.adjusted(vufw, vufw, -vufw, -vufw);
            _scaleRect.setRect(crleft, crtop + amtop, smx, crh - amtotal);
            _scaleGeom.setRect(crleft, _VURect.y(), smx, _VURect.height());
            _spacerRect.setRect(crleft + smx, crtop, _scaleDist, crh);
            d_scale.setGeometry(crleft, _VURect.y(), _VURect.height());
            break;
        }

        case ScaleRightOrBottom:
        {
            _VUFrameRect.setRect(crleft, yfin + amtop, crw - smx - _scaleDist, hfin - amtotal);
            _VURect = _VUFrameRect.adjusted(vufw, vufw, -vufw, -vufw);
            _spacerRect.setRect(crw - smx - _scaleDist, crtop, _scaleDist, crh);
            _scaleRect.setRect(crrightEnd - smx, crtop + amtop, smx, crh - amtotal);
            _scaleGeom.setRect(crrightEnd - smx, _VURect.y(), smx, _VURect.height());
            d_scale.setGeometry(crrightEnd - smx, _VURect.y(), _VURect.height());
            break;
        }

        case ScaleInside:
            _VUFrameRect.setRect(crleft, yfin + amtop, crw, hfin - amtotal);
            _VURect = _VUFrameRect.adjusted(vufw, vufw, -vufw, -vufw);
            _scaleRect.setRect(_VURect.x() + _VURect.width() - smx, crtop + amtop, smx, crh - amtotal);
            _scaleGeom.setRect(_VURect.x() + _VURect.width() - smx, _VURect.y(), smx, _VURect.height());
            _spacerRect.setRect(crleft, crtop, 0, 0);
            d_scale.setGeometry(_VURect.x() + _VURect.width() - 1, _VURect.y(), _VURect.height());
        break;

        case ScaleNone:
            _VUFrameRect.setRect(crleft, yfin + amtop, crw, hfin - amtotal);
            _VURect = _VUFrameRect.adjusted(vufw, vufw, -vufw, -vufw);
            _spacerRect.setRect(crleft, crtop, 0, 0);
            _scaleRect.setRect(crleft, crtop, 0, 0);
            _scaleGeom.setRect(crleft, crtop, 0, 0);
            // Give the translators something to work with.
            d_scale.setGeometry(_VURect.x(), _VURect.y(), _VURect.height());
            break;
      }
    }

  // The actual desired shape of the meter.
  QPainterPath vupath;
  vupath.addRoundedRect(_VURect, _radius, _radius);
  // Assign, since clear() might be a bit too new at 5.13
  _VUPath = vupath;

  QPainterPath rectpath;
  rectpath.addRect(rect());

  QPainterPath vufrpath;
  if(_frame)
  {
    vufrpath.addRoundedRect(_VUFrameRect, _radius, _radius);
    bkgpath = rectpath - vufrpath;
    vufrpath -= _VUPath;
  }
  else
  {
    bkgpath = rectpath - _VUPath;
  }

  _VUFramePath = vufrpath.simplified();

  // Assign, since clear() might be a bit too new at 5.13
  _bkgPath = bkgpath.simplified();

  DEBUG_METER(stderr, "Meter::resizeEvent: _bkgPath:\n");
  //printQPainterPath(_bkgPath);

  {
    const int wend  = _VURect.width();
    const int hend  = _VURect.height();
    const int w  = wend - 1;
    const int h  = hend - 1;
    const int left = _VURect.x();
    //const int rightEnd = left + wend;
    //const int right = rightEnd - 1;
    const int top = _VURect.y();
    const int botEnd = top + hend;
    //const int bot = botEnd - 1;

    if(_orient == Qt::Vertical)
    {
      if(_isLog)     // Meter type is dB...
      {
        int y1, y2;
        if(_reverseDirection)
        {
          y1 = d_scale.limTransform(yellowScale);
          y2 = d_scale.limTransform(redScale);
          if (_vu3d) {
              darkGradGreen.setStart(QPointF(left, top));
              darkGradGreen.setFinalStop(QPointF(left, y1));
              darkGradYellow.setStart(QPointF(left, y1));
              darkGradYellow.setFinalStop(QPointF(left, y2));
              darkGradRed.setStart(QPointF(left, y2));
              darkGradRed.setFinalStop(QPointF(left, botEnd));
          }

          lightGradGreen.setStart(QPointF(left, top));
          lightGradGreen.setFinalStop(QPointF(left, y1));
          lightGradYellow.setStart(QPointF(left, y1));
          lightGradYellow.setFinalStop(QPointF(left, y2));
          lightGradRed.setStart(QPointF(left, y2));
          lightGradRed.setFinalStop(QPointF(left, botEnd));
        }
        else
        {
          y1 = d_scale.limTransform(redScale);
          y2 = d_scale.limTransform(yellowScale);
          if (_vu3d) {
              darkGradGreen.setStart(QPointF(left, y2));
              darkGradGreen.setFinalStop(QPointF(left, botEnd));
              darkGradYellow.setStart(QPointF(left, y1));
              darkGradYellow.setFinalStop(QPointF(left, y2));
              darkGradRed.setStart(QPointF(left, top));
              darkGradRed.setFinalStop(QPointF(left, y1));
          }

          lightGradGreen.setStart(QPointF(left, y2));
          lightGradGreen.setFinalStop(QPointF(left, botEnd));
          lightGradYellow.setStart(QPointF(left, y1));
          lightGradYellow.setFinalStop(QPointF(left, y2));
          lightGradRed.setStart(QPointF(left, top));
          lightGradRed.setFinalStop(QPointF(left, y1));
        }
      }
      else
      {
        if (_vu3d)
        {
              darkGradGreen.setStart(QPointF(left, top));
              darkGradGreen.setFinalStop(QPointF(left, h));
        }

        lightGradGreen.setStart(QPointF(left, top));
        lightGradGreen.setFinalStop(QPointF(left, h));
      }
    }
    else   // Meter is horizontal
    {
      if(_isLog)     // Meter type is dB...
      {
        int x1, x2;
        if(_reverseDirection)
        {
          x1 = d_scale.limTransform(redScale);
          x2 = d_scale.limTransform(yellowScale);
          if (_vu3d) {
              darkGradGreen.setStart(QPointF(x2, top));
              darkGradGreen.setFinalStop(QPointF(w, top));
              darkGradYellow.setStart(QPointF(x1, top));
              darkGradYellow.setFinalStop(QPointF(x2, top));
              darkGradRed.setStart(QPointF(left, top));
              darkGradRed.setFinalStop(QPointF(x1, top));
          }

          lightGradGreen.setStart(QPointF(x2, top));
          lightGradGreen.setFinalStop(QPointF(w, top));
          lightGradYellow.setStart(QPointF(x1, top));
          lightGradYellow.setFinalStop(QPointF(x2, top));
          lightGradRed.setStart(QPointF(left, top));
          lightGradRed.setFinalStop(QPointF(x1, top));
        }
        else
        {
          x1 = d_scale.limTransform(yellowScale);
          x2 = d_scale.limTransform(redScale);
          if (_vu3d) {
              darkGradGreen.setStart(QPointF(left, top));
              darkGradGreen.setFinalStop(QPointF(x1, top));
              darkGradYellow.setStart(QPointF(x1, top));
              darkGradYellow.setFinalStop(QPointF(x2, top));
              darkGradRed.setStart(QPointF(x2, top));
              darkGradRed.setFinalStop(QPointF(w, top));
          }

          lightGradGreen.setStart(QPointF(left, top));
          lightGradGreen.setFinalStop(QPointF(x1, top));
          lightGradYellow.setStart(QPointF(x1, top));
          lightGradYellow.setFinalStop(QPointF(x2, top));
          lightGradRed.setStart(QPointF(x2, top));
          lightGradRed.setFinalStop(QPointF(w, top));
        }
      }
      else
      {
        if (_vu3d)
        {
            darkGradGreen.setStart(QPointF(left, top));
            darkGradGreen.setFinalStop(QPointF(w, top));
        }
        lightGradGreen.setStart(QPointF(left, top));
        lightGradGreen.setFinalStop(QPointF(w, top));
      }
    }
  }

  adjustScale();
}

void Meter::adjustScale()
{
  const double range = maxScale - minScale;
  if(range == 0.0)
    return;

  int maxMaj = 5;
  int maxMin = 3;
  double mstep = scaleStep();

  const QFontMetrics fm = fontMetrics();
  if(_orient == Qt::Horizontal)
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
      // Don't mess with the step in scale log mode, it can end up way too high.
      // Typically we want the scale log step to remain at 1 decade.
      // Here it was observed going to 19! FIXME: Find a way to scale it.
      if(!d_scale.scaleDiv().logScale() && _scaleRect.width() != 0)
      {
        const int fact = (int)(3.0 * range / (double)(_scaleRect.width())) + 1;
        mstep *= fact;
      }
    }
    else
    {
      maxMaj = (int)((double)(_scaleRect.width()) / (1.5 * ((double)unit_w)));
      if(maxMaj < 1)
        maxMaj = 1;
      if(maxMaj > 5)
        maxMaj = 5;
    }
    maxMin = (int)((double)(_scaleRect.width()) / (1.5 * ((double)unit_w)));
    if(maxMin < 1)
      maxMin = 1;
    if(maxMin > 5)
      maxMin = 5;
  }
  else // Vertical
  {
    int unit_h = fm.height();
    if(unit_h == 0)
      unit_h = 20;

    if(hasUserScale())
    {
      // Don't mess with the step in scale log mode, it can end up way too high.
      // Typically we want the scale log step to remain at 1 decade.
      // Here it was observed going to 19! FIXME: Find a way to scale it.
      if(!d_scale.scaleDiv().logScale() && _scaleRect.height() != 0)
      {
        const int fact = (int)(3.0 * range / (double)(_scaleRect.height())) + 1;
        mstep *= fact;
      }
    }
    else
    {
      maxMaj = (int)((double)(_scaleRect.height()) / (1.5 * ((double)unit_h)));
      if(maxMaj < 1)
        maxMaj = 1;
      if(maxMaj > 5)
        maxMaj = 5;
    }
    maxMin = (int)((double)(_scaleRect.height()) / (1.5 * ((double)unit_h)));
    if(maxMin < 1)
      maxMin = 1;
    if(maxMin > 5)
      maxMin = 5;
  }

  d_maxMajor = maxMaj;
  d_maxMinor = maxMin;
  if(hasUserScale())
  {
    const ScaleDiv& sd = d_scale.scaleDiv();
    // Keep the existing minimum and maximum, and log scale option.
    d_scale.setScale(sd.lBound(), sd.hBound(), d_maxMajor, d_maxMinor, mstep, sd.logScale());
  }
  else
    d_scale.setScale(minScale, maxScale, d_maxMajor, d_maxMinor, mstep, log());

  updateGeometry();
  update();
}

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void Meter::mousePressEvent(QMouseEvent*)
      {
      emit mousePress();
      }

bool Meter::log() const        { return _isLog; }
void Meter::setLog(bool v)     { _isLog = v; }
bool Meter::integer() const    { return _isInteger; }
void Meter::setInteger(bool v) { _isInteger = v; }
void Meter::setDBFactor(double v) { _dBFactor = v; _dBFactorInv = 1.0/_dBFactor; }

void Meter::setLogFactor(double v)
{
  if(_isLog)
  {
    // Set the new factor.
    _logFactor = v;
    // Reset the value.
    setVal(val / _logFactor, val / _logFactor, false);
    update();
    return;
  }

  // Set the new factor.
  _logFactor = v;
  update();
}

bool Meter::reverseDirection() const { return _reverseDirection; }
void Meter::setReverseDirection(bool v)
{
  _reverseDirection = v;
  updateGeometry();
  update();
}

Meter::ScalePos Meter::scalePos() const { return _scalePos; }
void Meter::setScalePos(const ScalePos& pos)
{
  _scalePos = pos;
  switch(_orient)
  {
    case Qt::Vertical:
      switch(_scalePos)
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
      switch(_scalePos)
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

int Meter::scaleDist() const { return _scaleDist; }
void Meter::setScaleDist(int d) {_scaleDist = d; updateGeometry(); update(); }

ScaleDraw::TextHighlightMode Meter::textHighlightMode() const { return d_scale.textHighlightMode(); }
void Meter::setTextHighlightMode(ScaleDraw::TextHighlightMode mode)
{
  d_scale.setTextHighlightMode(mode);
  updateGeometry();
  update();
}

bool Meter::showText() const { return _showText; }
void Meter::setShowText(bool v) { _showText = v; updateGeometry(); update(); }

Qt::Orientation Meter::orientation() const { return _orient; }
void Meter::setOrientation(Qt::Orientation o)
{
  _orient = o;
  setScalePos(_scalePos);
  //updateGeometry();
}

int Meter::radius() const { return _radius; }
void Meter::setRadius(int radius) { _radius = radius; updateGeometry(); update(); }
int Meter::vu3d() const { return _vu3d; }
void Meter::setVu3d(int vu3d) { _vu3d = vu3d; updateGeometry(); update(); }

void Meter::setFrame(bool frame, const QColor& color) { _frame = frame; _frameColor = color; updateGeometry(); update(); }
void Meter::setAlignmentMargins(const QMargins& mg) { _alignmentMargins = mg; updateGeometry(); update(); }

QSize Meter::VUSizeHint() const { return _VUSizeHint; }
void Meter::setVUSizeHint(const QSize& s) { _VUSizeHint = s; updateGeometry(); update(); }

} // namespace MusEGui
