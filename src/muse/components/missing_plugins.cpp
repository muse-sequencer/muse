//=========================================================
//  MusE
//  Linux Music Editor
//
//  missing_plugins.cpp
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

#include "missing_plugins.h"
#include "plugin_scan.h"
#include <QTableWidgetItem>

namespace MusEGui {

MissingPluginsDialog::MissingPluginsDialog(const MusECore::MissingPluginList &mpl, QWidget* parent, Qt::WindowFlags fl)
 : QDialog(parent, fl)
{
  setupUi(this);

  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

  const int rows = mpl.size();
  tableWidget->setRowCount(rows);
  const bool sort = tableWidget->isSortingEnabled();
  // Disable sorting while adding items in rows.
  tableWidget->setSortingEnabled(false);
  int currow = 0;
  for(MusECore::MissingPluginList::const_iterator i = mpl.cbegin(); i != mpl.cend(); ++i, ++currow)
  {
    tableWidget->setItem(currow, COL_LABEL, new QTableWidgetItem(i->_label));
    tableWidget->setItem(currow, COL_TYPE,  new QTableWidgetItem(
      QApplication::translate("MusEPlugin", MusEPlugin::pluginTypeToString(i->_type))));
    tableWidget->setItem(currow, COL_FILE,  new QTableWidgetItem(i->_file));
    tableWidget->setItem(currow, COL_URI,   new QTableWidgetItem(i->_uri));
  }
  // Restore sorting setting.
  tableWidget->setSortingEnabled(sort);
  tableWidget->setColumnWidth(COL_LABEL, 200);
  tableWidget->setColumnWidth(COL_TYPE, 100);
  tableWidget->setColumnWidth(COL_FILE, 200);
  // Last column is stretched.
}


} // namespace MuseGui
