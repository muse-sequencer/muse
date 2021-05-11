//=========================================================
//  MusE
//  Linux Music Editor
//
//  compact_knob.cpp
//  Adapted from Qwt Lib:
//  Copyright (C) 1997  Josef Wilgen
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Orcan Ogetbil (ogetbilo at sf.net) completely redesigned.
//  (C) Copyright 2016 - 2017 Tim E. Real (terminator356 on sourceforge). New CompactKnob based on Knob.
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

#include "background_painter.h"
#include "popup_double_spinbox.h"
#include "compact_knob.h"

//#include <stdio.h>
#include "muse_math.h"
#include "mmath.h"
#include "gconfig.h"

#include <QPalette>
#include <QLinearGradient>
//#include <QLocale>
#include <QFlags>
#include <QToolTip>
#include <QString>
#include <QFontMetrics>
//#include <QResizeEvent>
#include <QPainter>
#include <QEvent>
//#include <QPaintEvent>
//#include <QMouseEvent>
#include <QKeyEvent>

// For debugging output: Uncomment the fprintf section.
#define DEBUG_KNOB(dev, format, args...)  //fprintf(dev, format, ##args);


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
//   CompactKnob
//---------------------------------------------------------

CompactKnob::CompactKnob(QWidget* parent, const char* name,
               KnobLabelPos labelPos,
               const QString& labelText,
               const QString& valPrefix,
               const QString& valSuffix,
               const QString& specialValueText,
               const QColor& faceColor)
   : SliderBase(parent, name)
      {
      if(objectName().isEmpty())
        setObjectName(QStringLiteral("CompactKnob"));

      setMouseTracking(true);
      setEnabled(true);
      setFocusPolicy(Qt::WheelFocus);

      //setAutoFillBackground(false);
      //setAttribute(Qt::WA_NoSystemBackground);
      //setAttribute(Qt::WA_StaticContents);
      // This is absolutely required for speed! Otherwise painfully slow because of full background
      //  filling, even when requesting small udpdates! Background is drawn by us.
      //setAttribute(Qt::WA_OpaquePaintEvent);

      setBorderlessMouse(false);
      setCursorHoming(false);
      //setPagingButtons(Qt::NoButton);

      setEnableValueToolTips(true);
      setShowValueToolTipsOnHover(true);

      _bkgPainter = new ItemBackgroundPainter(this);

      _hovered = false;
      _labelHovered = false;
      _knobHovered = false;

      _editor = nullptr;
      _editMode = false;

      hasScale = false;

      d_xMargin       = 1;
      d_yMargin       = 1;
      d_borderWidth   = 4;
      d_shineWidth    = 1;
      d_totalAngle    = 270.0;
      d_scaleDist     = 1;
      d_symbol        = Line;
      d_dotWidth      = 4;
      d_maxScaleTicks = 11;
      d_knobWidth     = 30; // unused?
      _faceColSel     = false;
      //d_faceColor     = palette().color(QPalette::Window);
      //d_rimColor      = rimColor;
      //if(!d_rimColor.isValid())
      //  d_rimColor      = palette().mid().color();
      d_faceColor     = faceColor;
      if(!d_faceColor.isValid())
        d_faceColor     = palette().color(QPalette::Window);
      d_shinyColor    = palette().mid().color();
      d_curFaceColor  = d_faceColor;
      d_altFaceColor  = d_faceColor;
      d_markerColor   = palette().dark().color().darker(125);

      l_slope = 0;
      l_const = 100;

      d_labelPos = labelPos;
      d_labelText = labelText;
      d_valPrefix = valPrefix;
      d_valSuffix = valSuffix;
      d_specialValueText = specialValueText;

      _hasOffMode = false;
      _valueDecimals = 2;
      _off = false;
      d_offText = tr("off");
      _showLabel = true;
      _showValue = true;

      _style3d = true;
      _radius = 2;
      _drawChord = false;

      setUpdateTime(50);
      }

// Static.
QSize CompactKnob::getMinimumSizeHint(const QFontMetrics& fm,
                                        KnobLabelPos labelPos,
                                        bool showValue,
                                        bool showLabel,
                                        int xMargin,
                                        int yMargin)
{
  // Put some kind of limit so the knobs aren't tiny at small fonts.
  const int minh = 17;
  int label_h, label_h_2, fin_label_h;
  label_h = (fm.height() - fm.leading() - fm.descent()) + 1;
  label_h_2 = 2 * label_h - 1;

  if(showValue && showLabel)
    fin_label_h = label_h_2;
  else
    fin_label_h = fm.height() + 5;

  switch(labelPos) {
        case Left:
              return QSize(label_h_2 + 2 * xMargin, label_h_2 + 2 * yMargin);
              break;
        case Right:
              return QSize(label_h_2 + 2 * xMargin, label_h_2 + 2 * yMargin);
              break;
        case Top:
              return QSize(label_h_2 + 2 * xMargin, label_h_2 + fin_label_h + 2 * yMargin);
              break;
        case Bottom:
              return QSize(label_h_2 + 2 * xMargin, label_h_2 + fin_label_h + 2 * yMargin);
              break;
        case None:
              break;
        }
  return QSize(minh + 2 * xMargin, minh + 2 * yMargin);
}

