//=========================================================
//  MusE
//  Linux Music Editor
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  compact_slider.h
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

#ifndef __COMPACT_SLIDER_H__
#define __COMPACT_SLIDER_H__

//#include <QPixmap>

#include "sclif.h"
#include "sliderbase.h"
#include "scldraw.h"

class QPainterPath;
class QEvent;

namespace MusEGui {

class PopupDoubleSpinBox;
//class LCDPainter;

//---------------------------------------------------------
//   CompactSlider
//---------------------------------------------------------

class CompactSlider : public SliderBase, public ScaleIf
{
  Q_OBJECT

  public:
      enum ScalePos { None, Left, Right, Top, Bottom, Embedded };

      // Can be OR'd together.
      enum ActiveBorders { NoBorders = 0,
                           LeftBorder = 1,
                           RightBorder = 2,
                           TopBorder = 4,
                           BottomBorder = 8,
                           AllBorders = LeftBorder | RightBorder | TopBorder | BottomBorder,
                           AllBordersExceptTop = LeftBorder | RightBorder | BottomBorder,
                           AllBordersExceptBottom = LeftBorder | RightBorder | TopBorder,
                           AllBordersExceptLeft = RightBorder | TopBorder | BottomBorder,
                           AllBordersExceptRight = LeftBorder | TopBorder | BottomBorder,
                         };
      typedef int ActiveBorders_t;

      // Can be OR'd together.
      enum TextHighlightModes { //TextHighlightNone,
                               //TextHighlightAlways, 
                               TextHighlightOn = 1, 
                               TextHighlightSplit = 2,
                               TextHighlightShadow = 4,
                               //TextHighlightSplitAndShadow,
                               TextHighlightHover = 8,
                               TextHighlightFocus = 16
                               //TextHighlightHoverOrFocus
                              };
      typedef int TextHighlightMode_t;

      Q_PROPERTY( double lineStep READ lineStep WRITE setLineStep )
      Q_PROPERTY( double pageStep READ pageStep WRITE setPageStep )
      Q_PROPERTY( Qt::Orientation orientation READ orientation WRITE setOrientation )

      Q_PROPERTY( QSize margins READ margins WRITE setMargins )
      Q_PROPERTY( int xMargin READ xMargin WRITE setXMargin )
      Q_PROPERTY( int yMargin READ yMargin WRITE setYMargin )

      Q_PROPERTY( QColor barColor READ barColor WRITE setBarColor )
      Q_PROPERTY( QColor slotColor READ slotColor WRITE setSlotColor )
      Q_PROPERTY( QColor thumbColor READ thumbColor WRITE setThumbColor )

      Q_PROPERTY( QString labelText READ labelText WRITE setLabelText )
      Q_PROPERTY( QString valPrefix READ valPrefix WRITE setValPrefix )
      Q_PROPERTY( QString valSuffix READ valSuffix WRITE setValSuffix )
      Q_PROPERTY( QString specialValueText READ specialValueText WRITE setSpecialValueText )
      Q_PROPERTY( QString offText READ offText WRITE setOffText )
      Q_PROPERTY( int valueDecimals READ valueDecimals WRITE setValueDecimals )
      
  private:
    QRect d_sliderRect;
    // Whether the mouse is currently over the thumb 'hit' space.
    bool _mouseOverThumb;
    // Whether the mouse is over the entire control.
    bool _hovered;
    // Cached pixmap values.
    //QPixmap _onPixmap, _offPixmap;
    //QPainterPath* _onPath; 
    //QPainterPath* _offPath; 

    bool _detectThumb;
    bool _autoHideThumb;
    bool _hasOffMode;
    int d_thumbLength;
    int d_thumbHitLength;
    int d_thumbHalf;
    int d_thumbWidth;
    int d_thumbWidthMargin;
    int d_scaleDist;
    int d_xMargin;
    int d_yMargin;
    int d_mMargin;
    // Which of the coloured borders are active.
    // This allows them to be stacked or placed side by side
    //  without the border colours being too bright or annoying
    //  since the joining edges are twice as thick.
    ActiveBorders_t _activeBorders;

    QColor d_borderColor;
    QColor d_barColor;
    QColor d_slotColor;
    QColor d_thumbColor;
    
    QString d_labelText;
    QString d_valPrefix;
    QString d_valSuffix;
    QString d_specialValueText;
    QString d_offText;
    TextHighlightMode_t _textHighlightMode;
    int _valueDecimals;
    bool _off;
    // Whether to display the value, along with the text.
    bool _showValue;

    PopupDoubleSpinBox* _editor;
    bool _editMode;
    //LCDPainter* _LCDPainter;
    
    bool _entered; // Set on enter and leave.
    bool d_resized;
    bool d_autoResize;
    double d_scaleStep;

    Qt::Orientation d_orient;
    ScalePos d_scalePos;
    int d_bgStyle;
    
    // Two slightly different versions of the same thing: valuePixel is the current value scaled to
    //  a range up to and including the LAST pixel, while valueWidth is the current value scaled to 
    //  a range up to and including the last pixel PLUS ONE ie the 'pixel width'. Values start at zero.
    int d_valuePixel;
    int d_valuePixelWidth;

    //void updatePainterPaths();

  private slots:
    void editorReturnPressed();
    void editorEscapePressed();
  
