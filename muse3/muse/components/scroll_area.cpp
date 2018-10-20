//=========================================================
//  MusE
//  Linux Music Editor
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  scroll_area.cpp
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

#include <QSize>

#include "scroll_area.h"
#include "compact_slider.h"

namespace MusEGui {

//---------------------------------------------------------
//   CompactScrollArea
//---------------------------------------------------------

CompactScrollArea::CompactScrollArea(QWidget *parent)
            : QScrollArea(parent)
{
}
  
QSize CompactScrollArea::minimumSizeHint() const
{
  if(widget())
    return widget()->sizeHint();
  return QSize(16, 16);
}

//---------------------------------------------------------
//   CompactControllerRack
//---------------------------------------------------------

CompactControllerRack::CompactControllerRack(QWidget *parent, int minItems)
            : QScrollArea(parent), _minItems(minItems)
{
  _xItemMargin = 0;
  _yItemMargin = 0;
  _minItemSize = defaultItemSizeHint();
  _minSize = QSize(_minItemSize.width(), _minItems * _minItemSize.height()); // + 1; // TODO Instead of just +1, may need margins etc?
}
  
QSize CompactControllerRack::minimumSizeHint() const
{
  //return _minSize;
  const QSize sz = CompactSlider::getMinimumSizeHint(fontMetrics(), 
                                                     Qt::Horizontal, 
                                                     CompactSlider::None, 
                                                     _xItemMargin, 
                                                     _yItemMargin);
  return QSize(sz.width() + 2 * frameWidth(), _minItems * sz.height() + 2 * frameWidth());
}
 
QSize CompactControllerRack::defaultItemSizeHint()
{
//   // HACK: Get some idea of the height of each item - even with an empty scroll area.
//   CompactSlider* dummy_slider = new CompactSlider(this, 0, Qt::Horizontal, CompactSlider::None, "WWW");
//   dummy_slider->setValueDecimals(0);
//   dummy_slider->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum));
//   dummy_slider->setEnabled(true);
//   
//   const QSize sz = dummy_slider->sizeHint();
//   
//   delete dummy_slider;
//   
//   return sz;
  
  // HACK: Get some idea of the height of each item - even with an empty scroll area.
  return CompactSlider::getMinimumSizeHint(fontMetrics(), Qt::Horizontal, CompactSlider::None, _xItemMargin, _yItemMargin);
}

void CompactControllerRack::updateDefaultItemSizeHint()
{
  _minItemSize = defaultItemSizeHint();
  update();
}

void CompactControllerRack::updateMinimumSize()
{
  _minSize = QSize(_minItemSize.width(), _minItems * _minItemSize.height()); // + 1; // TODO Instead of just +1, may need margins etc?
  update();
}

void CompactControllerRack::setMinItems(int n)
{
  _minItems = n;
  _minItemSize = defaultItemSizeHint();
  _minSize = QSize(_minItemSize.width(), _minItems * _minItemSize.height()); // + 1; // TODO Instead of just +1, may need margins etc?
  update();
}

void CompactControllerRack::setItemMargins(int hor, int vert)
{
  _xItemMargin = hor;
  _yItemMargin = vert;
  _minItemSize = defaultItemSizeHint();
  _minSize = QSize(_minItemSize.width(), _minItems * _minItemSize.height()); // + 1; // TODO Instead of just +1, may need margins etc?
  //resize(this->size());  // FIXME Call something else like updateGeometry etc.
  updateGeometry();
}

} // namespace MusEGui