void CompactKnob::processSliderPressed(int)
{
//    bPressed = true;
   update();
}

void CompactKnob::processSliderReleased(int)
{
  update();

  DEBUG_KNOB(stderr,
    "CompactKnob::processSliderReleased trackingIsActive:%d val:%.20f valHasChanged:%d\n",
    trackingIsActive(), value(), valueHasChangedAtRelease());

// Changed. BUG: Was causing problems with sending changed even though it hadn't.
// It should never signal a change on release UNLESS tracking is off, because otherwise the last movement
//  already sent the last changed signal. FIXME Even this is still flawed. If no tracking, it would likely
//  still signal a change upon simple press and release even though nothing changed.
//   if((!tracking()) || valHasChanged())
  if(!trackingIsActive() && valueHasChangedAtRelease())
    emit valueStateChanged(value(), isOff(), id(), d_scrollMode);
}

QString CompactKnob::toolTipValueText(bool inclLabel, bool inclVal) const
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

void CompactKnob::showValueToolTip(QPoint /*p*/)
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
    QToolTip::showText(mapToGlobal(parentWidget() ? parentWidget()->pos() : pos()), txt, 0, QRect(), 3000);
    //QToolTip::showText(mapToGlobal(parentWidget() ? parentWidget()->pos() : pos()), txt);
  }
}

//------------------------------------------------------------
//  CompactKnob::setTotalAngle
//  Set the total angle by which the knob can be turned
//
//  Syntax
//  void CompactKnob::setTotalAngle(double angle)
//
//  Parameters
//  double angle  --  angle in degrees.
//
//  Description
//  The default angle is 270 degrees. It is possible to specify
//  an angle of more than 360 degrees so that the knob can be
//  turned several times around its axis.
//------------------------------------------------------------

void CompactKnob::setTotalAngle (double angle)
      {
      if (angle < 10.0)
            d_totalAngle = 10.0;
      else
            d_totalAngle = angle;
      d_scale.setAngleRange( -0.5 * d_totalAngle, 0.5 * d_totalAngle);
      }

//------------------------------------------------------------
// CompactKnob::setRange
// Set the range and step size of the knob
//
// Sets the parameters that define the shininess of the ring
// surrounding the knob and then proceeds by passing the
// parameters to the parent class' setRange() function.
//------------------------------------------------------------

void CompactKnob::setRange(double vmin, double vmax, double vstep, int pagesize, DoubleRange::ConversionMode mode)
      {
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
      SliderBase::setRange(vmin, vmax, vstep, pagesize, mode);
      }

void CompactKnob::setShowValue(bool show)
{
  _showValue = show;
  resize(size());
  updateGeometry(); // Required.
  update();
}

void CompactKnob::setShowLabel(bool show)
{
  _showLabel = show;
  resize(size());
  updateGeometry(); // Required.
  update();
}

void CompactKnob::setOff(bool v)
{
  if(v && !_hasOffMode)
    _hasOffMode = true;
  if(_off == v)
    return;
  _off = v;
  update();
  emit valueStateChanged(value(), isOff(), id(), d_scrollMode);
}

void CompactKnob::setHasOffMode(bool v)
{
  _hasOffMode = v;
  setOff(false);
}

