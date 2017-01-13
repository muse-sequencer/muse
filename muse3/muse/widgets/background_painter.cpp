//=========================================================
//  MusE
//  Linux Music Editor
//
//  background_painter.cpp
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

#include <QPainter>
#include <QBrush>
#include <QColor>
#include <QPalette>
#include <QLinearGradient>
#include <QRect>

#include "gconfig.h"
#include "background_painter.h"

// #include <stdio.h>
// For debugging output: Uncomment the fprintf section.
#define DEBUG_BACKGROUND_PAINTER(dev, format, args...) // fprintf(dev, format, ##args);

namespace MusEGui {

ItemBackgroundPainter::ItemBackgroundPainter()
{

}

void ItemBackgroundPainter::drawBackground(QPainter* painter,
                                           const QRect& fullRect,
                                           const QPalette& pal,
                                           int xMargin,
                                           int yMargin,
                                           const QRect& onRect,
                                           const QColor& activeColor)
{
  painter->save();
  painter->setRenderHint(QPainter::Antialiasing);
  painter->setPen(Qt::NoPen);

  bool onfull = false;
  if(!onRect.isNull())
    onfull = (onRect == fullRect);

  QColor acolor = activeColor.isValid() ? activeColor : MusEGlobal::config.rackItemBackgroundColor;

  QRect cr = QRect(fullRect.x() + xMargin, fullRect.y() + yMargin,
                    fullRect.width() - 2 * xMargin, fullRect.height() - 2 * yMargin);
  painter->fillRect(fullRect, pal.dark().color().darker(130));

  const QColor mask_edge = QColor(110, 110, 110, 55);
  const QColor mask_center = QColor(220, 220, 220, 55);
  QLinearGradient mask;
  mask.setColorAt(0, mask_edge);
  mask.setColorAt(0.5, mask_center);
  mask.setColorAt(1, mask_edge);
  mask.setStart(QPointF(0, cr.y()));
  mask.setFinalStop(QPointF(0, cr.y() + cr.height()));

  if(onRect.isNull() || !onfull)
  {
    int cw = fullRect.width();
    if(!onRect.isNull())
      cw -= onRect.width();
    const QRect knobclip(fullRect.x(), fullRect.y(), cw, fullRect.height());
    painter->setClipRect(knobclip);
    painter->setBrush(pal.dark());
    painter->drawRoundedRect(cr, 2, 2);
    painter->setClipRect(fullRect);
  }

  if(!onRect.isNull())
  {
    QRect labeldraw = QRect(onRect.x() + xMargin,
                            onRect.y() + yMargin,
                            onRect.width() - 2 * xMargin,
                            onRect.height() - 2 * yMargin);
    painter->setBrush(acolor);
    painter->drawRoundedRect(labeldraw, 2, 2);
  }

  painter->setBrush(mask);
  painter->drawRoundedRect(cr, 2, 2);

  painter->restore();
}

} // namespace MusEGui
