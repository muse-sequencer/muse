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

#include <QBrush>
#include <QLinearGradient>

#include "gconfig.h"
#include "background_painter.h"

// #include <stdio.h>
// For debugging output: Uncomment the fprintf section.
#define DEBUG_BACKGROUND_PAINTER(dev, format, args...) // fprintf(dev, format, ##args);

namespace MusEGui {

ItemBackgroundPainter::ItemBackgroundPainter(QObject* parent)
    : QObject(parent)
{

}

void ItemBackgroundPainter::drawBackground(QPainter* painter,
                                           const QRect& fullRect,
                                           const QPalette& pal,
                                           int xMargin,
                                           int yMargin,
                                           const QRect& onRect,
                                           int radius,
                                           bool style3d,
                                           QColor colSlotActive,
                                           QColor colBorder,
                                           QColor colSlot)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(Qt::NoPen);

    if (!colBorder.isValid())
        colBorder = pal.dark().color().darker(130);
    if (!colSlot.isValid())
        colSlot = pal.dark().color();
    if (!colSlotActive.isValid())
        colSlotActive = pal.highlight().color();

    bool onfull = false;
    if(!onRect.isNull())
        onfull = (onRect == fullRect);

    QRect cr = QRect(fullRect.x() + xMargin, fullRect.y() + yMargin,
                     fullRect.width() - 2 * xMargin, fullRect.height() - 2 * yMargin);
    painter->fillRect(fullRect, colBorder);

    if (onRect.isNull() || !onfull)
    {
        int cw = fullRect.width();
        if(!onRect.isNull())
            cw -= onRect.width();
        const QRect knobclip(fullRect.x(), fullRect.y(), cw, fullRect.height());
        painter->setClipRect(knobclip);
        painter->setBrush(colSlot);
        painter->drawRoundedRect(cr, radius, radius);
        painter->setClipRect(fullRect);
    }

    if(!onRect.isNull())
    {
        QRect labeldraw = QRect(onRect.x() + xMargin,
                                onRect.y() + yMargin,
                                onRect.width() - 2 * xMargin,
                                onRect.height() - 2 * yMargin);
        painter->setBrush(colSlotActive);
        painter->drawRoundedRect(labeldraw, radius, radius);
    }

    if (style3d) {
        const QColor mask_edge = QColor(110, 110, 110, 55);
        const QColor mask_center = QColor(220, 220, 220, 55);
        QLinearGradient mask;
        mask.setColorAt(0, mask_edge);
        mask.setColorAt(0.5, mask_center);
        mask.setColorAt(1, mask_edge);
        mask.setStart(QPointF(0, cr.y()));
        mask.setFinalStop(QPointF(0, cr.y() + cr.height()));

        painter->setBrush(mask);
        painter->drawRoundedRect(cr, radius, radius);
    }

    painter->restore();
}

} // namespace MusEGui