void CompactKnob::setValueState(double v, bool off, ConversionMode mode)
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
  if(value(mode) != v)
  {
    do_val_upd = true;
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
//.F  CompactKnob::valueChange
//  Notify change of value
//
//.u  Parameters
//  double x  --    new value
//
//.u  Description
//  Sets the slider's value to the nearest multiple
//          of the step size.
//------------------------------------------------------------

void CompactKnob::valueChange()
      {
      // Turn the control back on with any value set.
      // Wanted to make this flaggable, but actually we
      //  have to in order to see any value changing,
      if(isOff())
        setOff(false);

      recalcAngle();
      d_newVal++;

      update(_knobRect);
      if(_showValue)
        update(_labelRect);

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
      // Emit our own combined signal.
      if(trackingIsActive())
        emit valueStateChanged(value(), isOff(), id(), d_scrollMode);
      }

//------------------------------------------------------------
//.F  CompactKnob::getValue
//  Determine the value corresponding to a specified position
//
//.u  Parameters:
//  const QPoint &p -- point
//
//.u  Description:
//  Called by QwtSliderBase
//------------------------------------------------------------

double CompactKnob::getValue(const QPoint &p)
      {
      double newValue;
      double oneTurn;
      double eqValue;
      double arc;

    double dx = double((_knobRect.x() + _knobRect.width() / 2) - p.x() );
    double dy = double((_knobRect.y() + _knobRect.height() / 2) - p.y() );

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
//
//.F  CompactKnob::moveValue
//  Determine the value corresponding to a specified mouse movement.
//
//.u  Syntax
//.f  void CompactKnob::moveValue(const QPoint &deltaP, bool fineMode)
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
double CompactKnob::moveValue(const QPoint &deltaP, bool /*fineMode*/)
{
    // FIXME: To make fine mode workable, we need a way to make the adjustments 'multi-turn'.

    double oneTurn;
    double eqValue;

    const QPoint new_p = _lastMousePos + deltaP;

    const int cx = _knobRect.x() + _knobRect.width() / 2;
    const int cy = _knobRect.y() + _knobRect.height() / 2;

    const double last_dx = double(cx - _lastMousePos.x());
    const double last_dy = double(cy - _lastMousePos.y());
    const double last_arc = atan2(-last_dx, last_dy) * 180.0 / M_PI;

    const double dx = double(cx - new_p.x());
    const double dy = double(cy - new_p.y());
    const double arc = atan2(-dx, dy) * 180.0 / M_PI;

    const double val = value(ConvertNone);

//     if((fineMode || borderlessMouse()) && d_scrollMode != ScrDirect)
//     {
//       const double arc_diff = arc - last_arc;
//       const double dval_diff =  arc_diff * step();
//       const double new_val = val + dval_diff;
//       d_valAccum = new_val; // Reset.
//       return d_valAccum;
//     }

    const double min = minValue(ConvertNone);
    const double max = maxValue(ConvertNone);
    const double drange = max - min;

    const double last_val =  0.5 * (min + max) + (last_arc + d_nTurns * 360.0) * drange / d_totalAngle;
    const double new_val  =  0.5 * (min + max) + (arc + d_nTurns * 360.0) * drange / d_totalAngle;
    double dval_diff =  new_val - last_val;

    //if(fineMode)
    //  dval_diff /= 10.0;

    d_valAccum += dval_diff;

    DEBUG_KNOB(stderr, "CompactKnob::moveValue value:%.20f last_val:%.20f new_val:%.20f p dx:%d dy:%d drange:%.20f step:%.20f dval_diff:%.20f d_valAccum:%.20f\n",
                     val, last_val, new_val, deltaP.x(), deltaP.y(), drange, step(), dval_diff, d_valAccum);


    oneTurn = fabs(drange) * 360.0 / d_totalAngle;
    eqValue = val + d_mouseOffset;

    DEBUG_KNOB(stderr, "   oneTurn:%.20f eqValue:%.20f\n", oneTurn, eqValue);
    if(fabs(d_valAccum - eqValue) > 0.5 * oneTurn)
    {
      if (d_valAccum < eqValue)
      {
        d_valAccum += oneTurn;
        DEBUG_KNOB(stderr, "   added one turn, new d_valAccum:%.20f\n", d_valAccum);
      }
      else
      {
        d_valAccum -= oneTurn;
        DEBUG_KNOB(stderr, "   subtracted one turn, new d_valAccum:%.20f\n", d_valAccum);
      }
    }

    return d_valAccum;
}


//------------------------------------------------------------
//.-
//.F  CompactKnob::setScrollMode
//  Determine the scrolling mode and direction
//  corresponding to a specified position
//
//.u  Parameters
//  const QPoint &p -- point in question
//
//.u  Description
//  Called by QwtSliderBase
//------------------------------------------------------------
void CompactKnob::getScrollMode( QPoint &p, const Qt::MouseButton &button, const Qt::KeyboardModifiers& modifiers, int &scrollMode, int &direction)
{
  if(!_knobRect.contains(p))
  {
    scrollMode = ScrNone;
    direction = 0;
    return;
  }
  // If modifier or button is held, jump directly to the position at first.
  // After handling it, the caller can change to SrcMouse scroll mode.
  else if(modifiers & Qt::ControlModifier || button == Qt::MiddleButton)
  {
    scrollMode = ScrDirect;
    direction = 0;
    return;
  }

  int dx, dy, r;
  double arc;

  r = _knobRect.width() / 2;

  dx = _knobRect.x() + r - p.x();
  dy = _knobRect.y() + r - p.y();

  if((dx * dx) + (dy * dy) <= (r * r)) // point is inside the knob
  {
    scrollMode = ScrMouse;
    direction = 0;
  }
  else                // point lies outside
  {
    scrollMode = ScrTimer;
    arc = atan2(double(-dx), double(dy)) * 180.0 / M_PI;
    if ( arc < d_angle)
      direction = -1;
    else if (arc > d_angle)
      direction = 1;
    else
      direction = 0;
  }
}



//------------------------------------------------------------
//.F  CompactKnob::rangeChange
//  Notify a change of the range
//
//.u  Description
//  Called by QwtSliderBase
//------------------------------------------------------------

void CompactKnob::rangeChange()
{
    if (!hasUserScale())
      d_scale.setScale(minValue(), maxValue(), d_maxMajor, d_maxMinor);
    recalcAngle();
    SliderBase::rangeChange();
//     resize(size());

//     repaint();
    update();
}

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void CompactKnob::resizeEvent(QResizeEvent* ev)
      {
      SliderBase::resizeEvent(ev);

      const QRect& r = rect();

      if(_editor && _editor->isVisible())
        _editor->setGeometry(r);

      const int spacing = 0;
      int x, y, sz = 1;

      QFontMetrics fm = fontMetrics();

      int label_h, label_h_2, fin_label_h;
      label_h = (fm.height() - fm.leading() - fm.descent()) + 1;
      label_h_2 = 2 * label_h - 1;

      if(_showValue && _showLabel)
        fin_label_h = label_h_2;
      else
        fin_label_h = fm.height() + 5;

      switch(d_labelPos) {
        case Left:
              sz = r.height();
              x = r.x() + r.width() - sz;
              y = r.y();
              _knobRect.setRect(x, y, sz, sz);
              _labelRect.setRect(r.x(), r.y(), r.width() - sz - spacing, sz);
              break;
        case Right:
              sz = r.height();
              x = r.x();
              y = r.y();
              _knobRect.setRect(x, y, label_h_2 + 2 * d_xMargin, label_h_2 + 2 * d_yMargin);
              _labelRect.setRect(r.x() + label_h_2 + 2 * d_xMargin + spacing,
                                 r.y(),
                                 r.width() - label_h_2 - 2 * d_xMargin - spacing,
                                 label_h_2 + 2 * d_yMargin);
              break;
        case Top:  // TODO
              sz = r.width();
              x = r.x();
              y = r.y() + r.height() - sz;
              _knobRect.setRect(x, y, sz, sz);
              _labelRect.setRect(r.x(), r.y(), sz, r.height() - sz - spacing);
              break;
        case Bottom:
              sz = r.height();
              x = r.x();
              y = r.y();
              _knobRect.setRect(r.width() / 2 - label_h_2 / 2,
                                y,
                                label_h_2 + 2 * d_xMargin,
                                label_h_2 + 2 * d_yMargin);
              _labelRect.setRect(x,
                                 y + label_h_2 + 2 * d_yMargin + spacing,
                                 r.width(),
                                 fin_label_h + 2 * spacing);
              break;
        case None:
              sz = MusECore::qwtMin(r.height(), r.width());
              x = r.x();
              y = r.y();
              _knobRect.setRect(x, y, sz, sz);
              break;
        }

      x = _knobRect.x() - d_scaleDist;
      y = _knobRect.y() - d_scaleDist;
      int w = sz + 2 * d_scaleDist;

      d_scale.setGeometry(x, y, w, ScaleDraw::Round);
      }

void CompactKnob::showEditor()
{
  if(_editMode)
    return;

  if(!_editor)
  {
    DEBUG_KNOB(stderr, "   creating editor\n");
    _editor = new PopupDoubleSpinBox(this);
    _editor->setFrame(false);
    _editor->setContentsMargins(0, 0, 0, 0);
    _editor->setFocusPolicy(Qt::WheelFocus);
    connect(_editor, SIGNAL(returnPressed()), SLOT(editorReturnPressed()));
    connect(_editor, SIGNAL(escapePressed()), SLOT(editorEscapePressed()));
  }
  int w = width();
  //int w = _labelRect.width();
  //if (w < _editor->sizeHint().width())
  //  w = _editor->sizeHint().width();
  //_editor->setGeometry(0, 0, w, height());
  _editor->setGeometry(0, _labelRect.y(), w, _labelRect.height());
  DEBUG_KNOB(stderr, "   x:%d y:%d w:%d h:%d\n", _editor->x(), _editor->y(), w, _editor->height());
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

//------------------------------------------------------------
//    paintEvent
//------------------------------------------------------------

void CompactKnob::paintEvent(QPaintEvent*)
      {
      QPainter p(this);

      drawBackground(&p);

      p.setRenderHint(QPainter::Antialiasing, true);
      if(hasScale)
        d_scale.draw(&p, palette());
      drawKnob(&p, _knobRect);

      if(d_labelPos != None)
        drawLabel(&p);

      d_newVal = 0;
      }

//------------------------------------------------------------
//   CompactKnob::drawBackground
//------------------------------------------------------------

void CompactKnob::drawBackground(QPainter* painter)
{
  switch(d_labelPos)
  {
    case None:
    case Left:
    case Right:
      // Paint a background for the whole control.
      _bkgPainter->drawBackground(painter,
                                  rect(),
                                  palette(),
                                  d_xMargin,
                                  d_yMargin,
                                  hasOffMode() && ! isOff() ? _labelRect : QRect(),
                                  _radius, _style3d,
                                  _style3d ? QColor() : MusEGlobal::config.sliderBackgroundColor,
                                  _style3d ? QColor() : MusEGlobal::config.sliderBackgroundColor,
                                  _style3d ? QColor() : MusEGlobal::config.sliderBackgroundColor);
    break;

    case Top:
    case Bottom:
    {
      // Paint a separate background for the knob.
      // No, let's not paint a background for the knob. But if you want it, here it is:
/*
      //QRect kr(rect().x(), _knobRect.y(), rect().width(), _knobRect.height());
      QRect kr(_knobRect.x() - 2, _knobRect.y(), _knobRect.width() + 4, _knobRect.height());
      _bkgPainter->drawBackground(painter,
                                  kr,
                                  palette(),
                                  d_xMargin,
                                  d_yMargin);
*/

      // Paint a separate background for the label.
      _bkgPainter->drawBackground(painter,
                                  _labelRect,
                                  palette(),
                                  d_xMargin,
                                  d_yMargin,
                                  hasOffMode() && ! isOff() ? _labelRect : QRect(),
                                  2);
    }
    break;
  }
}

//------------------------------------------------------------
//   CompactKnob::drawKnob
//    const QRect &r --   borders of the knob
//------------------------------------------------------------

void CompactKnob::drawKnob(QPainter* p, const QRect& r)
{
    const QPalette& pal = palette();

    QRect aRect;
    aRect.setRect(r.x() + d_borderWidth,
                  r.y() + d_borderWidth,
                  r.width()  - 2*d_borderWidth,
                  r.height() - 2*d_borderWidth);

    int width = r.width() - 2 * d_xMargin;
    int height = r.height() - 2 * d_yMargin;
    int size = qMin(width, height);

    p->setRenderHint(QPainter::Antialiasing, true);


    if (_style3d) {
        //
        // draw the rim
        //
        QLinearGradient linearg(QPoint(r.x() + d_xMargin,r.y() + d_yMargin), QPoint(size, size));
        linearg.setColorAt(1 - M_PI_4, d_faceColor.lighter(125));
        linearg.setColorAt(M_PI_4, d_faceColor.darker(175));
        p->setBrush(linearg);
        p->setPen(Qt::NoPen);
        p->drawEllipse(r.x() + d_xMargin,r.y() + d_yMargin ,size, size);

        //
        // draw shiny surrounding
        //
        QPen pn;
        pn.setCapStyle(Qt::FlatCap);

        pn.setColor(d_shinyColor.lighter(l_const + fabs(value() * l_slope)));
        pn.setWidth(d_shineWidth * 2);
        p->setPen(pn);
        p->drawArc(aRect, 0, 360 * 16);

        //
        // draw button face
        //
        QRadialGradient gradient(//aRect.x() + size/2,
                                 //aRect.y() + size/2,
                                 aRect.x(),
                                 aRect.y(),
                                 size-d_borderWidth,
                                 aRect.x() + size/2-d_borderWidth,
                                 aRect.y() + size/2-d_borderWidth);
        gradient.setColorAt(0, d_curFaceColor.lighter(150));
        gradient.setColorAt(1, d_curFaceColor.darker(150));
        p->setBrush(gradient);

        p->setPen(Qt::NoPen);
        p->drawEllipse(aRect);

        //
        // draw marker
        //
        if (_style3d)
            drawMarker(p, d_angle, pal.currentColorGroup() == QPalette::Disabled ?
                           pal.color(QPalette::Disabled, QPalette::WindowText) : d_markerColor);
    } else {

        QPen pen(d_faceColor);
        pen.setWidth(3);
        p->setPen(pen);
        p->setBrush(MusEGlobal::config.sliderBackgroundColor);
        p->drawEllipse(r.x() + d_xMargin, r.y() + d_yMargin, size, size);

        if (_drawChord) {
            const int angle = static_cast<int>(d_totalAngle);
            const int degsubdiv = 16;
            p->setBrush(d_faceColor);
            p->drawChord(r.x() + d_xMargin, r.y() + d_yMargin, size, size,
                         (angle / 2 + 90) * degsubdiv, (360 - angle) * degsubdiv);
        }

        drawMarker(p, d_angle, d_faceColor);
    }
}

//------------------------------------------------------------
//.-
//.F  CompactKnob::drawMarker
//  Draw the marker at the knob's front
//
//.u  Parameters
//.p  QPainter *p --  painter
//  double arc  --  angle of the marker
//  const QColor &c  -- marker color
//
//.u  Syntax
//        void CompactKnob::drawMarker(QPainter *p)
//
//------------------------------------------------------------
void CompactKnob::drawMarker(QPainter *p, double arc, const QColor &c)
{
  QPen pn;
  int radius;
  double rb,re;
  double rarc;

  rarc = arc * M_PI / 180.0;
  double ca = cos(rarc);
  double sa = - sin(rarc);

  radius = _knobRect.width() / 2 - d_borderWidth + d_shineWidth;
  if (radius < 3) radius = 3;
  int ym = _knobRect.y() + radius + d_borderWidth - d_shineWidth;
  int xm = _knobRect.x() + radius + d_borderWidth - d_shineWidth;

  switch (d_symbol)
  {
    case Dot:
  {
      qreal dothalf = d_dotWidth / 2.0;
      p->setBrush(c);
      p->setPen(Qt::NoPen);
      rb = qMax(radius - 4.0 - dothalf, 0.0);
      p->drawEllipse(QPointF(xm - (sa * rb), ym - (ca * rb)), dothalf, dothalf);
  }
    break;

    case Line:
      pn.setColor(c);
      pn.setWidth(2);
      p->setPen(pn);

//       rb = MusECore::qwtMax(double((radius - 1) / 3.0), 0.0);
//       re = MusECore::qwtMax(double(radius - 1), 0.0);
//      rb = MusECore::qwtMax(((double(radius) - 0.5) / 3.0), 0.0);
      re = MusECore::qwtMax(double(radius) - 0.5, 0.0);

      p->setRenderHint(QPainter::Antialiasing, true);
      p->drawLine( xm,
                  ym,
                  xm - int(rint(sa * re)),
                  ym - int(rint(ca * re)));

//      if (!_style3d)
//          p->drawEllipse(QPoint(xm, ym), 1, 1);

    break;
  }
}

//------------------------------------------------------------
//   CompactKnob::drawLabel
//    const QRect &r --   borders of the knob
//------------------------------------------------------------

void CompactKnob::drawLabel(QPainter* painter)
{
  painter->save();
  painter->setRenderHint(QPainter::Antialiasing);

  const bool has_focus = hasFocus();

  if (_style3d) {
      if (has_focus)
      {
          if (_hovered)
              painter->setPen(QPen(QColor(239,239,239)));
          else
              painter->setPen(QPen(Qt::white));
      }
      else if (_hovered)
          painter->setPen(QPen(QColor(48,48,48)));
      else
          painter->setPen(QPen(Qt::black));

  } else {
      if (_hovered)
          painter->setPen(MusEGlobal::config.knobFontColor.lighter());
      else if (hasOffMode() && isOff())
          painter->setPen(MusEGlobal::config.knobFontColor.darker());
      else
          painter->setPen(MusEGlobal::config.knobFontColor);
  }

  int label_flags = 0;
  int value_flags = 0;
  switch(d_labelPos)
  {
    case Left:
      label_flags = Qt::AlignRight | ((_showValue && _showLabel) ?  Qt::AlignTop : Qt::AlignVCenter);
      value_flags = Qt::AlignRight | ((_showValue && _showLabel) ?  Qt::AlignBottom : Qt::AlignVCenter);
    break;

    case Right:
      label_flags = Qt::AlignLeft | ((_showValue && _showLabel) ?  Qt::AlignTop : Qt::AlignVCenter);
      value_flags = Qt::AlignLeft | ((_showValue && _showLabel) ?  Qt::AlignBottom : Qt::AlignVCenter);
    break;

    case Top:
      label_flags = Qt::AlignLeft | ((_showValue && _showLabel) ?  Qt::AlignTop : Qt::AlignVCenter);
      value_flags = Qt::AlignLeft | ((_showValue && _showLabel) ?  Qt::AlignBottom : Qt::AlignVCenter);
    break;

    case Bottom:
      label_flags = Qt::AlignLeft | ((_showValue && _showLabel) ?  Qt::AlignTop : Qt::AlignVCenter);
      value_flags = Qt::AlignLeft | ((_showValue && _showLabel) ?  Qt::AlignBottom : Qt::AlignVCenter);
    break;

    case None:
      return;
    break;
  }

  const QFontMetrics fm = fontMetrics();
  const int leading = fm.leading();
  const int descent = fm.descent();

  const QRect label_br = fm.boundingRect(d_labelText);
  const int label_bw = label_br.width();

  if(_showLabel)
  {
    QRect label_r = _labelRect.adjusted(3, -descent + d_yMargin + 1, -2, 0);

    int label_xoff = (label_r.width() - label_bw) / 2;
    if(label_xoff < 0)
      label_xoff = 0;

    label_r.adjust(label_xoff, 0, 0, 0);

    painter->drawText(label_r, label_flags, d_labelText);
  }

  if(_showValue)
  {
    const double minV = minValue(ConvertNone);
    const double val = value(ConvertNone);
    const QString val_txt = (val <= minV && !d_specialValueText.isEmpty()) ?
                                d_specialValueText : (d_valPrefix + locale().toString(val, 'f', _valueDecimals) + d_valSuffix);
    const QRect val_br = fm.boundingRect(val_txt);
    const int val_bw = val_br.width();
    QRect val_r = _labelRect.adjusted(3, -1, -2, descent + leading - d_yMargin - 2);
    int val_xoff = (val_r.width() - val_bw) / 2;
    if(val_xoff < 0)
      val_xoff = 0;
    val_r.adjust(val_xoff, 0, 0, 0);
    painter->drawText(val_r, value_flags, val_txt);
  }

  painter->restore();
}

void CompactKnob::mouseDoubleClickEvent(QMouseEvent* e)
{
  DEBUG_KNOB(stderr, "CompactKnob::mouseDoubleClickEvent\n");
  const Qt::MouseButtons buttons = e->buttons();
  const Qt::KeyboardModifiers keys = e->modifiers();

  if(buttons == Qt::LeftButton && !_editMode)
  {
    DEBUG_KNOB(stderr, "   left button\n");
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
  }

  e->ignore();
  SliderBase::mouseDoubleClickEvent(e);
}

void CompactKnob::editorReturnPressed()
{
  DEBUG_KNOB(stderr, "CompactKnob::editorReturnPressed\n");
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

void CompactKnob::editorEscapePressed()
{
  DEBUG_KNOB(stderr, "CompactKnob::editorEscapePressed\n");
  _editMode = false;
  if(_editor)
  {
    _editor->deleteLater();
    _editor = 0;
    setFocus();
  }
}

void CompactKnob::keyPressEvent(QKeyEvent* e)
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

void CompactKnob::leaveEvent(QEvent *e)
{
  if(_hovered)
  {
    _hovered = false;
    update();
  }
  if(_knobHovered)
  {
    _knobHovered = false;
    update(_knobRect);
  }
  if(_labelHovered)
  {
    _labelHovered = false;
    update(_labelRect);
  }
  e->ignore();
  SliderBase::leaveEvent(e);
}

void CompactKnob::mouseMoveEvent(QMouseEvent *e)
{
  e->ignore();
  SliderBase::mouseMoveEvent(e);
  if(!_hovered)
  {
    _hovered = true;
    update();
  }
  if(_knobRect.contains(e->pos()) != _knobHovered)
  {
    _knobHovered = !_knobHovered;
    update(_knobRect);
  }
  if(_labelRect.contains(e->pos()) != _labelHovered)
  {
    _labelHovered = !_labelHovered;
    update(_labelRect);
  }
}

void CompactKnob::mousePressEvent(QMouseEvent* e)
{
  const Qt::MouseButton button = e->button();
  const Qt::MouseButtons buttons = e->buttons();

  // Only one mouse button at a time! Otherwise bad things happen.
  if(buttons ^ button)
  {
    e->ignore();
    // Let ancestor handle the proper thing to do.
    SliderBase::mousePressEvent(e);
    return;
  }

  if(button == Qt::RightButton)
  {
    e->accept();
    // Clear everything.
    setMouseGrab(false);
    d_scrollMode = ScrNone;
    d_direction = 0;
    _pressed = false;
    emit sliderRightClicked(e->globalPos(), _id);
    return;
  }

  e->ignore();
  SliderBase::mousePressEvent(e);
}

// bool CompactKnob::event(QEvent* e)
// {
//   switch(e->type())
//   {
//     // FIXME: Doesn't work.
//     case QEvent::NonClientAreaMouseButtonPress:
//       DEBUG_KNOB(stderr, "CompactKnob::event NonClientAreaMouseButtonPress\n");
//       e->accept();
//       _editMode = false;
//       if(_editor)
//       {
//         _editor->deleteLater();
//         _editor = 0;
//       }
//       return true;
//     break;
//
//     default:
//     break;
//   }
//
//   //e->ignore();   // No, this causes duplicates! For ex. skipping tab focus.
//   return SliderBase::event(e);
// }

//------------------------------------------------------------
//
//.F  CompactKnob::setKnobWidth
//    Change the knob's width.
//
//.u  Syntax
//.f  void CompactKnob::setKnobWidth(int w)
//
//.u  Parameters
//.p  int w     --  new width
//
//.u  Description
//    The specified width must be >= 5, or it will be clipped.
//
//------------------------------------------------------------
void CompactKnob::setKnobWidth(int w)
{
    d_knobWidth = MusECore::qwtMax(w,5);
    resize(size());
//     repaint();
    update();
}

//------------------------------------------------------------
//
//.F  CompactKnob::setBorderWidth
//    Set the knob's border width
//
//.u  Syntax
//.f  void CompactKnob::setBorderWidth(int bw)
//
//.u  Parameters
//.p  int bw -- new border width
//
//------------------------------------------------------------
void CompactKnob::setBorderWidth(int bw)
{
    d_borderWidth = MusECore::qwtMax(bw, 0);
    resize(size());
//     repaint();
    update();
}

//------------------------------------------------------------
//.-
//.F  CompactKnob::recalcAngle
//    Recalculate the marker angle corresponding to the
//    current value
//
//.u  Syntax
//.f  void CompactKnob::recalcAngle()
//
//------------------------------------------------------------
void CompactKnob::recalcAngle()
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
void CompactKnob::setFaceColor(const QColor& c)
{
  d_faceColor = c;

//   if(!d_faceColor.isValid())
//     d_faceColor     = palette().color(QPalette::Window);
//   d_curFaceColor  = d_faceColor;
  if(!_faceColSel)
    update();
}

//------------------------------------------------------------
//  setAltFaceColor
//------------------------------------------------------------
void CompactKnob::setAltFaceColor(const QColor& c)
{
  d_altFaceColor = c;
  if(_faceColSel)
    update();
}

//------------------------------------------------------------
//  selectFaceColor
//------------------------------------------------------------
void CompactKnob::selectFaceColor(bool alt)
{
  _faceColSel = alt;
  if(alt)
    d_curFaceColor = d_altFaceColor;
  else
    d_curFaceColor = d_faceColor;
  update();
}

//------------------------------------------------------------
//  setShinyColor
//------------------------------------------------------------
void CompactKnob::setShinyColor(const QColor& c)
{
  d_shinyColor = c;
    update();
}

//------------------------------------------------------------
//  setMarkerColor
//------------------------------------------------------------
void CompactKnob::setMarkerColor(const QColor& c)
{
  d_markerColor = c;
  update();
}

//------------------------------------------------------------
//  setActiveColor
//------------------------------------------------------------
void CompactKnob::setActiveColor(const QColor& c)
{
  d_activeColor = c;
  update();
}

//------------------------------------------------------------
//
//.F  CompactKnob::setMargins
//  Set distances between the widget's border and
//  internals.
//
//.u  Syntax
//.f  void CompactKnob::setMargins(int hor, int vert)
//
//.u  Parameters
//.p  int hor, int vert -- Margins
//
//------------------------------------------------------------
void CompactKnob::setMargins(int hor, int vert)
{
    d_xMargin = MusECore::qwtMax(0, hor);
    d_yMargin = MusECore::qwtMax(0, vert);
    resize(this->size());
}

void CompactKnob::setMargins(QSize s)
{
  setMargins(s.width(), s.height());
}

void CompactKnob::setXMargin(int x)
{
  setMargins(x, d_yMargin);
}

void CompactKnob::setYMargin(int y)
{
  setMargins(d_xMargin, y);
}

//------------------------------------------------------------
//
//.F  CompactKnob::sizeHint
//  Return a recommended size
//
//.u  Syntax
//.f  QSize CompactKnob::sizeHint() const
//
//.u  Note
//  The return value of sizeHint() depends on the font and the
//  scale.
//------------------------------------------------------------

QSize CompactKnob::sizeHint() const
      {
      QSize sz = getMinimumSizeHint(fontMetrics(),
                                d_labelPos,
                                _showValue,
                                _showLabel,
                                d_xMargin,
                                d_yMargin);
      DEBUG_KNOB(stderr, "CompactKnob::sizeHint w:%d h:%d\n", sz.width(), sz.height());
      return sz;
      }


} // namespace MusEGui

