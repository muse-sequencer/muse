//=========================================================
//  MusE
//  Linux Music Editor
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  scroll_area.h
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

#ifndef __SCROLL_AREA_H__
#define __SCROLL_AREA_H__

#include <QScrollArea>

class QSize;

namespace MusEGui {

//---------------------------------------------------------
//   CompactScrollArea
//---------------------------------------------------------

class CompactScrollArea : public QScrollArea
{
  Q_OBJECT

  public:
    CompactScrollArea(QWidget* parent);
    virtual QSize minimumSizeHint() const;
};

//---------------------------------------------------------
//   CompactControllerRack
//---------------------------------------------------------

class CompactControllerRack : public QScrollArea
{
  Q_OBJECT

  private:
    int _minItems;
    QSize _minItemSize;
    QSize  _minSize;
    int _xItemMargin;
    int _yItemMargin;
    
  public:
    CompactControllerRack(QWidget* parent, int minItems = 0);
    virtual QSize minimumSizeHint() const;
    int minItems() const { return _minItems; }
    void setMinItems(int n);
    QSize defaultItemSizeHint();
    void updateDefaultItemSizeHint();
    void updateMinimumSize();
    
    void setItemMargins(int x, int y);
};

} // namespace MusEGui

#endif
