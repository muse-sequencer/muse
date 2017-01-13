//=========================================================
//  MusE
//  Linux Music Editor
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  compact_slider.cpp
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

#include <cmath>
#include "mmath.h"

#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QLocale>
#include <QEvent>
#include <QFlags>
#include <QToolTip>

#include "utils.h"
#include "popup_double_spinbox.h"
#include "compact_slider.h"
#include "slider.h"
//#include "icons.h"
//#include "lcd_widgets.h"

// For debugging output: Uncomment the fprintf section.
#define DEBUG_COMPACT_SLIDER(dev, format, args...) // fprintf(dev, format, ##args);


namespace MusEGui {

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
               const QColor& borderColor,
               const QColor& barColor,
               const QColor& slotColor,
               const QColor& thumbColor)
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

      setBorderlessMouse(false);
      setCursorHoming(false);
      setPagingButtons(Qt::NoButton);
      
      setEnableValueToolTips(true);
      
      //_LCDPainter = new LCDPainter();

      //_onPath = 0;
      //_offPath = 0;
      _editor = 0;
      _editMode = false;
      
      d_borderColor = borderColor;
      d_barColor = barColor;
      d_slotColor = slotColor;
      d_thumbColor = thumbColor;

      _maxAliasedPointSize = 8;
      
      d_labelText = labelText;
      d_valPrefix = valPrefix;
      d_valSuffix = valSuffix;
      d_specialValueText = specialValueText;
      
      _textHighlightMode = TextHighlightShadow | TextHighlightOn;
      
      _valueDecimals = 2;
      _off = false;
      d_offText = tr("off");
      _showValue = true;

      _detectThumb = false;
      _autoHideThumb = true;
      _hasOffMode = false;
      d_thumbLength = 0;
      d_thumbHitLength = 0;
      d_thumbHalf = d_thumbLength / 2;
      d_thumbWidth = 16;
      d_thumbWidthMargin = 0;
      _mouseOverThumb = false;
      _hovered = false;
      _activeBorders = AllBorders;

      d_scaleDist   = 4;
      d_scaleStep   = 0.0;
      d_scalePos    = scalePos;
      d_xMargin     = 1;
      d_yMargin     = 1;
      d_mMargin    = 1;

      _entered = false;

      setOrientation(orient);
      
      d_valuePixel = 0;
      d_valuePixelWidth = 0;
      
      //updatePixmaps();
      getActiveArea(); 
      getPixelValues();
      }

//------------------------------------------------------------
//.F  CompactSlider::~Slider
//    Destructor
//.u  Syntax
//.f  CompactSlider::~Slider()
//------------------------------------------------------------

CompactSlider::~CompactSlider()
      {
        //if(_onPath)
        //  delete _onPath;
        //if(_offPath)
        //  delete _offPath;

        //if(_LCDPainter)
        //  delete _LCDPainter;
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
              return QSize(16, font_height + 3 + 2 * yMargin);
              break;
        case Qt::Horizontal:
              return QSize(16, font_height + 3 + 2 * yMargin);
              break;
        }
  return QSize(10, 10);
}
      
void CompactSlider::processSliderPressed(int)
{
//    bPressed = true;
   update();
}

void CompactSlider::processSliderReleased(int)
{
  QPoint p = mapFromGlobal(QCursor::pos());
  getMouseOverThumb(p);
  
  update();
  
  DEBUG_COMPACT_SLIDER(stderr, 
    "CompactSlider::processSliderReleased trackingIsActive:%d val:%.20f valHasChanged:%d\n", 
    trackingIsActive(), value(), valueHasChangedAtRelease());
  
// Changed. BUG: Was causing problems with sending changed even though it hadn't.
// It should never signal a change on release UNLESS tracking is off, because otherwise the last movement
//  already sent the last changed signal. FIXME Even this is still flawed. If no tracking, it would likely 
//  still signal a change upon simple press and release even though nothing changed.
//   if((!tracking()) || valHasChanged())
  if(!trackingIsActive() && valueHasChangedAtRelease())
    emit valueStateChanged(value(), isOff(), id(), d_scrollMode);
}

