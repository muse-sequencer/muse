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
#include <QPaintEvent>
#include <QStyle>
#include <QStyleOption>

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

CompactToolButton::CompactToolButton(QWidget* parent, const QIcon& icon, bool hasFixedIconSize, bool drawFlat, const char* name)
         : QToolButton(parent), _icon(icon), _hasFixedIconSize(hasFixedIconSize), _drawFlat(drawFlat)
{
  setObjectName(name);
  _blinkPhase = false;
}

QSize CompactToolButton::sizeHint() const
{
  // TODO Ask style for margins.
  const QSize isz = iconSize();
// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
  const int fmw = fontMetrics().horizontalAdvance(text());
#else
  const int fmw = fontMetrics().width(text());
#endif
  const int fmh = fontMetrics().lineSpacing() + 5;

  const int iw = isz.width() + 2;
  const int ih = isz.height() + 2;

  const int w = (_hasFixedIconSize && iw > fmw) ? iw : fmw;
  const int h = (_hasFixedIconSize && ih > fmh) ? ih : fmh;

  return QSize(w, h);
}
      
void CompactToolButton::setHasFixedIconSize(bool v)
{
  _hasFixedIconSize = v;
  updateGeometry();
}

void CompactToolButton::setDrawFlat(bool v)
{
  _drawFlat = v;
  update();
}

void CompactToolButton::setIcon(const QIcon & icon)
{
  _icon = icon;
  update();
}

void CompactToolButton::setBlinkPhase(bool v)
{
  if(_blinkPhase == v)
    return;
  _blinkPhase = v;
  if(isEnabled())
    update();
}

void CompactToolButton::paintEvent(QPaintEvent* ev)
{
  if(!_drawFlat)
    QToolButton::paintEvent(ev);

  QIcon::Mode mode;
  if(isEnabled())
    mode = hasFocus() ? QIcon::Selected : QIcon::Normal;
  else
    mode = QIcon::Disabled;
  QIcon::State state = (isChecked() && (!_blinkPhase || !isEnabled())) ? QIcon::On : QIcon::Off;

  QPainter p(this);
  _icon.paint(&p, rect(), Qt::AlignCenter, mode, state);

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


} // namespace MusEGui
