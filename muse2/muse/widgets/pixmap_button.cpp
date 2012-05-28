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
#include <QPaintEvent>
#include <QMouseEvent>

#include "pixmap_button.h"

namespace MusEGui {
  
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

} // MusEGui