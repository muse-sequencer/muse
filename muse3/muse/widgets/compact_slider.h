//=========================================================
//  MusE
//  Linux Music Editor
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  compact_slider.h
//  (C) Copyright 2015 Tim E. Real (terminator356 on sourceforge)
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

#include <QDoubleSpinBox>
#include <QPixmap>

#include "sclif.h"
#include "sliderbase.h"
#include "scldraw.h"

class QPainterPath;

namespace MusEGui {

//---------------------------------------------------------
//   PopupDoubleSpinBox
//---------------------------------------------------------

class PopupDoubleSpinBox : public QDoubleSpinBox {
  Q_OBJECT

  private:
    bool _closePending;
    
  protected:
//       virtual void keyPressEvent(QKeyEvent*);
//       virtual void wheelEvent(QWheelEvent*);
//       virtual void focusOutEvent(QFocusEvent*);
    //virtual void paintEvent(QPaintEvent*);
    virtual bool event(QEvent*);
    
  signals:
//       void doubleClicked();
//       void ctrlDoubleClicked();
//       //void ctrlClicked();
    void returnPressed();
    void escapePressed();

  public:
    PopupDoubleSpinBox(QWidget* parent=0);
};
  
//---------------------------------------------------------
//   CompactSlider
//---------------------------------------------------------

class CompactSlider : public SliderBase, public ScaleIf
{
  Q_OBJECT

  public:
      enum ScalePos { None, Left, Right, Top, Bottom, Embedded };
      enum TextHighlightMode { TextHighlightNone, 
                               TextHighlightAlways, 
                               TextHighlightSplit,
                               TextHighlightShadow,
                               TextHighlightSplitAndShadow,
                               TextHighlightHover,
                               TextHighlightFocus,
                               TextHighlightHoverOrFocus };

  private:
      Q_PROPERTY( double lineStep READ lineStep WRITE setLineStep )
      Q_PROPERTY( double pageStep READ pageStep WRITE setPageStep )
      Q_PROPERTY( Qt::Orientation orientation READ orientation WRITE setOrientation )

//       Q_PROPERTY( QColor marginColor READ marginColor WRITE setMarginColor )
//       Q_PROPERTY( QColor fillColor READ fillColor WRITE setFillColor )
//       Q_PROPERTY( QColor textColor READ textColor WRITE setTextColor )
//       Q_PROPERTY( QColor barColor READ barColor WRITE setBarColor )
      Q_PROPERTY( QColor thumbColor READ thumbColor WRITE setThumbColor )

      Q_PROPERTY( QString labelText READ labelText WRITE setLabelText )
      Q_PROPERTY( QString valPrefix READ valPrefix WRITE setValPrefix )
      Q_PROPERTY( QString valSuffix READ valSuffix WRITE setValSuffix )
      Q_PROPERTY( QString specialValueText READ specialValueText WRITE setSpecialValueText )
      
    QRect d_sliderRect;
    // Whether the mouse is currently over the thumb 'hit' space.
    bool _mouseOverThumb;
    // Whether the mouse is over the entire control.
    bool _hovered;
    // Cached pixmap values.
    QPixmap _onPixmap, _offPixmap;
    QPainterPath* _onPath; 
    QPainterPath* _offPath; 

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

//     QColor d_marginColor;
//     QColor d_fillColor;
//     QColor d_textColor;
//     QColor d_barColor;
    QColor d_thumbColor;
    
    QString d_labelText;
    QString d_valPrefix;
    QString d_valSuffix;
    QString d_specialValueText;
    QString d_offText;
    TextHighlightMode _textHighlightMode;
    int _valueDecimals;
    bool _off;
    
    PopupDoubleSpinBox* _editor;
    bool _editMode;

    
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

//     bool bPressed;

//     void drawHsBgSlot(QPainter *, const QRect&, const QRect&,const QBrush&);
//     void drawVsBgSlot(QPainter *, const QRect&, const QRect&,const QBrush&);

    void updatePainterPaths();
    
