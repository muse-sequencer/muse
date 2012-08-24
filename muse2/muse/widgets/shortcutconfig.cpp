//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./muse/widgets/shortcutconfig.cpp $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
// Author: Mathias Lundgren <lunar_shuttle@users.sourceforge.net>, (C) 2003
//
// Copyright: Mathias Lundgren (lunar_shuttle@users.sourceforge.net) (C) 2003
//
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
//
// C++ Implementation: shortcutconfig
//
// Description:
// Dialog for configuring keyboard shortcuts

#include <QCloseEvent>
#include <QKeySequence>
#include <QString>
#include <QSettings>
#include <QApplication>

#include "shortcutconfig.h"
#include "shortcutcapturedialog.h"
#include "shortcuts.h"

namespace MusEGui {

ShortcutConfig::ShortcutConfig(QWidget* parent)
   : QDialog(parent)
   {
   setupUi(this);
   QSettings settings("MusE", "MusE-qt");
   restoreGeometry(settings.value("ShortcutConfig/geometry").toByteArray());

   connect(cgListView, SIGNAL(itemActivated(QTreeWidgetItem*, int )),
	     this, SLOT(categorySelChanged(QTreeWidgetItem*, int)));
   connect(scListView, SIGNAL(itemActivated(QTreeWidgetItem*, int )),
	   this, SLOT(shortcutSelChanged(QTreeWidgetItem*, int)));
   
   connect(defineButton, SIGNAL(pressed()), this, SLOT(assignShortcut()));
   connect(clearButton,  SIGNAL(pressed()), this, SLOT(clearShortcut()));
   connect(applyButton,  SIGNAL(pressed()), this, SLOT(assignAll()));

   current_category = ALL_SHRT;
   cgListView->sortItems(SHRT_CATEGORY_COL, Qt::AscendingOrder);
   _config_changed = false;

   //Fill up category listview:
   SCListViewItem* newItem;
   SCListViewItem* selItem = 0;
   for (int i=0; i < SHRT_NUM_OF_CATEGORIES; i++) {
         newItem = new SCListViewItem(cgListView, i);
         newItem->setText(SHRT_CATEGORY_COL, shortcut_category[i].name);
         if(shortcut_category[i].id_flag == current_category)
           selItem = newItem;
         }
   if(selItem)
     cgListView->setCurrentItem(selItem);  // Tim
   updateSCListView();

   }

void ShortcutConfig::updateSCListView(int category)
      {
      scListView->clear();
      SCListViewItem* newItem;
      //QString catpre;
      for (int i=0; i < SHRT_NUM_OF_ELEMENTS; i++) {
            if (shortcuts[i].type & category) {
                  newItem = new SCListViewItem(scListView, i);
                  newItem->setText(SHRT_DESCR_COL, qApp->translate("shortcuts", shortcuts[i].descr));
                  //if(category == ALL_SHRT)
                  //  catpre = QString(shortcut_category[shortcuts[i].type].name) + QString(": ");
                  //else 
                  //  catpre.clear();  
                  //newItem->setText(SHRT_DESCR_COL, catpre + tr(qApp->translate("shortcuts", shortcuts[i].descr));  // Tim
                  QKeySequence key = QKeySequence(shortcuts[i].key);
                  newItem->setText(SHRT_SHRTCUT_COL, key);
                  }
            }
      }

void ShortcutConfig::assignShortcut()
      {
      SCListViewItem* active = (SCListViewItem*) scListView->selectedItems()[0];
      int shortcutindex = active->getIndex();
      ShortcutCaptureDialog* sc = new ShortcutCaptureDialog(this, shortcutindex);
      int key = sc->exec();
      delete(sc);
      if (key != Rejected) {
            shortcuts[shortcutindex].key = key;
            QKeySequence keySequence = QKeySequence(key);
            active->setText(SHRT_SHRTCUT_COL, keySequence);
            _config_changed = true;
            }
      clearButton->setEnabled(true);
      defineButton->setDown(false);
      }

void ShortcutConfig::clearShortcut()
      {
      SCListViewItem* active = (SCListViewItem*) scListView->selectedItems()[0];
      int shortcutindex = active->getIndex();
      shortcuts[shortcutindex].key = 0; //Cleared
      active->setText(SHRT_SHRTCUT_COL,"");
      clearButton->setDown(false);
      clearButton->setEnabled(false);
      _config_changed = true;
      }

void ShortcutConfig::categorySelChanged(QTreeWidgetItem* i, int /*column*/)
      {
            SCListViewItem* item = (SCListViewItem*) i;
            current_category = shortcut_category[item->getIndex()].id_flag;
            updateSCListView(current_category);
      }

void ShortcutConfig::shortcutSelChanged(QTreeWidgetItem* in_item, int /*column*/)
      {
      defineButton->setEnabled(true);
      SCListViewItem* active = (SCListViewItem*) in_item;
      int index = active->getIndex();
      if (shortcuts[index].key != 0)
            clearButton->setEnabled(true);
      else
            clearButton->setEnabled(false);
      }

void ShortcutConfig::closeEvent(QCloseEvent* /*e*/) // prevent compiler warning : unused variable
      {
      QSettings settings("MusE", "MusE-qt");
      settings.setValue("ShortcutConfig/geometry", saveGeometry());
      done(_config_changed);
      }


void ShortcutConfig::assignAll()
      {
      applyButton->setDown(false);
      done(_config_changed);
      }

} // namespace MusEGui
