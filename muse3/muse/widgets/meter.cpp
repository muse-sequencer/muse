//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: meter.cpp,v 1.4.2.2 2009/05/03 04:14:00 terminator356 Exp $
//  redesigned by oget on 2011/08/15
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Orcan Ogetbil (ogetbilo at sf.net)
//  (C) Copyright 2011-2016 Tim E. Real (terminator356 on users DOT sourceforge DOT net)
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

#include <cmath>

#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QVector>
#include <QLocale>
#include <algorithm>

#include "meter.h"
// #include "utils.h"
#include "fastlog.h"
#include "muse_math.h"

// Just an experiment. Some undesirable effects, see below...
//#define _USE_CLIPPER 1 

namespace MusEGui {

//---------------------------------------------------------
//   Meter
//---------------------------------------------------------

Meter::Meter(QWidget* parent, 
             MeterType type, 
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
      
      mtype = type;
      _orient = orient;
      d_scale.setTextHighlightMode(textHighlightMode);
      _scaleDist   = 0;    // Leftover from class Slider. Maybe use later?
      _showText   = false;
      overflow    = false;
      cur_pixv      = -1;     // Flag as -1 to initialize in paint.
      last_pixv     = 0;
      cur_pixmax    = 0;
      last_pixmax   = 0;
      val         = 0.0;
      targetVal   = 0.0;
      targetValStep = 0.0;
      maxVal      = 0.0;
      targetMaxVal = 0.0;
      minScale    = scaleMin;
      maxScale    = scaleMax;
      yellowScale = -10;
      redScale    = 0;
      setLineWidth(0);
      setMidLineWidth(0);

      // rounding radii
      xrad = 4;
      yrad = 4;

      dark_red_end = QColor(0x8e0000);
      dark_red_begin = QColor(0x8e3800);

      dark_yellow_end = QColor(0x8e6800);
      dark_yellow_center = QColor(0x8e8e00);
      dark_yellow_begin = QColor(0x6a8400);

//       dark_green_end = QColor(0x467800);
//       dark_green_begin = QColor(0x007000);

      light_red_end = QColor(0xff0000);
      light_red_begin = QColor(0xdd8800);

      light_yellow_end = QColor(0xddcc00);
      light_yellow_center = QColor(0xffff00);
      light_yellow_begin = QColor(0xddff00);

//       light_green_end = QColor(0x88ff00);
//       light_green_begin = QColor(0x00ff00);

      mask_center = QColor(225, 225, 225, 64);
      mask_edge = QColor(30, 30, 30, 64);

      separator_color = QColor(0x666666);
      peak_color = QColor(0xeeeeee);

//       darkGradGreen.setColorAt(1, dark_green_begin);
//       darkGradGreen.setColorAt(0, dark_green_end);

      darkGradYellow.setColorAt(1, dark_yellow_begin);
      darkGradYellow.setColorAt(0.5, dark_yellow_center);
      darkGradYellow.setColorAt(0, dark_yellow_end);

      darkGradRed.setColorAt(1, dark_red_begin);
      darkGradRed.setColorAt(0, dark_red_end);

//       lightGradGreen.setColorAt(1, light_green_begin);
//       lightGradGreen.setColorAt(0, light_green_end);

      lightGradYellow.setColorAt(1, light_yellow_begin);
      lightGradYellow.setColorAt(0.5, light_yellow_center);
      lightGradYellow.setColorAt(0, light_yellow_end);

      lightGradRed.setColorAt(1, light_red_begin);
      lightGradRed.setColorAt(0, light_red_end);

      maskGrad.setColorAt(0, mask_edge);
      maskGrad.setColorAt(0.5, mask_center);
      maskGrad.setColorAt(1, mask_edge);

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
    if (!hasUserScale())
       d_scale.setScale(maxScale, minScale, d_maxMajor, d_maxMinor);
    update();
}

QSize Meter::sizeHint() const
{
  int w = 40;
  int h = 40;
  const QFontMetrics fm = fontMetrics();
  int msWidth = 0, msHeight = 0;

  if(_scalePos != None) 
  {
    msWidth = d_scale.maxWidth(fm, false);
    msHeight = d_scale.maxHeight(fm);

    switch(_orient) 
    {
      case Qt::Vertical:
      {
        const int smw = msWidth + _scaleDist;
        switch(_scalePos)
        {
          case Left:
          case Right:
            //w = 2*d_xMargin + d_thumbWidth + smw + 2;
            w = smw + 2;
          break;
          
          case InsideVertical:
          {
            //const int aw = smw > d_thumbWidth ? smw : d_thumbWidth;
            //w = 2*d_xMargin + aw + 2;
            w = smw + 2;
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
        const int smh = msHeight + _scaleDist;
        switch(_scalePos)
        {
          case Top:
          case Bottom:
            //h = 2*d_yMargin + d_thumbWidth + smh;
            h = smh;
          break;
          
          case InsideHorizontal:
          {
            //const int ah = smh > d_thumbWidth ? smh : d_thumbWidth;
            //h = 2*d_yMargin + ah;
            h = smh;
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
    switch(_orient) 
    {
      case Qt::Vertical:
            w = 16;
            break;
      case Qt::Horizontal:
            h = 16;
            break;
    }
  }
  //fprintf(stderr, "Meter::sizeHint w:%d, h:%d\n", w, h);
  return QSize(w, h);
}

//---------------------------------------------------------
//   updateText
//---------------------------------------------------------

void Meter::updateText(double val)
{
  if(val >= -60.0f)
    _text = locale().toString(val, 'f', 1);
  else
  {
    _text = QString("-");
    _text += QChar(0x221e); // The infinty character
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

      if(mtype == DBMeter)
      {
        double minScaleLin = muse_db2val(minScale);
        if((v >= minScaleLin && targetVal != v) || targetVal >= minScaleLin)
        {
          targetVal = v;
          ud = true;
        }
      }
      else
      {
        if(targetVal != v)
        {
          targetVal = v;
          ud = true;
        }
      }

      if(ud || (maxVal != max))
      {
         targetMaxVal = max;
         if(!fallingTimer.isActive())
         {
            fallingTimer.start(1000/std::max(30, _refreshRate));
         }
      }
      

}

void Meter::updateTargetMeterValue()
{
   double range = maxScale - minScale;
   int fw = frameWidth();
   int w  = width() - 2*fw;
   int h  = height() - 2*fw;
   QRect udRect;
   bool udPeak = false;
   bool ud = false;

   if(targetVal > val)
   {
      val = targetVal;
      targetValStep = 0;
      ud = true;
   }
   else if(targetVal < val)
   {
      targetValStep = (val - targetVal) / ((double)(1000 / std::max(30, _refreshRate + 1)) / 7.0f);
      val -= targetValStep;
      if(val < targetVal)
      {
         val = targetVal;
      }
      ud = true;
   }

   const double transl_val = val - minScale;
   
   if(maxVal != targetMaxVal)
   {
     maxVal = targetMaxVal;
     const double v = (mtype == DBMeter) ? (MusECore::fast_log10(maxVal) * 20.0) : maxVal;
     
     if(_orient == Qt::Vertical)
     { 
       cur_pixmax = maxVal == 0 ? fw : int(((maxScale - v) * h)/range);
       if(_showText)
         updateText(v);
       if(cur_pixmax > h) 
         cur_pixmax = h;
       // Not using regions. Just lump them together.
       udRect = QRect(fw, last_pixmax, w, 1) | QRect(fw, cur_pixmax, w, 1);
     }
     else
     {
       //cur_pixmax = maxVal == 0 ? fw : int(((maxScale - v) * w)/range);
       cur_pixmax = maxVal == 0 ? w - fw : int((v * w)/range);
       if(_showText)
         updateText(v);
       if(cur_pixmax > w) 
         cur_pixmax = w;
       // Not using regions. Just lump them together.
       udRect = QRect(last_pixmax, fw, 1, h) | QRect(cur_pixmax, fw, 1, w);
     }
     
     //printf("Meter::setVal peak cur_ymax:%d last_ymax:%d\n", cur_ymax, last_ymax);
     last_pixmax = cur_pixmax;
     ud = true;
     udPeak = true;
   }

   if(ud)
   {
     if(_orient == Qt::Vertical)
     {
       if(cur_pixv > h) 
         cur_pixv = h;
       if(mtype == DBMeter)
         cur_pixv = val == 0 ? h : int(((maxScale - (MusECore::fast_log10(val) * 20.0)) * h)/range);
         //cur_pixv = transl_val <= 0.0 ? h : int(((maxScale - (MusECore::fast_log10(val) * 20.0)) * h)/range);  // TODO
       else
         cur_pixv = val == 0 ? h : int(((maxScale - val) * h)/range);
         //cur_pixv = int(((maxScale - transl_val) * h)/range);     // TODO
       
      //printf("Meter::setVal cur_yv:%d last_yv:%d\n", cur_yv, last_yv);
      int y1, y2;
      if(last_pixv < cur_pixv) { y1 = last_pixv; y2 = cur_pixv; } else { y1 = cur_pixv; y2 = last_pixv; }
      last_pixv = cur_pixv;

      if(udPeak)
        update(udRect | QRect(fw, y1, w, y2 - y1 + 1));
        //repaint(udRect | QRect(fw, y1, w, y2 - y1 + 1));
      else
        update(QRect(fw, y1, w, y2 - y1 + 1));
        //repaint(QRect(fw, y1, w, y2 - y1 + 1));
     }
     else
     {
       if(cur_pixv > w) 
         cur_pixv = w;
       if(mtype == DBMeter)
         //cur_pixv = val == 0 ? 0 : int((MusECore::fast_log10(val) * 20.0 * w)/range);
         cur_pixv = transl_val <= 0.0 ? 0 : int((MusECore::fast_log10(transl_val) * 20.0 * w)/range); // FIXME: Comparison correct?
       else
         //cur_pixv =  int((val * w)/range);
         cur_pixv =  int((transl_val * w)/range);
       
      //printf("Meter::setVal cur_yv:%d last_yv:%d\n", cur_yv, last_yv);
      int x1, x2;
      if(last_pixv < cur_pixv) { x1 = last_pixv; x2 = cur_pixv; } else { x1 = cur_pixv; x2 = last_pixv; }
      last_pixv = cur_pixv;

      if(udPeak)
        update(udRect | QRect(x1, fw, x2 - x1 + 1, h));
        //repaint(udRect | QRect(x1, fw, x2 - x1 + 1, h));
      else
        update(QRect(x1, fw, x2 - x1 + 1, h));
        //repaint(QRect(x1, fw, x2 - x1 + 1, h));
     }
   }
   if(!ud)
   {
      fallingTimer.stop();
   }

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

void Meter::setRange(double min, double max)
      {
      if(min == minScale && max == maxScale)   // p4.0.45
        return;
      
      minScale = min;
      maxScale = max;
      cur_pixv = -1;  // Force re-initialization.
      
      if (!hasUserScale())
        d_scale.setScale(minScale, maxScale, d_maxMajor, d_maxMinor);
      
      update();
      }

//---------------------------------------------------------
//   setRefreshRate
//---------------------------------------------------------

void Meter::setRefreshRate(int rate)
{
  _refreshRate = rate;
}

void Meter::setPrimaryColor(const QColor& color)
{
  _primaryColor = color; 
  int r = 0;
  
  dark_green_begin = _primaryColor.darker(200);
  dark_green_end = dark_green_begin;
  r = dark_green_end.red() + 0x46;
  if(r > 255)
    r = 255;
  dark_green_end.setRed(r);
  
  light_green_begin = _primaryColor;
  light_green_end = light_green_begin;
  r = light_green_end.red() + 0x88;
  if(r > 255)
    r = 255;
  light_green_end.setRed(r);
  
  darkGradGreen.setColorAt(1, dark_green_begin);
  darkGradGreen.setColorAt(0, dark_green_end);

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
  const int fw = frameWidth();
  const int w  = width() - 2*fw;
  const int h  = height() - 2*fw;
  p.setRenderHint(QPainter::Antialiasing);

  //p.fillRect(0, 0, width(), height(), QColor(50, 50, 50));

  const double range = maxScale - minScale;     
  const double transl_val = val - minScale;
  
  bool textDrawn = false;
  const int rectCount = ev->region().rectCount();
  QVector<QRect> rects = ev->region().rects();
  for(int ri = 0; ri < rectCount; ++ri)
  {
    const QRect& rect = rects.at(ri);
    
    // Tested OK! Small non-overlapping rectangles.
    //fprintf(stderr, "Meter::paintEvent rcount:%d ridx:%d rx:%d ry:%d rw:%d rh:%d w:%d h:%d\n", 
    //                rectCount, ri, rect.x(), rect.y(), rect.width(), rect.height(), w, h);

    QPainterPath drawingPath, updatePath, finalPath, cornerPath;
    //bool updFull = false;
    
    // Initialize. Can't do in ctor, must be done after layouts have been done. Most reliable to do it here.
    if(cur_pixv == -1) 
    {
      if(_orient == Qt::Vertical)
      {
        if(mtype == DBMeter)
        {  
          cur_pixv = val == 0 ? h : int(((maxScale - (MusECore::fast_log10(val) * 20.0)) * h)/range);
          cur_pixmax = maxVal == 0 ? fw : int(((maxScale - (MusECore::fast_log10(maxVal) * 20.0)) * h)/range);
        }  
        else
        {  
          cur_pixv = val == 0 ? h : int(((maxScale - val) * h)/range);
          cur_pixmax = maxVal == 0 ? fw : int(((maxScale - maxVal) * h)/range);
        }  
        if(cur_pixv > h) cur_pixv = h;
        last_pixv = cur_pixv;
        if(cur_pixmax > h) cur_pixmax = h;
        last_pixmax = cur_pixmax;
        //updFull = true;
        updatePath.addRect(fw, fw, w, h);  // Update the whole thing
      }
      else
      {
        if(mtype == DBMeter)
        {  
          cur_pixv = transl_val <= 0.0 ? 0 : int(((MusECore::fast_log10(transl_val) * 20.0) * w)/range);
          cur_pixmax = maxVal <= 0.0 ? w - fw : int(((MusECore::fast_log10(maxVal) * 20.0) * w)/range);
        }  
        else
        {  
          cur_pixv = int((transl_val * w)/range);
          cur_pixmax = maxVal <= 0.0 ? w - fw : int((maxVal * w)/range);
        }  
        if(cur_pixv > w) cur_pixv = w;
        last_pixv = cur_pixv;
        if(cur_pixmax > w) cur_pixmax = w;
        last_pixmax = cur_pixmax;
        //updFull = true;
        updatePath.addRect(fw, fw, w, h);  // Update the whole thing
      }
    }
    else
      updatePath.addRect(rect.x(), rect.y(), rect.width(), rect.height());  // Update only the requested rectangle
    
    drawingPath.addRoundedRect(fw, fw, w, h, xrad, yrad);  // The actual desired shape of the meter
    finalPath = drawingPath & updatePath;

    // Draw corners as normal background colour.
    cornerPath = updatePath - finalPath;            // Elegantly simple. Path subtraction! Wee...
    if(!cornerPath.isEmpty())
      p.fillPath(cornerPath, palette().window());  
    
#ifdef _USE_CLIPPER
    p.setClipPath(finalPath);       //  Meh, nice but not so good. Clips at edge so antialising has no effect! Can it be done ?
#endif
    
    // Draw the red, green, and yellow sections.
    drawVU(p, rect, finalPath, cur_pixv);
    
    // Draw the peak white line.
    //if(updFull || (cur_ymax >= rect.y() && cur_ymax < rect.height()))
    {
      p.setRenderHint(QPainter::Antialiasing, false);  // No antialiasing. Makes the line fuzzy, double height, or not visible at all.

      //p.setPen(peak_color);
      //p.drawLine(fw, cur_ymax, w, cur_ymax); // Undesirable. Draws outside the top rounded corners.
      //
      //QPainterPath path; path.moveTo(fw, cur_ymax); path.lineTo(w, cur_ymax);  // ? Didn't work. No line at all.
      //p.drawPath(path & finalPath);
      QPainterPath path;
      if(_orient == Qt::Vertical)
        path.addRect(fw, cur_pixmax + cur_pixmax % 2 + 1, w, 1);
      else
        path.addRect(cur_pixmax + cur_pixmax % 2 + 1, fw, 1, h);
      path &= finalPath;
      if(!path.isEmpty())
        p.fillPath(path, QBrush(peak_color));
    }
    
    // Draw the transparent layer on top of everything to give a 3d look
    p.setRenderHint(QPainter::Antialiasing);  
    maskGrad.setStart(QPointF(fw, fw));
    if(_orient == Qt::Vertical)
      maskGrad.setFinalStop(QPointF(w, fw));
    else
      maskGrad.setFinalStop(QPointF(fw, h));
    
#ifdef _USE_CLIPPER
    p.fillRect(rect, QBrush(maskGrad));
#else
    //QPainterPath path; path.addRect(fw, fw, w);
    //p.fillPath(finalPath & path, QBrush(maskGrad));
    p.fillPath(finalPath, QBrush(maskGrad));
#endif      

    if(_showText)
    {
      const QRect rr(rect.y(), rect.x(), rect.height(), rect.width()); // Rotate 90 degrees.
      if(!textDrawn && rr.intersects(_textRect))
      {
        textDrawn = true;
        //fprintf(stderr, "   Drawing text:%s\n", _text.toLatin1().constData());
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
  
  if(_scalePos != None) 
  {
//  p.fillRect(rect(), palette().window());
    p.setRenderHint(QPainter::Antialiasing, false);
    d_scale.draw(&p, palette());
  }
}

//---------------------------------------------------------
//   drawVU
//---------------------------------------------------------

void Meter::drawVU(QPainter& p, const QRect& rect, const QPainterPath& drawPath, int pixv)
{
      int fw = frameWidth();
      int w  = width() - 2*fw;
      int h  = height() - 2*fw;

      // Test OK. We are passed small rectangles on small value changes.
      //printf("Meter::drawVU rx:%d ry:%d rw:%d rh:%d w:%d h:%d\n", rect.x(), rect.y(), rect.width(), rect.height(), w, h); 

      if(_orient == Qt::Vertical)
      {  
        //QRect pr(0, 0,  w, 0);
        if(mtype == DBMeter)     // Meter type is dB...
        {
          double range = maxScale - minScale;
          int y1 = int((maxScale - redScale) * h / range);
          int y2 = int((maxScale - yellowScale) * h / range);

          darkGradGreen.setStart(QPointF(fw, y2));
          darkGradGreen.setFinalStop(QPointF(fw, h));
          darkGradYellow.setStart(QPointF(fw, y1));
          darkGradYellow.setFinalStop(QPointF(fw, y2));
          darkGradRed.setStart(QPointF(fw, fw));
          darkGradRed.setFinalStop(QPointF(fw, y1));

          lightGradGreen.setStart(QPointF(fw, y2));
          lightGradGreen.setFinalStop(QPointF(fw, h));
          lightGradYellow.setStart(QPointF(fw, y1));
          lightGradYellow.setFinalStop(QPointF(fw, y2));
          lightGradRed.setStart(QPointF(fw, fw));
          lightGradRed.setFinalStop(QPointF(fw, y1));

  #ifdef _USE_CLIPPER
          if(yv < y1)
          {
            // Red section:
            pr.setTop(fw); pr.setHeight(yv);
            p.fillRect(pr, QBrush(darkGradRed));     // dark red  
            pr.setTop(yv); pr.setHeight(y1-yv);
            p.fillRect(pr & rect, QBrush(lightGradRed));     // light red
            
            // Yellow section:
            pr.setTop(y1); pr.setHeight(y2-y1);
            p.fillRect(pr & rect, QBrush(lightGradYellow));     // light yellow
            
            // Green section:
            pr.setTop(y2); pr.setHeight(h-y2);
            p.fillRect(pr & rect, QBrush(lightGradGreen));     // light green
          }
          else
          if(yv < y2)
          {
            // Red section:
            pr.setTop(fw); pr.setHeight(y1);
            p.fillRect(pr & rect, QBrush(darkGradRed));     // dark red  
            
            // Yellow section:
            pr.setTop(y1); pr.setHeight(yv-y1);
            p.fillRect(pr & rect, QBrush(darkGradYellow));     // dark yellow
            pr.setTop(yv); pr.setHeight(y2-yv);
            p.fillRect(pr & rect, QBrush(lightGradYellow));     // light yellow
            
            // Green section:
            pr.setTop(y2); pr.setHeight(h-y2);
            p.fillRect(pr & rect, QBrush(lightGradGreen));     // light green
          }
          else
          //if(yv <= y3)   
          {
            // Red section:
            pr.setTop(fw); pr.setHeight(y1);
            p.fillRect(pr & rect, QBrush(darkGradRed));     // dark red  
            
            // Yellow section:
            pr.setTop(y1); pr.setHeight(y2-y1);
            p.fillRect(pr & rect, QBrush(darkGradYellow));     // dark yellow
            
            // Green section:
            pr.setTop(y2); pr.setHeight(yv-y2);
            p.fillRect(pr & rect, QBrush(darkGradGreen));     // dark green
            pr.setTop(yv); pr.setHeight(h-yv);
            p.fillRect(pr & rect, QBrush(lightGradGreen));     // light green
          }
        }  
        else     // Meter type is linear...
        {
          pr.setTop(fw); pr.setHeight(yv);
          p.fillRect(pr & rect, QBrush(darkGradGreen));   // dark green
          pr.setTop(yv); pr.setHeight(h-yv);
          p.fillRect(pr & rect, QBrush(lightGradGreen));   // light green
        }

  #else   // NOT    _USE_CLIPPER

          if(pixv < y1)
          {
            // Red section:
            {
              QPainterPath path; path.addRect(fw, fw, w, pixv); path &= drawPath;
              if(!path.isEmpty())
                p.fillPath(path, QBrush(darkGradRed));       // dark red
            }
            {
              QPainterPath path; path.addRect(fw, pixv, w, y1-pixv); path &= drawPath;
              if(!path.isEmpty())
                p.fillPath(path, QBrush(lightGradRed));       // light red
            }
            
            // Yellow section:
            {
              QPainterPath path; path.addRect(fw, y1, w, y2-y1); path &= drawPath;
              if(!path.isEmpty())
                p.fillPath(path, QBrush(lightGradYellow));   // light yellow
            }
            
            // Green section:
            {
              QPainterPath path; path.addRect(fw, y2, w, h-y2); path &= drawPath;
              if(!path.isEmpty())
                p.fillPath(path, QBrush(lightGradGreen));   // light green
            }
          }
          else
          if(pixv < y2)
          {
            // Red section:
            {
              QPainterPath path; path.addRect(fw, fw, w, y1); path &= drawPath;
              if(!path.isEmpty())
                p.fillPath(path, QBrush(darkGradRed));       // dark red
            }
            
            // Yellow section:
            {
              QPainterPath path; path.addRect(fw, y1, w, pixv-y1); path &= drawPath;
              if(!path.isEmpty())
                p.fillPath(path, QBrush(darkGradYellow));   // dark yellow
            }
            {
              QPainterPath path; path.addRect(fw, pixv, w, y2-pixv); path &= drawPath;
              if(!path.isEmpty())
                p.fillPath(path, QBrush(lightGradYellow));   // light yellow
            }
            
            // Green section:
            {
              QPainterPath path; path.addRect(fw, y2, w, h-y2); path &= drawPath;
              if(!path.isEmpty())
                p.fillPath(path, QBrush(lightGradGreen));   // light green
            }
          }
          else
          //if(yv <= y3)   
          {
            // Red section:
            {
              QPainterPath path; path.addRect(fw, fw, w, y1); path &= drawPath;
              if(!path.isEmpty())
                p.fillPath(path, QBrush(darkGradRed));       // dark red
            }
            
            // Yellow section:
            {
              QPainterPath path; path.addRect(fw, y1, w, y2-y1); path &= drawPath;
              if(!path.isEmpty())
                p.fillPath(path, QBrush(darkGradYellow));   // dark yellow
            }
            
            // Green section:
            {
              QPainterPath path; path.addRect(fw, y2, w, pixv-y2); path &= drawPath;
              if(!path.isEmpty())
                p.fillPath(path, QBrush(darkGradGreen));   // dark green
            }
            {
              QPainterPath path; path.addRect(fw, pixv, w, h-pixv); path &= drawPath;
              if(!path.isEmpty())
                p.fillPath(path, QBrush(lightGradGreen));   // light green
            }
          }

          // Separators: 
          {
            QRect r(0, y1, w, 1); r &= rect;
            if(!r.isNull())
              p.fillRect(r, separator_color);  
          }  
          {
            QRect r(0, y2, w, 1); r &= rect;
            if(!r.isNull())
              p.fillRect(r, separator_color);  
          }  
        }  
        else      // Meter type is linear...
        {
          darkGradGreen.setStart(QPointF(fw, fw));
          darkGradGreen.setFinalStop(QPointF(fw, h));

          lightGradGreen.setStart(QPointF(fw, fw));
          lightGradGreen.setFinalStop(QPointF(fw, h));

          {
            QPainterPath path; path.addRect(fw, fw, w, pixv); path &= drawPath;
            if(!path.isEmpty())
              p.fillPath(path, QBrush(darkGradGreen));   // dark green
          }
          {
            QPainterPath path; path.addRect(fw, pixv, w, h-pixv); path &= drawPath;
            if(!path.isEmpty())
              p.fillPath(path, QBrush(lightGradGreen));   // light green
          }
        }

#endif  // NOT   _USE_CLIPPER

      }
      else    // Horizontal meter
      {
        //QRect pr(0, 0,  w, 0);
        if(mtype == DBMeter)     // Meter type is dB...
        {
          double range = maxScale - minScale;
          int x1 = int(redScale * w / range);
          int x2 = int(yellowScale * w / range);

          darkGradGreen.setStart(QPointF(x2, fw));
          darkGradGreen.setFinalStop(QPointF(w, fw));
          darkGradYellow.setStart(QPointF(x1, fw));
          darkGradYellow.setFinalStop(QPointF(x2, fw));
          darkGradRed.setStart(QPointF(fw, fw));
          darkGradRed.setFinalStop(QPointF(x1, fw));

          lightGradGreen.setStart(QPointF(x2, fw));
          lightGradGreen.setFinalStop(QPointF(w, fw));
          lightGradYellow.setStart(QPointF(x1, fw));
          lightGradYellow.setFinalStop(QPointF(x2, fw));
          lightGradRed.setStart(QPointF(fw, fw));
          lightGradRed.setFinalStop(QPointF(x1, fw));

  #ifdef _USE_CLIPPER
          if(yv < y1)
          {
            // Red section:
            pr.setTop(fw); pr.setHeight(yv);
            p.fillRect(pr, QBrush(darkGradRed));     // dark red  
            pr.setTop(yv); pr.setHeight(y1-yv);
            p.fillRect(pr & rect, QBrush(lightGradRed));     // light red
            
            // Yellow section:
            pr.setTop(y1); pr.setHeight(y2-y1);
            p.fillRect(pr & rect, QBrush(lightGradYellow));     // light yellow
            
            // Green section:
            pr.setTop(y2); pr.setHeight(h-y2);
            p.fillRect(pr & rect, QBrush(lightGradGreen));     // light green
          }
          else
          if(yv < y2)
          {
            // Red section:
            pr.setTop(fw); pr.setHeight(y1);
            p.fillRect(pr & rect, QBrush(darkGradRed));     // dark red  
            
            // Yellow section:
            pr.setTop(y1); pr.setHeight(yv-y1);
            p.fillRect(pr & rect, QBrush(darkGradYellow));     // dark yellow
            pr.setTop(yv); pr.setHeight(y2-yv);
            p.fillRect(pr & rect, QBrush(lightGradYellow));     // light yellow
            
            // Green section:
            pr.setTop(y2); pr.setHeight(h-y2);
            p.fillRect(pr & rect, QBrush(lightGradGreen));     // light green
          }
          else
          //if(yv <= y3)   
          {
            // Red section:
            pr.setTop(fw); pr.setHeight(y1);
            p.fillRect(pr & rect, QBrush(darkGradRed));     // dark red  
            
            // Yellow section:
            pr.setTop(y1); pr.setHeight(y2-y1);
            p.fillRect(pr & rect, QBrush(darkGradYellow));     // dark yellow
            
            // Green section:
            pr.setTop(y2); pr.setHeight(yv-y2);
            p.fillRect(pr & rect, QBrush(darkGradGreen));     // dark green
            pr.setTop(yv); pr.setHeight(h-yv);
            p.fillRect(pr & rect, QBrush(lightGradGreen));     // light green
          }
        }  
        else     // Meter type is linear...
        {
          pr.setTop(fw); pr.setHeight(yv);
          p.fillRect(pr & rect, QBrush(darkGradGreen));   // dark green
          pr.setTop(yv); pr.setHeight(h-yv);
          p.fillRect(pr & rect, QBrush(lightGradGreen));   // light green
        }

  #else   // NOT    _USE_CLIPPER

          if(pixv < x1)
          {
            // Red section:
            {
              QPainterPath path; path.addRect(fw, fw, pixv, h); path &= drawPath;
              if(!path.isEmpty())
                p.fillPath(path, QBrush(darkGradRed));       // dark red
            }
            {
              QPainterPath path; path.addRect(pixv, fw, x1-pixv, h); path &= drawPath;
              if(!path.isEmpty())
                p.fillPath(path, QBrush(lightGradRed));       // light red
            }
            
            // Yellow section:
            {
              QPainterPath path; path.addRect(x1, fw, x2-x1, h); path &= drawPath;
              if(!path.isEmpty())
                p.fillPath(path, QBrush(lightGradYellow));   // light yellow
            }
            
            // Green section:
            {
              QPainterPath path; path.addRect(x2, fw, w-x2, h); path &= drawPath;
              if(!path.isEmpty())
                p.fillPath(path, QBrush(lightGradGreen));   // light green
            }
          }
          else
          if(pixv < x2)
          {
            // Red section:
            {
              QPainterPath path; path.addRect(fw, fw, x1, h); path &= drawPath;
              if(!path.isEmpty())
                p.fillPath(path, QBrush(darkGradRed));       // dark red
            }
            
            // Yellow section:
            {
              QPainterPath path; path.addRect(x1, fw, pixv-x1, h); path &= drawPath;
              if(!path.isEmpty())
                p.fillPath(path, QBrush(darkGradYellow));   // dark yellow
            }
            {
              QPainterPath path; path.addRect(pixv, fw, x2-pixv, h); path &= drawPath;
              if(!path.isEmpty())
                p.fillPath(path, QBrush(lightGradYellow));   // light yellow
            }
            
            // Green section:
            {
              QPainterPath path; path.addRect(x2, fw, w-x2, h); path &= drawPath;
              if(!path.isEmpty())
                p.fillPath(path, QBrush(lightGradGreen));   // light green
            }
          }
          else
          //if(yv <= y3)   
          {
            // Red section:
            {
              QPainterPath path; path.addRect(fw, fw, x1, h); path &= drawPath;
              if(!path.isEmpty())
                p.fillPath(path, QBrush(darkGradRed));       // dark red
            }
            
            // Yellow section:
            {
              QPainterPath path; path.addRect(x1, fw, x2-x1, h); path &= drawPath;
              if(!path.isEmpty())
                p.fillPath(path, QBrush(darkGradYellow));   // dark yellow
            }
            
            // Green section:
            {
              QPainterPath path; path.addRect(x2, fw, pixv-x2, h); path &= drawPath;
              if(!path.isEmpty())
                p.fillPath(path, QBrush(darkGradGreen));   // dark green
            }
            {
              QPainterPath path; path.addRect(pixv, fw, w-pixv, h); path &= drawPath;
              if(!path.isEmpty())
                p.fillPath(path, QBrush(lightGradGreen));   // light green
            }
          }

          // Separators: 
          {
            QRect r(x1, 0, 1, h); r &= rect;
            if(!r.isNull())
              p.fillRect(r, separator_color);  
          }  
          {
            QRect r(x2, 0, 1, h); r &= rect;
            if(!r.isNull())
              p.fillRect(r, separator_color);  
          }  
        }  
        else      // Meter type is linear...
        {
          darkGradGreen.setStart(QPointF(fw, fw));
          darkGradGreen.setFinalStop(QPointF(w, fw));

          lightGradGreen.setStart(QPointF(fw, fw));
          lightGradGreen.setFinalStop(QPointF(w, fw));

          {
            QPainterPath path; path.addRect(fw, fw, pixv, h); path &= drawPath;
            if(!path.isEmpty())
              p.fillPath(path, QBrush(lightGradGreen));   // light green
          }
          {
            QPainterPath path; path.addRect(pixv, fw, w, h); path &= drawPath;
            if(!path.isEmpty())
              p.fillPath(path, QBrush(darkGradGreen));   // dark green
          }
        }

#endif  // NOT   _USE_CLIPPER
      }

}

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void Meter::resizeEvent(QResizeEvent* ev)
{
   // For some reason upon resizing we get double calls here and in paintEvent.
   //printf("Meter::resizeEvent w:%d h:%d\n", ev->size().width(), ev->size().height());

   cur_pixv = -1;  // Force re-initialization.
   QFrame::resizeEvent(ev);

   //update(); //according to docs, update will be called automatically
   
    QSize s = ev->size();
  
    const QFontMetrics fm = fontMetrics();
    // reposition slider
    if(_orient == Qt::Horizontal)
    {
      switch(_scalePos)
      {
        case Top:
            d_scale.setGeometry(this->rect().x(),
              this->rect().y() + s.height() - 1 - _scaleDist,
              s.width(),
              ScaleDraw::Top);
            break;
      
        case Bottom:
            d_scale.setGeometry(this->rect().x(),
              this->rect().y() + s.height() + _scaleDist,
              s.width(),
              ScaleDraw::Bottom);
            break;
      
        case InsideHorizontal:
            d_scale.setGeometry(this->rect().x(),
              this->rect().y() + d_scale.maxHeight(fm) + _scaleDist,
              s.width(),
              ScaleDraw::InsideHorizontal);
            break;
            
        default:
            break;
      }
    }
    else // d_orient == Qt::Vertical
    {
      switch(_scalePos)
      {
        case Left:
            d_scale.setGeometry(this->rect().x() - _scaleDist,
              this->rect().y(),
              s.height(),
              ScaleDraw::Left);
            break;
            
        case Right:
            d_scale.setGeometry(this->rect().x() + width() + _scaleDist,
              this->rect().y(),
              s.height(),
              ScaleDraw::Right);
            break;
            
        case InsideVertical:
        {
            const int mxlw = d_scale.maxLabelWidth(fm, false);
            const int sclw = d_scale.scaleWidth();
            
            d_scale.setGeometry(this->rect().x() + mxlw + sclw + _scaleDist,
              this->rect().y(),                                
              s.height(),
              ScaleDraw::InsideVertical);
        }
        break;
            
        default:
            break;
      }
    }
  
  adjustScale();
}

void Meter::adjustScale()
{
//   d_maxMinor = maxMin;
//   if(hasUserScale())
//     d_scale.setScale(minValue(), maxValue(), d_maxMajor, d_maxMinor, mstep, log());
//   else
//     d_scale.setScale(minValue(), maxValue(), d_maxMajor, d_maxMinor, log());
//   update();
//   const double range = maxScale() - minScale();
//   if(range == 0.0)
//     return;
// 
//   int maxMaj = 5;
//   int maxMin = 3;
//   double mstep = scaleStep();
// 
//   QFontMetrics fm = fontMetrics();
//   if(_orient == Qt::Horizontal)
//   {
//     int unit_w = fm.width("888.8888");
//     if(unit_w == 0)
//       unit_w = 20;
// 
//     if(hasUserScale())
//     {
//       if(d_sliderRect.width() != 0)
//       {
//         const int fact = (int)(3.0 * range / (double)(d_sliderRect.width())) + 1;
//         mstep *= fact;
//       }
//     }
//     else
//     {
//       maxMaj = (int)((double)(d_sliderRect.width()) / (1.5 * ((double)unit_w)));
//       if(maxMaj < 1)
//         maxMaj = 1;
//       if(maxMaj > 5)
//         maxMaj = 5;
//     }
//     maxMin = (int)((double)(d_sliderRect.width()) / (1.5 * ((double)unit_w)));
//     if(maxMin < 1)
//       maxMin = 1;
//     if(maxMin > 5)
//       maxMin = 5;
//   }
//   else
//   {
//     int unit_h = fm.height();
//     if(unit_h == 0)
//       unit_h = 20;
//     
//     if(hasUserScale())
//     {
//       if(d_sliderRect.height() != 0)
//       {
//         const int fact = (int)(3.0 * range / (double)(d_sliderRect.height())) + 1;
//         mstep *= fact;
//       }
//     }
//     else
//     {
//       maxMaj = (int)((double)(d_sliderRect.height()) / (1.5 * ((double)unit_h)));
//       if(maxMaj < 1)
//         maxMaj = 1;
//       if(maxMaj > 5)
//         maxMaj = 5;
//     }
//     maxMin = (int)((double)(d_sliderRect.height()) / (1.5 * ((double)unit_h)));
//     if(maxMin < 1)
//       maxMin = 1;
//     if(maxMin > 5)
//       maxMin = 5;
//   }
// 
//   //fprintf(stderr, "Slider::adjustScale: maxMaj:%d maxMin:%d scaleStep:%f\n", maxMaj, maxMin, mstep);
//   d_maxMajor = maxMaj;
}

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void Meter::mousePressEvent(QMouseEvent*)
      {
      emit mousePress();
      }

} // namespace MusEGui
