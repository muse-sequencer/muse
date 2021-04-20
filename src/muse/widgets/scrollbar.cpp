//=========================================================
//  MusE
//  Linux Music Editor
//    scrollbar.cpp
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2016 Tim E. Real (terminator356 on sourceforge)
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

#include "scrollbar.h"

#include <QWheelEvent>
//#include <QResizeEvent>

namespace MusEGui {

ScrollBar::ScrollBar(Qt::Orientation orientation, bool autoPageStep, QWidget* parent) : 
  QScrollBar(orientation, parent), _autoPageStep(autoPageStep) 
{
  
};

void ScrollBar::redirectedWheelEvent(QWheelEvent* e)
{
  if(isVisible())
    wheelEvent(e);
}

void ScrollBar::resizeEvent(QResizeEvent* e) 
{ 
  if(_autoPageStep)
    setPageStep(e->size().height());
}
  
} // namespace MusEGui
