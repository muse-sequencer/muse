//=========================================================
//  MusE
//  Linux Music Editor
//
//  compact_knob.h
//  Copyright (C) 1999-2011 by Werner Schweer and others
//  (C) Copyright 2011 Orcan Ogetbil (ogetbilo at sf.net) completely redesigned.
//  (C) Copyright 2016 - 2017 Tim E. Real (terminator356 on sourceforge)
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

#include <QPoint>
#include <QRect>
#include <QSize>
#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QResizeEvent>
#include <QPainter>
#include <QEvent>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QKeyEvent>

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
      Q_ENUM(Symbol)

      Q_PROPERTY( QSize margins READ margins WRITE setMargins )
      Q_PROPERTY( int xMargin READ xMargin WRITE setXMargin )
      Q_PROPERTY( int yMargin READ yMargin WRITE setYMargin )

      Q_PROPERTY( int knobWidth READ knobWidth WRITE setKnobWidth )
      Q_PROPERTY( double totalAngle READ totalAngle WRITE setTotalAngle )
      Q_PROPERTY( int borderWidth READ borderWidth WRITE setBorderWidth )

      Q_PROPERTY( QColor faceColor READ faceColor WRITE setFaceColor )
      Q_PROPERTY( QColor altFaceColor READ altFaceColor WRITE setAltFaceColor )
      Q_PROPERTY( QColor shinyColor READ shinyColor WRITE setShinyColor )
      Q_PROPERTY( QColor markerColor READ markerColor WRITE setMarkerColor )
      Q_PROPERTY( QColor activeColor READ activeColor WRITE setActiveColor )
      
      Q_PROPERTY( QString labelText READ labelText WRITE setLabelText )
      Q_PROPERTY( QString valPrefix READ valPrefix WRITE setValPrefix )
      Q_PROPERTY( QString valSuffix READ valSuffix WRITE setValSuffix )
      Q_PROPERTY( QString specialValueText READ specialValueText WRITE setSpecialValueText )
      Q_PROPERTY( QString offText READ offText WRITE setOffText )
      Q_PROPERTY( int valueDecimals READ valueDecimals WRITE setValueDecimals )

      Q_PROPERTY( bool style3d READ style3d WRITE setStyle3d )
      Q_PROPERTY( int radius READ radius WRITE setRadius )
      Q_PROPERTY( Symbol symbol READ symbol WRITE setSymbol )
      Q_PROPERTY( bool drawChord READ drawChord WRITE setDrawChord )

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
      // Whether to display the label.
      bool _showLabel;
      // Whether to display the value.
      bool _showValue;

      bool _style3d;
      int _radius;
      bool _drawChord;

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
      QColor d_activeColor;

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
//       virtual bool event(QEvent*);

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

      static QSize getMinimumSizeHint(const QFontMetrics& fm,
                                      //Qt::Orientation orient = Qt::Vertical,
                                      KnobLabelPos labelPos = None,
                                      bool showValue = true,
                                      bool showLabel = true,
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

      int knobWidth() const { return d_knobWidth; }
      void setKnobWidth(int w);
      double totalAngle() const { return d_totalAngle; }
      void setTotalAngle (double angle);
      int borderWidth() const { return d_borderWidth; }
      void setBorderWidth(int bw);

      void selectFaceColor(bool alt);
      bool selectedFaceColor() const { return _faceColSel; }
      QColor faceColor() const { return d_faceColor; }
      void setFaceColor(const QColor& c);
      QColor altFaceColor() const { return d_altFaceColor; }
      void setAltFaceColor(const QColor& c);
      QColor shinyColor() const { return d_shinyColor; }
      void setShinyColor(const QColor& c);
      QColor markerColor() const { return d_markerColor; }
      void setMarkerColor(const QColor& c);
      QColor activeColor() const { return d_activeColor; }
      void setActiveColor(const QColor& c);

      QString toolTipValueText(bool inclLabel, bool inclVal) const;

      bool showLabel() const { return _showLabel; }
      void setShowLabel(bool show);

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

      QSize margins() const { return QSize(d_xMargin, d_yMargin); }
      int xMargin() const { return d_xMargin; }
      int yMargin() const { return d_yMargin; }
      void setMargins(QSize);
      void setMargins(int x, int y);
      void setXMargin(int x);
      void setYMargin(int y);

      bool style3d() const { return _style3d; }
      void setStyle3d(const bool style3d) { _style3d = style3d; }
      int radius() const { return _radius; }
      void setRadius(const int radius) { _radius = radius; }
      Symbol symbol() const { return d_symbol; }
      void setSymbol(const Symbol s) { d_symbol = s; }
      bool drawChord() const { return _drawChord; }
      void setDrawChord(const bool draw) { _drawChord = draw; }

      virtual QSize sizeHint() const;
      };


} // namespace MusEGui

#endif
