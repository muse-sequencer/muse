//=========================================================
//  MusE
//  Linux Music Editor
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  lcd_widgets.h
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

#ifndef __LCD_WIDGETS_H__
#define __LCD_WIDGETS_H__

#include <QFrame>

class QPainter;
class QColor;
class QRect;
class QString;
class QEvent;
class QMouseEvent;
class QWheelEvent;
class QKeyEvent;

namespace MusEGui {

class PopupDoubleSpinBox;

//---------------------------------------------------------
//   LCDPainter
//---------------------------------------------------------

class LCDPainter
{
  public:
      enum ScalePos { None, Left, Right, Top, Bottom, Embedded };
      enum TextHighlightMode { TextHighlightNone, TextHighlightAlways, TextHighlightSplit, TextHighlightShadow };

  private:
    
  public:
    LCDPainter();
    
    void drawCharacter(QPainter* painter, const QRect& rect, char asciiChar);
    void drawText(QPainter* painter, const QRect& rect, const QString& text, int flags = 0);
};

//---------------------------------------------------------
//   LCDPatchEdit
//---------------------------------------------------------

class LCDPatchEdit : public QFrame
{
  Q_OBJECT

  //Q_PROPERTY(QString text READ text WRITE setText)
//  Q_PROPERTY( QColor readoutColor READ readoutColor WRITE setReadoutColor )

  public:
    enum PatchSections { HBankSection, LBankSection, ProgSection };
    enum PatchOrientation { PatchHorizontal = 0, PatchVertical };

  protected:
    PatchOrientation _orient;

    int _maxAliasedPointSize;
    int _xMargin;
    int _yMargin;
    int _sectionSpacing;
    int _currentPatch;
    int _lastValidPatch;
    int _lastValidHB;
    int _lastValidLB;
    int _lastValidProg;

    QColor _readoutColor;
    QColor _bgColor;
    QColor _bgActiveColor;
    QColor _borderColor;

    bool _style3d;
    int _radius;

    LCDPainter* _LCDPainter;

    int _id;
    int _fontPointMin;
    bool _fontIgnoreHeight;
    bool _fontIgnoreWidth;
    QString _text;
    QFont _curFont;
    bool _enableValueToolTips;

    QRect _HBankRect;
    QRect _LBankRect;
    QRect _ProgRect;
    QRect _HBankFieldRect;
    QRect _LBankFieldRect;
    QRect _ProgFieldRect;

    bool _HBankHovered;
    bool _LBankHovered;
    bool _ProgHovered;

    PopupDoubleSpinBox* _editor;
    bool _editMode;
    int _curEditSection;

    bool autoAdjustFontSize();
    // The total active drawing area, not including margins.
    QRect activeDrawingArea() const;

    void showEditor();
    // Show a handy tooltip value box.
    void showValueToolTip(QPoint, int section = -1);

    virtual void paintEvent(QPaintEvent*);
    virtual void mouseMoveEvent(QMouseEvent*);
    virtual void wheelEvent(QWheelEvent*);
    virtual void mouseDoubleClickEvent(QMouseEvent*);
    virtual void resizeEvent(QResizeEvent*);
    virtual void mousePressEvent(QMouseEvent*);
    virtual void mouseReleaseEvent(QMouseEvent*);
    virtual void enterEvent(QEvent*);
    virtual void leaveEvent(QEvent*);
    virtual void keyPressEvent(QKeyEvent*);
    virtual bool event(QEvent*);

  protected slots:
    void editorReturnPressed();
    void editorEscapePressed();

  signals:
    void pressed(QPoint p, int id, Qt::MouseButtons buttons, Qt::KeyboardModifiers keys);
    void released(QPoint p, int id, Qt::MouseButtons buttons, Qt::KeyboardModifiers keys);
    void valueChanged(int value, int id);
    void rightClicked(QPoint p, int id);

  public:
    explicit LCDPatchEdit(QWidget* parent = 0,
                         int minFontPoint = 5,
                         bool ignoreHeight = true, bool ignoreWidth = false,
                         const QString& text = QString(),
                         const QColor& readoutColor = QColor(0,255,255),
                         Qt::WindowFlags flags = Qt::Widget);

    virtual ~LCDPatchEdit();

    // The width of a character, not including inter-character space, in a given active area.
    static int charWidth(const QRect& aRect);
    // The amount of space between the blocks of digits, for a given character width.
    static int readoutMargin(int charWidth);

    static QSize getMinimumSizeHint(const QFontMetrics& fm,
                                    int xMargin = 0,
                                    int yMargin = 0,
                                    PatchOrientation orient = PatchHorizontal
                                   );

    int id() const             { return _id; }
    void setId(int i)          { _id = i; }

    void setReadoutOrientation(PatchOrientation);

    void setReadoutColor(const QColor& c) { _readoutColor = c; update(); }
    void setBgColor(const QColor& c) { _bgColor = c; update(); }
    void setBgActiveColor(const QColor& c) { _bgActiveColor = c; update(); }
    void setBorderColor(const QColor& c) { _borderColor = c; update(); }
    void setStyle3d(const bool s) { _style3d = s; }
    void setRadius(const int r) { _radius = r; }

    int value() const;
    void setValue(int v);
    void setLastValidPatch(int v);
    void setLastValidBytes(int hbank, int lbank, int prog);

    bool valueToolTipsEnabled() const { return _enableValueToolTips; }
    void setEnableValueToolTips(bool v) { _enableValueToolTips = v; }
    QString toolTipValueText(int section = -1) const;

    // At what point size to switch from aliased text to non-aliased text. Zero means always use anti-aliasing.
    // Here in CompactPatchEdit, this only affects the CompactSliders so far, not the patch label.
    // If -1, no value has been set and default is each widget's setting.
    int maxAliasedPointSize() const { return _maxAliasedPointSize; }
    // Sets at what point size to switch from aliased text (brighter, crisper but may look too jagged and unreadable with some fonts)
    //  to non-aliased text (dimmer, fuzzier but looks better). Zero means always use anti-aliasing. Default is each widget's setting.
    // Here in CompactPatchEdit, this only affects the CompactSliders so far, not the patch label.
    void setMaxAliasedPointSize(int sz);

    const QString& text() const { return _text; }
    void setText(const QString& txt);

    int fontPointMin() const { return _fontPointMin; }
    void setFontPointMin(int point);

    bool fontIgnoreWidth() const { return _fontIgnoreWidth; }
    bool fontIgnoreHeight() const { return _fontIgnoreHeight; }
    void setFontIgnoreDimensions(bool ignoreHeight, bool ignoreWidth = false);

    void setMargins(int hor, int vert);

    virtual QSize sizeHint() const;
};


} // namespace MusEGui

#endif
