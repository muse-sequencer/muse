//
// C++ Interface: ssplugingui
//
// Description:
//
//
// Author: Mathias Lundgren <lunar_shuttle@users.sf.net>, (C) 2004
//  Contributer: (C) Copyright 2011 Tim E. Real (terminator356 at users.sourceforge.net)
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
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef __SIMPLER_PLUGIN_GUI_H__
#define __SIMPLER_PLUGIN_GUI_H__

#include <QTreeWidgetItem>

#include "ui_simplepluginchooserbase.h"
#include "libsimpleplugin/simpler_plugin.h"

namespace MusESimplePlugin {

//-------------------------------
// SimplerPluginChooser 
//-------------------------------
class SimplerPluginChooser : public QDialog, Ui::SimplePluginChooserBase
{
   Q_OBJECT
   private:
      Plugin* selectedPlugin;
      
   protected:

   public:
      SimplerPluginChooser(QWidget* parent);
      static Plugin* getPlugin(QWidget* parent);
      Plugin* getSelectedPlugin() { return selectedPlugin; }

   private slots:
      void okPressed();
      void cancelPressed();
      void selectionChanged();
      void doubleClicked(QTreeWidgetItem* item);

   private:
      QTreeWidgetItem* selectedItem;
      Plugin* findSelectedPlugin();
};

} // namespace MusESimplePlugin

#endif

