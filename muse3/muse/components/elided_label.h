//=========================================================
//  MusE
//  Linux Music Editor
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  elided_label.h
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

#ifndef __ELIDED_LABEL_H__
#define __ELIDED_LABEL_H__

#include <QFrame>

namespace MusEGui {

//---------------------------------------------------------
//   ElidedLabel
//---------------------------------------------------------

class ElidedLabel : public QFrame
{
  Q_OBJECT
  // FIXME: Found some problems here: If this was enabled, text() was
  //        intermittently including accelerator key characters, causing
  //        for example midi controller stream text rapid flickering
  //        between normal and accelerator key text.
  //       Maybe it should be const QString& ?
  //Q_PROPERTY(QString text READ text WRITE setText)

  //Q_PROPERTY(Qt::TextElideMode elideMode READ elideMode WRITE setElideMode)
//  Q_PROPERTY( QColor activeColor READ activeColor WRITE setActiveColor )

    Q_PROPERTY( bool style3d READ style3d WRITE setStyle3d )
    Q_PROPERTY( int radius READ radius WRITE setRadius )

  private:
    int _id;
    bool _hasOffMode;
    bool _off;
    Qt::TextElideMode _elideMode;
    Qt::Alignment _alignment;
    int _fontPointMin;
    bool _fontIgnoreHeight;
    bool _fontIgnoreWidth;
    // leave defaults to background painter
    QColor _bgActiveColor;
    QColor _bgColor;
    QColor _borderColor;
    QColor _fontColor{Qt::black};
    QColor _fontActiveColor{Qt::white};
    bool _style3d;
    int _radius;
    QString _text;
    QFont _curFont;
    // Whether the mouse is over the entire control.
    bool _hovered;

    bool autoAdjustFontSize();
    
  protected:
    virtual void paintEvent(QPaintEvent*);
    virtual void resizeEvent(QResizeEvent*);
    virtual void mousePressEvent(QMouseEvent*);
    virtual void mouseReleaseEvent(QMouseEvent*);
    virtual void mouseMoveEvent(QMouseEvent*);
    virtual void mouseDoubleClickEvent(QMouseEvent*);
    virtual void leaveEvent(QEvent*);
    virtual void keyPressEvent(QKeyEvent*);

  signals:
    void pressed(QPoint p, int id, Qt::MouseButtons buttons, Qt::KeyboardModifiers keys);
    void released(QPoint p, int id, Qt::MouseButtons buttons, Qt::KeyboardModifiers keys);
    void returnPressed(QPoint p, int id, Qt::KeyboardModifiers keys);
    void doubleClicked();

  public:
    explicit ElidedLabel(QWidget* parent = 0, 
                         Qt::TextElideMode elideMode = Qt::ElideNone, 
                         Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignVCenter,
                         //int maxFontPoint = 10,
                         int minFontPoint = 5,
                         bool ignoreHeight = true, bool ignoreWidth = false,
                         const QString& text = QString(), 
                         const char* name = 0,
                         Qt::WindowFlags flags = Qt::Widget);
  
    virtual QSize sizeHint() const;
    
    int id() const             { return _id; }
    void setId(int i)          { _id = i; }
    
    Qt::TextElideMode elideMode() const { return _elideMode; }
    void setElideMode(Qt::TextElideMode mode) { _elideMode = mode; update(); }

    Qt::Alignment alignment() const { return _alignment; }
    void setAlignment(Qt::Alignment align) { _alignment = align; update(); }

    bool hasOffMode() const { return _hasOffMode; }
    void setHasOffMode(bool v);
    bool isOff() const { return _off; }
    // Sets the off state and emits valueStateChanged signal if required.
    void setOff(bool v);
    // Both value and off state changed combined into one setter.
    // By default it is assumed that setting a value naturally implies resetting the 'off' state to false.
    // Emits valueChanged and valueStateChanged signals if required.
    // Note setOff and SliderBase::setValue are also available.
    //void setValueState(double v, bool off = false, ConversionMode mode = ConvertDefault);