QString CompactSlider::toolTipValueText(bool inclLabel, bool inclVal) const
{ 
  const double minV = minValue(ConvertNone);
  const double val = value(ConvertNone);
  const QString comp_val_text = isOff() ? d_offText :
                                ((val <= minV && !d_specialValueText.isEmpty()) ? 
                                d_specialValueText : (d_valPrefix + locale().toString(val, 'f', _valueDecimals) + d_valSuffix));
  QString txt;
  if(inclLabel)
    txt += d_labelText;
  if(inclLabel && inclVal)
    txt += QString(": ");
  if(inclVal)
  {
    txt += QString("<em>");
    txt += comp_val_text;
    txt += QString("</em>");
  }
  return txt;
}

void CompactSlider::showValueToolTip(QPoint /*p*/)
{ 
  const QString txt = toolTipValueText(true, true);
  if(!txt.isEmpty())
  {
    // Seems to be a small problem with ToolTip: Even if we force the font size,
    //  if a previous tooltip was showing from another control at another font size,
    //  it refuses to change font size. Also, if we supply the widget to showText(),
    //  it refuses to change font size and uses the widget's font size instead.
    // Also, this craziness with ToolTip's self-offsetting is weird: In class CompactKnob
    //  it is best when we supply the parent's position, while in class CompactSlider
    //  it is best when we supply the widget's position - and it STILL isn't right!
    // Supplying the widget's position to CompactKnob, or parent's position to CompactSlider
    //  actually makes the offsetting worse!
    if(QToolTip::font().pointSize() != 10)
    {
      QFont fnt = font();
      fnt.setPointSize(10);
      QToolTip::setFont(fnt);
      QToolTip::hideText();
    }
    //QToolTip::showText(p, txt, this, QRect(), 1000);
    //QToolTip::showText(p, txt, 0, QRect(), 1000);
    QToolTip::showText(mapToGlobal(pos()), txt, 0, QRect(), 3000);
    //QToolTip::showText(mapToGlobal(pos()), txt);
    //QToolTip::showText(mapToGlobal(parentWidget() ? parentWidget()->pos() : pos()), txt, parentWidget() ? parentWidget() : this, QRect(), 3000);
  }
}

void CompactSlider::setActiveBorders(ActiveBorders_t borders)
{
  _activeBorders = borders;
  resize(size());
  updateGeometry();
  update();
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


// //------------------------------------------------------------
// //.-
// //.F  CompactSlider::fontChange
// //  Notify change in font
// //  
// //.u  Syntax
// //.f   CompactSlider::fontChange(const QFont &oldFont)
// //
// //------------------------------------------------------------
// void CompactSlider::fontChange(const QFont & /*oldFont*/)
// {
//     repaint();
// }

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
  const QRect r = d_sliderRect;
  const double val = value(ConvertNone);

  if(borderlessMouse() && d_scrollMode != ScrDirect)
  {
    DEBUG_COMPACT_SLIDER(stderr, "CompactSlider::getValue value:%.20f p x:%d y:%d step:%.20f x change:%.20f\n", 
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
      rv =  min + rint( (max - min) * (1.0 - dpos / dheight) / step() ) * step();
    }
  }
  return(rv);
}


