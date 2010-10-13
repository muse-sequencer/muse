//
// C++ Implementation: shortcutconfig
//
// Description:
// Dialog for configuring keyboard shortcuts
//
// Author: Mathias Lundgren <lunar_shuttle@users.sourceforge.net>, (C) 2003
//
// Copyright: Mathias Lundgren (lunar_shuttle@users.sourceforge.net) (C) 2003
//
//
#include <qpushbutton.h>
#include <qkeysequence.h>
//Added by qt3to4:
#include <QCloseEvent>
#include "shortcutconfig.h"
#include "shortcutcapturedialog.h"
#include "shortcuts.h"

ShortcutConfig::ShortcutConfig(QWidget* parent, const char* name)
   : ShortcutConfigBase(parent, name, true)
   {
   connect(cgListView, SIGNAL(selectionChanged(Q3ListViewItem* )),
            this, SLOT(categorySelChanged(Q3ListViewItem*)));
   connect(scListView, SIGNAL(selectionChanged(Q3ListViewItem* )),
            this, SLOT(shortcutSelChanged(Q3ListViewItem*)));
   connect(defineButton, SIGNAL(pressed()), this, SLOT(assignShortcut()));
   connect(clearButton,  SIGNAL(pressed()), this, SLOT(clearShortcut()));
   connect(applyButton,  SIGNAL(pressed()), this, SLOT(assignAll()));

   current_category = ALL_SHRT;
   cgListView->setSorting(SHRT_CATEGORY_COL, -1);
   _config_changed = false;

   //Fill up category listview:
   SCListViewItem* newItem;
   for (int i=0; i < SHRT_NUM_OF_CATEGORIES; i++) {
         newItem = new SCListViewItem(cgListView, i);
         newItem->setText(SHRT_CATEGORY_COL, shortcut_category[i].name);
         cgListView->insertItem(newItem);
         }
   updateSCListView();
   }

void ShortcutConfig::updateSCListView(int category)
      {
      scListView->clear();
      SCListViewItem* newItem;
      for (int i=0; i < SHRT_NUM_OF_ELEMENTS; i++) {
            if (shortcuts[i].type & category) {
                  newItem = new SCListViewItem(scListView, i);
                  newItem->setText(SHRT_DESCR_COL, tr(shortcuts[i].descr));
                  QKeySequence key = QKeySequence(shortcuts[i].key);
                  newItem->setText(SHRT_SHRTCUT_COL, key);
                  }
            }
      }

void ShortcutConfig::assignShortcut()
      {
      SCListViewItem* active = (SCListViewItem*) scListView->selectedItem();
      int shortcutindex = active->getIndex();
      ShortcutCaptureDialog* sc = new ShortcutCaptureDialog(this, "sccapture", shortcutindex);
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
      SCListViewItem* active = (SCListViewItem*) scListView->selectedItem();
      int shortcutindex = active->getIndex();
      shortcuts[shortcutindex].key = 0; //Cleared
      active->setText(SHRT_SHRTCUT_COL,"");
      clearButton->setDown(false);
      clearButton->setEnabled(false);
      _config_changed = true;
      }

void ShortcutConfig::categorySelChanged(Q3ListViewItem* i)
      {
            SCListViewItem* item = (SCListViewItem*) i;
            current_category = shortcut_category[item->getIndex()].id_flag;
            updateSCListView(current_category);
      }

void ShortcutConfig::shortcutSelChanged(Q3ListViewItem* in_item)
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
      done(_config_changed);
      }


void ShortcutConfig::assignAll()
      {
      applyButton->setDown(false);
      done(_config_changed);
      }