  private slots:
    //void editingFinished();
    void editorReturnPressed();
    void editorEscapePressed();
  
  protected:
    /* virtual void drawSlider (QPainter *p, const QRect &r); */
    
    //  Determine the value corresponding to a specified mouse location.
    //  If borderless mouse is enabled p is a delta value not absolute, so can be negative.
    double getValue(const QPoint &p);
    //  Determine scrolling mode and direction.
    void getScrollMode( QPoint &p, const Qt::MouseButton &button, int &scrollMode, int &direction);
    
    // Two slightly different versions of the same thing: getValuePixel gets the current value scaled to
    //  a range up to and including the LAST pixel, while getValueWidth gets the current value scaled to 
    //  a range up to and including the last pixel PLUS ONE ie the 'pixel width'. Return values start at zero.
  //   int getValuePixel() const;
  //   int getValueWidth() const;
    
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
    void fontChange(const QFont &oldFont);

  public:
    CompactSlider(QWidget *parent = 0, const char *name = 0,
          Qt::Orientation orient = Qt::Horizontal,
          ScalePos scalePos = None,
          const QString& labelText = QString(), 
          const QString& valPrefix = QString(), 
          const QString& valSuffix = QString(),
          const QString& specialValueText = QString(), 
          QColor thumbColor = QColor(255, 255, 0));
    
    virtual ~CompactSlider();
    
    static QSize getMinimumSizeHint(const QFontMetrics& fm, 
                                    Qt::Orientation orient = Qt::Vertical,
                                    ScalePos scalePos = None,
                                    int xMargin = 0, 
                                    int yMargin = 0);
    
    void setThumbLength(int l);
    void setThumbWidth(int w);

    void setOrientation(Qt::Orientation o);
    Qt::Orientation orientation() const;

    double lineStep() const;
    double pageStep() const;

    void setLineStep(double);
    void setPageStep(double);
    
//     QColor marginColor() const { return d_marginColor; }
//     void setMarginColor(const QColor& c) { d_marginColor = c; update(); }
//     QColor fillColor() const { return d_fillColor; }
//     void setFillColor(const QColor& c) { d_fillColor = c; update(); }
//     QColor textColor() const { return d_textColor; }
//     void setTextColor(const QColor& c) { d_textColor = c; update(); }
//     QColor barColor() const { return d_barColor; }
//     void setBarColor(const QColor& c) { d_barColor = c; update(); }
    QColor thumbColor() const { return d_thumbColor; }
    void setThumbColor(const QColor& c) { d_thumbColor = c; update(); }
    // Whether the user must click on the thumb or else anywhere in the control to move the value. 
    bool detectThumb() const { return _detectThumb; }
    // Set whether the user must click on the thumb or else anywhere in the control to move the value. 
    void setDetectThumb(bool v) { _detectThumb = v; update(); }
    

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
    TextHighlightMode textHighlightMode() const { return _textHighlightMode; }
    void setTextHighlightMode(TextHighlightMode mode) { _textHighlightMode = mode; update(); }
    int valueDecimals() const { return _valueDecimals; }
    void setValueDecimals(int d) { if(d < 0) return; _valueDecimals = d; update(); }

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

    int thumbWidthMargin() const { return d_thumbWidthMargin; }
    void setThumbWidthMargin(int m) { d_thumbWidthMargin = m; update(); }
    int thumbHitLength() const { return d_thumbHitLength; }
    void setThumbHitLength(int l) { d_thumbHitLength = l; }
    bool autoHideThumb() const { return _autoHideThumb; }
    void setAutoHideThumb(bool v) {_autoHideThumb = v; }
    
    virtual QSize sizeHint() const;
//     virtual QSize minimumSizeHint() const;
  
  public slots:
    void processSliderPressed(int);
    void processSliderReleased(int);
    
  signals:
    // Both value and off state changed combined into one signal. 
    // Note the SliderBase::valueChanged signal is also available.
    void valueStateChanged(double value, bool off, int id);
};

} // namespace MusEGui

#endif