    QString text() const { return _text; }
    void setText(const QString& txt);
    
//    QColor activeColor() const { return _activeColor; }
//    void setActiveColor(const QColor& c) { _activeColor = c; update(); }

    void setBgColor(const QColor& c) { _bgColor = c; update(); }
    void setBgActiveColor(const QColor& c) { _bgActiveColor = c; update(); }
    void setBorderColor(const QColor& c) { _borderColor = c; update(); }
    void setFontColor(const QColor& c) { _fontColor = c; update(); }
    void setFontActiveColor(const QColor& c) { _fontActiveColor = c; update(); }

    bool style3d() const { return _style3d; }
    void setStyle3d(const bool style3d) { _style3d = style3d; }
    int radius() const { return _radius; }
    void setRadius(const int radius) { _radius = radius; }

    int fontPointMin() const { return _fontPointMin; }
    void setFontPointMin(int point);

    bool fontIgnoreWidth() const { return _fontIgnoreWidth; }
    bool fontIgnoreHeight() const { return _fontIgnoreHeight; }
    void setFontIgnoreDimensions(bool ignoreHeight, bool ignoreWidth = false);
};

//---------------------------------------------------------
//   ElidedTextLabel
//---------------------------------------------------------

class ElidedTextLabel : public QFrame
{
  Q_OBJECT
  // FIXME: Found some problems here: If this was enabled, text() was
  //        intermittently including accelerator key characters, causing
  //        for example midi controller stream text rapid flickering
  //        between normal and accelerator key text.
  //       Maybe it should be const QString& ?
  //Q_PROPERTY(QString text READ text WRITE setText)

  //Q_PROPERTY(Qt::TextElideMode elideMode READ elideMode WRITE setElideMode)

  private:
    int _id;
    bool _hasOffMode;
    bool _off;
    Qt::TextElideMode _elideMode;
    Qt::Alignment _alignment;
    QString _text;
    QString _tooltipText;
    // Whether the mouse is over the entire control.
    bool _hovered;

    bool autoAdjustFontSize();
    
  protected:
    virtual void paintEvent(QPaintEvent*);
    virtual void mouseMoveEvent(QMouseEvent*);
    virtual void leaveEvent(QEvent*);
    virtual void keyPressEvent(QKeyEvent*);

  signals:
    void returnPressed(QPoint p, int id, Qt::KeyboardModifiers keys);

  public:
    explicit ElidedTextLabel(
      QWidget* parent = 0,
      const char* name = 0,
      Qt::WindowFlags flags = Qt::Widget
      );
  
    explicit ElidedTextLabel(
      const QString& text,
      QWidget* parent = 0,
      const char* name = 0,
      Qt::WindowFlags flags = Qt::Widget
      );
  
    virtual QSize sizeHint() const;
    
    int id() const             { return _id; }
    void setId(int i)          { _id = i; }
    
    Qt::TextElideMode elideMode() const { return _elideMode; }
    void setElideMode(Qt::TextElideMode mode) { _elideMode = mode; update(); }

    Qt::Alignment alignment() const { return _alignment; }
    void setAlignment(Qt::Alignment align) { _alignment = align; update(); }

    bool hasOffMode() const { return _hasOffMode; }
    void setHasOffMode(bool v);
    bool isOff() const { return _off; }
    // Sets the off state and emits valueStateChanged signal if required.
    void setOff(bool v);
    // Both value and off state changed combined into one setter.
    // By default it is assumed that setting a value naturally implies resetting the 'off' state to false.
    // Emits valueChanged and valueStateChanged signals if required.
    // Note setOff and SliderBase::setValue are also available.
    //void setValueState(double v, bool off = false, ConversionMode mode = ConvertDefault);

    QString text() const { return _text; }
    void setText(const QString& txt);

    // This tooltip text is appened to the normal text, to form a compound tooltip.
    QString tooltipText() const { return _tooltipText; }
    void setTooltipText(const QString& txt);
};



class PaletteSwitcher : public ElidedLabel
{
    Q_OBJECT

    // derive own class so it can be customized from stylesheet...
    // TODO: make real switcher with two mutually exclusive buttons
};


} // namespace MusEGui

#endif
