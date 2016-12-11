//=========================================================
//  MusE
//  Linux Music Editor
//
//  compact_knob.h
//  Copyright (C) 1999-2011 by Werner Schweer and others
//  (C) Copyright 2011 Orcan Ogetbil (ogetbilo at sf.net) completely redesigned.
//  (C) Copyright 2016 Tim E. Real (terminator356 on sourceforge)
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

#ifndef __COMPACT_KNOB_H__
#define __COMPACT_KNOB_H__

#include "sliderbase.h"
#include "sclif.h"

class QPoint;
class QRect;
class QSize;
class QColor;
class QFont;
class QFontMetrics;
class QResizeEvent;
class QPainter;
class QEvent;
class QPaintEvent;
class QMouseEvent;
class QKeyEvent;

namespace MusEGui {

class PopupDoubleSpinBox;
class ItemBackgroundPainter;

//---------------------------------------------------------
//   CompactKnob
//---------------------------------------------------------

class CompactKnob : public SliderBase, public ScaleIf
      {
  Q_OBJECT

   public:
      enum KnobLabelPos { None, Left, Right, Top, Bottom };
      enum Symbol { Line, Dot };

   private:
      KnobLabelPos d_labelPos;

      bool _hasOffMode;

      QString d_labelText;
      QString d_valPrefix;
      QString d_valSuffix;
      QString d_specialValueText;
      QString d_offText;
      int _valueDecimals;
      bool _off;
      // Whether to display the value, below the text.
      bool _showValue;

      PopupDoubleSpinBox* _editor;
      bool _editMode;

      ItemBackgroundPainter* _bkgPainter;

   private slots:
      void editorReturnPressed();
      void editorEscapePressed();

   protected:
      bool hasScale;
      // Whether the mouse is over the entire control.
      bool _hovered;
      // Whether the mouse is over the knob.
      bool _knobHovered;
      // Whether the mouse is over the label.
      bool _labelHovered;

      int d_xMargin;
      int d_yMargin;
      int d_borderWidth;
      int d_shineWidth;
      int d_scaleDist;
      int d_maxScaleTicks;
      int d_newVal;
      int d_knobWidth;
      int d_dotWidth;

      Symbol d_symbol;
      double d_angle;
      double d_oldAngle;
      double d_totalAngle;
      double d_nTurns;

      double l_const;
      double l_slope;

      QRect  _labelRect;
      QRect  _knobRect;
      bool _faceColSel;
      QColor d_faceColor;
      QColor d_altFaceColor;
      QColor d_shinyColor;
      QColor d_curFaceColor;
      QColor d_markerColor;

      void recalcAngle();
      void valueChange();
      void rangeChange();
      void drawBackground(QPainter*);
      void drawKnob(QPainter* p, const QRect &r);
      void drawMarker(QPainter* p, double arc, const QColor &c);
      void drawLabel(QPainter* p);
      void showEditor();

      virtual void resizeEvent(QResizeEvent*);
      virtual void paintEvent(QPaintEvent*);
      virtual void mouseMoveEvent(QMouseEvent*);
      virtual void mousePressEvent(QMouseEvent*);
      virtual void mouseDoubleClickEvent(QMouseEvent*);
      virtual void keyPressEvent(QKeyEvent*);
      virtual void leaveEvent(QEvent*);
      virtual bool event(QEvent*);

      double getValue(const QPoint &p);
      //  Determine the value corresponding to a specified mouse movement.
      double moveValue(const QPoint& /*deltaP*/, bool /*fineMode*/ = false);
      void getScrollMode( QPoint &p, const Qt::MouseButton &button, const Qt::KeyboardModifiers& modifiers, int &scrollMode, int &direction );
      void scaleChange()             { repaint(); }
      void fontChange(const QFont &) { repaint(); }

      virtual void processSliderPressed(int);
      virtual void processSliderReleased(int);
      // Show a handy tooltip value box.
      virtual void showValueToolTip(QPoint);

   signals:
      // Both value and off state changed combined into one signal.
      // In typical automation use, this signal should be ignored in ScrDirect scroll mode.
      // ScrDirect mode happens only once upon press with a modifier.
      // In ScrDirect mode the slider sends both pressed AND changed signals
      //  since the position jumps to the pressed location.
      // Note the SliderBase::valueChanged signal is also available.
      void valueStateChanged(double value, bool off, int id, int scrollMode);

   public:
      CompactKnob(QWidget* parent = 0, const char *name = 0,
          KnobLabelPos labelPos = None,
          const QString& labelText = QString(),
          const QString& valPrefix = QString(),
          const QString& valSuffix = QString(),
          const QString& specialValueText = QString(),
          const QColor& faceColor = QColor());
      ~CompactKnob();

      static QSize getMinimumSizeHint(const QFontMetrics& fm,
                                      //Qt::Orientation orient = Qt::Vertical,
                                      KnobLabelPos labelPos = None,
                                      bool showValue = true,
                                      int xMargin = 0,
                                      int yMargin = 0);

      void setRange(double vmin, double vmax, double vstep = 0.0,
                    int pagesize = 1, DoubleRange::ConversionMode mode = ConvertDefault);

      QString labelText() const { return d_labelText; };
      void setLabelText(const QString& t) { d_labelText = t; update(); }
      QString valPrefix() const { return d_valPrefix; };
      void setValPrefix(const QString& t) { d_valPrefix = t; update(); }
      QString valSuffix() const { return d_valSuffix; };
      void setValSuffix(const QString& t) { d_valSuffix = t; update(); }
      QString specialValueText() const { return d_specialValueText; };
      void setSpecialValueText(const QString& t) { d_specialValueText = t; update(); }
      QString offText() const { return d_offText; };
      void setOffText(const QString& t) { d_offText = t; update(); }
      int valueDecimals() const { return _valueDecimals; }
      void setValueDecimals(int d) { if(d < 0) return; _valueDecimals = d; update(); }

      void setKnobWidth(int w);
      void setTotalAngle (double angle);
      void setBorderWidth(int bw);
      void selectFaceColor(bool alt);
      bool selectedFaceColor() { return _faceColSel; }
      QColor faceColor() { return d_faceColor; }
      void setFaceColor(const QColor& c);
      QColor altFaceColor() { return d_altFaceColor; }
      void setAltFaceColor(const QColor& c);
      QColor shinyColor() { return d_shinyColor; }
      void setShinyColor(const QColor& c);
      QColor markerColor() { return d_markerColor; }
      void setMarkerColor(const QColor& c);

      QString toolTipValueText(bool inclLabel, bool inclVal) const;

      bool showValue() const { return _showValue; }
      void setShowValue(bool show);

      bool hasOffMode() const { return _hasOffMode; }
      void setHasOffMode(bool v);
      bool isOff() const { return _off; }
      // Sets the off state and emits valueStateChanged signal if required.
      void setOff(bool v);
      // Both value and off state changed combined into one setter.
      // By default it is assumed that setting a value naturally implies resetting the 'off' state to false.
      // Emits valueChanged and valueStateChanged signals if required.
      // Note setOff and SliderBase::setValue are also available.
      void setValueState(double v, bool off = false, ConversionMode mode = ConvertDefault);

      void setMargins(int x, int y);

      virtual QSize sizeHint() const;
      };


} // namespace MusEGui

#endif
