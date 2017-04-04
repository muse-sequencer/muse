//=============================================================================
//  MusE
//  Linux Music Editor
//  pixmap_button.cpp
//  (C) Copyright 2011 Tim E. Real (terminator356 on users.sourceforge.net)
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
//=============================================================================

#include <QString>
#include <QWidget>
#include <QSize>
#include <QFont>
#include <QPainter>
#include <QPixmap>
#include <QIcon>
#include <QPaintEvent>
#include <QMouseEvent>

#include "pixmap_button.h"

namespace MusEGui {
  
//------------------------------------------
//   PixmapButton
//------------------------------------------

PixmapButton::PixmapButton(QWidget* parent)
             : QWidget(parent)
{
  _onPixmap = 0;
  _offPixmap = 0;
  _margin = 0;
  _checked = false;
  _checkable = false;
    
  QFont fnt = font();
  fnt.setPointSize(8);
  setFont(fnt);
}

PixmapButton::PixmapButton(QPixmap* on_pixmap, QPixmap* off_pixmap, int margin, QWidget* parent, const QString& text) 
             : QWidget(parent)
{
  _text = text;
  _onPixmap = on_pixmap;
  _offPixmap = off_pixmap;
  _margin = margin;
  _checked = false;
  _checkable = false;
  if(_offPixmap)
    setMinimumSize(_offPixmap->size().width() + 2*_margin, _offPixmap->size().height() + 2*_margin);
  else
    setMinimumSize(10 + 2*_margin, 10 + 2*_margin);
  //font().s
    
  QFont fnt = font();
  fnt.setPointSize(8);
  setFont(fnt);
}

QSize PixmapButton::minimumSizeHint () const
{
  return QSize(10, 10);
}

void PixmapButton::setMargin(int v) 
{ 
  _margin = v; 
  if(_offPixmap)
    setMinimumSize(_offPixmap->size().width() + 2*_margin, _offPixmap->size().height() + 2*_margin);
  update(); 
}

void PixmapButton::setOffPixmap(QPixmap* pm)
{
  _offPixmap = pm;
  if(_offPixmap)
    setMinimumSize(_offPixmap->size().width() + 2*_margin, _offPixmap->size().height() + 2*_margin);
  else
    setMinimumSize(10 + 2*_margin, 10 + 2*_margin);
  update();
}

void PixmapButton::setOnPixmap(QPixmap* pm)
{
  _onPixmap = pm;
  update();
}

void PixmapButton::setCheckable(bool v)
{
  _checkable = v;
  if(!_checkable)
    _checked = false;
  update();
}

void PixmapButton::setChecked(bool v)
{
  if(!_checkable)
    return;
  if(_checked == v)
    return;
  _checked = v;
  update();
  emit toggled(_checked);
}

void PixmapButton::setDown(bool v)
{
  if(!_checkable)
    return;
  if(_checked == v)
    return;
  _checked = v;
  update();
}

void PixmapButton::paintEvent(QPaintEvent* e)
{
    QWidget::paintEvent(e);
    QPainter p(this);
    int w2 = width() / 2;
    int h2 = height() / 2;
    int mw = _offPixmap->size().width();
    int mh = _offPixmap->size().height();
    int mw2 = mw / 2;
    int mh2 = mh / 2;
    if(!_text.isEmpty())
      //p.drawText(w2 - mw2, h2 - mh2, mw, mh, *pm);
      p.drawText(_margin, height() - _margin, _text);
    else
    {  
      QPixmap* pm = _checked ? _onPixmap : _offPixmap;  
      if(pm)
        p.drawPixmap(w2 - mw2, h2 - mh2, mw, mh, *pm);
    }  
}

void PixmapButton::mousePressEvent(QMouseEvent* e)
{
  if(_checkable)
    _checked = !_checked;
  update();
  
  emit pressed();
  if(_checkable)
    emit toggled(_checked);
  
  //e->setAccepted(true);   // This makes menu not close when mouse is released. May be desireable with many small buttons... 
  QWidget::mousePressEvent(e);   // Hm, need this so menus can close.
}

void PixmapButton::mouseReleaseEvent(QMouseEvent* e)
{
  emit clicked(_checked);
  
  //e->setAccepted(true);   // This makes menu not close when mouse is released. May be desireable with many small buttons... 
  QWidget::mouseReleaseEvent(e); // Hm, need this so menus can close.
}

void PixmapButton::contextMenuEvent(QContextMenuEvent * e)
{
  e->accept();
}


//------------------------------------------
//   IconButton
//------------------------------------------

IconButton::IconButton(QWidget* parent, const char* name)
             : QWidget(parent)
{
  setObjectName(name);
  _blinkPhase = false;
  _iconSetB = false;
  _iconSize = QSize(16, 16);
  _onIcon = 0;
  _offIcon = 0;
  _onIconB = 0;
  _offIconB = 0;
  _margin = 0;
  _checked = false;
  _checkable = false;
}

IconButton::IconButton(QIcon* on_icon, QIcon* off_icon, QIcon* on_iconB, QIcon* off_iconB,
                       bool hasFixedIconSize, bool drawFlat,
                       const QString& text, int margin, QWidget* parent, const char* name)
             : QWidget(parent),
               _onIcon(on_icon), _offIcon(off_icon), _onIconB(on_iconB), _offIconB(off_iconB),
               _hasFixedIconSize(hasFixedIconSize), _drawFlat(drawFlat), _text(text), _margin(margin)
{
  setObjectName(name);
  _blinkPhase = false;
  _iconSetB = false;
  _checked = false;
  _checkable = false;
  _iconSize = QSize(16, 16);
}

QSize IconButton::sizeHint() const
{
  // TODO Ask style for margins.
  const QSize isz = iconSize();
  const int fmh = fontMetrics().lineSpacing() + 5;

  const int iw = isz.width() + 2;
  const int ih = isz.height() + 2;

  const int h = (_hasFixedIconSize && ih > fmh) ? ih : fmh;
  const int w = (_hasFixedIconSize && iw > h) ? iw : h + 2;

  return QSize(w, h);
}

QSize IconButton::minimumSizeHint () const
{
  return QSize(10, 10);
}

void IconButton::setText(QString txt)
{
  _text = txt;
  updateGeometry();
}

void IconButton::setIconSize(const QSize sz)
{
  _iconSize = sz;
  updateGeometry();
}

void IconButton::setMargin(int v)
{
  _margin = v;
  update();
}

void IconButton::setOffIcon(QIcon* pm)
{
  _offIcon = pm;
  update();
}

void IconButton::setOnIcon(QIcon* pm)
{
  _onIcon = pm;
  update();
}

void IconButton::setOffIconB(QIcon* pm)
{
  _offIconB = pm;
  update();
}

void IconButton::setOnIconB(QIcon* pm)
{
  _onIconB = pm;
  update();
}

void IconButton::setIconSetB(bool v)
{
  _iconSetB = v;
  update();
}

void IconButton::setCheckable(bool v)
{
  _checkable = v;
  if(!_checkable)
    _checked = false;
  update();
}

void IconButton::setChecked(bool v)
{
  if(!_checkable)
    return;
  if(_checked == v)
    return;
  _checked = v;
  update();
  emit toggled(_checked);
}

void IconButton::setDown(bool v)
{
  if(!_checkable)
    return;
  if(_checked == v)
    return;
  _checked = v;
  update();
}

void IconButton::paintEvent(QPaintEvent* ev)
{
//   if(!_drawFlat)
//     QToolButton::paintEvent(ev);

  QIcon::Mode mode;
  if(isEnabled())
    mode = hasFocus() ? QIcon::Selected : QIcon::Normal;
  else
    mode = QIcon::Disabled;
  QIcon::State state = (isChecked() && (!_blinkPhase || !isEnabled())) ? QIcon::On : QIcon::Off;

  QIcon* ico = 0;
  QPainter p(this);
  if(!_text.isEmpty())
    //p.drawText(w2 - mw2, h2 - mh2, mw, mh, *pm);
    p.drawText(_margin, height() - _margin, _text);
  else
  {
    if(_iconSetB)
      ico = _checked ? _onIconB : _offIconB;
    else
      ico = _checked ? _onIcon : _offIcon;

    if(ico)
      ico->paint(&p, rect(), Qt::AlignCenter, mode, state);
  }

// TODO Bah! Just want a mouse-over rectangle for flat mode but some styles do this or that but not the other thing.
//   if(const QStyle* st = style())
//   {
//     st = st->proxy();
// //     QStyleOptionToolButton o;
// //     initStyleOption(&o);
// //     o.rect = rect();
// //     //o.state |= QStyle::State_MouseOver;
// //     o.state = QStyle::State_Active |
// //               QStyle::State_Enabled |
// //               QStyle::State_AutoRaise | // This is required to get rid of the panel.
// //               QStyle::State_MouseOver;
// //     st->drawPrimitive(QStyle::PE_PanelButtonTool, &o, &p);
//
// //     QStyleOptionFrame o;
// //     //initStyleOption(&o);
// //     o.rect = rect();
// //     o.features = QStyleOptionFrame::Rounded;
// //     o.frameShape = QFrame::Box;
// //     o.lineWidth = 2;
// //     o.midLineWidth = 4;
// //     o.state |= QStyle::State_MouseOver;
// //     st->drawPrimitive(QStyle::PE_Frame, &o, &p);
//
//
//     QStyleOptionFocusRect o;
//     //o.QStyleOption::operator=(option);
//     //o.rect = st->subElementRect(QStyle::SE_ItemViewItemFocusRect, &option);
//     o.rect = rect();
//     o.state |= QStyle::State_KeyboardFocusChange;
//     o.state |= QStyle::State_Item |
//                QStyle::State_Active |
//                QStyle::State_Enabled |
//                QStyle::State_HasFocus |
//
//                //QStyle::State_Raised |
//                QStyle::State_Sunken |
//
//                QStyle::State_Off |
//                //QStyle::State_On |
//
//                QStyle::State_Selected |
//
//                //QStyle::State_AutoRaise | // This is required to get rid of the panel.
//
//                QStyle::State_MouseOver;
//
// //     QPalette::ColorGroup cg =
// //                         (option.state & QStyle::State_Enabled) ? QPalette::Normal : QPalette::Disabled;
// //     o.backgroundColor = option.palette.color(cg,
// //                         (option.state & QStyle::State_Selected) ? QPalette::Highlight : QPalette::Window);
//     st->drawPrimitive(QStyle::PE_FrameFocusRect, &o, &p);
//
//   }


  ev->accept();
}

void IconButton::mousePressEvent(QMouseEvent* e)
{
  if(_checkable)
    _checked = !_checked;
  update();

  emit pressed();
  if(_checkable)
    emit toggled(_checked);

//   //e->setAccepted(true);   // This makes menu not close when mouse is released. May be desireable with many small buttons...
//   QWidget::mousePressEvent(e);   // Hm, need this so menus can close.
  e->accept();
}

void IconButton::mouseReleaseEvent(QMouseEvent* e)
{
  emit clicked(_checked);

//   //e->setAccepted(true);   // This makes menu not close when mouse is released. May be desireable with many small buttons...
//   QWidget::mouseReleaseEvent(e); // Hm, need this so menus can close.
  e->accept();
}

void IconButton::contextMenuEvent(QContextMenuEvent * e)
{
  e->accept();
}

void IconButton::setHasFixedIconSize(bool v)
{
  _hasFixedIconSize = v;
  updateGeometry();
}

void IconButton::setDrawFlat(bool v)
{
  _drawFlat = v;
  update();
}

void IconButton::setBlinkPhase(bool v)
{
  if(_blinkPhase == v)
    return;
  _blinkPhase = v;
  if(isEnabled())
    update();
}


} // MusEGui