  protected:
    // At what point size to switch from aliased text (brighter, crisper but may look too jagged and unreadable with some fonts) 
    //  to non-aliased text (dimmer, fuzzier but looks better). Zero means always use anti-aliasing.
    int _maxAliasedPointSize;
    
    //  Determine the value corresponding to a specified mouse location.
    //  If borderless mouse is enabled p is a delta value not absolute, so can be negative.
    double getValue(const QPoint &p);
    //  Determine the value corresponding to a specified mouse movement.
    double moveValue(const QPoint &deltaP, bool fineMode = false);
    //  Determine scrolling mode and direction.
    void getScrollMode( QPoint &p, const Qt::MouseButton &button, const Qt::KeyboardModifiers& modifiers, int &scrollMode, int &direction);
    
    // Update internal values.
    void getPixelValues();
    void getActiveArea();
    void getMouseOverThumb(QPoint &p);
    
    void showEditor();

    virtual void resizeEvent(QResizeEvent*);
    virtual void paintEvent (QPaintEvent*);
    virtual void mouseMoveEvent(QMouseEvent*);
    virtual void mouseDoubleClickEvent(QMouseEvent*);
    virtual void keyPressEvent(QKeyEvent*);
    virtual void leaveEvent(QEvent*);
    virtual void enterEvent(QEvent*);
    virtual bool event(QEvent*);

    void valueChange();
    void rangeChange();
    void scaleChange();
    //void fontChange(const QFont &oldFont);

    //virtual void updatePixmaps();
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
    CompactSlider(QWidget *parent = 0, const char *name = 0,
          Qt::Orientation orient = Qt::Horizontal,
          ScalePos scalePos = None,
          const QString& labelText = QString(),
          const QString& valPrefix = QString(),
          const QString& valSuffix = QString(),
          const QString& specialValueText = QString(),
          const QColor& borderColor = QColor(),
          const QColor& barColor = QColor(228,203,36),
          const QColor& slotColor = QColor(),
          const QColor& thumbColor = QColor());
    
    virtual ~CompactSlider();
    
    static QSize getMinimumSizeHint(const QFontMetrics& fm, 
                                    Qt::Orientation orient = Qt::Vertical,
                                    ScalePos scalePos = None,
                                    int xMargin = 0, 
                                    int yMargin = 0);

    // Which of the coloured borders are active.
    // This allows them to be stacked or placed side by side
    //  without the border colours being too bright or annoying
    //  since the joining edges are twice as thick.
    ActiveBorders_t activeBorders() const { return _activeBorders; }
    void setActiveBorders(ActiveBorders_t borders);

    void setThumbLength(int l);
    void setThumbWidth(int w);

    void setOrientation(Qt::Orientation o);
    Qt::Orientation orientation() const;

    double lineStep() const;
    double pageStep() const;

    void setLineStep(double);
    void setPageStep(double);
    
    QColor borderColor() const { return d_borderColor; }
    void setBorderColor(const QColor& c) { d_borderColor = c; update(); }
    QColor barColor() const { return d_barColor; }
    void setBarColor(const QColor& c) { d_barColor = c; update(); }
    QColor slotColor() const { return d_slotColor; }
    void setSlotColor(const QColor& c) { d_slotColor = c; update(); }
    QColor thumbColor() const { return d_thumbColor; }
    void setThumbColor(const QColor& c) { d_thumbColor = c; update(); }
    // Whether the user must click on the thumb or else anywhere in the control to move the value. 
    bool detectThumb() const { return _detectThumb; }
    // Set whether the user must click on the thumb or else anywhere in the control to move the value. 
    void setDetectThumb(bool v) { _detectThumb = v; update(); }
    
    // At what point size to switch from aliased text to non-aliased text. Zero means always use anti-aliasing. Default 8.
    int maxAliasedPointSize() const { return _maxAliasedPointSize; }
    // Sets at what point size to switch from aliased text (brighter, crisper but may look too jagged and unreadable with some fonts) 
    //  to non-aliased text (dimmer, fuzzier but looks better). Zero means always use anti-aliasing.
    void setMaxAliasedPointSize(int sz) { if(sz<0)sz=0;_maxAliasedPointSize = sz; update(); }
    
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
    TextHighlightMode_t textHighlightMode() const { return _textHighlightMode; }
    void setTextHighlightMode(TextHighlightMode_t mode) { _textHighlightMode = mode; update(); }
    int valueDecimals() const { return _valueDecimals; }
    void setValueDecimals(int d) { if(d < 0) return; _valueDecimals = d; update(); }

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
    
    QSize margins() const { return QSize(d_xMargin, d_yMargin); }
    int xMargin() const { return d_xMargin; }
    int yMargin() const { return d_yMargin; }
    void setMargins(QSize);
    void setMargins(int x, int y);
    void setXMargin(int x);
    void setYMargin(int y);

    int thumbWidthMargin() const { return d_thumbWidthMargin; }
    void setThumbWidthMargin(int m) { d_thumbWidthMargin = m; update(); }
    int thumbHitLength() const { return d_thumbHitLength; }
    void setThumbHitLength(int l) { d_thumbHitLength = l; }
    bool autoHideThumb() const { return _autoHideThumb; }
    void setAutoHideThumb(bool v) {_autoHideThumb = v; }
    
    virtual QSize sizeHint() const;
};

} // namespace MusEGui

#endif
