//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2003 Mathias Lundgren (lunar_shuttle@users.sourceforge.net)
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "shortcutconfig.h"
#include "shortcutcapturedialog.h"
#include "shortcuts.h"

ShortcutConfig::ShortcutConfig(QWidget* parent)
   : QDialog(parent)
      {
      setupUi(this);
      connect(cgListView, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
            this, SLOT(categorySelChanged(QTreeWidgetItem*)));
      connect(scListView, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
            this, SLOT(shortcutSelChanged(QTreeWidgetItem*)));
      connect(defineButton, SIGNAL(pressed()), this, SLOT(assignShortcut()));
      connect(clearButton,  SIGNAL(pressed()), this, SLOT(clearShortcut()));
      connect(applyButton,  SIGNAL(pressed()), this, SLOT(assignAll()));

      current_category = ALL_SHRT;
//TD   cgListView->setSorting(SHRT_CATEGORY_COL, -1);
      _config_changed = false;

      //Fill up category listview:
      for (int i=0; i < SHRT_NUM_OF_CATEGORIES; i++) {
            QTreeWidgetItem* newItem = new QTreeWidgetItem;
            newItem->setText(SHRT_CATEGORY_COL, tr(shortcut_category[i].name));
            newItem->setData(0, 1, i);
            cgListView->addTopLevelItem(newItem);
            }
      updateSCListView();
      }

void ShortcutConfig::updateSCListView(int category)
      {
      scListView->clear();
      for (int i=0; i < SHRT_NUM_OF_ELEMENTS; i++) {
            if (shortcuts[i].type & category) {
                  QTreeWidgetItem* newItem;
                  newItem = new QTreeWidgetItem;
                  newItem->setText(SHRT_DESCR_COL, shortcuts[i].descr);
                  QKeySequence key = QKeySequence(shortcuts[i].key);
                  newItem->setText(SHRT_SHRTCUT_COL, key);
                  newItem->setData(0, 1, i);
                  scListView->addTopLevelItem(newItem);
                  }
            }
      }

void ShortcutConfig::assignShortcut()
      {
      QTreeWidgetItem* active = scListView->currentItem();
      int shortcutindex = active->data(0, 1).toInt();
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
      QTreeWidgetItem* active = scListView->currentItem();
      int shortcutindex = active->data(0, 1).toInt();
      shortcuts[shortcutindex].key = 0; //Cleared
      active->setText(SHRT_SHRTCUT_COL, "");
      clearButton->setDown(false);
      clearButton->setEnabled(false);
      _config_changed = true;
      }

void ShortcutConfig::categorySelChanged(QTreeWidgetItem* i)
      {
      int idx = i->data(0, 1).toInt();
      current_category = shortcut_category[idx].id_flag;
      updateSCListView(current_category);
      }

void ShortcutConfig::shortcutSelChanged(QTreeWidgetItem* active)
      {
      defineButton->setEnabled(true);
      int index = active->data(0, 1).toInt();
      if (!shortcuts[index].key.isEmpty())
            clearButton->setEnabled(true);
      else
            clearButton->setEnabled(false);
      }

void ShortcutConfig::closeEvent(QCloseEvent*)
      {
      done(_config_changed);
      }

void ShortcutConfig::assignAll()
      {
      applyButton->setDown(false);
      done(_config_changed);
      }

QString ShortcutConfig::Translate(const char* locstr)
      {
      //printf("In: %s - Trans1: %s\n", locstr, tr(locstr).toLatin1().data());
      return tr(locstr);
      }
