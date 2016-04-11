//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ttoolbutton.cpp,v 1.1 2004/02/21 16:53:50 wschweer Exp $
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
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
#include <QStyle>

#include "ttoolbutton.h"
#include "gconfig.h"
#include "icons.h"

namespace MusEGui {

//---------------------------------------------------------
//   TransparentToolButton
//---------------------------------------------------------
  
//---------------------------------------------------------
//   drawButton
//---------------------------------------------------------

void TransparentToolButton::drawButton(QPainter* p)
      {
      int w = width();
      int h = height();
      QIcon::Mode mode = isEnabled() ? QIcon::Normal : QIcon::Disabled;
      QIcon::State state = isChecked() ? QIcon::On : QIcon::Off;
      const QPixmap pm(icon().pixmap(style()->pixelMetric(QStyle::PM_SmallIconSize), mode, state));
      p->drawPixmap(QPoint((w - pm.width())/2, (h - pm.height())/2), pm);
      }
      
//---------------------------------------------------------
//   CompactToolButton
//---------------------------------------------------------

CompactToolButton::CompactToolButton(QWidget* parent, const QIcon& icon_A, const QIcon& icon_B, const char* name)
         : QToolButton(parent), _icon_A(icon_A), _icon_B(icon_B)
{
  setObjectName(name);
  _useIcon_B = false;
  setIcon(_icon_A);
}

QSize CompactToolButton::sizeHint() const
{
  // TODO Ask style for margins.
  const QSize isz = iconSize();
  const QSize tsz = fontMetrics().size(0, text());
  
  const int iw = isz.width() + 2;
  const int ih = isz.height() + 2;
  const int tw = tsz.width() + 4;
  const int th = tsz.height() + 2;

  const int w = iw > tw ? iw : tw;
  const int h = ih > th ? ih : th;
  
  return QSize(w, h);
}
      
void CompactToolButton::setIconA(const QIcon& ico)
{
  _icon_A = ico;
  if(!_useIcon_B)
    setIcon(_icon_A);
}

void CompactToolButton::setIconB(const QIcon& ico)
{
  _icon_B = ico;
  if(_useIcon_B)
    setIcon(_icon_B);
}

void CompactToolButton::setCurrentIcon(bool v)
{
  if(_useIcon_B == v)
    return;
  _useIcon_B = v;
  if(_useIcon_B)
    setIcon(_icon_B);
  else
    setIcon(_icon_A);
}


} // namespace MusEGui
