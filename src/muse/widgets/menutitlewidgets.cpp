//=========================================================
//  MusE
//  Linux Music Editor
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  menutitlewidgets.cpp
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

#include <QPalette>
#include <QColor>

#include "menutitlewidgets.h"

namespace MusEGui {

MenuTitleLabel::MenuTitleLabel(const QString &text, QWidget *parent, Qt::WindowFlags f)
    : QLabel(text, parent, f)
{
  setAlignment(Qt::AlignCenter);
  setAutoFillBackground(true);
  // FIXME This doesn't play well with stylesheets. If a stylesheet is set it MUST set this colour
  //        because this code has no effect when a sheet is set.
  //       Instead, if we set a stylesheet here, then we cannot override it from a master sheet.
  //       What to do...?
//   const QColor c = palette().color(backgroundRole()).darker(120);
//   QPalette palette;
//   palette.setColor(backgroundRole(), c);
//   setPalette(palette);
  setBackgroundRole(QPalette::Mid);
//   setForegroundRole(QPalette::Light);
//   setStyleSheet(QString("background-color: rgb(%1,%2,%3);").arg(c.red()).arg(c.green()).arg(c.blue()));
}

MenuTitleLabel::MenuTitleLabel(QWidget *parent, Qt::WindowFlags f)
    : QLabel(parent, f)
{
  setAlignment(Qt::AlignCenter);
  setAutoFillBackground(true);
  setBackgroundRole(QPalette::Mid);
}

} // namespace MusEGui

