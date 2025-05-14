//=========================================================
//  MusE
//  Linux Music Editor
//
//  missing_plugins.h
//  (C) Copyright 2025 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifndef __MISSING_PLUGINS_H__
#define __MISSING_PLUGINS_H__

#include "ui_missing_plugins_base.h"
#include "plugin.h"

namespace MusEGui {

class MissingPluginsDialog : public QDialog, public Ui::MissingPluginsDialogBase {
      Q_OBJECT

   enum TableCols { COL_LABEL = 0, COL_TYPE, COL_FILE, COL_URI };

   public:
      MissingPluginsDialog(
        const MusECore::MissingPluginList &mpl,
        QWidget* parent = 0,
        Qt::WindowFlags fl = Qt::Dialog);
      };

} // namespace MusEGui

#endif
