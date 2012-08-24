//=============================================================================
//  MusE
//  Linux Music Editor
//  (C) Copyright 1999-2001 Werner Schweer (ws@seh.de)
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

#include <QLabel>

#include "menutitleitem.h"

namespace MusEGui {

//---------------------------------------------------------
//   MenuTitleItem
//---------------------------------------------------------

MenuTitleItem::MenuTitleItem(const QString& ss, QWidget* parent)
  : QWidgetAction(parent)
      {
        s = ss;
        // Don't allow to click on it.
        setEnabled(false);
        // Just to be safe, set to -1 instead of default 0.
        setData(-1);
      }

QWidget* MenuTitleItem::createWidget(QWidget *parent)
{
  QLabel* l = new QLabel(s, parent);
  l->setAlignment(Qt::AlignCenter);
  l->setAutoFillBackground(true);
  //QPalette palette;
  //palette.setColor(label->backgroundRole(), c);
  //l->setPalette(palette);
  l->setBackgroundRole(QPalette::Dark);
  return l;
}

} // namespace MusEGui
