//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: genset.cpp,v 1.7.2.8 2009/12/01 03:52:40 terminator356 Exp $
//
//  (C) Copyright 2001-2004 Werner Schweer (ws@seh.de)
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

#include <stdio.h>

#include <QFileDialog>
#include <QRect>
#include <QShowEvent>

#include "mdisettings.h"
#include "app.h"
#include "gconfig.h"
#include "midiseq.h"
#include "globals.h"
#include "icons.h"

namespace MusEGui {

MdiSettings::MdiSettings(TopWin::ToplevelType t, QWidget* parent) : QWidget(parent)
{
  _type=t;
  setupUi(this);
  
  groupBox->setTitle(TopWin::typeName(t)); 
  update_settings();
}


void MdiSettings::update_settings()
{
  isSubwinCheckbox->setChecked(TopWin::_defaultSubwin[_type]);
  shareSubwinCheckbox->setChecked(TopWin::_sharesWhenSubwin[_type]);
  shareFreeCheckbox->setChecked(TopWin::_sharesWhenFree[_type]);
}

void MdiSettings::apply_settings()
{
  TopWin::_defaultSubwin[_type] = isSubwinCheckbox->isChecked();
  TopWin::_sharesWhenSubwin[_type] = shareSubwinCheckbox->isChecked();
  TopWin::_sharesWhenFree[_type] = shareFreeCheckbox->isChecked();
}

} // namespace MusEGui
