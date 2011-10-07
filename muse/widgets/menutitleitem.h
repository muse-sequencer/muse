//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: menutitleitem.h,v 1.1.2.1 2009/06/10 00:34:59 terminator356 Exp $
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
//
//=========================================================

#ifndef __MENU_TITLE_ITEM_H__
#define __MENU_TITLE_ITEM_H__

#include <QWidgetAction>

namespace MusEGui {

//---------------------------------------------------------
//   MenuTitleItem
//---------------------------------------------------------

class MenuTitleItem : public QWidgetAction { 
      Q_OBJECT
   private:
      
        
      QString s;

   public:
      MenuTitleItem(const QString&, QWidget* /*parent*/);
      QWidget* createWidget(QWidget* /*parent*/);
      };

} // namespace MusEGui
#endif
