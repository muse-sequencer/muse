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

#include "menutitleitem.h"
#include "menutitlewidgets.h"

namespace MusEGui {

//---------------------------------------------------------
//   MenuTitleItem
//---------------------------------------------------------

MenuTitleItem::MenuTitleItem(const QString& ss, QWidget* parent)
  : QWidgetAction(parent)
      {
        setObjectName("menuTitleItem");
        s = ss;
        // Don't allow to click on it.
        setEnabled(false);
        // Just to be safe, set to -1 instead of default 0.
        setData(-1);
      }

QWidget* MenuTitleItem::createWidget(QWidget *parent)
{
  MenuTitleLabel* l = new MenuTitleLabel(s, parent);
  return l;
}

} // namespace MusEGui
