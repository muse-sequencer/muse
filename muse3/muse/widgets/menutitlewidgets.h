//=========================================================
//  MusE
//  Linux Music Editor
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  menutitlewidgets.h
//  (C) Copyright 2019 Tim E. Real (terminator356 on sourceforge)
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

#ifndef __MENU_TITLE_WIDGETS_H__
#define __MENU_TITLE_WIDGETS_H__

#include <QWidget>
#include <QLabel>
#include <QString>

namespace MusEGui {

//---------------------------------------------------------
//   MenuTitleLabel
//---------------------------------------------------------

class MenuTitleLabel : public QLabel { 
    Q_OBJECT

  public:
    MenuTitleLabel(const QString &text, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    MenuTitleLabel(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
};

} // namespace MusEGui
#endif

