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

//---------------------------------------------------------
//   ShortcutConfig
//---------------------------------------------------------

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
            newItem->setData(0, Qt::UserRole, i);
            cgListView->addTopLevelItem(newItem);
            }
      updateSCListView();
      }

//---------------------------------------------------------
//   updateSCListView
//---------------------------------------------------------

void ShortcutConfig::updateSCListView(int category)
      {
      scListView->clear();
      foreach (Shortcut* s, shortcuts) {
            if (s && (s->type & category)) {
                  QTreeWidgetItem* newItem;
                  newItem = new QTreeWidgetItem;
                  newItem->setText(SHRT_DESCR_COL, tr(s->descr));
                  QKeySequence seq = s->key;
                  newItem->setText(SHRT_SHRTCUT_COL, s->key.toString(QKeySequence::NativeText));
                  newItem->setData(0, Qt::UserRole, s->xml);
                  scListView->addTopLevelItem(newItem);
                  }
            }
      }

//---------------------------------------------------------
//   assignShortcut
//---------------------------------------------------------

void ShortcutConfig::assignShortcut()
      {
      QTreeWidgetItem* active = scListView->currentItem();
      Shortcut* s = shortcuts[active->data(0, Qt::UserRole).toString()];
      ShortcutCaptureDialog sc(s, this);
      if (sc.exec()) {
            s->key = sc.getKey();
            active->setText(SHRT_SHRTCUT_COL, s->key.toString(QKeySequence::NativeText));
            if (s->action)
                  s->action->setShortcut(s->key);
            _config_changed = true;
            }
      clearButton->setEnabled(true);
      }

//---------------------------------------------------------
//   clearShortcut
//---------------------------------------------------------

void ShortcutConfig::clearShortcut()
      {
      QTreeWidgetItem* active = scListView->currentItem();
      Shortcut* s = shortcuts[active->data(0, Qt::UserRole).toString()];
      s->key = 0;
      active->setText(SHRT_SHRTCUT_COL, "");
      clearButton->setEnabled(false);
      _config_changed = true;
      }

//---------------------------------------------------------
//   categorySelChanged
//---------------------------------------------------------

void ShortcutConfig::categorySelChanged(QTreeWidgetItem* i)
      {
      int idx = i->data(0, Qt::UserRole).toInt();
      current_category = shortcut_category[idx].id_flag;
      updateSCListView(current_category);
      }

//---------------------------------------------------------
//   shortcutSelChanged
//---------------------------------------------------------

void ShortcutConfig::shortcutSelChanged(QTreeWidgetItem* active)
      {
      defineButton->setEnabled(active != 0);
      if (active == 0) {
            clearButton->setEnabled(false);
            return;
            }
      Shortcut* s = shortcuts[active->data(0, Qt::UserRole).toString()];
      clearButton->setEnabled(s && !s->key.isEmpty());
      }

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void ShortcutConfig::closeEvent(QCloseEvent*)
      {
      done(_config_changed);
      }

//---------------------------------------------------------
//   assignAll
//---------------------------------------------------------

void ShortcutConfig::assignAll()
      {
      done(_config_changed);
      }
