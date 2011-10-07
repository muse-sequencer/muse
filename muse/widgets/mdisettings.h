//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: genset.h,v 1.3 2004/01/25 09:55:17 wschweer Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
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

#ifndef __MDISETTINGS_H__
#define __MDISETTINGS_H__

#include <QWidget>
#include "ui_mdisettings_base.h"
#include "cobject.h"

namespace MusEGui {

//---------------------------------------------------------
//   GlobalSettingsConfig
//---------------------------------------------------------

class MdiSettings : public QWidget, private Ui::MdiSettingsBase
{
  Q_OBJECT
  
  private:
    TopWin::ToplevelType _type;

  public:
    MdiSettings(TopWin::ToplevelType t, QWidget* parent=0);
    void update_settings();
    void apply_settings();
    TopWin::ToplevelType type() { return _type; }
};

} // namespace MusEGui

#endif
