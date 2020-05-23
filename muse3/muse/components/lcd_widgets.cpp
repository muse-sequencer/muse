//=========================================================
//  MusE
//  Linux Music Editor
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  lcd_widgets.cpp
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

#include <QPainter>
#include <QBrush>
#include <QColor>
#include <QPalette>
#include <QRect>
#include <QString>

#include <QMouseEvent>
#include <QWheelEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QFontMetrics>
#include <QToolTip>

#include "muse_math.h"

#include "popup_double_spinbox.h"
#include "midictrl.h"
#include "background_painter.h"
#include "lcd_widgets.h"

// #include <stdio.h>

// For debugging output: Uncomment the fprintf section.
#define DEBUG_LCD_WIDGETS(dev, format, args...) // fprintf(dev, format, ##args);

namespace MusEGui {

LCDPainter::LCDPainter()
{
  
}

void LCDPainter::drawCharacter(QPainter* painter, const QRect& rect, char asciiChar)
{
  const int left   = rect.x();
  const int right  = rect.x() + rect.width() - 1;
  const int top    = rect.y();
  const int bottom = rect.y() + rect.height() - 1;
  const int half   = rect.y() + round(double(rect.height()) / 2.0) - 1;

  const int ileft   = left + 1;
  const int iright  = right - 1;
  const int itop    = top + 1;
  const int ibottom = bottom - 1;
  const int ihalfp   = half + 1;
  const int ihalfm   = half - 1;

  switch(asciiChar)
  {
    case 0x30:
      painter->drawLine(left, top, iright, top);
      painter->drawLine(right, top, right, ibottom);
      painter->drawLine(ileft, bottom, right, bottom);
      painter->drawLine(left, itop, left, bottom);
    break;

    case 0x31:
      painter->drawLine(right, top, right, bottom);
    break;

    case 0x32:
      painter->drawLine(left, top, iright, top);
      painter->drawLine(right, top, right, ihalfm);
      painter->drawLine(left, half, right, half);
      painter->drawLine(left, ihalfp, left, bottom);
      painter->drawLine(ileft, bottom, right, bottom);
    break;

    case 0x33:
      painter->drawLine(left, top, iright, top);
      painter->drawLine(right, top, right, ibottom);
      painter->drawLine(left, bottom, right, bottom);
      painter->drawLine(ileft, half, iright, half);
    break;

    case 0x34:
      painter->drawLine(left, top, left, ihalfm);
      painter->drawLine(left, half, iright, half);
      painter->drawLine(right, top, right, bottom);
    break;

    case 0x35:
      painter->drawLine(ileft, top, right, top);
      painter->drawLine(left, top, left, ihalfm);
      painter->drawLine(left, half, right, half);
      painter->drawLine(right, ihalfp, right, bottom);
      painter->drawLine(left, bottom, iright, bottom);
    break;

    case 0x36:
      painter->drawLine(ileft, top, right, top);
      painter->drawLine(left, top, left, bottom);
      painter->drawLine(ileft, bottom, right, bottom);
      painter->drawLine(right, half, right, ibottom);
      painter->drawLine(ileft, half, iright, half);
    break;

    case 0x37:
      painter->drawLine(left, top, iright, top);
      painter->drawLine(right, top, right, bottom);
    break;

    case 0x38:
      painter->drawLine(left, top, left, bottom);
      painter->drawLine(right, top, right, bottom);
      painter->drawLine(ileft, top, iright, top);
      painter->drawLine(ileft, half, iright, half);
      painter->drawLine(ileft, bottom, iright, bottom);
    break;

    case 0x39:
      painter->drawLine(left, top, iright, top);
      painter->drawLine(right, top, right, bottom);
      painter->drawLine(left, bottom, iright, bottom);
      painter->drawLine(left, itop, left, half);
      painter->drawLine(ileft, half, iright, half);
    break;

    case 0x2d: // Minus (dash) '-'
      painter->drawLine(left, half, right, half);
    break;
  }

}

void LCDPainter::drawText(QPainter* painter, const QRect& rect, const QString& text, int flags)
{
  if(text.isEmpty())
    return;

  int sz = rect.height();

  if(sz < 7)
    sz = 7;
  const int sz3 = round(double(sz) / 2.8);

  const int margin = 1 + sz3 / 6;

  const int chars = text.size();

  QRect r;
  int invidx;
  char c;
  int curpos = 0;

  const int y = rect.y();

  // Assume right alignment if unspecified.
  if(flags & Qt::AlignLeft)
  {
    curpos = rect.x();
    for(int idx = 0; idx < chars; ++idx)
    {
      c = text.at(idx).toLatin1();
      r = QRect(curpos, y, sz3, sz);
      drawCharacter(painter, r, c);
      curpos += sz3 + margin;
    }
  }
  else
  {
    curpos = rect.x() + rect.width();
    for(int idx = 0; idx < chars; ++idx)
    {
      invidx = chars - 1 - idx;
      c = text.at(invidx).toLatin1();
      curpos -= sz3 + margin;
      r = QRect(curpos, y, sz3, sz);
      drawCharacter(painter, r, c);
    }
  }
}

//------------------------------------------
//   LCDPatchEdit
//------------------------------------------

LCDPatchEdit::LCDPatchEdit(QWidget* parent,
                         int minFontPoint,
                         bool ignoreHeight, bool ignoreWidth,
                         const QString& text,
                         const QColor& readoutColor,
                         Qt::WindowFlags flags)
    : QFrame(parent, flags),
    _readoutColor(readoutColor),
    _fontPointMin(minFontPoint),
    _fontIgnoreHeight(ignoreHeight),
    _fontIgnoreWidth(ignoreWidth),
    _text(text)
{
  if(objectName().isEmpty())
    setObjectName(QStringLiteral("LCDPatchEdit"));

  setMouseTracking(true);
  setEnabled(true);
  setFocusPolicy(Qt::WheelFocus);
  //setFocusPolicy(Qt::NoFocus);

//   setAutoFillBackground(false);
//   setAttribute(Qt::WA_NoSystemBackground);
//   //setAttribute(Qt::WA_StaticContents);
//   // This is absolutely required for speed! Otherwise painfully slow because of full background
//   //  filling, even when requesting small udpdates! Background is drawn by us.
//   setAttribute(Qt::WA_OpaquePaintEvent);

  _orient = PatchHorizontal;

  _style3d = true;
  _radius = 2;

  _enableValueToolTips = true;
  _editor = nullptr;
  _editMode = false;
  _curEditSection = 0;

  _xMargin = 1;
  _yMargin = 2;
  _sectionSpacing = 4;

  _HBankHovered = false;
  _LBankHovered = false;
  _ProgHovered = false;

  _LCDPainter = new LCDPainter();

  _maxAliasedPointSize = -1;
  _lastValidPatch = _currentPatch = MusECore::CTRL_VAL_UNKNOWN;
  _lastValidHB = _lastValidLB = _lastValidProg = MusECore::CTRL_VAL_UNKNOWN;
  _id = -1;

  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  _curFont = font();
  autoAdjustFontSize();

  setToolTip(tr("High bank: Low bank: Program\n(Ctrl-double-click on/off)"));
}

LCDPatchEdit::~LCDPatchEdit()
{
  if(_LCDPainter)
    delete _LCDPainter;
}

// Static.
QSize LCDPatchEdit::getMinimumSizeHint(const QFontMetrics& fm,
                                       int xMargin,
                                       int yMargin,
                                       PatchOrientation orient
                                      )
{
  const int font_height = fm.height();
  int fin_h = 1;
  int fin_w = 1;
  QRect aRect;
  switch(orient)
  {
    case PatchHorizontal:
      fin_h = font_height + 1 + 2 * yMargin; // +1 for extra anti-aliasing space.
      aRect.setHeight(font_height);
    break;
    case PatchVertical:
      fin_h = 3 * (font_height + 3) + 2 * yMargin; // +1 for extra anti-aliasing space.
      aRect.setHeight(font_height);
    break;
  }

  const int sz2 = charWidth(aRect);
  const int margin = readoutMargin(sz2);

  const int w = 2 * (sz2 + margin) + 1 + margin; // Three digit spaces: two full and one special slim consideration for leading '1'.
  const int spacing = 4;

  switch(orient)
  {
    case PatchHorizontal:
      fin_w = 2 * xMargin + 2 * spacing + 3 * w + 2;
    break;

    case PatchVertical:
      fin_w = w + spacing;
    break;
  }

  return QSize(fin_w, fin_h);
}

void LCDPatchEdit::setReadoutOrientation(PatchOrientation orient)
{
  _orient = orient;
  resize(this->size());
  update();
}

//void LCDPatchEdit::setReadoutColor(const QColor& c)
//{
//  _readoutColor = c;
//  update();
//}

void LCDPatchEdit::setMaxAliasedPointSize(int sz)
{
  if(sz<0)sz=0;
  _maxAliasedPointSize = sz;
  update();
}

void LCDPatchEdit::setMargins(int hor, int vert)
{
  if(hor < 0)
    hor = 0;
  if(vert < 0)
    vert = 0;
  _xMargin = hor;
  _yMargin = vert;
  resize(this->size());
}

QSize LCDPatchEdit::sizeHint() const
{
  return getMinimumSizeHint(fontMetrics(),
                            _xMargin,
                            _yMargin,
                            _orient
                           );
}

void LCDPatchEdit::paintEvent(QPaintEvent* e)
{
  e->ignore();
  QFrame::paintEvent(e);
  e->accept();
  if(rect().width() <= 0 || rect().height() <= 0)
    return;
  QPainter painter(this);
//   painter.setFont(_curFont);
  painter.setFont(font());

  const int sec_spc2 = _sectionSpacing / 2;

  int hb = (_currentPatch >> 16) & 0xff;
  int lb = (_currentPatch >> 8) & 0xff;
  int pr = _currentPatch & 0xff;
  const bool is_unk = _currentPatch == MusECore::CTRL_VAL_UNKNOWN;
  const int hboff = is_unk || hb > 127;
  const int lboff = is_unk || lb > 127;
  const int proff = is_unk || pr > 127;

  const int colon1x = _HBankFieldRect.x() + _HBankFieldRect.width() + sec_spc2 - 1;
  const int colon2x = _LBankFieldRect.x() + _LBankFieldRect.width() + sec_spc2 - 1;
  // Arbitrary: Just use hbank.
  const int colon_y1 = _HBankFieldRect.y() + _HBankFieldRect.height() / 3;
  const int colon_y2 = _HBankFieldRect.y() + 2 * _HBankFieldRect.height() / 3;

  QString hbstr, lbstr, prgstr;
  QColor hbcol, lbcol, prgcol, hbcolaa, lbcolaa, prgcolaa, oncolaa, offcolaa;
  const QPalette& pal = palette();
  QColor offCol(pal.text().color());
  QColor onCol(_readoutColor);
  oncolaa = onCol;
  oncolaa.setAlpha(180);
  offcolaa = offCol;
  offcolaa.setAlpha(180);
  if(!isEnabled())
  {
    const qreal aafactor = 0.75;
    onCol.setHsvF(onCol.hsvHueF(),
                  onCol.hsvSaturationF(),
                  onCol.valueF() * aafactor);
    offCol.setHsv(offCol.hsvHueF(),
                  offCol.hsvSaturationF(),
                  offCol.valueF() * aafactor);
    oncolaa.setHsvF(oncolaa.hsvHueF(),
                    oncolaa.hsvSaturationF(),
                    oncolaa.valueF() * aafactor,
                    oncolaa.alphaF());
    offcolaa.setHsv(offcolaa.hsvHueF(),
                    offcolaa.hsvSaturationF(),
                    offcolaa.valueF() * aafactor,
                    offcolaa.alphaF());
  }

  if(proff)
  {
    prgstr = QString("--");
    prgcol = offCol;
    prgcolaa = offcolaa;
  }
  else
  {
    prgstr = QString::number(pr + 1);
    prgcol = onCol;
    prgcolaa = oncolaa;
  }

  if(hboff)
  {
    hbstr = QString("--");
    hbcol = offCol;
    hbcolaa = offcolaa;
  }
  else
  {
    hbstr = QString::number(hb + 1);
    hbcol = onCol;
    hbcolaa = oncolaa;
  }

  if(lboff)
  {
    lbstr = QString("--");
    lbcol = offCol;
    lbcolaa = offcolaa;
  }
  else
  {
    lbstr = QString::number(lb + 1);
    lbcol = onCol;
    lbcolaa = oncolaa;
  }

  if(_HBankHovered)
  {
    hbcol = hbcol.lighter(135);
    hbcolaa = hbcolaa.lighter(135);
  }
  if(_LBankHovered)
  {
    lbcol = lbcol.lighter(135);
    lbcolaa = lbcolaa.lighter(135);
  }
  if(_ProgHovered)
  {
    prgcol = prgcol.lighter(135);
    prgcolaa = prgcolaa.lighter(135);
  }

  ItemBackgroundPainter ibp;

  switch(_orient)
  {
    case PatchHorizontal:
      painter.setRenderHint(QPainter::Antialiasing, false);
      ibp.drawBackground(&painter, rect(), pal, 1, 1, QRect(),
                         _radius, _style3d, nullptr, _borderColor, _bgColor);

      painter.setPen(offCol);
      painter.drawPoint(colon1x, colon_y1);
      painter.drawPoint(colon1x, colon_y2);

      painter.setPen(offCol);
      painter.drawPoint(colon2x, colon_y1);
      painter.drawPoint(colon2x, colon_y2);
    break;

    case PatchVertical:
      ibp.drawBackground(&painter, _HBankRect, pal);
      ibp.drawBackground(&painter, _LBankRect, pal);
      ibp.drawBackground(&painter, _ProgRect, pal);
    break;
  }

  painter.setPen(hbcolaa);
  painter.setRenderHint(QPainter::Antialiasing, true);
  _LCDPainter->drawText(&painter, _HBankFieldRect, hbstr);
  painter.setPen(hbcol);
  painter.setRenderHint(QPainter::Antialiasing, false);
  _LCDPainter->drawText(&painter, _HBankFieldRect, hbstr);

  painter.setPen(lbcolaa);
      painter.setRenderHint(QPainter::Antialiasing, true);
  _LCDPainter->drawText(&painter, _LBankFieldRect, lbstr);
  painter.setPen(lbcol);
  painter.setRenderHint(QPainter::Antialiasing, false);
  _LCDPainter->drawText(&painter, _LBankFieldRect, lbstr);

  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setPen(prgcolaa);
  _LCDPainter->drawText(&painter, _ProgFieldRect, prgstr);
  painter.setPen(prgcol);
  painter.setRenderHint(QPainter::Antialiasing, false);
  _LCDPainter->drawText(&painter, _ProgFieldRect, prgstr);
}

int LCDPatchEdit::value() const
{
  return _currentPatch;
}

void LCDPatchEdit::setValue(int v)
{
  if(_currentPatch == v)
    return;
  _currentPatch = v;
  update();
}

void LCDPatchEdit::setLastValidPatch(int v)
{
  if(_lastValidPatch == v)
    return;
  _lastValidPatch = v;
  //update(); // Not required.
}

void LCDPatchEdit::setLastValidBytes(int hbank, int lbank, int prog)
{
  if(_lastValidHB != hbank)
  {
    _lastValidHB   = hbank;
    //update(); // Not required.
  }

  if(_lastValidLB != lbank)
  {
    _lastValidLB   = lbank;
    //update(); // Not required.
  }

  if(_lastValidProg != prog)
  {
    _lastValidProg = prog;
    //update(); // Not required.
  }
}

//---------------------------------------------------------
//   autoAdjustFontSize
//   w: Widget to auto adjust font size
//   s: String to fit
//   ignoreWidth: Set if dealing with a vertically constrained widget - one which is free to resize horizontally.
//   ignoreHeight: Set if dealing with a horizontally constrained widget - one which is free to resize vertically.
//---------------------------------------------------------

bool LCDPatchEdit::autoAdjustFontSize()
{
// FIXME: Disabled for now, the font modulates back and forth, not very good ATM.
//        May have to revert to the font-checking iteration loop scheme.

//   QFont fnt = font(); // This is the maximum font.
//   int max = fnt.pointSize();
//   int min = _fontPointMin;
//
//   // In case the max or min was obtained from QFont::pointSize() which returns -1
//   //  if the font is a pixel font, or if min is greater than max...
//   // Limit the minimum and maximum sizes to something at least readable.
//   if(max < 4)
//     max = 4;
//   if(min < 4)
//     min = 4;
//   if(max < min)
//     max = min;
//
//   //qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());
//   //QRectF r = boundingRect();
//   QRectF r = rect();
//   //QFont f = painter->font();
//
//
//   //if(ignoreWidth || req_w == 0) // Also avoid divide by zero below.
//   if(_fontIgnoreWidth || _text.isEmpty()) // Also avoid divide by zero below.
//   {
//     if(fnt.pointSize() != max)
//     {
//       fnt.setPointSize(max);
// //       setFont(fnt);
//       _curFont = fnt;
//       update();
//     }
//   }
//   else
//   {
//     //qreal aspectRatio = painter->fontMetrics().lineSpacing() / painter->fontMetrics().averageCharWidth();
//     qreal aspectRatio = fontMetrics().lineSpacing() / fontMetrics().averageCharWidth();
// //     int pixelsize = sqrt(r.width() * r.height() / aspectRatio / (_text.length() * 3)) * aspectRatio;
//     int pixelsize = sqrt(r.width() * r.height() / aspectRatio / _text.length()) * aspectRatio;
//     fnt.setPixelSize(pixelsize);
//     //int flags = Qt::AlignCenter|Qt::TextDontClip|Qt::TextWordWrap;
//     int flags = Qt::AlignCenter;
//     //if ((pixelsize * lod) < 13)
//     //    flags |= Qt::TextWrapAnywhere;
//     QFontMetricsF fmf(fnt);
//     QRectF tbr = fmf.boundingRect(r, flags, _text);
//     pixelsize = fnt.pixelSize() * qMin(r.width() * 0.95 / tbr.width(), r.height() * 0.95 / tbr.height());
// //     if(pixelsize < min)
// //       pixelsize = min;
// //     else if(pixelsize > max)
// //       pixelsize = max;
//     fnt.setPixelSize(pixelsize);
//     const QFontInfo fi(fnt);
//     const int pointsize = fi.pointSize();
//     if(pointsize <= min)
//       fnt.setPointSize(min);
//     else if(pointsize >= max)
//       fnt.setPointSize(max);
// //     setFont(fnt);
//     _curFont = fnt;
//     //painter->drawText(r,flags,stitle);
//     update();
//   }

  // Force minimum height. Use the expected height for the highest given point size.
  // This way the mixer strips aren't all different label heights, but can be larger if necessary.
  // Only if ignoreHeight is set (therefore the height is adjustable).
//   if(_fontIgnoreHeight)
//   {
// // FIXME Disabled for now, as per above.
// //     fnt.setPointSize(max);
// //     const QFontMetrics fm(fnt);
//     const QFontMetrics fm(font());
//
//     // Set the label's minimum height equal to the height of the font.
//     setMinimumHeight(fm.height() + 2 * frameWidth());
//   }
//
  return true;
}

void LCDPatchEdit::setText(const QString& txt)
{
  if(_text == txt)
    return;
  _text = txt;
  autoAdjustFontSize();
  update();
}

QRect LCDPatchEdit::activeDrawingArea() const
{
  // +1 for extra anti-aliasing space.
  return rect().adjusted(_xMargin, _yMargin + 1, -_xMargin, -_yMargin);
}

// Static.
int LCDPatchEdit::charWidth(const QRect& aRect)
{
  int sz = aRect.height();
  if(sz < 7)
    sz = 7;
  return round(double(sz) / 2.8);
}

// Static.
int LCDPatchEdit::readoutMargin(int charWidth)
{
  return 1 + charWidth / 6;
}

void LCDPatchEdit::resizeEvent(QResizeEvent* e)
{
  e->ignore();
  QFrame::resizeEvent(e);
  e->accept();
  autoAdjustFontSize();

  const QFontMetrics fm = fontMetrics();
  const int font_height = fm.height();

  QRect aRect = activeDrawingArea();
  switch(_orient)
  {
    case PatchHorizontal:
    break;

    case PatchVertical:
      aRect.setHeight(font_height);
    break;
  }

  const int sz2 = charWidth(aRect);
  const int margin = readoutMargin(sz2);

  const int rectw3 = (aRect.width() - margin * 2) / 3;
  const int w = 2 * (sz2 + margin) + 1 + margin; // Three digit spaces: two full and one special slim consideration for leading '1'.
  int spacing = rectw3 - w;
  if(spacing < 4)
    spacing = 4;
  if(spacing > 16)
    spacing = 16;
  _sectionSpacing = spacing;
  const int indent = aRect.width() / 2 - w / 2;

  int x0, x1, x2;
  int recty1 = 0, recty2 = 0, recty3 = 0, recth = 0;
  int frecty1, frecty2, frecty3, frecth;

  switch(_orient)
  {
    case PatchHorizontal:
      x0 = aRect.x() + _xMargin;
      x1 = x0 + w + _sectionSpacing;
      x2 = x1 + w + _sectionSpacing;

      recty1 = recty2 = recty3 = rect().y() + _yMargin;
      recth = rect().height() - 2 * _yMargin;

      frecty1 = frecty2 = frecty3 = aRect.y();
      frecth = aRect.height();


      _HBankFieldRect.setRect(x0, frecty1, w, frecth);
      _LBankFieldRect.setRect(x1, frecty2, w, frecth);
      _ProgFieldRect.setRect (x2, frecty3, w, frecth);
    break;

    case PatchVertical:
      x0 = x1 = x2 = aRect.x() + indent; // + _xMargin;

      recth = font_height + 3;
      recty1 = rect().y() + _yMargin;
      recty2 = recty1 + recth;
      recty3 = recty2 + recth;

      frecth = font_height;
      frecty1 = aRect.y() + 1;
      frecty2 = frecty1 + frecth + 3;
      frecty3 = frecty2 + frecth + 3;

      _HBankFieldRect.setRect(x0 + _sectionSpacing / 2 + _xMargin, frecty1, w, frecth);
      _LBankFieldRect.setRect(x1 + _sectionSpacing / 2 + _xMargin, frecty2, w, frecth);
      _ProgFieldRect.setRect (x2 + _sectionSpacing / 2 + _xMargin, frecty3, w, frecth);
    break;
  }

  _HBankRect.setRect(x0, recty1, w + _sectionSpacing, recth);
  _LBankRect.setRect(x1, recty2, w + _sectionSpacing, recth);
  _ProgRect.setRect (x2, recty3, w + _sectionSpacing, recth);

  update();
}

void LCDPatchEdit::mouseMoveEvent(QMouseEvent *e)
{
  //fprintf(stderr, "LCDPatchEdit::mouseMoveEvent\n");
  e->ignore();
  QFrame::mouseMoveEvent(e);
  e->accept();

  QPoint p = e->pos();

  bool doupd = false;

  if(_HBankRect.contains(p) != _HBankHovered)
  {
    _HBankHovered = !_HBankHovered;
    doupd = true;
  }

  if(_LBankRect.contains(p) != _LBankHovered)
  {
    _LBankHovered = !_LBankHovered;
    doupd = true;
  }

  if(_ProgRect.contains(p) != _ProgHovered)
  {
    _ProgHovered = !_ProgHovered;
    doupd = true;
  }

  if(doupd)
    update();
}

void LCDPatchEdit::mousePressEvent(QMouseEvent* e)
{
  Qt::MouseButtons buttons = e->buttons();
  e->accept();
  emit pressed(e->pos(), _id, buttons, e->modifiers());

  if(buttons == Qt::RightButton)
    emit rightClicked(e->globalPos(), _id);
}

void LCDPatchEdit::mouseReleaseEvent(QMouseEvent* e)
{
  e->accept();
  emit released(e->pos(), _id, e->buttons(), e->modifiers());
}

void LCDPatchEdit::enterEvent(QEvent *e)
{
  //fprintf(stderr, "LCDPatchEdit::enterEvent\n");
  QPoint p = mapFromGlobal(cursor().pos());

  bool doupd = false;

  if(_HBankRect.contains(p) != _HBankHovered)
  {
    _HBankHovered = !_HBankHovered;
    doupd = true;
  }

  if(_LBankRect.contains(p) != _LBankHovered)
  {
    _LBankHovered = !_LBankHovered;
    doupd = true;
  }

  if(_ProgRect.contains(p) != _ProgHovered)
  {
    _ProgHovered = !_ProgHovered;
    doupd = true;
  }

  e->ignore();
  QFrame::enterEvent(e);
  e->accept();
  if(doupd)
    update();
}

void LCDPatchEdit::leaveEvent(QEvent *e)
{
  //fprintf(stderr, "LCDPatchEdit::leaveEvent\n");
  bool doupd = false;

  if(_HBankHovered)
  {
    _HBankHovered = false;
    doupd = true;
  }
  if(_LBankHovered)
  {
    _LBankHovered = false;
    doupd = true;
  }

  if(_ProgHovered)
  {
    _ProgHovered = false;
    doupd = true;
  }

  e->ignore();
  QFrame::leaveEvent(e);
  e->accept();
  if(doupd)
    update();
}




void LCDPatchEdit::setFontIgnoreDimensions(bool ignoreHeight, bool ignoreWidth)
{
  _fontIgnoreWidth = ignoreWidth;
  _fontIgnoreHeight = ignoreHeight;
  autoAdjustFontSize();
  update();
}

void LCDPatchEdit::setFontPointMin(int point)
{
  _fontPointMin = point;
  autoAdjustFontSize();
  update();
}

void LCDPatchEdit::editorReturnPressed()
{
  DEBUG_LCD_WIDGETS(stderr, "LCDPatchEdit::editorReturnPressed\n");
  _editMode = false;
  if(_editor)
  {
    int hb = (_currentPatch >> 16) & 0xff;
    int lb = (_currentPatch >> 8) & 0xff;
    int pr = _currentPatch & 0xff;
    const bool is_unk = _currentPatch == MusECore::CTRL_VAL_UNKNOWN;
    const int lasthb = (_lastValidPatch >> 16) & 0xff;
    const int lastlb = (_lastValidPatch >> 8) & 0xff;
    const int lastpr = _lastValidPatch & 0xff;
    const bool last_is_unk = _lastValidPatch == MusECore::CTRL_VAL_UNKNOWN;
    int new_val = _currentPatch;

    switch(_curEditSection)
    {
      case HBankSection:
        hb = _editor->value();
        if(hb == 0)
          hb = 0xff;
        else
        {
          --hb;
          if(is_unk)
          {
            lb = lastlb;
            pr = lastpr;
            if(last_is_unk)
            {
              lb = 0xff;
              pr = 0;
            }
          }
        }
        new_val = ((hb & 0xff) << 16) | ((lb & 0xff) << 8) | (pr & 0xff);
      break;

      case LBankSection:
        lb = _editor->value();
        if(lb == 0)
          lb = 0xff;
        else
        {
          --lb;
          if(is_unk)
          {
            hb = lasthb;
            pr = lastpr;
            if(last_is_unk)
            {
              hb = 0xff;
              pr = 0;
            }
          }
        }
        new_val = ((hb & 0xff) << 16) | ((lb & 0xff) << 8) | (pr & 0xff);
      break;

      case ProgSection:
        pr = _editor->value();
        if(pr == 0)
          new_val = MusECore::CTRL_VAL_UNKNOWN;
        else
        {
          --pr;
          if(is_unk)
          {
            hb = lasthb;
            lb = lastlb;
            if(last_is_unk)
            {
              hb = 0xff;
              lb = 0xff;
            }
          }
          new_val = ((hb & 0xff) << 16) | ((lb & 0xff) << 8) | (pr & 0xff);
        }
      break;
    }

    if(value() != new_val)
    {
      setValue(new_val);
      emit valueChanged(value(), _id);
    }

    _editor->deleteLater();
    _editor = 0;
  }
  setFocus(); // FIXME There are three sections. Need to clear focus for now.
//   clearFocus();
}

void LCDPatchEdit::editorEscapePressed()
{
  DEBUG_LCD_WIDGETS(stderr, "LCDPatchEdit::editorEscapePressed\n");
  _editMode = false;
  if(_editor)
  {
    _editor->deleteLater();
    _editor = 0;
    setFocus(); //FIXME There are three sections. Need to clear focus for now.
//     clearFocus();
  }
}

void LCDPatchEdit::keyPressEvent(QKeyEvent* e)
{
  switch (e->key())
  {
    case Qt::Key_Return:
    case Qt::Key_Enter:
    {
      // A disabled spinbox up or down button will pass the event to the parent! Causes pseudo 'wrapping'. Eat it up.
      if(!_editor || !_editor->hasFocus())
        showEditor();
      e->accept();
      return;
    }
    break;

    default:
    break;
  }

  e->ignore();
  QFrame::keyPressEvent(e);
}

bool LCDPatchEdit::event(QEvent* e)
{
  switch(e->type())
  {
    // FIXME: Doesn't work.
    case QEvent::NonClientAreaMouseButtonPress:
      DEBUG_LCD_WIDGETS(stderr, "LCDPatchEdit::event NonClientAreaMouseButtonPress\n");
      e->accept();
      _editMode = false;
      if(_editor)
      {
        _editor->deleteLater();
        _editor = 0;
      }
      return true;
    break;

    default:
    break;
  }

  return QFrame::event(e);
}

void LCDPatchEdit::wheelEvent(QWheelEvent* e)
{
  QPoint p = e->pos();

  bool doupd = false;

  if(_HBankRect.contains(p) != _HBankHovered)
  {
    _HBankHovered = !_HBankHovered;
    doupd = true;
  }

  if(_LBankRect.contains(p) != _LBankHovered)
  {
    _LBankHovered = !_LBankHovered;
    doupd = true;
  }

  if(_ProgRect.contains(p) != _ProgHovered)
  {
    _ProgHovered = !_ProgHovered;
    doupd = true;
  }

  if(doupd)
    update();

  int hb = (_currentPatch >> 16) & 0xff;
  int lb = (_currentPatch >> 8) & 0xff;
  int pr = _currentPatch & 0xff;
  const bool is_unk = _currentPatch == MusECore::CTRL_VAL_UNKNOWN;
  const int hboff = is_unk || hb > 127;
  const int lboff = is_unk || lb > 127;
  const int proff = is_unk || pr > 127;

  const int lasthb = (_lastValidPatch >> 16) & 0xff;
  const int lastlb = (_lastValidPatch >> 8) & 0xff;
  const int lastpr = _lastValidPatch & 0xff;
  const bool last_is_unk = _lastValidPatch == MusECore::CTRL_VAL_UNKNOWN;

  const int lasthboff = last_is_unk || lasthb > 127;
  const int lastlboff = last_is_unk || lastlb > 127;
  const int lastproff = last_is_unk || lastpr > 127;

//   const QPoint pixelDelta = e->pixelDelta();
  const QPoint angleDegrees = e->angleDelta() / 8;
  int delta = 0;
//   if(!pixelDelta.isNull())
//     delta = pixelDelta.y();
//   else
  if(!angleDegrees.isNull())
    delta = angleDegrees.y() / 15;

  int section = -1;
  int new_val = _currentPatch;

  if(_HBankHovered)
  {
    section = HBankSection;
    if(delta > 0 || !hboff)
    {
      if(hboff)
      {
        hb = _lastValidHB;
        if(lasthboff)
          hb = 0;
        if(is_unk)
        {
          lb = lastlb;
          pr = lastpr;
          if(last_is_unk)
          {
            lb = 0xff;
            pr = 0;
          }
        }
      }
      else
      {
        hb += delta;
        if(hb < 0)
          hb = 0xff;
        else if(hb > 127)
          hb = 127;
      }
      new_val = ((hb & 0xff) << 16) | ((lb & 0xff) << 8) | (pr & 0xff);
    }
  }
  else if(_LBankHovered)
  {
    section = LBankSection;
    if(delta > 0 || !lboff)
    {
      if(lboff)
      {
        lb = _lastValidLB;
        if(lastlboff)
          lb = 0;
        if(is_unk)
        {
          hb = lasthb;
          pr = lastpr;
          if(last_is_unk)
          {
            hb = 0xff;
            pr = 0;
          }
        }
      }
      else
      {
        lb += delta;
        if(lb < 0)
          lb = 0xff;
        else if(lb > 127)
          lb = 127;
      }
      new_val = ((hb & 0xff) << 16) | ((lb & 0xff) << 8) | (pr & 0xff);
    }
  }
  else if(_ProgHovered)
  {
    section = ProgSection;
    if(delta > 0 || !proff)
    {
      if(proff)
      {
        pr = _lastValidProg;
        if(lastproff)
          pr = 0;
        if(is_unk)
        {
          hb = lasthb;
          lb = lastlb;
          if(last_is_unk)
          {
            hb = 0xff;
            lb = 0xff;
          }
        }
        new_val = ((hb & 0xff) << 16) | ((lb & 0xff) << 8) | (pr & 0xff);
      }
      else
      {
        pr += delta;
        if(pr < 0)
          new_val = MusECore::CTRL_VAL_UNKNOWN;
        else
        {
          if(pr > 127)
            pr = 127;
          new_val = ((hb & 0xff) << 16) | ((lb & 0xff) << 8) | (pr & 0xff);
        }
      }
    }
  }
  else
  {
    e->ignore();
    return QFrame::wheelEvent(e);
  }

  e->accept();

  if(value() != new_val)
  {
    setValue(new_val);
    // Show a handy tooltip value box.
    if(_enableValueToolTips)
      showValueToolTip(e->globalPos(), section);
    emit valueChanged(value(), _id);
  }
  //fprintf(stderr, "LCDPatchEdit::wheelEvent _HBankHovered:%d _LBankHovered:%d _ProgHovered:%d\n", _HBankHovered, _LBankHovered, _ProgHovered);
}

void LCDPatchEdit::mouseDoubleClickEvent(QMouseEvent* e)
{
  const Qt::MouseButtons buttons = e->buttons();
  const Qt::KeyboardModifiers keys = e->modifiers();

  if(buttons == Qt::LeftButton && !_editMode)
  {
    DEBUG_LCD_WIDGETS(stderr, "   left button\n");
    if(keys == Qt::ControlModifier)
    {
      if(_HBankHovered || _LBankHovered || _ProgHovered)
      {
        int hb = (_currentPatch >> 16) & 0xff;
        int lb = (_currentPatch >> 8) & 0xff;
        int pr = _currentPatch & 0xff;
        const bool is_unk = _currentPatch == MusECore::CTRL_VAL_UNKNOWN;
        const int hboff = is_unk || hb > 127;
        const int lboff = is_unk || lb > 127;
        const int proff = is_unk || pr > 127;
        const int lasthb = (_lastValidPatch >> 16) & 0xff;
        const int lastlb = (_lastValidPatch >> 8) & 0xff;
        const int lastpr = _lastValidPatch & 0xff;
        const bool last_is_unk = _lastValidPatch == MusECore::CTRL_VAL_UNKNOWN;
        const bool last_hb_is_unk = _lastValidHB   == MusECore::CTRL_VAL_UNKNOWN;
        const bool last_lb_is_unk = _lastValidLB   == MusECore::CTRL_VAL_UNKNOWN;
        const bool last_pr_is_unk = _lastValidProg == MusECore::CTRL_VAL_UNKNOWN;
        int new_val = _currentPatch;

        if(_HBankHovered)
        {
          if(hboff)
          {
            hb = _lastValidHB;
            if(last_hb_is_unk)
              hb = 0;
            if(is_unk)
            {
              lb = lastlb;
              pr = lastpr;
              if(last_is_unk)
              {
                lb = 0xff;
                pr = 0;
              }
            }
          }
          else
            hb = 0xff;
          new_val = ((hb & 0xff) << 16) | ((lb & 0xff) << 8) | (pr & 0xff);
        }
        else if(_LBankHovered)
        {
          if(lboff)
          {
            lb = _lastValidLB;
            if(last_lb_is_unk)
              lb = 0;
            if(is_unk)
            {
              hb = lasthb;
              pr = lastpr;
              if(last_is_unk)
              {
                hb = 0xff;
                pr = 0;
              }
            }
          }
          else
            lb = 0xff;
          new_val = ((hb & 0xff) << 16) | ((lb & 0xff) << 8) | (pr & 0xff);
        }
        else if(_ProgHovered)
        {
          if(proff)
          {
            pr = _lastValidProg;
            if(last_pr_is_unk)
              pr = 0;
            if(is_unk)
            {
              hb = lasthb;
              lb = lastlb;
              if(last_is_unk)
              {
                hb = 0xff;
                lb = 0xff;
              }
            }
            new_val = ((hb & 0xff) << 16) | ((lb & 0xff) << 8) | (pr & 0xff);
          }
          else
            new_val = MusECore::CTRL_VAL_UNKNOWN;
        }

        if(new_val != value())
        {
          setValue(new_val);
          emit valueChanged(value(), id());
        }
        e->accept();
        return;
      }
    }
    // A disabled spinbox up or down button will pass the event to the parent! Causes pseudo 'wrapping'. Eat it up.
    else if(keys == Qt::NoModifier && (!_editor || !_editor->hasFocus()))
    {
      int sec = -1;
      if(_HBankHovered)
        sec = HBankSection;
      else if(_LBankHovered)
        sec = LBankSection;
      else if(_ProgHovered)
        sec = ProgSection;

      if(sec != -1)
      {
        _curEditSection = sec;
        showEditor();
        e->accept();
        return;
      }
    }
  }

  e->ignore();
  QFrame::mouseDoubleClickEvent(e);
}

void LCDPatchEdit::showEditor()
{
  if(_editMode)
    return;

  if(!_editor)
  {
    DEBUG_LCD_WIDGETS(stderr, "   creating editor\n");
    _editor = new PopupDoubleSpinBox(this);
    _editor->setFrame(false);
    _editor->setFocusPolicy(Qt::WheelFocus);
    _editor->setDecimals(0);
    _editor->setSpecialValueText(tr("off"));
    _editor->setMinimum(0);
    _editor->setMaximum(128);
    connect(_editor, SIGNAL(returnPressed()), SLOT(editorReturnPressed()));
    connect(_editor, SIGNAL(escapePressed()), SLOT(editorEscapePressed()));
  }
  int w = width();
  //if (w < _editor->sizeHint().width())
  //  w = _editor->sizeHint().width();
  QRect r;
  const bool is_unk = _currentPatch == MusECore::CTRL_VAL_UNKNOWN;
  switch(_curEditSection)
  {
    case HBankSection:
    {
      r = _HBankRect;
      int hb = (_currentPatch >> 16) & 0xff;
      if(is_unk || hb < 0 || hb > 127)
        hb = 0;
      else
        ++hb;
      _editor->setValue(hb);
    }
    break;

    case LBankSection:
    {
      r = _LBankRect;
      int lb = (_currentPatch >> 8) & 0xff;
      if(is_unk || lb < 0 || lb > 127)
        lb = 0;
      else
        ++lb;
      _editor->setValue(lb);
    }
    break;

    case ProgSection:
    {
      r = _ProgRect;
      int pr = _currentPatch & 0xff;
      if(is_unk || pr < 0 || pr > 127)
        pr = 0;
      else
        ++pr;
      _editor->setValue(pr);
    }
    break;
  }

  switch(_orient)
  {
    case PatchHorizontal:
      _editor->setGeometry(0, 0, w, height());
    break;

    case PatchVertical:
      _editor->setGeometry(0, r.y(), w, r.height());
    break;
  }

  DEBUG_LCD_WIDGETS(stderr, "   x:%d y:%d w:%d h:%d\n", _editor->x(), _editor->y(), w, _editor->height());
  _editor->selectAll();
  _editMode = true;
  _editor->show();
  _editor->setFocus();
}

QString LCDPatchEdit::toolTipValueText(int section) const
{
  const int hb = (_currentPatch >> 16) & 0xff;
  const int lb = (_currentPatch >> 8) & 0xff;
  const int pr = _currentPatch & 0xff;
  const bool is_unk = _currentPatch == MusECore::CTRL_VAL_UNKNOWN;
  const int hboff = is_unk || hb > 127;
  const int lboff = is_unk || lb > 127;
  const int proff = is_unk || pr > 127;

  const QString off_text = tr("off");
  const QString hb_desc_text = tr("High bank");
  const QString lb_desc_text = tr("Low bank");
  const QString pr_desc_text = tr("Program");
  const QString hbtext = hboff ? off_text : QString::number(hb + 1);
  const QString lbtext = lboff ? off_text : QString::number(lb + 1);
  const QString prtext = proff ? off_text : QString::number(pr + 1);

  switch(section)
  {
    case HBankSection:
      return QString("%1: %2")
                    .arg(hb_desc_text).arg(hbtext);
    break;

    case LBankSection:
      return QString("%1: %2")
                    .arg(lb_desc_text).arg(lbtext);
    break;

    case ProgSection:
      return QString("%1: %2")
                    .arg(pr_desc_text).arg(prtext);
    break;

    default:
      return QString("%1: %2\n%3: %4\n%5: %6")
                    .arg(hb_desc_text).arg(hbtext)
                    .arg(lb_desc_text).arg(lbtext)
                    .arg(pr_desc_text).arg(prtext);
    break;
  }
}

void LCDPatchEdit::showValueToolTip(QPoint /*p*/, int section)
{
  const QString txt = toolTipValueText(section);
  if(!txt.isEmpty())
  {
    QToolTip::showText(mapToGlobal(pos()), txt, 0, QRect(), 3000);
  }
}


} // namespace MusEGui
