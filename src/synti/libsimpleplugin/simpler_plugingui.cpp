//
// C++ Implementation: ssplugingui
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
#include "simpler_plugingui.h"

#define SS_PLUGINCHOOSER_NAMECOL     0
#define SS_PLUGINCHOOSER_LABELCOL    1
#define SS_PLUGINCHOOSER_INPORTSCOL  2
#define SS_PLUGINCHOOSER_OUTPORTSCOL 3
#define SS_PLUGINCHOOSER_CREATORCOL  4

namespace MusESimplePlugin {

//---------------------------------------------------------
//   getPlugin
//---------------------------------------------------------

Plugin* SimplerPluginChooser::getPlugin(QWidget* parent)
{
      SimplerPluginChooser* dialog = new SimplerPluginChooser(parent);
      Plugin* p = 0;
      int rv = dialog->exec();
      if(rv)
        p = dialog->getSelectedPlugin();
      delete dialog;
      return p;
}


SimplerPluginChooser::SimplerPluginChooser(QWidget* parent)
      :QDialog(parent)
      {
      setupUi(this);
      selectedPlugin = 0;

      for (iPlugin i=plugins.begin(); i !=plugins.end(); i++) {
            //Support for only 2 or 1 inport/outports
            if ( ((*i)->outports() == 2 || (*i)->outports() == 1) && ((*i)->inports() == 2 || (*i)->inports() == 1) ) {
                  QTreeWidgetItem* tmpItem = new QTreeWidgetItem(effectsListView);
                  tmpItem->setText(SS_PLUGINCHOOSER_NAMECOL, (*i)->name());
                  tmpItem->setText(SS_PLUGINCHOOSER_LABELCOL, (*i)->label());
                  tmpItem->setText(SS_PLUGINCHOOSER_INPORTSCOL, QString::number((*i)->inports()));
                  tmpItem->setText(SS_PLUGINCHOOSER_OUTPORTSCOL, QString::number((*i)->outports()));
                  tmpItem->setText(SS_PLUGINCHOOSER_CREATORCOL, (*i)->maker());
                  effectsListView->addTopLevelItem(tmpItem);
                  }
            }
      connect(okButton, SIGNAL(pressed()), SLOT(okPressed()));
      connect(cancelButton, SIGNAL(pressed()), SLOT(cancelPressed()));
      
      connect(effectsListView, SIGNAL(itemSelectionChanged()), SLOT(selectionChanged()));
      connect(effectsListView, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), SLOT(doubleClicked(QTreeWidgetItem*)));
      }

void SimplerPluginChooser::selectionChanged()
      {
      selectedItem = effectsListView->currentItem();  
      }

void SimplerPluginChooser::okPressed()
      {
      selectedPlugin = findSelectedPlugin();
      done(QDialog::Accepted);
      }

void SimplerPluginChooser::cancelPressed()
      {
      done(QDialog::Rejected);
      }

void SimplerPluginChooser::doubleClicked(QTreeWidgetItem* item)
      {
      selectedItem = item;
      selectedPlugin = findSelectedPlugin();
      done(QDialog::Accepted);
      }

Plugin* SimplerPluginChooser::findSelectedPlugin()
      {
      if(!selectedItem)
        return 0;
      Plugin* selected = 0;
      for (iPlugin i = plugins.begin(); i != plugins.end(); i++) {
            if ((*i)->name() == selectedItem->text(SS_PLUGINCHOOSER_NAMECOL))
                  selected = (*i);
            }
      return selected;
      }

} // namespace MusESimplePlugin
      