//------------------------------------------------------------
//
//.F  CompactSlider::moveValue
//  Determine the value corresponding to a specified mouse movement.
//
//.u  Syntax
//.f  void SliderBase::moveValue(const QPoint &deltaP, bool fineMode)
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
double CompactSlider::moveValue(const QPoint &deltaP, bool fineMode)
{
  double rv;
  const QRect r = d_sliderRect;

  const double val = value(ConvertNone);

  if((fineMode || borderlessMouse()) && d_scrollMode != ScrDirect)
  {
    DEBUG_COMPACT_SLIDER(stderr, "CompactSlider::moveValue value:%.20f p x:%d y:%d step:%.20f x change:%.20f\n", 
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
      
      DEBUG_COMPACT_SLIDER(stderr, "CompactSlider::moveValue value:%.20f p dx:%d dy:%d drange:%.20f step:%.20f dval_diff:%.20f rv:%.20f\n", 
                       val, deltaP.x(), deltaP.y(), drange, step(), dval_diff, rv);
    }
  }
  else
  {
    if(r.height() <= d_thumbLength)
      rv = 0.5 * (min + max);
    else
    {
      const double dpos = double(deltaP.y());
      const double dheight = double(r.height() - d_thumbLength);
      const double dval_diff = (drange * dpos) / dheight;
      d_valAccum += dval_diff;
      rv = rint(d_valAccum / step()) * step();
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
void CompactSlider::getScrollMode( QPoint &p, const Qt::MouseButton &button, const Qt::KeyboardModifiers& modifiers, int &scrollMode, int &direction )
{
  // If modifier or button is held, jump directly to the position at first.
  // After handling it, the caller can change to SrcMouse scroll mode.
  if(modifiers & Qt::ControlModifier || button == Qt::MidButton)
  {
    scrollMode = ScrDirect;
    direction = 0;
    return;
  }
  
//   if((!detectThumb() || borderlessMouse()))
  if(borderlessMouse() && button != Qt::NoButton && d_sliderRect.contains(p))
  {
//     if(button != Qt::NoButton && d_sliderRect.contains(p))
//     {
      scrollMode = ScrMouse;
      direction = 0;
      return;
//     }
  }
  else
  {
    if(cursorHoming() && button == Qt::LeftButton)
    {
      if(d_sliderRect.contains(p))
      {
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
      if (d_orient == Qt::Horizontal)
        currentPos = p.x() - d_sliderRect.x();
      else
        currentPos = p.y() - d_sliderRect.y();

      if(d_sliderRect.contains(p))
      {
  //         if ((currentPos > d_valuePixel - d_thumbHalf)  
  //             && (currentPos < d_valuePixel + d_thumbHalf))
        if (!detectThumb() || 
            ((currentPos >= d_valuePixel - d_thumbHitLength / 2)  
             && (currentPos <= d_valuePixel + d_thumbHitLength / 2)))
        {
          scrollMode = ScrMouse;
          direction = 0;
          return;
        }
        else if(pagingButtons().testFlag(button))
        {
          scrollMode = ScrPage;
          if(((currentPos > d_valuePixel) && (d_orient == Qt::Horizontal))
              || ((currentPos <= d_valuePixel) && (d_orient != Qt::Horizontal)))
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
  const double minV = minValue(ConvertNone);
  const double maxV = maxValue(ConvertNone);
  const double range = maxV - minV;
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

  const int req_thumb_margin = d_thumbLength == 0 ? 0 : ((d_thumbHalf - d_xMargin) > 1 ? (d_thumbHalf - d_xMargin) : 1);

  const int label_to_val_margin = 6;

  const QPen orig_pen = p.pen();
  
  const QColor& margin_color = isEnabled() ? (d_borderColor.isValid() ? d_borderColor : pal.color(QPalette::Active, QPalette::Button)) :
                                             pal.color(QPalette::Disabled, QPalette::Button);
  QColor border_color;
  
  if(_textHighlightMode & TextHighlightFocus)
    border_color = _hovered ? pal.color(isEnabled() ? QPalette::Active : QPalette::Disabled, QPalette::Highlight).lighter() : margin_color;
  else
    //border_color = hasFocus() ? pal.color(isEnabled() ? QPalette::Active : QPalette::Disabled, QPalette::Highlight).lighter() : margin_color;
    border_color = isEnabled() ? (hasFocus() ? margin_color.lighter() : margin_color) : pal.color(QPalette::Disabled, QPalette::Highlight);

  const QColor c4 = isEnabled() ? (d_slotColor.isValid() ? d_slotColor : pal.color(QPalette::Active, QPalette::Dark)) :
                    pal.color(QPalette::Disabled, QPalette::Dark);
  const QColor c3 = c4.darker(125);

  QColor inactive_border_color;

  if(_textHighlightMode & TextHighlightFocus)
    //inactive_border_color = _hovered ? pal.color(isEnabled() ? QPalette::Active : QPalette::Disabled, QPalette::Highlight).lighter() : margin_color;
    inactive_border_color = isEnabled() ? (_hovered ? border_color : c3) : pal.color(QPalette::Disabled, QPalette::Highlight);
  else
    //border_color = hasFocus() ? pal.color(isEnabled() ? QPalette::Active : QPalette::Disabled, QPalette::Highlight).lighter() : margin_color;
    inactive_border_color = isEnabled() ? (hasFocus() ? border_color : c3) : pal.color(QPalette::Disabled, QPalette::Highlight);

  // Draw margins:
  if(d_yMargin)
  {
    // Top
    //if(_activeBorders & TopBorder)
      p.fillRect(geo.x(),
                  geo.y(),
                  geo.width(),
                  d_yMargin,
                  _activeBorders & TopBorder ? border_color : inactive_border_color);
  
    // Bottom
    //if(_activeBorders & BottomBorder)
      p.fillRect(geo.x(),
                  geo.height() - d_yMargin,
                  geo.width(),
                  d_yMargin,
                  _activeBorders & BottomBorder ? border_color : inactive_border_color);
  }
  
  if(d_xMargin)
  {
    // Left
    //if(_activeBorders & LeftBorder)
      p.fillRect(geo.x(),
                  geo.y(),
                  d_xMargin,
                  geo.height(),
                  _activeBorders & LeftBorder ? border_color : inactive_border_color);
    
    // Right
    //if(_activeBorders & RightBorder)
      p.fillRect(geo.width() - d_xMargin,
                  geo.y(),
                  d_xMargin,
                  geo.height(),
                  _activeBorders & RightBorder ? border_color : inactive_border_color);
  }
  
  // Extra left margin
  //if(_activeBorders & LeftBorder)
    p.fillRect(d_xMargin,
                d_yMargin,
                req_thumb_margin,
                geo.height() - 2 * d_yMargin,
                _activeBorders & LeftBorder ? margin_color : inactive_border_color);
  
  // Extra right margin
  //if(_activeBorders & RightBorder)
    p.fillRect(
                geo.width() - d_xMargin - req_thumb_margin,
                d_yMargin,
                req_thumb_margin,
                geo.height() - 2 * d_yMargin,
                _activeBorders & RightBorder ? margin_color : inactive_border_color);
  
//   const QPainterPath& onPath = *_onPath;
//   
//   // Draw corners as normal background colour. Path subtraction.
//   QPainterPath cornerPath;
//   cornerPath.addRect(d_sliderRect);
//   cornerPath -= onPath;
//   if(!cornerPath.isEmpty())
//     //p.fillPath(cornerPath, palette().window().color().darker(125));
//     //p.fillPath(cornerPath, palette().shadow().color().darker(125));
//     p.fillPath(cornerPath, isEnabled() ? (d_slotColor.isValid() ? d_slotColor : palette().color(QPalette::Active, QPalette::Dark)) :
//                                          palette().color(QPalette::Disabled, QPalette::Dark));

  int x = d_sliderRect.x();
  int y = d_sliderRect.y();
  int h = d_sliderRect.height();

  const int x1 = x;
  const int w1 = d_valuePixelWidth;

  const int y1 = y;
  const int y2 = y1 + h - 1;
  
  QLinearGradient linearGrad_a(x, y1, x, y2);
  
  const QColor c2 = isEnabled() ? (d_barColor.isValid() ? d_barColor : pal.color(QPalette::Active, QPalette::Highlight)) : 
                    pal.color(QPalette::Disabled, QPalette::Highlight);
  const QColor c1 = c2.darker(125);
  linearGrad_a.setColorAt(0, c1);
  linearGrad_a.setColorAt(0.5, c2);
  linearGrad_a.setColorAt(1, c1);
  
  QPainterPath onPath;
  MusECore::addRoundedPath(&onPath, QRect (x1, y, w1, h), 4, 4, 
    (MusECore::Corner) (MusECore::UpperRight | MusECore::LowerRight) );
  if(!onPath.isEmpty())
    p.fillPath(onPath, linearGrad_a);


  linearGrad_a.setColorAt(0, c3);
  linearGrad_a.setColorAt(0.5, c4);
  linearGrad_a.setColorAt(1, c3);
  
  QPainterPath offPath;
  offPath.addRect(d_sliderRect);
  offPath -= onPath;
  if(!offPath.isEmpty())
    p.fillPath(offPath, linearGrad_a);
    
    
  const double minV = minValue(ConvertNone);
  const double val = value(ConvertNone);
  
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

//   p.setClipPath(onPath);
//   p.fillPath(onPath, _onPixmap);
  
//   p.setClipPath(offPath);
//   p.fillPath(offPath, _offPixmap);
  
  // Restore the clipper to full size.
//   p.setClipRect(geo);
  
  if((!_autoHideThumb || _mouseOverThumb) && d_thumbLength > 0)
  {
    DEBUG_COMPACT_SLIDER(stderr, "CompactSlider::PaintEvent _mouseOverThumb:%d\n", _mouseOverThumb);
    const QRect thumb_rect((d_orient == Qt::Horizontal) ?
                          QRect(d_sliderRect.x() + d_valuePixel - d_thumbHalf, 
                                d_sliderRect.y() + d_thumbWidthMargin,
                                d_thumbLength,
                                d_sliderRect.height() - 2 * d_thumbWidthMargin) : 
                          QRect(d_sliderRect.x() + d_thumbWidthMargin,
                                d_sliderRect.y() + d_valuePixel - d_thumbHalf,
                                d_sliderRect.width() - 2 * d_thumbWidthMargin,
                                d_thumbLength));
    p.fillRect(thumb_rect, isEnabled() ? (d_thumbColor.isValid() ? d_thumbColor : pal.color(QPalette::Active, QPalette::Mid)) :
                                         pal.color(QPalette::Disabled, QPalette::Mid));
  }

  const QFont& fnt = font();
  QFont aliased_fnt(fnt);
  // Turn off anti-aliasing for sharper text. if we want it:
  if(fnt.pointSize() <= _maxAliasedPointSize)
  {
    aliased_fnt.setFamily("Sans");
    //aliased_fnt.setHintingPreference(QFont::PreferVerticalHinting);
    //aliased_fnt.setStyleStrategy(QFont::PreferBitmap);
    aliased_fnt.setStyleStrategy(QFont::NoAntialias);
    //p.setFont(aliased_fnt);
  }
  const QFontMetrics aliased_fm(aliased_fnt);

  const bool show_val = _showValue;

  const QFontMetrics fm = p.fontMetrics();
  
  const QRect text_area(d_sliderRect.adjusted(1, 1, -1, -1));
  
  const QString comp_val_text = isOff() ? d_offText :
                                ((val <= minV && !d_specialValueText.isEmpty()) ? 
                                d_specialValueText : (d_valPrefix + locale().toString(val, 'f', _valueDecimals) + d_valSuffix));
  //const int val_width = fm.width(comp_val_text);
  const int val_width = aliased_fm.width(comp_val_text);
  int vx = text_area.width() - val_width;
  if(vx < 0)
    vx = 0;
  const QRect val_area(vx, text_area.y(), val_width, text_area.height());
  
  int lw = text_area.width();
  if(show_val)
    lw = lw - val_width - label_to_val_margin;
  if(lw < 0)
    lw = 0;
  QRect label_area(text_area.x(), text_area.y(), lw, text_area.height());
  
  //const QString elided_label_text = fm.elidedText(d_labelText, Qt::ElideMiddle, text_area.width());
//   const QString elided_label_text = fm.elidedText(d_labelText, Qt::ElideMiddle, label_area.width());
  const QString elided_label_text = d_labelText;
  if(!show_val)
  {
    const QRect label_br = fm.boundingRect(d_labelText);
    const int label_bw = label_br.width();
    int label_xoff = (label_area.width() - label_bw) / 2;
    if(label_xoff < 0)
      label_xoff = 0;
    label_area.adjust(label_xoff, 0, 0, 0);
  }

  //const int label_width = fm.width(elided_label_text);
  
  //const bool show_both = (text_area.width() - val_width) > (label_area.width() + 2);
  //const bool show_both = label_area.width() > label_to_val_margin;
  const bool show_label = show_val || label_area.width() > label_to_val_margin;

  const bool on  = _textHighlightMode & TextHighlightOn;
  const bool shd = _textHighlightMode & TextHighlightShadow;
  const bool spl = _textHighlightMode & TextHighlightSplit;
  const bool hov = _textHighlightMode & TextHighlightHover;
  const bool foc = _textHighlightMode & TextHighlightFocus;
  
  const bool is_hov = hov && _hovered;
  const bool is_foc = foc && hasFocus();

  // Normal text:
  p.setPen(Qt::black);
  //QRect text_bkg = text_area;
  QRect label_bkg = label_area;
  QRect val_bkg = val_area;
  if(shd)
  {
    //text_bkg.adjust(1, 1, 1, 1);
    val_bkg.adjust(1, 1, 1, 1);
    label_bkg.adjust(1, 1, 1, 1);
    if(show_val)
    {
      p.setFont(aliased_fnt);
      //p.drawText(text_bkg, Qt::AlignRight | Qt::AlignVCenter, comp_val_text);
      p.drawText(val_bkg, Qt::AlignRight | Qt::AlignVCenter, comp_val_text);
      //_LCDPainter->drawText(&p, val_bkg, comp_val_text, Qt::AlignRight | Qt::AlignVCenter);
      p.setFont(fnt);
    }
    if(show_label)
      //p.drawText(text_bkg, Qt::AlignLeft | Qt::AlignVCenter, elided_label_text);
      p.drawText(label_bkg, Qt::AlignLeft | Qt::AlignVCenter, elided_label_text);
  }
  
  if((!shd && ((!on || spl) && !is_hov && !is_foc)))
  {
    if(spl)
      p.setClipRect(bkg_area);
    else
      // Restore the clipper to full size.
      p.setClipRect(geo);
    
    if(show_val)
    {
      p.setFont(aliased_fnt);
      //p.drawText(text_area, Qt::AlignRight | Qt::AlignVCenter, comp_val_text);
      p.drawText(val_area, Qt::AlignRight | Qt::AlignVCenter, comp_val_text);
      //_LCDPainter->drawText(&p, val_area, comp_val_text, Qt::AlignRight | Qt::AlignVCenter);
      p.setFont(fnt);
    }
    if(show_label)
      //p.drawText(text_area, Qt::AlignLeft | Qt::AlignVCenter, elided_label_text);
      p.drawText(label_area, Qt::AlignLeft | Qt::AlignVCenter, elided_label_text);
  }
  
  // Highlighted text:
  if(on || is_hov || is_foc)
  {
    p.setPen(Qt::white);
    if(spl && !is_hov && !is_foc)
      p.setClipRect(bar_area);
    else
      // Restore the clipper to full size.
      p.setClipRect(geo);

    if(show_val)
    {
      p.setFont(aliased_fnt);
      //p.drawText(text_area, Qt::AlignRight | Qt::AlignVCenter, comp_val_text);
      p.drawText(val_area, Qt::AlignRight | Qt::AlignVCenter, comp_val_text);
      //_LCDPainter->drawText(&p, val_area, comp_val_text, Qt::AlignRight | Qt::AlignVCenter);
      p.setFont(fnt);
    }
    if(show_label)
      //p.drawText(text_area, Qt::AlignLeft | Qt::AlignVCenter, elided_label_text);
      p.drawText(label_area, Qt::AlignLeft | Qt::AlignVCenter, elided_label_text);
  }
}

void CompactSlider::mouseMoveEvent(QMouseEvent *e)
{
  e->ignore();
  SliderBase::mouseMoveEvent(e);
  
  QPoint p = e->pos();
  
  const bool oldv = _mouseOverThumb;
  getMouseOverThumb(p);
  if(_autoHideThumb &&_mouseOverThumb != oldv)
    update();
}

void CompactSlider::mouseDoubleClickEvent(QMouseEvent* e)
{
  DEBUG_COMPACT_SLIDER(stderr, "CompactSlider::mouseDoubleClickEvent\n");
  const Qt::MouseButtons buttons = e->buttons();
  const Qt::KeyboardModifiers keys = e->modifiers();
  
  if(buttons == Qt::LeftButton && _mouseOverThumb && !_editMode)
  {  
    DEBUG_COMPACT_SLIDER(stderr, "   left button\n");
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

void CompactSlider::editorReturnPressed()
{
  DEBUG_COMPACT_SLIDER(stderr, "CompactSlider::editorReturnPressed\n");
  _editMode = false;
  if(_editor)
  {
    if(value() != _editor->value())
      setValue(_editor->value());
    _editor->deleteLater();
    _editor = 0;
    //setFocus();
    clearFocus();
  }
}

void CompactSlider::editorEscapePressed()
{
  DEBUG_COMPACT_SLIDER(stderr, "CompactSlider::editorEscapePressed\n");
  _editMode = false;
  if(_editor)
  {
    _editor->deleteLater();
    _editor = 0;
    //setFocus();
    clearFocus();
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

  e->ignore();
  SliderBase::keyPressEvent(e);
}

void CompactSlider::enterEvent(QEvent *e)
{
  _entered = true;
  if(!_hovered)
  {
    _hovered = true;
    if(_textHighlightMode & TextHighlightHover)
        update();
  }

  e->ignore();
  SliderBase::enterEvent(e);
}

void CompactSlider::leaveEvent(QEvent *e)
{
  _entered = false;
  if(!_pressed)
  {
    if(_hovered)
    {
      _hovered = false;
    }
    if(_textHighlightMode & TextHighlightHover)
        update();
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
      DEBUG_COMPACT_SLIDER(stderr, "CompactSlider::event NonClientAreaMouseButtonPress\n");
      e->accept();
      _editMode = false;
      if(_editor)
      {
        _editor->deleteLater();
        _editor = 0;
      }
      return true;
    break;
    
//     case QEvent::PaletteChange:
//       DEBUG_COMPACT_SLIDER(stderr, "CompactSlider::event PaletteChange\n");
//       updatePixmaps();
//     break;
//     
//     case QEvent::EnabledChange:
//       DEBUG_COMPACT_SLIDER(stderr, "CompactSlider::event EnabledChange\n");
//       updatePixmaps();
//     break;
    
//     case QEvent::ActivationChange:
//     case QEvent::WindowActivate:
//     case QEvent::WindowDeactivate:
//       DEBUG_COMPACT_SLIDER(stderr, "CompactSlider::event ActivationChange, WindowActivate, or WindowDeactivate\n");
//       updatePixmaps();
//     break;
    
    default:
    break;
  }
  
  e->ignore();
  return SliderBase::event(e);
}


// void CompactSlider::updatePixmaps()
// {
//   getActiveArea(); 
//   getPixelValues();
//   
//   int x = d_sliderRect.x();
//   int y = d_sliderRect.y();
//   int w = d_sliderRect.width();
//   int h = d_sliderRect.height();
// 
//   _onPixmap  = QPixmap(w, h);
//   _offPixmap = QPixmap(w, h);
// 
//   const int y1 = y;
//   const int y2 = y1 + h - 1;
// 
// 
//   const QPalette& pal = palette();
//   
//   QLinearGradient linearGrad_a(x, y1, x, y2);
//   
//   
//   const QColor c2 = isEnabled() ? (d_barColor.isValid() ? d_barColor : pal.color(QPalette::Active, QPalette::Highlight)) : 
//                     pal.color(QPalette::Disabled, QPalette::Highlight);
//   const QColor c1 = c2.darker(125);
//   
//   linearGrad_a.setColorAt(0, c1);
//   linearGrad_a.setColorAt(0.5, c2);
//   linearGrad_a.setColorAt(1, c1);
//   
//   if(w > 0 && h > 0)
//   {
//     QPainter p;
//     p.begin(&_onPixmap);
//     p.fillRect(0, 0, w, h, linearGrad_a);
//     p.end();
//     p.begin(&_offPixmap);
//       p.fillRect(0, 0, w, h, pal.window());
//     p.end();
//   }
// 
//   updatePainterPaths();
// }

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
    
  //updatePixmaps();
  getActiveArea(); 
  getPixelValues();

  if(_editor && _editor->isVisible())
    _editor->setGeometry(rect());
}

void CompactSlider::showEditor()
{
  if(_editMode)
    return;
  
  if(!_editor)
  {
    DEBUG_COMPACT_SLIDER(stderr, "   creating editor\n");
    _editor = new PopupDoubleSpinBox(this);
    _editor->setFrame(false);
    _editor->setFocusPolicy(Qt::WheelFocus);
    connect(_editor, SIGNAL(returnPressed()), SLOT(editorReturnPressed()));
    connect(_editor, SIGNAL(escapePressed()), SLOT(editorEscapePressed()));
  }
  int w = width();
  //if (w < _editor->sizeHint().width())
  //  w = _editor->sizeHint().width();
  _editor->setGeometry(0, 0, w, height());
  DEBUG_COMPACT_SLIDER(stderr, "   x:%d y:%d w:%d h:%d\n", _editor->x(), _editor->y(), w, _editor->height());
  _editor->setDecimals(_valueDecimals);
  _editor->setSingleStep(step());
  _editor->setPrefix(valPrefix());
  _editor->setSuffix(valSuffix());
  _editor->setMinimum(minValue());
  _editor->setMaximum(maxValue());
  _editor->setValue(value());
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
//   getScrollMode(p, Qt::LeftButton, Qt::KeyboardModifiers(), scrollMode, direction);
  getScrollMode(p, Qt::NoButton,  Qt::KeyboardModifiers(), scrollMode, direction);
  const bool v = scrollMode == ScrMouse;
  //if(_mouseOverThumb != v && !(left_btn && !v))
//   if(_mouseOverThumb != v && !(bPressed && !v))
  if(_mouseOverThumb != v && !(_pressed && !v))
  {
    DEBUG_COMPACT_SLIDER(stderr, "CompactSlider::getMouseOverThumb setting _mouseOverThumb:%d to v:%d\n", _mouseOverThumb, v);
    _mouseOverThumb = v;
  }
  const bool hv = rect().contains(p);
//   if(_hovered != hv && !bPressed)
  if(_hovered != hv && !_pressed)
    _hovered = hv;
}      

void CompactSlider::setShowValue(bool show)
{
  _showValue = show;
  resize(size());
  updateGeometry(); // Required.
  update();
}

void CompactSlider::setOff(bool v)
{ 
  if(v && !_hasOffMode)
    _hasOffMode = true;
  if(_off == v)
    return;
  _off = v; 
  update(); 
  emit valueStateChanged(value(), isOff(), id(), d_scrollMode); 
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
    emit valueStateChanged(value(), isOff(), id(), d_scrollMode); 
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

      //updatePainterPaths();

      QPoint p = mapFromGlobal(QCursor::pos());

      getMouseOverThumb(p);
      update();
      
      // HACK
      // In direct mode let the inherited classes (this) call these in their valueChange() methods, 
      //  so that they may be called BEFORE valueChanged signal is emitted by the setPosition() call above.
      // ScrDirect mode only happens once upon press with a modifier. After that, another mode is set.
      // Hack: Since valueChange() is NOT called if nothing changed, in that case these are called for us by the SliderBase.
      if(d_scrollMode == ScrDirect)
      {
        processSliderPressed(id());
        emit sliderPressed(id());
      }
      
      // Emits valueChanged if tracking enabled.
      SliderBase::valueChange();
      // Emit our own combined signal.
      if(trackingIsActive())
        emit valueStateChanged(value(), isOff(), id(), d_scrollMode);
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
//     repaint();
    update();
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
      return getMinimumSizeHint(fontMetrics(), d_orient, d_scalePos, d_xMargin, d_yMargin);
      }

//---------------------------------------------------------
//   setOrientation
//---------------------------------------------------------

void CompactSlider::setOrientation(Qt::Orientation o)
      {
      d_orient = o;
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

// void CompactSlider::updatePainterPaths()
// {
//   if(_offPath)
//     delete _offPath;
//   if(_onPath)
//     delete _onPath;
//   
//   _offPath = new QPainterPath;
//   _onPath = new QPainterPath;
// 
//   int x = d_sliderRect.x();
//   int y = d_sliderRect.y();
//   int w = d_sliderRect.width();
//   int h = d_sliderRect.height();
// 
//   const int x1 = x;
//   const int w1 = d_valuePixelWidth;
// 
//   const int x2 = x + d_valuePixelWidth;
//   const int w2 = w - d_valuePixelWidth;
//   
//   MusECore::addRoundedPath(_onPath, QRect (x1, y, w1, h), 4, 4, 
//     //(MusECore::Corner) (MusECore::UpperLeft | MusECore::UpperRight | MusECore::LowerLeft | MusECore::LowerRight) );
//     //(MusECore::Corner) (MusECore::UpperLeft | MusECore::LowerLeft) );
//     (MusECore::Corner) (MusECore::UpperRight | MusECore::LowerRight) );
//   
//   MusECore::addRoundedPath(_offPath, QRect (x2, y, w2, h), 4, 4, 
//     //(MusECore::Corner) (MusECore::UpperLeft | MusECore::UpperRight | MusECore::LowerLeft | MusECore::LowerRight) );
//     (MusECore::Corner) (MusECore::UpperRight | MusECore::LowerRight) );
// }
      
} // namespace MusEGui
